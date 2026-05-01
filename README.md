# PicoC on STM32H750

本项目将 PicoC 移植到 STM32H750，基于 USART1 提供串口交互式 REPL 与整文件加载执行能力，并配套 Windows 上位机工具实现串口连接、源码上传、结果查看与基础测试。目标是以最小可用方案在 MCU 上跑通轻量级 C 脚本解释执行流程。

This project ports PicoC to STM32H750 and provides an interactive UART REPL plus whole-file source upload and execution over USART1, together with a Windows host tool for serial connection, source upload, result viewing, and basic testing. The focus is a minimal and practical embedded C scripting workflow on MCU.

## Features

- PicoC REPL over `USART1`
- Whole-file upload with `:load / :end / :abort`
- Basic expression evaluation and multi-line input
- Windows host tool for serial interaction and upload
- Keil MDK project for STM32H750

## Repository Layout

- [Core](F:/work/work/PICO/h750/UART_DMA_H750/Core): STM32 application code
- [picoc](F:/work/work/PICO/h750/UART_DMA_H750/picoc): PicoC source and STM32 platform port
- [tools/picoc_host/src](F:/work/work/PICO/h750/UART_DMA_H750/tools/picoc_host/src): Windows host tool source
- [tools/picoc_host/release/PicoCHost](F:/work/work/PICO/h750/UART_DMA_H750/tools/picoc_host/release/PicoCHost): packaged Windows release

## Current Scope

- Script source comes from serial input only
- No filesystem-backed source loading on target
- Minimal built-in standard library support
- Focused on bring-up and practical interaction rather than full desktop PicoC compatibility

## Build and Run

### Firmware

- Open the Keil project under `MDK-ARM`
- Build and flash the STM32H750 target
- Connect over `USART1` at `115200`

### Host Tool

Source mode:

```powershell
cd tools/picoc_host
run_host.bat
```

Release mode:

- Run `PicoCHost.exe` from `tools/picoc_host/release/PicoCHost`

## License

This repository uses the MIT License. See [LICENSE](F:/work/work/PICO/h750/UART_DMA_H750/LICENSE).
