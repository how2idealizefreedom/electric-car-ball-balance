# 2026 全国大学生电子设计竞赛 H 题 · 水平平台小球平衡小车

> 基于 TI MSPM0 + Code Composer Studio 的竞赛项目：通过 HMG996 舵机 / 步进机构驱动倾斜平台，
> 配合视觉（开环）算法，使小球在平台上保持稳定 / 完成指定运动。

## 项目简介

本仓库为 2026 年电赛 H 题「水平平台上的小球平衡小车」的完整参赛资料集合，包含：

- **控制算法**：直流电机 PID 调速、底盘循迹（PID_car / LINE_CAR）、平台调平控制
- **机械结构**：钢球平衡机构、HMG996 舵机装配的三维模型（SolidWorks）
- **硬件资料**：WHEELTEC R1 底盘、TB6612 电机驱动、MPU6050 姿态传感器、天猛星拓展板、OLED 模块
- **文档报告**：算法说明、电赛选拔赛资料（设计报告 / 测试报告 / 代码工程）、官方说明与答疑

## 目录结构

```
电赛H题-水平小球平衡小车/
├─ docs/          文档与报告（算法说明、电赛资料、官方说明/答疑）
├─ firmware/      源码固件（CCS / MSPM0G3507 工程）
│   ├─ 10_DC_MOTOR_PID_1   直流电机 PID 调速
│   ├─ 11_PID_car          循迹小车 PID
│   ├─ H2_LINE_CAR0z       主线工程（含接线/测试指南文档）
│   └─ 4-testhhw           测试工程
├─ mechanical/    机械结构三维模型（SolidWorks：钢球平衡机构、舵机装配）
├─ hardware/      硬件资料（底盘、电机驱动、传感器、拓展板、PCB、OLED）
│   └─ 电赛ti_PCB文件      TI 竞赛 PCB（Gerber / 原理图）
├─ tools/         工具（XCOM 串口调试、mspm0-skill）
└─ .gitignore
```

## 硬件平台

| 模块 | 说明 |
| --- | --- |
| 主控 | TI MSPM0G3507 |
| 开发环境 | Code Composer Studio (Theia) 20.5 |
| 底盘 | WHEELTEC R1 系列 |
| 电机驱动 | TB6612（带稳压 D153C） |
| 姿态传感器 | MPU6050 |
| 执行机构 | HMG996 舵机 / 步进机构 |
| 显示 | 0.96 寸 OLED |

## 如何编译 / 运行

1. 安装 [Code Composer Studio (Theia)](https://www.ti.com/tool/CCSTUDIO) 20.5 及以上版本。
2. 导入 `firmware/` 下对应的工程（File → Import → CCS Projects）。
3. 根据 `firmware/H2_LINE_CAR0z/` 内的 `P0_TEST_GUIDE.md` ~ `P2_FINAL_LAP_TEST.md` 依次完成接线与测试。

## 说明

- 为控制仓库体积，**大视频（*.mp4/*.avi）、压缩包（*.zip/*.rar/*.7z）和编译产物已被 `.gitignore` 忽略**，
  这些文件仍保留在本地目录中，仅供本地查阅；如需在 GitHub 上展示，请改用 Git LFS 或 Release 附件。
- 本仓库仅用于学习与技术交流；竞赛作品的知识产权归作者所有，转载请注明出处。
- 推送到 GitHub 前，请先 `git config user.name` / `git config user.email` 设置为你自己的账号，并酌情添加 `LICENSE`。

---
*Generated with WorkBuddy · 整理于 2026-10-09*
