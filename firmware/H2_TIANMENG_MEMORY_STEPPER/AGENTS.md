# H2_TIANMENG_MEMORY_STEPPER 协作规则

- 本工程只用于立创·天猛星 MSPM0G3507（LQFP-64 PM）和外接 D24A；不再迁移地猛星版本，也不启用扩展板板载 TB6612。
- `empty.syscfg` 是外设和管脚的唯一配置源，不得手改 `Debug/ti_msp_dl_config.*`。
- P1 参数来自稳定底座。示教/回放、步进机构和之后的 MPU/编码器优化必须分别验证；任何一次实车测试只改变一类参数。
- 复位、下载和调试暂停期间 PB15/D24A STBY 必须保持低，PA25/A4988 ENABLE 必须保持高。
- PA10/PA11 保留 CH340，PA19/PA20 保留 SWD，PA0/PA1 保留 MPU6050，PB21/PB22 为板载键/LED。
- PA22/PA24/PA25/PB8 专用于 A4988；PA26/PA27/PB9 专用于外接 K2/K3/K4。
- 不使用 PA2、PA18、PA21、PA23 作为普通 GPIO。
- 编译前依次运行 `check_syscfg.py`、`run_sysconfig.py`，再由 CCS 构建。
- 车轮未架空、机构运动范围未清空、STBY/ENABLE 安全态未确认时，不得烧录或运行可驱动执行器的固件。
