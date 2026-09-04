# Platform patches for my_ecfw_zephyr

These patches carry **Leo / ls101x PoC** fixes that cannot live only in the
application tree (SoC headers, linker Kconfig, HAL CMake sources, eSPI driver,
etc.).

Board DTS tweaks (`sysevent-base`, KCS `kcs-env-addr`, `&espi status`) and
the `fw_desc_gen.py` **post-build invocation** live in this repo
(`boards/linkedsemi/ls101x_ehl_tcp_3l/`, `boards/ls101x_evb.overlay`,
root `CMakeLists.txt`) and are **not** patched here. The script itself
(`app_version = 0xff`) is in the `ls_sdk` patch.

## Layout

```text
patches/
  zephyr/0001-ls101x-poc-platform-fixes.patch
  ls_sdk/0001-zephyr-cmake-leo-link-sources.patch
```

Apply against clean checkouts of:

- `zephyr` @ GitHub `linkedsemi` (this patch regenerated vs `6e8f81b41a57`)
- `ls_sdk` @ manifest revision (`main`)

### Zephyr patch contents (high level)

| Area | Why |
|------|-----|
| `drivers/espi/espi_ls.c` | Soft-recover, `espi_driver_api`, BOOT_DONE timing, SUS_WARN@0x41 / SUS_ACK@0x40（边沿）, Leo clock/reset/`espi_en`；ISR 明细为 `LOG_DBG`；POST `0x80` sink（写丢弃）；未注册 peri 口回 CycType `0x0E` |
| `drivers/espi/CMakeLists.txt` | `espi_ls_sio.c` 跟 `CONFIG_ESPI_LS_SIO`；`espi_ls_sio_uart.c` 跟 `CONFIG_ESPI_LS_SIO_UART` |
| `drivers/espi/espi_ls_sio.c` | ITE 进配置 `87→01→55→55`；Chip ID 来自 DT；LDN 0x00–0x19；0x30/60/61/70；未选/未知读 0 |
| `drivers/espi/espi_ls_sio_uart.c` | Host COM1/2：DWUART 直通；COM3/4：LSUART 16550 翻译；动态 8 口、DLAB 波特、edge IRQ |
| `dts/bindings/espi/linkedsemi,ls-sio.yaml` | `chip-id` |
| `dts/bindings/espi/linkedsemi,ls-sio-uart.yaml` | SIO UART 子节点 |
| `include/.../ls101x-pinctrl.h` | `DWUART1/2_CTSN` / `RTSN` 别名 |
| `dts/.../ls101x.dtsi` | eSPI/LPC clock offset `LS_CCTL_CLKG` (was wrongly `CLKG0`) |
| `soc/.../espi_lpc/espi_lpc_common.h` | Build when `ESPI_LS`；`vw_boot_done_sent` / SUS 边沿状态；**Leo S02：bit2=S3、bit0=S5**；`GET_PEER_DEV` 只累加带 `kcs-env-addr` 的 sibling（避免 SIO 破坏 KCS peer） |
| posix / e906 ISR / ls101x soc stubs | Existing PoC compile bring-up |

### ls_sdk patch contents

| Area | Why |
|------|-----|
| `zephyr/CMakeLists.txt` | Leo 链接 `static_buffer` / `flash_inst1.c` / `int_call_asm.S` |
| `tools/leo/fw_desc_gen.py` | `app_version` `0x1` → `0xff`（与当前烧录镜像一致） |

## Apply

From the west workspace root:

```powershell
python my_ecfw_zephyr/scripts/apply_patches.py
```

Useful flags:

```powershell
python my_ecfw_zephyr/scripts/apply_patches.py --check    # dry-run
python my_ecfw_zephyr/scripts/apply_patches.py --reverse  # remove
```

Re-run after every `west update` that refreshes `zephyr` / `ls_sdk`.

**Windows note:** `*.patch` must stay LF. This repo has `.gitattributes`
(`patches/**/*.patch text eol=lf`). `apply_patches.py` also strips CRLF before
`git apply` so a bad checkout still works.

## Upstream

Prefer merging these fixes into company `zephyr` / `ls_sdk` branches, then
delete the corresponding patch files from this directory.
