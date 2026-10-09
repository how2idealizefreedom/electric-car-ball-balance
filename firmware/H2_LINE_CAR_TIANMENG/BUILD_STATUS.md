# 构建状态

- 目标：立创·天猛星 MSPM0G3507，LQFP-64(PM)，MSPM0 SDK 2.11.00.07，TI Arm Clang 4.0.4.LTS。
- `check_syscfg.py`：通过；识别 CCS/XDS110、64 引脚封装和全部管脚分配。
- `run_sysconfig.py`：通过；GPIO、TIMG0 双路 PWM、TIMA0 5 ms 控制周期、UART0、I2C0 均无资源冲突。
- 稳定 P1 主程序：编译和链接通过。
- `mpu6050.c`：独立编译通过；将启动探测打开后，与主程序一起链接通过。
- 可烧录文件：`Debug/H2_LINE_CAR_TIANMENG.out`。该文件默认关闭 MPU6050 启动探测，适合先做循迹硬件验证。

唯一 SysConfig 提示为 TIMA0 在 STOP/STANDBY 低功耗模式下不保持寄存器。本程序不进入这两种低功耗模式，因此不影响当前运行。

CCS 20.5 无界面工程导入服务在本机启动超时，所以交付目录没有复制临时生成的 makefile；已用同一 SysConfig、SDK 和 TI 编译/链接器完成等价构建验证。首次在 CCS 导入后执行 `Clean Project`、`Build Project`，CCS 会生成该工程自己的 Debug 构建文件。

尚未烧录或实车验证天猛星版本。烧录前必须按 `TIANMENG_WIRING_AND_TEST.md` 架空车轮并确认 PB15/STBY 的待机电压为 0 V。
