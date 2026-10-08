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
## C/C++ 格式化

使用 [MetaHookSv/FormatValidation](https://github.com/MetaHookSv/FormatValidation)
共享工具及固定版本 **clang-format 23.1.3**，采用 DiligentCore 风格（4 空格，保留
include 顺序）。为 CMake 使用的 Python 解释器安装格式工具：

```sh
python -m pip install clang-format==23.1.3
cmake -S . -B build/format "-DFORMAT_VALIDATION_ONLY=ON"
cmake --build build/format --target format-check
cmake --build build/format --target format
```

格式专用配置需要 CMake 3.21+、Git、Python 3.9+（CI 使用 3.12）及构建生成器；
使用 `-G Ninja` 可无需 Visual Studio。它不准备原生 SDK 或游戏依赖。
格式目标需显式执行，不加入普通 DLL 构建。使用 Visual Studio 生成器时，执行目标
需追加 `--config Debug` 或 `--config Release`。

聚合仓库注入 `FORMAT_VALIDATION_SOURCE_PATH=thirdparty/FormatValidation`。
独立组件支持该 CMake 参数及同名环境变量；为空时通过 FetchContent 获取固定工具
提交。相对路径应加引号，例如
`"-DFORMAT_VALIDATION_SOURCE_PATH=../../thirdparty/FormatValidation"`。
配置时在仓库根目录生成被 gitignore 的 `.clang-format` 供编辑器使用；格式规则应
在共享仓库修改，不修改生成副本。可通过 `FORMAT_VALIDATION_CLANG_FORMAT_EXECUTABLE`
指定工具路径，但版本仍须与固定版本一致。

检查覆盖 `src/`、`include/`、`tests/` 中维护的 C/C++ 文件，包括未被 Git 忽略的新文件。
相对仓库根目录的排除规则位于 `.clang-format-ignore`；第三方源和构建产物不纳入检查。
`clang-format` workflow 在 push、pull request 和手动运行时执行全量检查。