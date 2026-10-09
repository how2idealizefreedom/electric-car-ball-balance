# 地猛星 / 天猛星双 CCS 工程构建状态

日期：2026-07-31

## 地猛星

- 工程：`H2_LINE_BALL_STEPPER_DIMENG`
- 器件/封装：MSPM0G3507，LQFP-48(PT)
- SysConfig：1.26.2，通过
- SDK：MSPM0 SDK 2.11.00.07
- 编译器：TI Arm Clang 4.0.4 LTS
- 完整 clean build：通过
- 输出：`Debug/H2_LINE_BALL_STEPPER_DIMENG.out`

## 天猛星

- 工程：`H2_LINE_BALL_STEPPER_TIANMENG`
- 器件/封装：MSPM0G3507，LQFP-64(PM)
- SysConfig：1.26.2，通过
- SDK：MSPM0 SDK 2.11.00.07
- 编译器：TI Arm Clang 4.0.4 LTS
- `main.c`、`mpu6050.c`、SysConfig 生成代码、启动代码的编译及最终链接：通过
- 输出：`Debug/H2_LINE_BALL_STEPPER_TIANMENG.out`

## 提示与验证边界

SysConfig 仅提示 CONTROL_TICK 在 STOP/STANDBY 中不保持寄存器。本固件不进入这两种低功耗模式，不影响当前测试。

本次未烧录，也未宣称步进电机实物已经验证。烧录前必须先确认 A4988 采样电阻、VREF、线圈配对、100 uF VMOT 电容、共地和机构架空。
