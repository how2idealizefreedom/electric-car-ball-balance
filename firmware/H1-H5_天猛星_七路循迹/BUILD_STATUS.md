# 构建状态

- 工程：`H1_H5_TIANMENG_7LINE`
- 基线：`H1_H5_TIANMENG`
- 目标：立创·天猛星 MSPM0G3507，LQFP-64(PM)
- MSPM0 SDK：2.11.00.07
- SysConfig：1.26.2+4477
- TI Arm Clang：4.0.4.LTS
- 日期：2026-08-01

## 结果

- SysConfig 独立验证：通过。
- 七路生成宏：S1、S2、S3、S4、S5、S7、S8 均存在，S6/PB26 不存在。
- TI Arm Clang 全部源文件编译和链接：通过，无编译器诊断。
- Flash：`0x49C8 / 0x20000`。
- SRAM：`0x8EB / 0x8000`。
- 输出：`Debug/H1_H5_TIANMENG_7LINE.out`。

SysConfig 仅报告 CONTROL_TICK/TIMA0 在 STOP/STANDBY 不保留寄存器；程序只使用 `__WFI()`，没有进入 STOP/STANDBY。

CCS headless 首次导入在本机服务端超时，因此 Debug 目录没有 CCS 自动生成的 makefile；当前 `.out` 使用工程声明的同一 SysConfig、SDK 和 TI Arm Clang 命令行完成。用户在 CCS 中首次导入并 Build Project 后，CCS 会生成自己的构建文件。构建成功不代表七路传感器实车循迹已经验证成功。
