# PicoC 上位机工具

用于当前 `STM32H750 + PicoC + USART1` 固件的 Windows 串口上位机。

## 功能
- 串口连接和断开
- 实时控制台输出
- 手工发送 PicoC 命令
- 上传本地 `.c` 文件并自动执行
- 中止当前上传
- 保存控制台日志
- 显示 PicoC 当前模式：`REPL`、`LOAD`、`BUSY`

## 运行环境
- Windows
- Python 3.9+

## 安装依赖

```powershell
pip install -r requirements.txt
```

如果安装失败，先清掉代理环境变量再试：

```powershell
set HTTP_PROXY=
set HTTPS_PROXY=
pip install -r requirements.txt
```

## 直接运行

```powershell
python app.py
```

或直接双击：

```text
run_host.bat
```

## 打包 EXE

```powershell
build_exe.bat
```

打包完成后会生成两个目录：

```text
dist\PicoCHost
release\PicoCHost
```

给别人使用时，直接把 `release\PicoCHost` 整个文件夹发给对方即可。

## 基本使用流程
1. 给板子上电并连接串口。
2. 点击“刷新”，选择正确的 COM 口，波特率保持 `115200`。
3. 点击“连接”。
4. 等待板端输出 `picoc> ` 或 `load> `。
5. 运行本地 `.c` 文件：
   - 点击“浏览”
   - 选择文件
   - 点击“上传并执行”
6. 如需中止当前上传，点击“中止”。
7. 如需保存日志，点击“保存日志”。

## 协议假设
- REPL 提示符：`picoc> `
- 文件模式提示符：`load> `
- 文件模式命令：`:load`、`:end`、`:abort`
- 文件执行完成后板端输出：`ready for next file`

## 说明
- 当前工具只适配这套 STM32 PicoC 固件。
- 不包含源码编辑器、断点调试或二进制传输协议。
- 文件上传使用纯文本方式发送，只统一换行格式，不改写源码内容。
