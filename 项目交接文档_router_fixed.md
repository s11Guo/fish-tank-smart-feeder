# Fish Tank 智能鱼缸系统交接文档

整理时间：2026-06-28

## 当前可用版本

本版本已经通过 ST-Link 烧录到 STM32F405，并在电脑端完成 TCP/App 模拟测试。

当前推荐连接方式：

- 手机和电脑连接路由器 WiFi：`ZTE-c6Ts6b`
- ESP8266 通过 STA 模式加入同一路由器
- App/TCP 客户端连接：`192.168.5.7:9000`
- 备用设备热点：`FishTank_STM32`
- 备用热点密码：`12345678`
- 备用热点 IP：`192.168.4.1`
- TCP 端口：`9000`

注意：`192.168.5.7` 是本次调试时路由器分配给 ESP8266 的地址。如果路由器重启或 DHCP 重新分配，串口日志中查看：

```text
+CIFSR:STAIP,"192.168.5.x"
```

App 端需要连接这个新的 `192.168.5.x:9000`。

## 本次关键修改

### 1. WiFi 改为 AP+STA

文件：

- `Core/Inc/ESP8266.h`
- `Core/Src/ESP8266.c`

修改内容：

- 保留备用热点 `FishTank_STM32`
- 新增路由器 STA 配置：

```c
#define WIFI_STA_SSID     "ZTE-c6Ts6b"
#define WIFI_STA_PASSWORD "20070319qq"
```

- 初始化流程从纯 AP 改为 AP+STA：

```text
AT+CWMODE=3
AT+CWSAP="FishTank_STM32","12345678",1,3
AT+CWJAP="ZTE-c6Ts6b","20070319qq"
AT+CIPSERVER=1,9000
AT+CIFSR
```

串口启动成功日志应包含：

```text
WIFI CONNECTED
WIFI GOT IP
+CIFSR:STAIP,"192.168.5.7"
[ESP] Init result: OK! Server on :9000
```

### 2. 修复 CIPSEND 卡死和超时

文件：

- `Core/Src/ESP8266.c`

问题表现：

- App 高频发送按键时，ESP8266 回包 `+IPD` 和 `AT+CIPSEND` 可能混在一起
- 旧代码直接扫描原始字节，可能吞掉 ESP 回包
- 旧代码“发完就走”，没有真正等待 `SEND OK`
- 会出现 `CIPSEND timeout`、`WATCHDOG reset esp_ready`、App 收不到 JSON

修复内容：

- 单独记录 `>` 提示符：

```c
static uint8_t prompt_seen = 0;
```

- `read_line()` 遇到 `>` 时不再把它混入普通行：

```c
if (c == '>') {
    prompt_seen = 1;
    continue;
}
```

- CIPSEND 分成两个阶段：

```text
phase 1：等待 >
phase 2：发送 JSON 后等待 SEND OK / SEND FAIL
```

- 只在 `cs_pending == 1` 时匹配 `>`，只在 `cs_pending == 2` 时匹配 `SEND OK`

修复后完整 `app_sim.py` 44 条命令测试全部返回 OK，没有 timeout。

### 3. OLED/I2C 稳定性保留修复

文件：

- `Core/Src/oled.c`
- `Core/Src/i2c.c`

当前保留的修复：

- `OLED_Refresh()` 不再在刷新前发送 `0xAE` 关闭屏幕，避免 I2C 中途失败导致屏幕保持黑屏
- OLED I2C 写命令/数据返回 `HAL_StatusTypeDef`
- I2C PB6/PB7 使用内部上拉：

```c
GPIO_InitStruct.Pull = GPIO_PULLUP;
```

## 编译和烧录

工程路径：

```text
MDK-ARM/fish.uvprojx
```

本机已验证工具链：

```text
D:\keil5\UV4\UV4.exe
D:\keil5\ARM\ARMCC\bin\fromelf.exe
C:\Users\24678\.platformio\packages\tool-stm32duino\texane-stlink\st-flash.exe
```

命令行编译：

```powershell
cd C:\Users\24678\Documents\fish\stm32_fish_debug\MDK-ARM
D:\keil5\UV4\UV4.exe -b fish.uvprojx -j0 -o build.log
```

生成 bin：

```powershell
D:\keil5\ARM\ARMCC\bin\fromelf.exe --bin --output fish\fish.bin fish\fish.axf
```

烧录：

```powershell
C:\Users\24678\.platformio\packages\tool-stm32duino\texane-stlink\st-flash.exe write fish\fish.bin 0x08000000
```

本次最终构建结果：

```text
0 Error(s), 0 Warning(s)
Flash written and verified
```

## 测试方式

电脑保持连接 `ZTE-c6Ts6b`，运行：

```powershell
python C:\Users\24678\Documents\fish\stm32_fish_debug\app_sim.py --host 192.168.5.7 --port 9000 --loops 1
```

本次验证结果：

```text
TCP connected
44 command tests
全部 OK
无 TIMEOUT
```

串口：

```text
COM3
115200 baud
```

串口可看到：

```text
[TCP] Client connected
[TCP] cmd: b/u/d/e/r
[PUB] {...json...}
[CS] send xxx bytes
[ESP] SEND OK
[CS] done
```

## App 使用说明

如果 App 可以填写地址：

```text
IP: 192.168.5.7
Port: 9000
```

如果 App 写死地址不能改，需要二选一：

1. 修改 App，让它连接 `192.168.5.7:9000`
2. 手机改连备用热点 `FishTank_STM32`，App 连接 `192.168.4.1:9000`

当前固件同时支持这两种网络路径。

## 文件说明

```text
Core/                  STM32 用户代码
Drivers/               STM32 HAL/CMSIS 依赖
MDK-ARM/fish.uvprojx   Keil 工程
MDK-ARM/fish/fish.hex  当前构建产物
MDK-ARM/fish/fish.bin  当前烧录产物
app_sim.py             电脑端 App 模拟测试脚本
项目交接文档_router_fixed.md  本文档
```

## 当前结论

当前版本已经解决：

- 电脑不能长时间连接设备热点导致调试中断的问题
- `FishTank` 名称和路由器/旧热点冲突的问题
- ESP8266 `CIPSEND` 发送 JSON 后偶发卡死的问题
- App 模拟测试中命令 timeout 的问题
- OLED 刷新中途失败可能黑屏的问题

当前推荐交付给 App 端的信息：

```text
同一路由器 WiFi: ZTE-c6Ts6b
设备 IP: 192.168.5.7
端口: 9000
备用热点: FishTank_STM32 / 12345678
备用热点地址: 192.168.4.1:9000
```
