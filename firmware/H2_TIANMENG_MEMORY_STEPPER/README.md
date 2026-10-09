# H2_TIANMENG_MEMORY_STEPPER

立创·天猛星 MSPM0G3507（LQFP-64）一体化 CCS 工程，使用外接 D24A 驱动四轮，保留已经验证过的 P1 灰度循迹，并增加：

- 首圈示教：P1 控制车辆，K2/K3 可在困难弯道人工追加左/右转向。
- 路线记忆：每 40 ms 记录一次左右轮最终 PWM，RAM 最多保存 48 s。
- 路线回放：记忆 PWM 为主，灰度 P1 只做 ±60 的小幅纠偏；短时丢线仍按记忆继续。
- 42BYGH23-A-21DH + A4988：车辆待机时可用 K2/K3 进行 ±20 微步点动；车辆启动前自动禁用步进驱动。

当前路线只保存在 RAM，复位或断电后会丢失。先验证示教/回放能稳定完成一圈，再把实测路线导出并固化，之后再融合 MPU6050 航向和编码器速度以抵抗电池电压变化。

工程入口和资料：

- `empty.syscfg`：唯一引脚/外设配置源。
- `main.c`：车辆状态机、P1、示教回放、按键与遥测。
- `track_memory.c/.h`：路线 RAM。
- `a4988.h`：带加减速、位置限幅和 HOME 保护的步进驱动。
- `TIANMENG_WIRING_AND_TEST.md`：整车接线。
- `MEMORY_STEPPER_TEST.md`：严格分阶段的首次测试步骤。

导入 CCS 后工程名为 `H2_TIANMENG_MEMORY_STEPPER`。不要烧录目录中旧名称的 `.out`；本次验证产物位于 `verified_build/H2_TIANMENG_MEMORY_STEPPER.out`。
