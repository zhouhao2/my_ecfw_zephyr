# my_ecfw_zephyr

凌思 Leo（LS101x / LSEC）嵌入式控制器（EC）固件，基于 Zephyr，参考 Intel [`ecfw-zephyr`](https://github.com/intel/ecfw-zephyr) 应用层架构移植。

当前目标板：`ls101x_ehl_tcp_3l`（产品载体 **EHL_TCP_3L** 主机主板；**硬件 eSPI VW + 主板开关机已通，可进 Windows**）。Host 按 Super I/O 工作，**不 decode ACPI EC 0x62/0x66**。开发板 `ls101x_evb` 仍保留，供 EVB / CI 使用。

## 命名对照

| 名称 | 含义 |
|------|------|
| Leo | `ls_sdk` SoC 目录 / 产品线代号 |
| LS101x / LS1010 | Zephyr SoC |
| `ls101x_ehl_tcp_3l` | 本仓 out-of-tree 板：EHL_TCP_3L 主板（默认验证载体） |
| `ls101x_evb` | Zephyr 树内开发板（EVB / CI） |
| LSEC | 固件品牌 / Boot 魔数 `LSEC_APP` |
| RiverSuzhou | 文档中的芯片别名 |

## 仓库角色

本仓是 **west manifest 应用仓**（`self.path: my_ecfw_zephyr`），依赖：

- `zephyr`：Linkedsemi 分支（Zephyr 4.0）
- `ls_sdk`：HAL / bootloader / 镜像后处理工具

Intel `ecfw-zephyr` 仅作只读参考，不在此仓内改造。

## 目录结构

```text
my_ecfw_zephyr/
├── app/                 # 应用：启动、电源、SMC、外设等
├── boards/              # board 宏、overlay；OOT 板 ls101x_ehl_tcp_3l
├── drivers/             # Hub 后端（gpio_ls / fan_ls / acpi_ls / espi_hub…）
├── include/             # 公共头文件
├── linker/              # 链接脚本片段（若有）
├── misc/                # task_handler、flashhdr 等
├── patches/             # 无法收入本仓的 zephyr / ls_sdk 补丁
├── scripts/             # apply_patches.py 等
├── AI_Outputs/          # 解读 reviews/ + 调试 debug_notes/（非运行时依赖）
├── prj.conf             # 默认 Kconfig
├── west.yml             # west manifest
├── CMakeLists.txt
└── Kconfig
```

## 工作区初始化

在空目录或已有 west 工作区中：

```powershell
# 若以本仓为 manifest
west init -m <本仓 URL> --mr <branch>
west update

# 或在已有工作区切换 manifest
# 编辑 .west/config：manifest.path = my_ecfw_zephyr
west update
```

典型布局：

```text
<workspace>/
├── .west/
├── my_ecfw_zephyr/    # 本仓
├── zephyr/
└── ls_sdk/
```

### 平台补丁（必做）

`west update` 拉下来的 `zephyr` / `ls_sdk` 是干净树。本仓 **不要求** 向这两个仓库提交，但构建前需打上 PoC 平台补丁：

```powershell
cd <workspace>
python my_ecfw_zephyr/scripts/apply_patches.py
```

说明见 [`patches/README.md`](patches/README.md)。`west update` 之后请重新执行。

本仓内已直接承载（无需补丁）：

- `boards/linkedsemi/ls101x_ehl_tcp_3l/`：产品板 DTS（UART3 PH06/PH07 @921600、`sysevent-base`、`ls-host-kcs` 0x60/0x64、`ls-sio` 0x2E/0x2F；关 PWM 以免 PE15 被 PWM8 占用）
- `boards/linkedsemi/ls101x_ehl_tcp_3l.c`：PE15/PD03 去 mux + hi-Z 输入
- `ls101x_evb` overlay / 适配仍保留，行为与原先 PoC 一致
- 根 `CMakeLists.txt`：`BOARD_ROOT` + `fw_desc_gen.py` 后处理（写入 `LSEC_APP` @`0x2000`）；`app_version=0xff` 见 ls_sdk 补丁
- 解读文档：[`AI_Outputs/reviews/`](AI_Outputs/reviews/README.md)（eSPI 分层、SIO 四路 COM）
- 调试过程记录：[`AI_Outputs/debug_notes/`](AI_Outputs/debug_notes/README.md)（非运行时依赖）

## 构建

工具链示例（玄铁 + Zephyr SDK）：

```powershell
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\Users\<user>\zephyr-sdk-0.17.1"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "cross-compile"
$env:CROSS_COMPILE = "D:\LinkedSemi\gcc\Xuantie-900-gcc-elf-newlib-mingw-V3.0.2\bin\riscv64-unknown-elf-"

cd <workspace>
python my_ecfw_zephyr/scripts/apply_patches.py
west build -b ls101x_ehl_tcp_3l my_ecfw_zephyr
# EVB / CI：west build -b ls101x_evb my_ecfw_zephyr
```

产物：

| 文件 | 说明 |
|------|------|
| `build/zephyr/zephyr.elf` | ELF |
| `build/zephyr/zephyr.bin` | 烧录用 BIN（含后处理） |
| `build/zephyr/zephyr.hex` | HEX（`fw_desc_gen` 生成） |

### 镜像后处理（重要）

Leo `boot_ram` 要求：

| Flash 偏移 | 内容 |
|------------|------|
| `0x0000`–`0x1FFF` | info + SBL（构建时嵌入 `__info_array`） |
| **`0x2000`** | **`LSEC_APP` 描述头 + CRC** |
| `0x2020` | 签名区（可为 0） |
| **`0x2060`** | **应用入口** |

与 `ls_sdk` example 一致，本仓根 `CMakeLists.txt` 在链接后自动执行：

```text
python ls_sdk/tools/leo/fw_desc_gen.py build/zephyr/zephyr.bin
```

若缺少该步骤，`0x2000` 描述头为空，EC 模式下可能卡在 bootloader，**无串口日志**。也可手动补跑上述命令修复已生成的 BIN。

## 烧录与串口

- 烧录文件：`build/zephyr/zephyr.bin`（确认已做过 `fw_desc_gen`）
- 控制台 / Shell：`uart3`
  - TX：**PH07**
  - RX：**PH06**
  - 波特率：**921600**
  - Shell：已启用（`CONFIG_SHELL` + serial backend）

## PoC 现状与裁剪

默认 `prj.conf` **已打开硬件 eSPI + KCS**：

- `CONFIG_ESPI=y` → 链接真 `espi_hub.c`（非 stub）
- `CONFIG_KCS=y` → `acpi_ls` 走 `e8042`；板级 DTS 增加 `ls-host-kcs` 译码 0x60/0x64（KBC status，不是 ACPI 0x62/0x66）
- `ls-sio` 注册 Super I/O 配置口 0x2E/0x2F（BIOS 探 KBC 前会扫）；EHL 伪装 ITE IT8786（`chip-id = 0x8786`）
- `ls-sio-uart` 将 Host COM1–COM4 接到 Leo UART（COM1/2=DWUART，COM3/4=LSUART 16550 翻译）；**不是** Zephyr console，也不是 `host_vuart`
- `CONFIG_LS_ECFW_POC_SIM_PWRSEQ=n` → 电源态仅随 Host VW（SLP / SUS_WARN）；时序由主板硬件完成，EC **不驱动 RSMRST**
- `&espi { status = "okay"; sysevent-base = ... }`；PWM 关闭（勿占用 PE15）
- POST `0x80` 由 zephyr `espi_ls` 注册 sink（写丢弃、读 0）；未注册 peri 口仍回 CycType `0x0E`

**板上已验证（2026-08，载体 EHL_TCP_3L）：**

- DN_CNFG → `TARGET_BOOT_DONE` → `SUS_WARN@0x41` / `SUS_ACK@0x40`（边沿）→ SLP / PLTRST → `G3→S0`
- Host Peripheral IO 正常；主板**开/关机正常，可进入 Windows**
- 关键修复已入库：PE15 勿复用 PWM8；Leo `DN_VWIR_S02` 为 **bit2=SLP_S3#，bit0=SLP_S5#**（见 zephyr 补丁）

仍关闭 / 精简路径：

- `CONFIG_PECI=n`、`CONFIG_FLASH=n`、`CONFIG_IPMI=n` 等
- `poc_smchost`（仅 QUERY/READ/WRITE 最小集，未链完整 `smchost.c`）

已启用的 Hub 后端骨架：`gpio_ls`、`fan_ls`、`i2c_hub`、`acpi_ls`、`espi_hub`。

首期不包含：DnX、SAF、DeepSx、完整热管理、键盘/PS2、OOB、完整电池栈等。

**平台依赖（勿漏）：** eSPI soft-recover / SUS 索引 / **Leo SLP 位定义** / SIO CMake 与 `GET_PEER_DEV` / POST `0x80` sink / 时钟偏移等在
`patches/zephyr/0001-ls101x-poc-platform-fixes.patch`；干净 `west update` 后必须
`python my_ecfw_zephyr/scripts/apply_patches.py`。

下一阶段：四口压测与串口鼠标；非阻塞 VW IRQ（避免四口并发 sending timeout）。未挂 peri 的运行时 IO 仍会 `0x0E`。

## 许可证

Apache-2.0（与 Zephyr / 参考 ecfw 应用层一致处沿用原 SPDX）。
