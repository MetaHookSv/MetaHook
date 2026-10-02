---
title: suggested_commands
type: note
permalink: metahook/suggested-commands
---

# MetaHook build and validation commands

Adapted from MetaHookSv's command note for the standalone CMake workspace. Run these from the repository root unless calling a script by its full path.

## Requirements and entrypoints

Visual Studio 2022 with x86 C++ tools and Windows SDK, CMake 3.21+, Git and Python 3.8+. CMake, Git and Python must be on PATH. Initial dependency preparation and normal gamedata synchronization need network access.

```bat
scripts\build-MetaHook-x86-Debug.bat
scripts\build-MetaHook-x86-Release.bat
```

Each script configures, builds and installs its configuration; failures return nonzero. Existing `SolutionDir` overrides the root, otherwise it is derived from the script location. Dependency preparation is inside CMake, including the fixed VC-LTL download/hash check.

## Direct CMake usage

```bat
cmake -S . -B build/x86/Debug -G "Visual Studio 17 2022" -A Win32 -DCMAKE_INSTALL_PREFIX="%CD%/install/x86/Debug"
cmake --build build/x86/Debug --config Debug --target install --parallel
```

Replace both `Debug` path components and `--config Debug` with `Release` for Release. The Visual Studio generator selects the build configuration through `--config`.

`install/x86/<configuration>/` contains `MetaHook.exe`, PDB and `svencoop/metahook/gamedata/`. It is not copied to a game directory automatically. There is no standalone `Release_AVX2`, `Release_blob`, plugin or installer target.

## Offline and data validation

After dependencies have been prepared, disable online synchronization for an existing build tree:

```bat
cmake -S . -B build/x86/Debug -DMETAHOOK_SYNC_GAMEDATA=OFF
cmake --build build/x86/Debug --config Debug --target install
python scripts/sync-gamedata.py --target-dir install/x86/Debug/svencoop/metahook/gamedata --validate-only
```

OFF installs any existing data without downloading; it does not promise that gamedata exists or is current. Restore normal synchronization with `-DMETAHOOK_SYNC_GAMEDATA=ON`. Target data paths must end in `metahook/gamedata`.

The inherited full consumer gate is separate:

```bat
python scripts/validate-gamedata.py install/x86/Debug/svencoop/metahook/gamedata
```

It retains external plugin consumer requirements. Passing the build-time synchronization validator is not equivalent to running this full gate. The old `scripts/tests` suite was not migrated; do not claim it ran in this checkout.

## Inspection

```bat
git status --short
git diff
git submodule status
```

Inspect source through local FastCtx tools and begin with [[metahook/project-overview]]. Commit/push only when requested. MemoryModulePP's initial local-only submodule commit must be published before a remote recursive checkout is available.
