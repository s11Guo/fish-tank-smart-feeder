# Fish Tank Smart Feeder - 智能鱼缸训饵系统

基于 STM32F405 的智能鱼缸训饵系统，通过双电机 7 阶段比例投喂实现幼鱼平稳换料，集成温度、TDS、光照三路传感器，支持手机 App 远程控制、OLED 本地显示、ASRPRO 语音命令。

## 功能特性

- **双电机比例训饵**：7 阶段 BO:AO 从 7:1 到 1:7 渐进过渡，脉冲搅拌防结块
- **三路传感器**：DS18B20 温度 (10s 均值)、TDS 水质 (四档)、光敏 Plan B 自动补光
- **三通道交互**：物理按键 + OLED 128x64 + Flutter 手机 App (WiFi TCP) + ASRPRO 语音
- **WiFi 双模**：AP+STA，同时提供热点和路由器接入，CIPSEND 两阶段可靠传输
- **容错机制**：ESP8266 看门狗自恢复、OLED 强制刷新、断线自动重连

## 硬件

- 主控：STM32F405RGT6 @ 168MHz
- 显示：SH1106 OLED 128×64 (I2C 0x3C)
- WiFi：ESP8266 (USART2 115200)
- 语音：ASRPRO (USART3 9600)
- 传感器：DS18B20 (1-Wire)、TDS (ADC)、光敏电阻 (ADC)
- 执行器：双电机 PWM、LED PWM、加热棒、双水泵
- 四个物理按键 (U/D/E/B)

## 目录结构

```
├── Core/               STM32 固件源码 (.c/.h)
│   ├── Src/            业务代码 (main, ESP8266, ui, feed, temp, light, ...)
│   └── Inc/            头文件
├── Drivers/            STM32 HAL 库 (CubeMX 生成)
├── MDK-ARM/            Keil MDK v5 工程
│   └── fish.uvprojx
├── App/                Flutter 手机 App
│   ├── lib/main.dart   单文件 658 行
│   └── pubspec.yaml
├── tools/
│   └── app_sim.py      PC 端 TCP 测试脚本
└── README.md
```

## 编译与烧录

**STM32 固件：**
1. 用 Keil MDK v5 打开 `MDK-ARM/fish.uvprojx`
2. 编译器：ARMCC V5.06
3. Build (F7)，0 Error 0 Warning
4. ST-Link 烧录

**Flutter App：**
```bash
cd App
flutter pub get
flutter run
```
App 连接地址：`192.168.5.7:9000` (STA 模式) 或 `192.168.4.1:9000` (AP 模式)

**测试脚本：**
```bash
pip install pyserial  # 可选，用于串口监控
python tools/app_sim.py --host 192.168.5.7 --port 9000 --loops 1
```

## WiFi 配置

| 项目 | 参数 |
|------|------|
| AP 热点 | FishTank_STM32 / 12345678 |
| AP IP | 192.168.4.1 |
| STA 连接 | ZTE-c6Ts6b / 20070319qq |
| STA IP | DHCP 分配 (当前 192.168.5.7) |
| TCP 端口 | 9000 |

修改 WiFi 配置：编辑 `Core/Inc/ESP8266.h` 中的 `WIFI_SSID`、`WIFI_STA_SSID` 等宏。

## App 通信协议

**STM32 → App (JSON, 每 1000ms 发布)：**
```json
{"temp":25.0,"tds":200,"led":50,"lmode":0,"heater":0,"pump":0,"run":0,"page":"Menu","cursor":0,"sub":0,"action":0}
```

**App → STM32 (TCP 单字符)：**
`u` = 上, `d` = 下, `e` = 确认, `b` = 返回, `r` = 刷新

## 致谢

本项目为嵌入式系统课程设计作品。
