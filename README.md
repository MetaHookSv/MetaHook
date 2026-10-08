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

1. Build or obtain the launcher (see [Build instruction](docs/en/build-instruction.md)).
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

- [Build instruction](docs/en/build-instruction.md)
- [Compatibility](docs/en/compatibility.md)
- [MetaHook API](docs/en/metahook-api.md)
- [Game Symbols](docs/en/game-symbols.md)
- [CI/CD](docs/en/ci-cd.md)

Chinese translations live under [`docs/zh-CN/`](docs/zh-CN/compatibility.md).

## License

MetaHook is available under the MIT License; see `LICENSE`. Bundled third-party
sources keep their own license files.
## C/C++ formatting

Formatting uses [MetaHookSv/FormatValidation](https://github.com/MetaHookSv/FormatValidation)
and clang-format **23.1.3**, with the DiligentCore style (4 spaces, preserved include
order). Install the formatter for the Python interpreter used by CMake:

```sh
python -m pip install clang-format==23.1.3
cmake -S . -B build/format "-DFORMAT_VALIDATION_ONLY=ON"
cmake --build build/format --target format-check
cmake --build build/format --target format
```

The format-only configuration needs CMake 3.21+, Git, Python 3.9+ (CI uses 3.12),
and a build generator; `-G Ninja` works without Visual Studio. It prepares no native
SDK or game dependencies. Formatting targets are explicit and are not part of a
normal DLL build. With a Visual Studio generator, add `--config Debug` or
`--config Release` when building a formatting target.

The aggregate provides `FORMAT_VALIDATION_SOURCE_PATH=thirdparty/FormatValidation`.
Standalone components accept that CMake variable or its environment counterpart;
if empty, FetchContent downloads the fixed tooling commit. Quote relative paths,
for example `"-DFORMAT_VALIDATION_SOURCE_PATH=../../thirdparty/FormatValidation"`.
Configuration generates the ignored root `.clang-format` for editors; change the
shared style rather than that generated copy. An optional
`FORMAT_VALIDATION_CLANG_FORMAT_EXECUTABLE` selects an explicit formatter, whose
version must still match the pin.

Checks cover owned C/C++ files in `src/`, `include/`, and `tests/`, including
non-ignored new files. Repository-relative exclusions live in `.clang-format-ignore`.
Third-party sources and build artifacts are excluded. The `clang-format` workflow
checks the full scope on pushes, pull requests, and manual runs.