# MetaHook

Windows x86 MetaHook launcher, extracted from MetaHookSv commit
`11a852774b1725d02735aeb348c32a7bf454507c`. Public headers and launcher
behavior are preserved. Plugins and PluginLibs are not built by this project.

## Build

Requirements: Visual Studio 2022 with Desktop development with C++, an x86
MSVC toolchain and Windows SDK, CMake 3.21 or newer, Git, and Python 3.8 or newer.
Put CMake, Git and Python on PATH. Internet access is needed for initial
dependency preparation and the default gamedata synchronization.

```bat
scripts\build-MetaHook-x86-Debug.bat
scripts\build-MetaHook-x86-Release.bat
```

The scripts work from any current directory. An existing `SolutionDir` overrides
the project root; otherwise the scripts locate it relative to their own path.
They configure Visual Studio 2022 Win32 projects, build, and install. Any failed
step returns a nonzero exit code. Build trees are `build/x86/Debug` and
`build/x86/Release`; install trees are `install/x86/Debug` and
`install/x86/Release`.

Each install tree contains `MetaHook.exe`, `MetaHook.pdb`, and validated gamedata
under `svencoop/metahook/gamedata`. Copy the gamedata directory to the applicable
game's `<mod>/metahook/` directory when deploying. No game installation is
modified automatically. This is the launcher distribution, not a complete
plugin installation.

Direct CMake configuration performs the same dependency preparation:

```bat
cmake -S . -B build/x86/Debug -G "Visual Studio 17 2022" -A Win32 -DCMAKE_INSTALL_PREFIX="%CD%/install/x86/Debug"
cmake --build build/x86/Debug --config Debug --target install --parallel
```

Debug uses `/MTd`; Release uses `/MT`. Both use VC-LTL's existing msvcrt mode.
The targets retain their original Windows subsystem, image base, DPI behavior
and configuration-specific optimization/link settings. Only x86 Debug and
Release are supported.

## Dependencies

Detours, Capstone, RapidJSON, Chocobo1Hash and Musa.Veil retain their MetaHookSv
submodule URLs and exact commits. CMake initializes missing submodules without
tracking remote branches or overwriting an existing working checkout.

MemoryModulePP is a local component repository at `thirdparty/MemoryModulePP`,
with origin `https://github.com/MetaHookSv/MemoryModulePP`. Its source is copied
unchanged from the baseline, with a new CMake static-library target. The initial
component commit is local only: **recursive cloning from GitHub cannot obtain
it until that commit is published**. Keep the local component repository when
using this unpublished workspace. No fallback to an unrelated upstream version
is performed.

VC-LTL is not a submodule. CMake downloads `VC-LTL-Binary.7z` from the official
Chuyu-Team/VC-LTL5 `v5.3.1` release, verifies SHA-256
`7a18799ed3aa84a225610a5447a56bc534c5c98ccb8dec05caba0e3f633431ad`,
and uses the package's CMake helper. Downloads and extracted files live in
`thirdparty/cache`, which is ignored by Git. The shared cache is locked during
preparation. A validated extracted package is reused on later configurations.
`METAHOOK_DEPENDENCY_CACHE_DIR` selects another cache on first configuration;
`VC_LTL_Root` selects its extracted package directory.

## Gamedata

`METAHOOK_SYNC_GAMEDATA` defaults to `ON`. Every build checks the existing
GoldSrc_VibeSignatures index using the original synchronization/validation
logic; unchanged snapshots are reused. Download or validation failures fail
the build instead of silently using stale data.

For offline compilation after dependencies have been prepared:

```bat
cmake -S . -B build/x86/Debug -DMETAHOOK_SYNC_GAMEDATA=OFF
cmake --build build/x86/Debug --config Debug --target install
```

This installs any existing gamedata without downloading it. Re-enable with
`-DMETAHOOK_SYNC_GAMEDATA=ON`. `METAHOOK_GAMEDATA_DIR` may point to an existing
dataset; its path must end in `metahook/gamedata` as required by the updater.

Build success does not validate game startup, plugin loading or gameplay.
Third-party sources retain their license files; MetaHook's license is in
`LICENSE`.

## Migration verification

Verified locally with CMake 3.31.12, Visual Studio 2022 / MSVC 19.44.35228,
and Windows SDK 10.0.26100.0:

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
  original icon/version resources, per-monitor DPI manifest, and msvcrt imports.
- Generated launcher, Capstone and MemoryModule projects use the expected CRT
  settings and VC-LTL paths; none depend on the old checkout. All 615 migrated
  files match their source bytes, and the source repository remains clean.

Local evidence is under the ignored `build/verification/` directory. Capstone
emits existing CMake policy deprecation warnings; a clean Debug link reports
LNK4075 from VC-LTL's `libvcruntimed.lib` because incremental linking is disabled,
as in the original launcher project. These did not prevent either build.
Game startup/plugin compatibility and remote recursive cloning remain untested.
