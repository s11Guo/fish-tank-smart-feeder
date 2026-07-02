#include "ESP8266.h"
#include "app.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================
 *  1. TX / RX / Ring Buffer / Line
 * ============================================================ */
static void tx_byte(char c) {
    while (!(USART2->SR & USART_SR_TXE));
    USART2->DR = (uint8_t)c;
}


#define RB_SIZE  512
static uint8_t  rb[RB_SIZE];
static volatile uint16_t rb_w = 0;
static uint16_t rb_r = 0;

void ESP8266_RxHandler(uint8_t byte) {
    uint16_t n = (rb_w + 1) % RB_SIZE;
    if (n != rb_r) { rb[rb_w] = byte; rb_w = n; }
}
static int rb_get(void) {
    if (rb_r == rb_w) return -1;
    uint8_t c = rb[rb_r]; rb_r = (rb_r + 1) % RB_SIZE; return c;
}

#define LINE_MAX 256
static char lb[LINE_MAX];
static int  li = 0;
static uint8_t prompt_seen = 0;

static int read_line(char *out, int max) {
    int c, len;
    while ((c = rb_get()) >= 0) {
        if (c == '>') {
            prompt_seen = 1;
            continue;
        }
        if (c == '\n') {
            lb[li] = '\0'; len = li; li = 0;
            if (len > 0 && lb[len-1] == '\r') lb[len-1] = '\0';
            if (len > 0) { strncpy(out, lb, max-1); out[max-1]='\0'; return 1; }
            continue;
        }
        if (li < (int)sizeof(lb)-1) lb[li++] = (char)c;
    }
    return 0;
}

/* ============================================================
 *  2. AT 引擎
 * ============================================================ */
static struct {
    char cmd[256], exp[32];
    uint32_t deadline;
    int8_t   res;
    uint8_t  sent;
} atq;

static void at_send(const char *cmd, const char *exp, uint32_t ms) {
    strncpy(atq.cmd, cmd, sizeof(atq.cmd)-1); atq.cmd[sizeof(atq.cmd)-1]='\0';
    strncpy(atq.exp, exp, sizeof(atq.exp)-1); atq.exp[sizeof(atq.exp)-1]='\0';
    atq.deadline = HAL_GetTick() + ms;
    atq.res = 0; atq.sent = 0;
}
static void at_clear(void) { memset(&atq, 0, sizeof(atq)); }

/* ============================================================
 *  3. 全局变量
 * ============================================================ */
float    g_esp_temp   = 25.0f;
float    g_esp_tds    = 200.0f;
uint16_t g_esp_light  = 3750;
uint8_t  g_esp_heater = 0;
uint8_t  g_esp_pump   = 0;
uint8_t  g_esp_run    = 0;
char     g_esp_page[20]="Menu";

/* 命令缓冲: 单槽, 新覆盖旧 */
static char cmd_latest = 0;
static volatile uint8_t cmd_pending = 0;

/* action 标志: App 端光标闪烁 */
uint8_t g_esp_action = 0;
uint8_t g_esp_sub    = 0;
uint8_t g_esp_lmode  = 0;

uint8_t ESP8266_CmdAvailable(void) {
    return cmd_pending;
}
char ESP8266_GetCmd(void) {
    if (!cmd_pending) return 0;
    char c = cmd_latest;
    cmd_latest = 0;
    cmd_pending = 0;
    return c;
}

/* ============================================================
 *  4. CIPSEND (单连接模式)
 * ============================================================ */
static uint8_t  cs_pending;
static uint16_t cs_len;
static uint8_t  cs_buf[512];
static uint8_t  cs_timeout_cnt;        /* 连续超时计数, >=3 触发自救 */

static void cs_start(uint16_t len, const uint8_t *data) {
    char c[32]; snprintf(c,sizeof(c),"AT+CIPSEND=0,%d",len);
    printf("[CS] start len=%d\r\n", len);
    /* 直接用 HAL_UART_Transmit 发送 AT+CIPSEND 命令 */
    HAL_UART_Transmit(&huart2, (uint8_t*)c, strlen(c), 5000);
    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 1000);
    at_send(c, ">", 5000);  /* 仍设置 atq 用于 cs_handle 等待 ">" */
    cs_pending = 1;
    cs_len = len;
    if (data) memcpy(cs_buf, data, len);
}

static void cs_handle(void) {
    if (!cs_pending) return;
    /* 超时保护 */
    if (atq.deadline && (int32_t)(HAL_GetTick() - atq.deadline) >= 0) {
        printf("[CS] timeout phase %d\r\n", cs_pending);
        cs_pending = 0; at_clear();
        if (++cs_timeout_cnt >= 3) {
            printf("[ESP] WATCHDOG: reset esp_ready\r\n");
            esp_ready = 0;
            cs_timeout_cnt = 0;
        }
        return;
    }
    if (cs_pending == 1) {
        if (prompt_seen || atq.res == 1) {
            prompt_seen = 0;
            printf("[CS] send %d bytes\r\n", cs_len);
            uint16_t i; for(i=0;i<cs_len;i++) tx_byte((char)cs_buf[i]);
            cs_pending = 2;
            atq.res = 0;
            atq.deadline = HAL_GetTick() + 2500;
            return;
        }
        if (atq.res == -1) {
            cs_pending = 0; at_clear();
            if (++cs_timeout_cnt >= 3) {
                printf("[ESP] WATCHDOG: reset esp_ready\r\n");
                esp_ready = 0;
                cs_timeout_cnt = 0;
            }
            return;
        }
        /* '>' 提示符不带 '\n', read_line 收不到, 直接扫原始字节 */
        int ch;
        while (0 && (ch = rb_get()) >= 0) {
            if (ch == '>') { atq.res = 1; break; }
        }
        if (0) {
            printf("[CS] send %d bytes\r\n", cs_len);
            uint16_t i; for(i=0;i<cs_len;i++) tx_byte((char)cs_buf[i]);
            cs_pending = 0; at_clear();   /* 发完就走, 不等 SEND OK */
            cs_timeout_cnt = 0;           /* 成功, 清零看门狗 */
        }
    } else if (cs_pending == 2) {
        if (atq.res == 1) {
            printf("[CS] done\r\n");
            cs_pending = 0; at_clear();
            cs_timeout_cnt = 0;
        } else if (atq.res == -1) {
            printf("[CS] send failed\r\n");
            cs_pending = 0; at_clear();
            if (++cs_timeout_cnt >= 3) {
                printf("[ESP] WATCHDOG: reset esp_ready\r\n");
                esp_ready = 0;
                cs_timeout_cnt = 0;
            }
        }
    }
}

/* ============================================================
 *  5. 运行时状态
 * ============================================================ */
uint8_t        esp_ready;
static uint32_t pub_next;
static uint8_t  client_ok;
static uint8_t  passthrough;

void ESP8266_SetPassthrough(uint8_t en) { passthrough = en; }
void ESP8266_ForcePublish(void)       { pub_next = HAL_GetTick(); }

/* ============================================================
 *  6. 非阻塞初始化状态机
 * ============================================================ */
enum {
    INIT_IDLE = 0,
    INIT_BOOT_WAIT,       /* 2s 等 ESP8266 启动 */
    INIT_AT,              /* AT */
    INIT_CIPSERVER_OFF,   /* AT+CIPSERVER=0 (cleanup, ignore err) */
    INIT_CIPCLOSE_0,      /* AT+CIPCLOSE=0 */
    INIT_CIPCLOSE_1,
    INIT_CIPCLOSE_2,
    INIT_CIPCLOSE_3,
    INIT_CIPCLOSE_4,
    INIT_CWMODE,          /* AT+CWMODE=2 */
    INIT_CWMODE_DLY,      /* 200ms delay */
    INIT_CIPMUX,          /* AT+CIPMUX=1 */
    INIT_CIPMUX_DLY,      /* 200ms delay */
    INIT_CWSAP,           /* AT+CWSAP=... */
    INIT_CWJAP,           /* AT+CWJAP=router */
    INIT_AP_WAIT,         /* 4s AP stable */
    INIT_CIPSERVER_ON,    /* AT+CIPSERVER=1,port */
    INIT_CIFSR,           /* AT+CIFSR, print AP IP */
    INIT_DONE = 100,
    INIT_FAIL = 255
};

static uint8_t  init_state = INIT_IDLE;
static uint32_t init_tick;
static char     init_cmd[128];

/* 发送一条 AT 命令 */
static void init_send(const char *cmd, const char *exp, uint32_t ms) {
    printf("[ESP] send: %s\r\n", cmd);
    HAL_UART_Transmit(&huart2, (uint8_t*)cmd, strlen(cmd), 5000);
    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 1000);
    at_send(cmd, exp, ms);
}

/* 检查 AT 响应: 0=未到, 1=匹配, -1=ERROR/超时 */
static int init_check(const char *exp) {
    char line[LINE_MAX];
    if ((int32_t)(HAL_GetTick() - atq.deadline) >= 0) return -1;
    while (read_line(line, sizeof(line))) {
        printf("%s\r\n", line);
        if (strstr(line, exp)) return 1;
        if (strstr(line, "ERROR")) return -1;
    }
    return 0;
}

/* 检查 AT 响应, 但 ERROR 不当作失败 (cleanup 命令用) */
static int init_check_ok_or_any(const char *exp) {
    char line[LINE_MAX];
    if ((int32_t)(HAL_GetTick() - atq.deadline) >= 0) return 1;
    while (read_line(line, sizeof(line))) {
        printf("%s\r\n", line);
        if (strstr(line, exp) || strstr(line, "ERROR")) return 1;
    }
    return 0;
}

/* ---- 非阻塞初始化 API ---- */

void ESP8266_InitStart(void) {
    if (init_state != INIT_IDLE && init_state != INIT_FAIL) return;
    printf("[ESP] Non-blocking init start\r\n");
    at_clear();
    prompt_seen = 0;
    cs_pending = 0;
    cs_timeout_cnt = 0;
    passthrough = 0;
    client_ok  = 0;
    cmd_latest = 0;
    cmd_pending= 0;
    /* 清空残留 */
    { char d[128]; while (read_line(d, sizeof(d))) {} }
    init_state = INIT_BOOT_WAIT;
    init_tick  = HAL_GetTick();
}

uint8_t ESP8266_InitPoll(void) {
    int r;

    switch (init_state) {
    case INIT_IDLE:
        return 0;

    case INIT_FAIL:
        init_state = INIT_IDLE;
        printf("[ESP] Init result: FAIL\r\n");
        return 2;

    case INIT_DONE:
        init_state = INIT_IDLE;
        pub_next = HAL_GetTick() + TCP_PUB_INTERVAL;
        printf("[ESP] Init result: OK! Server on :%d\r\n", TCP_SERVER_PORT);
        return 1;

    /* ---- 延迟状态 ---- */
    case INIT_BOOT_WAIT:
        if (HAL_GetTick() - init_tick >= 2000) {
            init_state = INIT_AT;
            init_send("AT", "OK", 3000);
        }
        break;

    case INIT_CWMODE_DLY:
        if (HAL_GetTick() - init_tick >= 200) {
            init_state = INIT_CIPMUX;
            init_send("AT+CIPMUX=1", "OK", 3000);
        }
        break;

    case INIT_CIPMUX_DLY:
        if (HAL_GetTick() - init_tick >= 200) {
            init_state = INIT_CWSAP;
            snprintf(init_cmd, sizeof(init_cmd),
                     "AT+CWSAP=\"%s\",\"%s\",1,3", WIFI_SSID, WIFI_PASSWORD);
            init_send(init_cmd, "OK", 8000);
        }
        break;

    case INIT_AP_WAIT:
        if (HAL_GetTick() - init_tick >= 4000) {
            init_state = INIT_CIPSERVER_ON;
            snprintf(init_cmd, sizeof(init_cmd),
                     "AT+CIPSERVER=1,%d", TCP_SERVER_PORT);
            init_send(init_cmd, "OK", 8000);
        }
        break;

    /* ---- 关键 AT 命令 (ERROR = 失败) ---- */
    case INIT_AT:
        r = init_check("OK");
        if (r == 1)      { init_state = INIT_CIPSERVER_OFF; goto send_cleanup; }
        else if (r == -1) goto init_fail;
        break;

    case INIT_CWMODE:
        r = init_check("OK");
        if (r == 1)      { init_state = INIT_CWMODE_DLY; init_tick = HAL_GetTick(); }
        else if (r == -1) goto init_fail;
        break;

    case INIT_CIPMUX:
        r = init_check("OK");
        if (r == 1)      { init_state = INIT_CIPMUX_DLY; init_tick = HAL_GetTick(); }
        else if (r == -1) goto init_fail;
        break;

    case INIT_CWSAP:
        r = init_check("OK");
        if (r == 1)      { init_state = INIT_CWJAP;
                           snprintf(init_cmd, sizeof(init_cmd),
                                    "AT+CWJAP=\"%s\",\"%s\"", WIFI_STA_SSID, WIFI_STA_PASSWORD);
                           init_send(init_cmd, "OK", 20000); }
        else if (r == -1) goto init_fail;
        break;

    case INIT_CWJAP:
        r = init_check("OK");
        if (r == 1)      { init_state = INIT_AP_WAIT; init_tick = HAL_GetTick();
                           printf("[ESP] Router joined, wait net stable 4s...\r\n"); }
        else if (r == -1){ init_state = INIT_AP_WAIT; init_tick = HAL_GetTick();
                           printf("[ESP] Router join failed, AP fallback only\r\n"); }
        break;

    case INIT_CIPSERVER_ON:
        r = init_check("OK");
        if (r == 1)      { init_state = INIT_CIFSR; init_send("AT+CIFSR","OK",3000); }
        else if (r == -1) goto init_fail;
        break;

    case INIT_CIFSR:
        r = init_check("OK");
        if (r == 1)      { init_state = INIT_DONE; }
        else if (r == -1) goto init_fail;
        break;

    /* ---- cleanup 命令 (ERROR 不致命) ---- */
    case INIT_CIPSERVER_OFF:
        if (init_check_ok_or_any("OK")) { init_state = INIT_CIPCLOSE_0; goto send_cleanup; }
        break;
    case INIT_CIPCLOSE_0:
        if (init_check_ok_or_any("OK")) { init_state = INIT_CIPCLOSE_1; goto send_cleanup; }
        break;
    case INIT_CIPCLOSE_1:
        if (init_check_ok_or_any("OK")) { init_state = INIT_CIPCLOSE_2; goto send_cleanup; }
        break;
    case INIT_CIPCLOSE_2:
        if (init_check_ok_or_any("OK")) { init_state = INIT_CIPCLOSE_3; goto send_cleanup; }
        break;
    case INIT_CIPCLOSE_3:
        if (init_check_ok_or_any("OK")) { init_state = INIT_CIPCLOSE_4; goto send_cleanup; }
        break;
    case INIT_CIPCLOSE_4:
        if (init_check_ok_or_any("OK")) { init_state = INIT_CWMODE; init_send("AT+CWMODE=3","OK",3000); }
        break;
    }

    return 0;

send_cleanup:
    switch (init_state) {
    case INIT_CIPSERVER_OFF: init_send("AT+CIPSERVER=0","OK",3000); break;
    case INIT_CIPCLOSE_0:    init_send("AT+CIPCLOSE=0","OK",3000); break;
    case INIT_CIPCLOSE_1:    init_send("AT+CIPCLOSE=1","OK",3000); break;
    case INIT_CIPCLOSE_2:    init_send("AT+CIPCLOSE=2","OK",3000); break;
    case INIT_CIPCLOSE_3:    init_send("AT+CIPCLOSE=3","OK",3000); break;
    case INIT_CIPCLOSE_4:    init_send("AT+CIPCLOSE=4","OK",3000); break;
    }
    return 0;

init_fail:
    printf("[ESP] Init FAIL at state %d\r\n", init_state);
    init_state = INIT_FAIL;
    return 2;
}

/* ============================================================
 *  7. 运行时处理
 * ============================================================ */
void ESP8266_Process(void) {
    char line[LINE_MAX];
    while (read_line(line, sizeof(line))) {
        printf("[ESP] %s\r\n", line);   /* AT traffic visible on USART1 */
        if (passthrough) continue;

        /* 客户端接入/断开 */
        if (strstr(line, ",CONNECT")) { client_ok=1; printf("[TCP] Client connected\r\n"); }
        if (strstr(line, ",CLOSED"))  { client_ok=0; printf("[TCP] Connection closed\r\n"); }

        /* +IPD,<len>:<data> */
        {
            const char *ipd = strstr(line, "+IPD,");
            if (ipd) {
                const char *pa = ipd+5;
                const char *col = strchr(pa, ':');
                if (col) {
                    const char *ds = col;
                    while (ds > pa && *(ds-1) != ',') ds--;
                    int dlen = atoi(ds);
                    if (dlen > 0) {
                        printf("[TCP] +IPD %d bytes\r\n", dlen);
                        const char *data = col + 1;
                        /* 只取最后一字节, 新覆盖旧 */
                        cmd_latest = data[dlen - 1];
                        cmd_pending = 1;
                        printf("[TCP] cmd: %c\r\n", cmd_latest);
                    }
                }
            }
        }

        /* AT 响应匹配 (CIPSEND 阶段) */
        if (cs_pending == 2 && atq.res == 0) {
            if (strstr(line, "SEND OK")) {
                atq.res = 1;
            } else if (strstr(line, "SEND FAIL") || strstr(line, "ERROR")) {
                atq.res = -1;
            }
        }
        if (cs_pending == 1 && atq.res==0) {
            if (strstr(line, atq.exp))       atq.res = 1;
            else if (atq.exp[0]=='>' && strstr(line,"OK")) atq.res = 1;
            else if (strstr(line, "ERROR"))  atq.res = -1;
        }

        cs_handle();
    }

    if (passthrough) return;

    /* ===== 定时发布 ===== */
    if (client_ok && (int32_t)(HAL_GetTick()-pub_next)>=0 && !cs_pending) {
        char json[320];
        uint8_t cursor = 0xFF; /* 255 = no cursor */
        switch (current_page) {
            case PAGE_MENU:
            case PAGE_FEED_SUBMENU: cursor = menu_index; break;
            case PAGE_FEED_TRAIN:   cursor = train_cursor; break;
            case PAGE_FEED_MANUAL:  cursor = feed_cursor; break;
            case PAGE_TRAIN_SETTING: cursor = train_setting_cursor; break;
            case PAGE_AUTO:         cursor = auto_cursor; break;
            case PAGE_LIGHT:        cursor = light_cursor; break;
            case PAGE_TEMP:         cursor = temp_editing; break;
            case PAGE_PUMP:         cursor = pump_cursor; g_esp_sub = pump_sub; break;
            default: g_esp_sub = 0; break;
        }
        snprintf(json,sizeof(json),
            "{\"temp\":%.1f,\"tds\":%.0f,\"led\":%u,\"lmode\":%u,\"heater\":%u,\"pump\":%u,\"run\":%u,\"page\":\"%s\",\"cursor\":%u,\"sub\":%u,\"action\":%u}\r\n",
            g_esp_temp,g_esp_tds,g_esp_light,g_esp_lmode,g_esp_heater,g_esp_pump,g_esp_run,g_esp_page,
            cursor, g_esp_sub, g_esp_action);
        printf("[PUB] %s\r\n", json);
        cs_start((uint16_t)strlen(json), (const uint8_t*)json);
        pub_next=HAL_GetTick()+TCP_PUB_INTERVAL;
        g_esp_action = 0;  /* 发送后清除 */
    }

    cs_handle();
}

void ESP8266_Send(const char *d, int l) { (void)d; (void)l; }
