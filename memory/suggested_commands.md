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

`install/x86/<configuration>/` contains `MetaHook.exe`, `MetaHook_blob.exe`, their PDBs, `SDL2.dll` and `SDL3.dll` (when `METAHOOK_BUILD_SDL` is ON) and `svencoop/metahook/gamedata/`. It is not copied to a game directory automatically. Blob support is the `MetaHook_blob` target built in both configurations; there is no standalone `Release_AVX2`, `Release_blob`, plugin or installer target.

## Offline and data validation

After dependencies have been prepared, disable online synchronization for an existing build tree:

```bat
cmake -S . -B build/x86/Debug -DMETAHOOK_SYNC_GAMEDATA=OFF
cmake --build build/x86/Debug --config Debug --target install
python scripts/sync-gamedata.py --manifest scripts/manifests/metahook.json --target-dir install/x86/Debug/svencoop/metahook/gamedata --validate-only
```

OFF installs any existing data without downloading; it does not promise that gamedata exists or is current. Restore normal synchronization with `-DMETAHOOK_SYNC_GAMEDATA=ON`. Target data paths must end in `metahook/gamedata`.

A normal (online) synchronization with the persistent cache:

```bat
python scripts/sync-gamedata.py --manifest scripts/manifests/metahook.json --target-dir build/x86/Debug/assets/svencoop/metahook/gamedata --temp-root build/x86/Debug/gamedata-sync
```

The raw upstream snapshots and the last index are kept under `--temp-root/raw`; a rerun reuses them, and if the index is unreachable the cached index is used so the build can proceed offline.

The manifest-mode release gate, and the separate full consumer gate:

```bat
python scripts/validate-gamedata.py install/x86/Debug/svencoop/metahook/gamedata --manifest scripts/manifests/metahook.json
python scripts/validate-gamedata.py install/x86/Debug/svencoop/metahook/gamedata --full-catalog
```

Manifest mode checks the pruned output against the launcher's required symbols. `--full-catalog` retains the external plugin consumer requirements and needs the complete upstream catalog; it must not be run against a pruned output. Passing the build-time synchronization validator is not equivalent to running the full gate. The old `scripts/tests` suite was not migrated; do not claim it ran in this checkout.

## Inspection

```bat
git status --short
git diff
git submodule status
```

Inspect source through local FastCtx tools and begin with [[metahook/project-overview]]. Commit/push only when requested. MemoryModulePP's pinned commit is published; see [[metahook/build-and-verification]] for the remote clone verification record.

## GitHub Actions

`.github/workflows/livebuild.yml` runs the x86 Release build for `main` pushes,
pull requests and manual runs, then uploads `MetaHook-windows-x86.7z` directly
with `actions/upload-artifact@v7` and `archive: false`, without a ZIP wrapper.
`.github/workflows/msbuild.yml` builds `v*` tag pushes and publishes the 7z archive as a
GitHub Release asset. Both use `.github/actions/build-windows-x86/action.yml`
to call the existing Release script, run the installed manifest-mode gamedata
gate and package `MetaHook.exe`, `MetaHook.pdb`, `SDL2.dll`, `SDL3.dll` and
`svencoop/` as `MetaHook-windows-x86.7z`. Packaging
uses the runner's 7-Zip CLI and runs `7z t` before publishing archive outputs.
