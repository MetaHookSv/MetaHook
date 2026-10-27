[Back to README](../../README.md) | [中文](../zh-CN/build-instruction.md)

# Build instruction

This page covers the build, install and dependency details of MetaHook.

## Requirements

Visual Studio 2022 with Desktop development with C++, an x86 MSVC toolchain and Windows SDK, CMake 3.21 or newer, Git, and Python 3.8 or newer, all on `PATH`. The first configuration needs network access to prepare dependencies and synchronize gamedata.

## Build

```bat
scripts\build-MetaHook-x86-Debug.bat
scripts\build-MetaHook-x86-Release.bat
```

## SDL runtime

`METAHOOK_BUILD_SDL` defaults to `ON`. SDL3 (`thirdparty/SDL3`, hzqst/SDL `main`) and sdl2-compat (`thirdparty/sdl2-compat`, hzqst/sdl2-compat `VGUI2`) are built from their fixed submodule commits with shared DLLs and the parent's static CRT. The original SDL feature selection is preserved; the launcher does not acquire a new SDL link dependency. Direct CMake users can set `-DMETAHOOK_BUILD_SDL=OFF` for a launcher-only build.

Other plugins can consume these headers without building SDL again, for example:

```bat
<FullPath-to-Renderer>\scripts\build-Renderer-x86-Release.bat "-DSDL2_INCLUDE_DIRS=<FullPath-to-MetaHook>/install/x86/Release/include" "-DSDL3_INCLUDE_DIRS=<FullPath-to-MetaHook>/install/x86/Release/include"
```

## Gamedata synchronization

Gamedata synchronization is manifest-driven.

`scripts/manifests/metahook.json` declares the game versions and the exact symbols the launcher resolves

The synchronizer downloads the upstream catalog into a persistent cache under `build/x86/<configuration>/gamedata-sync/`, prunes each version to those symbols (and to the payload fields the loader reads), and publishes `index.json` plus one `<gameVersion>.json` per version.

Because the cache is kept, later builds reuse it and can build offline.

A plugin can ship its own manifest with the same schema and run `scripts/sync-gamedata.py --manifest <its manifest>` to produce its own catalog.

The release gate is `scripts/validate-gamedata.py <dir> --manifest scripts/manifests/metahook.json` (add `--full-catalog` to also run the external plugin consumer gates against a complete catalog).
