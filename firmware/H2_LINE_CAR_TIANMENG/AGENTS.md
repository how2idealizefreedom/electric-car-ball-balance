# H2_LINE_CAR_TIANMENG 协作规则

- 本工程只用于立创·天猛星 MSPM0G3507（LQFP-64 PM），不得把地猛星 LQFP-48 管脚配置复制回来。
- `empty.syscfg` 是外设和管脚的唯一配置源，不得手改 `Debug/ti_msp_dl_config.*` 等生成文件。
- 当前循迹算法必须保持与已验证的地猛星 P1 稳定版本一致；任何提速、滤波、斜坡、丢线搜索或转向极性修改必须单独试验，一次只改一项。
- D24A 的 STBY 必须由 PB15 控制并有默认下拉；复位、下载和调试暂停期间保持低电平。
- PA10/PA11 保留给板载 CH340，PA19/PA20 保留 SWD，PA0/PA1 保留扩展板 MPU6050 I2C，PB21/PB22 分别是板载按键/LED。
- 不使用 PA2、PA18、PA21、PA23 作为普通 GPIO。
- 编译前依次运行 `check_syscfg.py`、`run_sysconfig.py`，再由 CCS 生成和构建 Debug 配置。
- 未架空车轮、未确认方向和 STBY 安全状态时不得烧录会启动电机的固件。
