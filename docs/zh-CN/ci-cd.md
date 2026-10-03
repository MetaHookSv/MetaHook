[返回 README](../../README.md) | [English](../en/ci-cd.md)

# 自动化构建

- [LiveBuild](../../.github/workflows/livebuild.yml) 在推送到 `main`、面向 `main` 的拉取请求以及手动运行时构建 Windows x86 Release。直接从工作流运行的摘要页下载 `MetaHook-windows-x86.7z`。
- [Build](../../.github/workflows/msbuild.yml) 在推送 `v*` 标签时运行，创建包含 `MetaHook-windows-x86.7z` 的 `MetaHook-<tag>` GitHub Release。

两个工作流都使用 Release 构建脚本，并依据启动器 manifest 校验安装后的 gamedata。归档在根目录包含 `MetaHook.exe`、`MetaHook.pdb`、`SDL2.dll`、`SDL3.dll`，以及来自 `install/x86/Release` 的完整 `svencoop/` 目录。打包使用 7-Zip，并在上传前校验归档完整性。

两个工作流都会递归检出固定的依赖。MemoryModulePP 的初始提交 `d3c042a` 发布到其配置的远端。

构建、安装与依赖细节见[构建说明](build-instruction.md)。
