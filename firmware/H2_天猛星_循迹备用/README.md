# H2_LINE_CAR_TIANMENG

这是立创·天猛星 MSPM0G3507（LQFP-64）备用工程。控制算法逐行继承地猛星已经实车验证稳定的 P1 版本，只迁移板级管脚，不包含后来导致突然加速、向外冲线的斜坡和激进丢线搜索改动。

主要资源：

- 板载 LED：PB22；板载按键：PB21；CH340：PA10/PA11。
- D24A：PA12/PA13 两组 20 kHz PWM，PB17/PB19 与 PA16/PB24 四个方向信号，PB15 安全 STBY。
- 八路灰度：PB5、PB16、PB12、PB13、PB23、PB26、PB27、PB25，顺序为车头朝前时从左到右 S1～S8。
- MPU6050：扩展板固定 PA0=SDA、PA1=SCL，地址 0x68；底层驱动已加入，但默认不在启动时探测，不影响今晚循迹。

请先阅读 `TIANMENG_WIRING_AND_TEST.md`。此工程和 `H2_LINE_CAR` 相互独立，导入 CCS 时工程名为 `H2_LINE_CAR_TIANMENG`。
