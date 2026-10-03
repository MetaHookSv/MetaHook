# MetaHook

[中文文档](README.zh-CN.md)

MetaHook is a launcher for GoldSrc engine games for client-side modding, letting
users write their own plugins.

## What it does

- Starts the engine (`hw.dll` or `sw.dll`) and drives its run loop.
- Loads plugins and manages their lifecycle.
- Provides plugins with modding APIs: engine symbol resolution, hooking (inline,
  VFT, IAT and inline-patch), memory and disassembly access, and more.
- Loads engine symbol data from gamedata. The gamedata is provided by the
  upstream [GoldSrc_VibeSignatures](https://github.com/HLND2T/GoldSrc_VibeSignatures) project.

## Supported targets

- System: Windows x86. (Linux may be supported in the future.)
- Engine: GoldSrc, GoldSrc Blob, GoldSrc HL25, SvEngine and CoF.
- Plugin: plugin interfaces are negotiated from V4 down to V1; the current public
  API version is 115.

## Installation

At runtime MetaHook needs a 32-bit Windows system and a supported GoldSrc game
installation.

1. Build or obtain the launcher (see [Getting started](docs/en/build-instruction.md)).
2. Put `MetaHook.exe` in the game's root directory, where the original game
   executable lives.
3. Copy `svencoop/metahook/gamedata` from the install tree to
   `<game>/<mod>/metahook/gamedata`.
4. The SDL runtime package (`SDL2.dll`, `SDL3.dll`) is built and installed
   alongside the launcher.

Launching:

- Launch with `MetaHook.exe -game <mod>` on the command line; or simply rename
  `MetaHook.exe` to `<mod>.exe`, for example: `MetaHook.exe` -> `svencoop.exe`.
- Plugins are loaded from `<mod>/metahook/configs/plugins.lst`.
- Plugins are not part of this repository. Install them and their shared
  dependencies under `<mod>/metahook/` separately.

## Documentation

- [Getting started: build, install and dependencies](docs/en/build-instruction.md)
- [Compatibility: ABI compatibility and behavioral changes](docs/en/compatibility.md)
- [Features: engine detection, hooking, Blob and Mirror-DLL APIs](docs/en/features.md)
- [Game Symbol API: gamedata-backed symbol query and resolution](docs/en/api.md)
- [Automated builds: CI workflows and release archives](docs/en/ci.md)

Chinese translations live under [`docs/zh-CN/`](docs/zh-CN/compatibility.md).

## License

MetaHook is available under the MIT License; see `LICENSE`. Bundled third-party
sources keep their own license files.