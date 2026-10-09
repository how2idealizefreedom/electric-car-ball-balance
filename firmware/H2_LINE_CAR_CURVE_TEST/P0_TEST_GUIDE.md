# H2_LINE_CAR：P0 首次验证指南

本轮只验证 MCU 工具链和安全固件，不验证电机、编码器、灰度或循迹算法。

## 0. 测试成功的定义

以下四项全部成立才算 P0 通过：

1. CCS 能导入 `H2_LINE_CAR`。
2. CCS Build 显示 0 errors，并生成 `Debug/H2_LINE_CAR.out`。
3. XDS110 能把程序下载到 MSPM0G3507。
4. 程序运行后 PA14 板载 LED 周期闪烁，PB24 对 GND 始终约为 0 V。

串口输出属于可选验证，不影响 P0 是否通过。

## 1. 上电前安全检查

首次验证只保留 MCU 和下载器：

```text
电脑 USB -> MSPM0G3507 开发板
XDS110 -> MSPM0G3507 的 SWD 接口
```

必须做到：

- D24A 电源开关置于 OFF。
- 12 V 电池与 D24A 断开，或者至少完全不要连接 D24A 到 MCU 的任何控制线。
- PB24、PA10、PA11 暂时都不要接外部模块。
- 开发板平放在绝缘桌面，检查没有散落导线短接 3.3 V、5 V 和 GND。
- 如果 MSPM0 已由 USB 供电，不要再用 XDS110 的电源输出给开发板供电；XDS110 只用于目标电压参考、GND、SWDIO、SWCLK 和可选 NRST。

## 2. 在 CCS 中导入工程

1. 打开 CCS 20.5。
2. 选择 `File -> Import Project(s)`；若界面显示为 Eclipse 风格，则选择 `File -> Import -> Code Composer Studio -> CCS Projects`。
3. 选择工程目录：

   `C:\Users\Lenovo\Documents\Codex\2026-07-29\ccs-8-agent-skil-2024-h\outputs\H2_LINE_CAR`

4. 确认识别到工程名 `H2_LINE_CAR`。
5. 不要勾选“Copy projects into workspace”（工程已经在固定目录中）。
6. 点击 Import/Finish。

期望结果：左侧工程树出现 `H2_LINE_CAR`，可以看到 `main.c`、`empty.syscfg` 和 `targetConfigs/MSPM0G3507.ccxml`。

## 3. 编译

1. 单击左侧 `H2_LINE_CAR`，确保它是当前工程。
2. 点击工具栏锤子图标，或右键工程选择 `Build Project`。
3. 首次构建会先运行 SysConfig，再编译和链接，等待其结束。
4. 查看 Build Console 最后一段。

通过标准：

- Problems 中 `Errors = 0`。
- 工程下出现 `Debug` 文件夹。
- `Debug` 中出现 `H2_LINE_CAR.out`。

警告和错误不要混为一谈；若出现任何 error，停止下载，把 Build Console 从第一条 error 开始完整复制给 Codex。

## 4. 下载并运行

1. 保持 D24A 和所有执行机构断开。
2. 接好并上电 XDS110，开发板继续由 USB 供电。
3. 点击 Debug/小甲虫图标。
4. CCS 下载完成后通常会停在 `main()`，此时 LED 不闪是正常的。
5. 点击绿色 Continue/Resume 三角形，或按 F8，让程序继续运行。

期望结果：地猛星 PA14 对应的板载 LED 持续闪烁。该 LED 为高电平点亮；当前程序每次翻转之间约有半秒量级延时，因此肉眼应能明显看到亮灭变化。

如果点击 Debug 后 CCS 提示找不到目标、连接失败或器件不匹配，不要执行 Mass Erase、Factory Reset、Unlock 或 BSL Recovery；保存完整报错并停止。

## 5. 验证电机安全输出

保持 D24A 不连接：

1. 万用表切到直流电压档。
2. 黑表笔接开发板 GND。
3. 红表笔接 PB24（地猛星 H3 排针第 7 脚）。
4. 在程序运行、复位以及暂停调试三种状态下各测一次。

通过标准：PB24 始终约为 `0 V`。PB24 是后续 D24A 的 STBY 控制脚，低电平表示电机驱动禁能。

若测到接近 3.3 V，立即停止，不连接 D24A，并回报测量状态和照片。

## 6. 可选串口验证

地猛星板载 CH340 已连接 PA10/PA11。优先使用开发板 Type-C 接口枚举出的 CH340 串口，不要再把外接 USB-UART 并接到 PA10/PA11：

```text
开发板 Type-C -> 电脑 USB
```

- 串口参数：115200、8 data bits、no parity、1 stop bit。
- 按开发板 Reset 后，应看到一次：

  `H2_LINE_CAR P0 SAFE - SDK 2.11.00.07`

不要把 RS-232 电平接口或 5 V TTL 串口直接接到 MSPM0。

## 7. 回报模板

完成后直接复制下面内容填写：

```text
P0测试日期：
CCS导入：成功 / 失败
CCS Build：0 errors / 失败
是否生成 Debug/H2_LINE_CAR.out：是 / 否
XDS110下载：成功 / 失败
下载后是否点击 Continue/F8：是 / 否
PA14板载LED：闪烁 / 常亮 / 常灭
PB24运行时电压：
PB24复位时电压：
PB24暂停调试时电压：
串口（可选）：正常 / 未测试 / 异常
其他现象：
```

失败时附上 Build Console/Debug Console 的完整报错截图；不要只截最后一行。
