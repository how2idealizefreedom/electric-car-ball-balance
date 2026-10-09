# P0 构建状态（2026-07-29）

- 板型已纠正并锁定：立创·地猛星 MSPM0G3507，`LQFP-48(PT)`；板载用户 LED 为 `PA14`，安全 STBY 保持 `PB24`（H3-7）。旧的天猛星/`LQFP-64(PM)`/`PB22` 配置不再有效。
- 静态工程检查：通过；识别 SDK `2.11.0.07`、SysConfig `1.26.2`、XDS110 目标配置，以及 `PA14`、`PB24`、`PA10/PA11` 管脚分配。
- SysConfig CLI：通过；使用 `C:\TI\mspm0_sdk_2_11_00_07` 对 `LQFP-48(PT)` 配置生成 6 个文件，无错误、无管脚冲突。
- CCS 工程原生构建：通过；已执行 `Debug clean all`，SysConfig 重新生成后由 TI Arm Clang `4.0.4.LTS` 成功编译和链接 `Debug/H2_LINE_CAR.out`。
- 生成头文件复核：`STATUS_LED_PORT=GPIOA`、`STATUS_LED_LED_PIN=DL_GPIO_PIN_14`；`DRV_SAFE_PORT=GPIOB`、`DRV_SAFE_STBY_PIN=DL_GPIO_PIN_24`。
- 烧录：未执行。当前阶段尚未授权/完成安全控制线连接，也没有必要让电机板带电。

下一步在 CCS 图形界面对已导入工程执行一次 `Clean Project` 和 `Build Project`，再下载新生成的 `Debug/H2_LINE_CAR.out`。运行后应观察 PA14 板载 LED 闪烁，同时 PB24 对 GND 约为 0 V。

## P1 循迹构建（2026-07-29）

- P0 四项已由用户实板确认通过。
- SysConfig 增加：PA12/PA13 TIMG0 双路 20 kHz PWM、PA15/PA16/PA17/PA22 电机方向、八路灰度输入、PA24 上拉按键、TIMA0 5 ms 周期中断。
- 独立 SysConfig CLI 验证通过；唯一附加输出是 TIMER 在 STOP/STANDBY 不保留寄存器的 `info`。本固件不进入 STOP/STANDBY，不影响当前运行。
- CCS 工程原生 `Debug clean all` 通过；TI Arm Clang 编译和链接无 error，生成新的 `Debug/H2_LINE_CAR.out`。
- 应用已包含：复位安全禁能、按键启动/急停、左右分组电机、八灰度加权 PD、400 ms 丢线故障、起点横线屏蔽、返回 A 点短刹、CH340 遥测。
- 硬件行为尚未验证：灰度 S1～S8 位序、四轮实际正方向、3.3 V 到 D24A 的逻辑接口和首轮循迹参数必须按 `P1_WIRING_AND_TEST.md` 实测。
- 首次架空结果：CH340 COM5 与启动状态机正常；前轮 B/C 向前而后轮 A/D 向后，判定为两只后轮电机动力极性相反，需要交换 Motor A、Motor D 各自两根电机动力线。左右分组控制逻辑保持不变。
- 根据空载观感，初始直行 PWM 从 35% 下调到 22%，最大限制暂设 35%；修改后再次 `Debug clean all` 编译链接通过。
- 灰度极性最终以逐项白底/黑线直接测试为准：白底低、黑线高，固件不取反。先前依据运动过程 `M=231` 反推成白底高的判断已撤销。
- 后轮不便交换动力线时，方向扇出改为交叉接法：PA15→BIN1+AIN2、PA16→BIN2+AIN1、PA17→CIN1+DIN2、PA22→CIN2+DIN1。修改后的固件再次 clean all 通过。
