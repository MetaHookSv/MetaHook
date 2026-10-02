# MetaHook

MetaHook is a Windows launcher for GoldSrc engine games that hosts client-side
plugins. It starts the game engine, loads the plugins listed in a mod's
`plugins.lst`, and exposes a stable public API for those plugins to build
against.

This repository is the standalone launcher plus its public headers. It was
extracted from MetaHookSv and keeps the original launcher behavior and the
plugin-facing ABI.

## What it does

- Starts the engine (`hw.dll` or `sw.dll`) and drives its run loop.
- Loads client-side plugins and manages their lifecycle.
- Provides plugins with engine symbol resolution, hooking (inline, VFT, IAT and
  inline-patch), memory and disassembly access, and DLL load notifications.
- Serves engine symbol data from a validated, pruned gamedata catalog.

## Supported targets

- Windows x86.
- GoldSrc engine variants tracked by the gamedata catalog: GoldSrc, GoldSrc
  HL25, SvEngine and CoF.
- Plugin interfaces are negotiated from V4 down to V1; the current public API
  version is 115.

## Installation

At runtime MetaHook needs a 32-bit Windows system and a supported GoldSrc game
installation.

1. Build or obtain the launcher (see [Building](#building)).
2. Put `MetaHook.exe` in the game's root directory, where the original game
   executable lives.
3. Copy `svencoop/metahook/gamedata` from the install tree to
   `<game>/<mod>/metahook/gamedata`. The catalog must match the game and mod you
   launch. Each game version is a single stable `<gameVersion>.json` (for
   example `hl-4554.json`), pruned to the symbols the launcher resolves, so the
   directory stays small and old versions do not accumulate. The launcher also
   merges any nested `metahook/gamedata/**/index.json`, which is how a plugin
   ships its own catalog alongside the launcher's.
4. The SDL runtime package (`SDL2.dll`, `SDL3.dll`) is built and installed
   alongside the launcher for games using these forks. Deploy the matching pair
   together when updating those runtimes; building does not modify a game installation.

Launching:

- The launcher derives the mod from its own executable name, so rename it to
  match the game (`hl.exe` selects `valve`, `svencoop.exe` selects `svencoop`),
  or pass `-game <mod>` explicitly.
- Plugins are loaded from `<mod>/metahook/configs/plugins.lst`, relative to the
  game root.
- Plugins are not part of this repository. Install them and their shared
  dependencies under `<mod>/metahook/` separately.

If the catalog has no entry matching the loaded module, the launcher stops with
an error instead of guessing symbol addresses.

## Building

Requirements: Visual Studio 2022 with Desktop development with C++, an x86 MSVC
toolchain and Windows SDK, CMake 3.21 or newer, Git, and Python 3.8 or newer,
all on `PATH`. The first configuration needs network access to prepare
dependencies and synchronize gamedata.

```bat
scripts\build-MetaHook-x86-Debug.bat
scripts\build-MetaHook-x86-Release.bat
```

Each script configures, builds and installs one configuration, and returns a
nonzero exit code on failure. The install tree
(`install/x86/<configuration>/`) contains `MetaHook.exe`, its PDB, SDL2/SDL3 DLLs,
and validated gamedata under `svencoop/metahook/gamedata`. SDL SDK headers are
installed under `include/SDL2` and `include/SDL3`, import libraries under `lib`,
with upstream CMake package files and licenses also retained.

`METAHOOK_BUILD_SDL` defaults to `ON`. SDL3 and sdl2-compat are built from the
original fixed fork commits with shared DLLs, static CRT and the parent's VC-LTL
settings. The original SDL feature selection is preserved; the launcher does
not acquire a new SDL link dependency. Direct CMake users can set
`-DMETAHOOK_BUILD_SDL=OFF` for a launcher-only build.

Renderer consumes these headers without building SDL again, for example:

```bat
D:\Renderer\scripts\build-Renderer-x86-Release.bat "-DSDL2_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include" "-DSDL3_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include"
```

Gamedata synchronization is manifest-driven. `scripts/manifests/metahook.json`
declares the game versions and the exact symbols the launcher resolves; the
synchronizer downloads the upstream catalog into a persistent cache under
`build/x86/<configuration>/gamedata-sync/`, prunes each version to those symbols
(and to the payload fields the loader reads), and publishes `index.json` plus one
`<gameVersion>.json` per version. Because the cache is kept, later builds reuse
it and can build offline. A plugin can ship its own manifest with the same schema
and run `scripts/sync-gamedata.py --manifest <its manifest>` to produce its own
catalog. The release gate is `scripts/validate-gamedata.py <dir> --manifest
scripts/manifests/metahook.json` (add `--full-catalog` to also run the external
plugin consumer gates against a complete catalog).

Offline builds and direct CMake usage are documented in
`memory/suggested_commands.md`; dependency pinning, build internals and the
migration verification record are in `memory/build_and_verification.md`.

## Automated builds

- [LiveBuild](.github/workflows/livebuild.yml) builds Windows x86 Release on
  pushes to `main`, pull requests targeting `main`, and manual runs. Download
  the timestamped `MetaHook-*` artifact from the workflow run's summary.
- [Build](.github/workflows/msbuild.yml) runs when a `v*` tag is pushed and
  creates a `MetaHook-<tag>` GitHub Release with `MetaHook-windows-x86.zip`.

Both workflows use the Release build script, validate the installed gamedata
against the launcher manifest, and archive the contents of `install/x86/Release`
with the launcher, PDB, SDL runtimes, SDK and licenses at the ZIP root.

Both workflows check out the pinned dependencies recursively. MemoryModulePP's
initial commit `d3c042a` is published to its configured remote.

## License

MetaHook is available under the MIT License; see `LICENSE`. Bundled third-party
sources keep their own license files.

Plugins, PluginLibs and installer/tools are maintained in the upstream
MetaHookSv project, not in this repository.
