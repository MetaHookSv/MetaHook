[Back to README](../../README.md) | [中文](../zh-CN/ci-cd.md)

# Automated builds

- [LiveBuild](../../.github/workflows/livebuild.yml) builds Windows x86 Release on pushes to `main`, pull requests targeting `main`, and manual runs. Download `MetaHook-windows-x86.7z` directly from the workflow run's summary.
- [Build](../../.github/workflows/msbuild.yml) runs when a `v*` tag is pushed and creates a `MetaHook-<tag>` GitHub Release with `MetaHook-windows-x86.7z`.

Both workflows use the Release build script and validate the installed gamedata against the launcher manifest. The archive contains `MetaHook.exe`, `MetaHook.pdb`, `SDL2.dll`, `SDL3.dll` at its root and the complete `svencoop/` directory from `install/x86/Release`. Packaging uses 7-Zip and verifies archive integrity before uploading.

Both workflows check out the pinned dependencies recursively. MemoryModulePP's initial commit `d3c042a` is published to its configured remote.
