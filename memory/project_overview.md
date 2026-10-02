---
title: project_overview
type: note
permalink: metahook/project-overview
---

# MetaHook Project Overview

## Purpose and boundary

MetaHook is the standalone Windows x86 launcher and public API for GoldSrc/SvEngine client-side plugins. It was extracted from MetaHookSv commit `11a852774b1725d02735aeb348c32a7bf454507c`; initial standalone CMake integration is commit `2f69a31`.

- **Loader** (`src/`): engine startup, symbol resolution, hook infrastructure, plugin lifecycle and DLL notifications. See [[metahook/meta-hook]].
- **Public interfaces** (`include/metahook.h`, `include/Interface/`, HLSDK and other shared headers): retained paths and ABI for external consumers.
- **GameData** (`src/GameData.*`): immutable symbol catalog and module identities. See [[metahook/game-data]].
- Plugins such as Renderer, BulletPhysics and VGUI2Extension, plus PluginLibs and installer/tools, are outside this checkout. Their implementation notes remain in MetaHookSv.

## Architecture conventions

- Trust upstream gamedata correctness; reuse existing parsing/query/release gates instead of adding a new defensive system for hypothetical incorrect RVAs, lengths or cross-snapshot metadata.
- Resolve catalog-backed addresses through `MH_LoadEngine_ResolveSymbol` / `MH_ResolveGameSymbol`; plugin consumers use `ResolveGameSymbol`. Do not restore signature/string/reverse-search fallbacks for those symbols.
- Resolve against the real module base. Module CRC64 identifies the binary; the game/mod name alone does not identify the loaded module.
- Preserve public ABI and existing versioned `cbSize` behavior; API version at migration is 115.

## Build and dependencies

- Windows x86, Visual Studio 2022 v143, C++20, CMake 3.21+; Debug and Release use `/MTd` and `/MT` with VC-LTL.
- `CMakeLists.txt` defines the launcher and static dependencies. `cmake/Dependencies.cmake` initializes missing submodules and downloads/verifies VC-LTL 5.3.1 using the package's helper.
- Fixed source submodules: Detours, Capstone, RapidJSON, Chocobo1Hash, Musa.Veil and MemoryModulePP.
- MemoryModulePP's initial commit `d3c042a` is local only at migration time; publish it before expecting a remote recursive clone to work.
- Python 3.8+ runs gamedata synchronization/validation. There is no renderer, OpenGL, physics or SDL build dependency for this launcher target.
- Commands: [[metahook/suggested-commands]]. Build internals, dependency pinning and migration verification: [[metahook/build-and-verification]]. User-facing deployment: root `README.md`.

## Layout and entry points

- `src/launcher.cpp`: process entry and engine-run loop.
- `src/metahook.cpp`: public API, engine adaptation, hooks and plugin lifecycle.
- `src/LoadBlob.cpp`, `src/LoadDllNotification.cpp`: legacy module handling and notifications.
- `include/metahook.h`, `include/Interface/IPlugins.h`: public host/plugin contracts.
- `scripts/`: two configuration-specific build entrypoints and gamedata tools.
- `build/x86/<configuration>/`: generated projects, objects and staged gamedata; ignored.
- `install/x86/<configuration>/`: executable, PDB and `svencoop/metahook/gamedata`; ignored.
- Runtime plugins/configs live in the target game's `<mod>/metahook/` tree, not in repository-root `plugins.lst`.

The source supports GoldSrc, GoldSrc HL25, SvEngine, CoF and legacy blob paths. Actual startup requires a matching catalog identity and data. Blob compilation is enabled by `_DEBUG` or `METAHOOK_BLOB_SUPPORT`; the current Release target does not enable it. These are implementation capabilities, not a new game compatibility certification.

## Migrated knowledge

The following notes were selected from `D:/MetaHookSv/memory` and adapted for this workspace:

| Note | Content retained |
| --- | --- |
| [[metahook/meta-hook]] | Launcher responsibilities, lifecycle, hooks and limitations |
| [[metahook/game-data]] | Catalog architecture, parser/query contracts, API evolution and validation |
| [[metahook/privatevars/metahook-privatevars]] | Engine symbol inventory, cvar branches and engine classification |
| [[metahook/plugin-system]] | Host-facing plugin interfaces, loading and lifecycle |
| [[metahook/code-styles]] | Applicable source conventions |
| [[metahook/suggested-commands]] | Current CMake build/install and gamedata commands |
| [[metahook/task-completion]] | Scope-appropriate verification and delivery checks |

This overview is the eighth migrated note. Detailed plugin implementation/history, graphics capture workflows, installer/Steam-location tools and the obsolete plugin disassembly-location workflow were not imported. The original material remains in the [source knowledge tree](https://github.com/hzqst/MetaHookSv/tree/11a852774b1725d02735aeb348c32a7bf454507c/memory).

Notes use `metahook/` permalinks. No Basic Memory project registration was changed: the currently available server is pinned to `metahooksv`, so these local notes must not be written through that project. Read the local markdown until a project bound to this checkout is configured.
