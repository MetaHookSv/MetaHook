---
title: build_and_verification
type: note
permalink: metahook/build-and-verification
---

# MetaHook build internals and verification record

Internal build mechanics, dependency pinning and the migration verification
record for the standalone launcher. User-facing deployment and a short build
entry point live in the root `README.md`; runnable commands are in
[[metahook/suggested-commands]].

## CMake internals

- `CMakeLists.txt` defines the launcher plus static Detours, Capstone,
  RapidJSON, Chocobo1Hash, Musa.Veil and MemoryModulePP targets;
  `cmake/Dependencies.cmake` initializes missing submodules and prepares
  VC-LTL.
- Debug uses `/MTd`, Release uses `/MT`; both keep VC-LTL's existing msvcrt
  mode.
- Targets retain the original Windows subsystem, image base `0x1400000`, the
  per-monitor DPI manifest, icon/version resources and configuration-specific
  optimization/link settings. Only x86 Debug and Release are supported.
- `METAHOOK_DEPENDENCY_CACHE_DIR` selects a non-default dependency cache on
  first configuration; `VC_LTL_Root` selects an already extracted VC-LTL
  package. Downloads and extraction are cached under `thirdparty/cache`
  (Git-ignored), guarded by a lock during preparation, and a validated package
  is reused on later configurations.
- Gamedata: `METAHOOK_SYNC_GAMEDATA` (default `ON`) checks the existing
  GoldSrc_VibeSignatures index on every build and reuses unchanged snapshots;
  download or validation failures fail the build instead of using stale data.
  `OFF` installs existing data without downloading. `METAHOOK_GAMEDATA_DIR`
  points at an existing dataset and its path must end in `metahook/gamedata`.

## Dependency pinning

- Detours, Capstone, RapidJSON, Chocobo1Hash and Musa.Veil keep their
  MetaHookSv submodule URLs and exact commits. CMake initializes missing
  submodules without tracking remote branches or overwriting an existing
  working checkout.
- MemoryModulePP is a local component repository at `thirdparty/MemoryModulePP`
  (origin `https://github.com/MetaHookSv/MemoryModulePP`), copied unchanged from
  the baseline and given a CMake static-library target. Its initial commit
  `d3c042a` is local only: **a recursive clone from GitHub cannot obtain it
  until that commit is published**. No fallback to an unrelated upstream
  version is performed.
- VC-LTL is not a submodule. CMake downloads `VC-LTL-Binary.7z` from the
  official Chuyu-Team/VC-LTL5 `v5.3.1` release and verifies SHA-256
  `7a18799ed3aa84a225610a5447a56bc534c5c98ccb8dec05caba0e3f633431ad`.

## Migration verification (historical)

Verified locally with CMake 3.31.12, Visual Studio 2022 / MSVC 19.44.35228 and
Windows SDK 10.0.26100.0:

- Debug and Release entrypoints build and install successfully, including
  incremental calls from outside the project directory.
- A fresh CMake cache downloads and verifies VC-LTL; a fresh offline-mode build
  installs the executable and PDB without gamedata.
- Both normal installs contain 21 snapshots that pass the updater's offline
  validation. A deliberately invalid gamedata URL fails the build even when
  previous snapshots exist.
- Invalid dependency archives and CMake configuration errors propagate through
  both batch entrypoints as nonzero exit codes without invoking the build.
- Both executables are x86 Windows GUI images at base `0x1400000`, with the
  original icon/version resources, per-monitor DPI manifest and msvcrt imports.
- Generated launcher, Capstone and MemoryModule projects use the expected CRT
  settings and VC-LTL paths; none depend on the old checkout. All 615 migrated
  files match their source bytes, and the source repository remains clean.

Local evidence lived under the ignored `build/verification/` directory. Capstone
emits existing CMake policy deprecation warnings; a clean Debug link reports
LNK4075 from VC-LTL's `libvcruntimed.lib` because incremental linking is
disabled, as in the original launcher project. Neither prevented a build.

## SDL runtime and SDK ownership (2026-10-02)

- Trigger: the user moved SDL build ownership out of Renderer and explicitly requested that this repository build/install SDL2 and SDL3.
- Constraint: preserve the existing fork commits, feature choices, static CRT and VC-LTL configuration; do not modify vendor source or add an SDL link dependency to the launcher.
- Implementation: `METAHOOK_BUILD_SDL` defaults ON. `cmake/SDL.cmake` builds SDL3 first, then sdl2-compat against its `SDL3::Headers` target. Both use the parent's VC-LTL settings. Vendor install rules retain headers, generated revision headers, import libraries, package metadata and licenses; DLLs go beside MetaHook.exe via `CMAKE_INSTALL_BINDIR=.`. No game directory is modified.
- Pins: SDL3 (`https://github.com/hzqst/SDL`) at `3d20d7638918bdda5f428674b4acb3cc5b85d93e`; sdl2-compat (`https://github.com/hzqst/sdl2-compat-fork`) at `c24acad1544e91f413a0f628a160ca43e20bd995`. Source checkouts remain unmodified.
- Feature selection matches the original MetaHookSv scripts: shared SDL3, no static SDL3/test library/test executables, render/GPU/joystick/haptic/WASAPI disabled; sdl2-compat shared, test executables and CPack disabled. Its upstream helper libraries remain part of SDK installation.
- Verification: both normal build entrypoints configured, compiled, synchronized gamedata and installed with exit 0. Both configs installed SDL2.dll/SDL3.dll, include/SDL2 and include/SDL3 (including generated SDL_revision.h), import libraries and licenses. Generated SDL targets use MTd/MT and the expected VC-LTL paths. PE inspection confirms x86; SDL3 imports msvcrt.dll, while sdl2-compat retains its original CRT-free linkage. An x86 smoke program linked to the installed SDL2 import library successfully calls SDL_Init(0), SDL_GetVersion (2.32.57), and SDL_Quit with each installed DLL pair.
- Consumer verification: Renderer builds from the installed SDK in both configs; compiler dependency logs confirm installed SDL2 header paths and both configurations pass 4/4 CTest. Debug also verifies SDL3_INCLUDE_DIRS is optional. Missing SDL2 paths and invalid SDL3 paths fail configuration with exit 1. Renderer contains no SDL submodule or SDL build target.
- Launcher-only configuration with `METAHOOK_BUILD_SDL=OFF` succeeds. This smoke test does not validate window creation, audio/video devices, game startup or gameplay. Logs and temporary smoke artifacts are under ignored build/verification/.

## Dependency include cleanup (2026-10-02)

- Trigger: remove redundant launcher include paths and consume Capstone headers through its target.
- Constraint: the pinned Capstone CMake file uses directory-scoped includes without publishing target usage requirements. The launcher previously included `<capstone.h>` through an explicit `include/capstone` path.
- Implementation: the parent adds the Capstone `include` directory to `capstone-static`'s interface, and `src/metahook.cpp` uses `<capstone/capstone.h>`. Remove the launcher's explicit Capstone path and redundant RapidJSON `include/rapidjson` path; retain RapidJSON `include` for existing `<rapidjson/...>` references. No vendor files are changed.
- Verification: both normal Debug/Release batch entrypoints build and install with exit 0, including gamedata checks for 21 unchanged snapshots and installation of MetaHook.exe/PDB. Debug retains the known LNK4075 warning. Logs: `build/verification/include-target-<configuration>.log`.
- Scope: header lookup only; no API or runtime behavior changes, and no game/plugin smoke test. The historical byte-identical source migration record predates this include directive adjustment.

## Runtime and verification limitations

- Build success does not validate game startup, plugin loading or gameplay.
- Game startup/plugin compatibility and remote recursive cloning remain untested
  in this standalone checkout.
- The inherited `scripts/tests` suite was not migrated; do not report its
  results as current standalone results.

Related: [[metahook/project-overview]], [[metahook/suggested-commands]],
[[metahook/task-completion]].
