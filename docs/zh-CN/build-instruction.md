[返回 README](../../README.md) | [English](../en/build-instruction.md)

# 快速开始

本页涵盖 MetaHook 的构建、安装与依赖细节。

## 依赖要求

Visual Studio 2022（含 C++ 桌面开发工作负载）、x86 MSVC 工具链和 Windows SDK、CMake 3.21 或更新版本、Git，以及 Python 3.8 或更新版本，且均位于 `PATH`。首次配置需要网络访问以准备依赖并同步 gamedata。

## 构建

```bat
scripts\build-MetaHook-x86-Debug.bat
scripts\build-MetaHook-x86-Release.bat
```

## SDL 运行时

`METAHOOK_BUILD_SDL` 默认为 `ON`。SDL3 与 sdl2-compat 使用共享 DLL、静态 CRT 和父项目的 VC-LTL 设置，从原有的固定 fork 提交构建。原始 SDL 特性选择保持不变；启动器未引入新的 SDL 链接依赖。直接使用 CMake 的用户可设置 `-DMETAHOOK_BUILD_SDL=OFF` 进行仅启动器的构建。

其他插件无需重新构建 SDL 即可消费这些头文件，例如：

```bat
<FullPath-to-Renderer>\scripts\build-Renderer-x86-Release.bat "-DSDL2_INCLUDE_DIRS=<FullPath-to-MetaHook>/install/x86/Release/include" "-DSDL3_INCLUDE_DIRS=<FullPath-to-MetaHook>/install/x86/Release/include"
```

## gamedata 同步

gamedata 同步由 manifest 驱动。

`scripts/manifests/metahook.json` 声明游戏版本和启动器解析的确切符号

同步器将上游 catalog 下载到 `build/x86/<configuration>/gamedata-sync/` 下的持久缓存，把每个版本裁剪为这些符号（以及 loader 读取的 payload 字段），并发布 `index.json` 加每个版本一个 `<gameVersion>.json`。

由于缓存保留，后续构建会复用并可离线构建。

插件可以随附使用相同 schema 的自己的 manifest，并运行 `scripts/sync-gamedata.py --manifest <its manifest>` 生成自己的 catalog。

发布门禁为 `scripts/validate-gamedata.py <dir> --manifest scripts/manifests/metahook.json`（追加 `--full-catalog` 还会针对完整 catalog 运行外部插件消费者门禁）。