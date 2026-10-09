# 构建状态

- 工程：`H1_H5_TIANMENG`
- 目标：立创·天猛星 MSPM0G3507，LQFP-64(PM)
- CCS：20.5.0
- TI Arm Clang：4.0.4.LTS
- MSPM0 SDK：2.11.00.07
- SysConfig：1.26.2+4477
- 构建日期：2026-08-01（Asia/Shanghai）

## 结果

- SysConfig 静态检查：通过
- SysConfig 隔离生成：通过
- C 源文件编译：通过，无诊断
- 链接：通过
- Flash 使用：0x46F8 / 0x20000
- SRAM 使用：0x8E1 / 0x8000
- 输出：`Debug/H1_H5_TIANMENG.out`

本次构建包含 SSD1306 后端、扩展板模式键、VGA OpenMV帧兼容解析和PB3 HOME预置状态机：OLED为I2C0 PA28/SDA、PA31/SCL、400 kHz；逻辑K4为扩展板KEY2/PB11；球位置输入为UART1 PB7/RX、PB6/TX、115200 8N1。

SysConfig 报告 CONTROL_TICK/TIMA0 在 STOP/STANDBY 不保持寄存器。本程序仅使用 `__WFI()`，没有请求 STOP/STANDBY。

OLED、PB11模式键和A4988点动已有用户实板正反馈。新增OpenMV标签帧解析、`HOME/HST`遥测和自动回零状态机仅完成构建，尚未烧录或实板验证。`A4988_AUTO_HOME_READY=0`，本构建不会上电自动运动。SysConfig仅报告CONTROL_TICK在STOP/STANDBY不保持寄存器；程序只使用`__WFI()`，该信息不影响本次构建结论。构建成功不代表新增功能硬件验证成功。
