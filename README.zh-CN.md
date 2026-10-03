# MetaHook

[English README](README.md)

MetaHook 是一个用于客户端modding，允许用户为其编写插件的GoldSrc引擎游戏的启动器

## 功能

- 启动引擎（`hw.dll` 或 `sw.dll`）并驱动其运行循环。
- 加载插件并管理其生命周期。
- 为插件提供引擎符号解析、hook（inline、VFT、IAT 与 inline-patch）、内存与反汇编访问等用于modding的API。
- 从 gamedata 加载引擎符号数据。gamedata 由上游项目[GoldSrc_VibeSignatures](https://github.com/HLND2T/GoldSrc_VibeSignatures)提供。

## 支持的目标

- 系统：Windows x86。（将来可能会支持Linux）
- 引擎：GoldSrc、GoldSrc Blob、GoldSrc HL25、SvEngine 与 CoF。
- 插件：插件接口从 V4 向下协商至 V1；当前公共 API 版本为 115。

## 安装

运行时 MetaHook 需要 32 位 Windows 系统和一个受支持的 GoldSrc 游戏安装。

1. 构建或获取启动器（见[构建说明](docs/zh-CN/build-instruction.md)）。
2. 将 `MetaHook.exe` 放到游戏根目录，即原版游戏可执行文件所在的位置。
3. 把安装树中的 `svencoop/metahook/gamedata` 复制到 `<game>/<mod>/metahook/gamedata`。
4. SDL 运行时包（`SDL2.dll`、`SDL3.dll`）随启动器一起构建并安装

启动：

- 以命令行传入 `MetaHook.exe -game <mod>`启动；或者直接将`MetaHook.exe`改名为`<mod>.exe`，比如：`MetaHook.exe` -> `svencoop.exe`。
- 插件从 `<mod>/metahook/configs/plugins.lst` 加载
- 插件不属于本仓库。请单独将它们及其共享依赖安装到 `<mod>/metahook/` 下。

## 文档

- [构建说明](docs/zh-CN/build-instruction.md)
- [兼容性](docs/zh-CN/compatibility.md)
- [MetaHook API](docs/zh-CN/metahook-api.md)
- [游戏符号 API](docs/zh-CN/game-symbols.md)
- [自动化构建](docs/zh-CN/ci-cd.md)

英文原文位于 [`docs/en/`](docs/en/compatibility.md)。

## 许可证

MetaHook 采用 MIT License，见 `LICENSE`。随附的第三方源码保留各自的许可证文件。