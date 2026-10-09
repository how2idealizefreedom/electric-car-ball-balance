# 天猛星线路记忆与步进电机首次测试

严格按 A→B→C→D 顺序进行。任一阶段不符合预期就停止，不要同时排查车辆和摆杆。

## A. 只上 MCU 电，执行器均断电

1. 拔下扩展板红色 TB6612；D24A 的 12 V 与 A4988 的 VMOT 都先断开。
2. CCS 导入 `H2_TIANMENG_MEMORY_STEPPER`，执行 Clean/Build。确认输出名正好是 `H2_TIANMENG_MEMORY_STEPPER.out`。
3. 烧录后打开板载 CH340：115200、8N1。
4. 应看到：

```text
H2 TIANMENG MEMORY+P1+A4988 - SDK 2.11.00.07
K1 START/STOP; K2 LEFT/JOG-; K3 RIGHT/JOG+; K4 MODE
A4988 DEFAULT DISABLED; D24A DEFAULT DISABLED
MODE,TEACH,MEM=0,VALID=0
```

5. PB22 慢闪；万用表测 PB15 对 GND 约 0 V，PA25 对 GND约 3.3 V。
6. 白底应为 `M=0`，中央 S4/S5 压黑线应为 `M=24`。
7. 按 K4，串口在 `MODE,TEACH` 与 `MODE,REPLAY` 间切换。无路线时即使选 REPLAY，按 K1 也会打印 `WARN,NO_MEMORY,FALLBACK_TEACH`。

## B. 只验证 A4988，D24A 仍断电

1. 按 `A4988_WIRING_AND_TEST.md` 调好 VREF；断电后再接 42BYGH23。
2. 架空连杆、清空整个运动范围，再接通 A4988 的 12 V。
3. K2 每按一次目标减 20 微步，K3 每按一次目标加 20 微步；串口分别打印 `A4988,JOG,-20` 或 `+20`。
4. K2、K3 同时按应打印 `A4988,DISABLED`，电机失去保持力。
5. 如果方向与机构定义相反，断电后交换同一组线圈的 `1A/1B`，不要交换两组线圈，也不要先改程序。
6. 临时把 PB8 与 GND 短接，确认只能阻止负方向继续越过零位。正式装车前必须增加可靠 HOME。

## C. 车辆示教圈，A4988 断开 VMOT

1. 架空四轮，接通 D24A，按 K1；四轮都必须朝车头方向。再次按 K1必须进入 `STATE,FAULT,BUTTON_STOP` 且立即停轮。
2. 落地，把车准确放在 A 点。K4 选 `MODE,TEACH`。
3. 按 K1 启动。直线和能自行通过的弯道不要按 K2/K3。
4. 车头需要向左但 P1 转向不足时按住 K2；需要向右时按住 K3。松开后恢复 P1。人工修正会一起记录。
5. 返回 A 点时应出现：

```text
MEM,SAVED,N=...
STATE,BRAKE,T=...
STATE,FINISHED,T=...
MODE,REPLAY,MEM=...,VALID=1
```

如果中途因 K1 停车或 `LINE_LOST` 故障，本次路线会作废，必须重新 TEACH。

## D. 路线回放圈

1. 不复位、不重新上电；RAM 路线在复位/断电后会丢失。
2. 车辆重新对准 A 点，确认串口为 `MODE,REPLAY`。
3. 按 K1。此时记忆的左右轮 PWM 是主控制，灰度误差只在 ±60 范围内辅助纠偏。
4. 串口 `MODE=R`，`MEM=当前索引/总样本数` 应持续增加。短暂 `M=0` 时车辆仍按记忆走；连续丢线 1.5 s 才进入故障。
5. 若回放整体提前/滞后，先记录完整串口、实际圈时、电池电压和冲出位置，不要直接改 P1。下一步应加入编码器速度闭环或 MPU6050 航向对齐，而不是继续扩大灰度纠偏。

## 当前限制

- 时间回放要求每次从同一 A 点、同一方向和接近的电池电压启动。
- 尚未使用编码器与 MPU6050 校正路程/航向。
- 路线没有写入 Flash；稳定后再做串口导出和固化，避免调试期间频繁擦写片内 Flash。
- 步进点动只是机构与接线调试，不是钢球闭环控制。K230 位置数据、HOME 和摆杆几何标定完成后再接入球控状态机。
