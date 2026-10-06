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
- Gamedata: `METAHOOK_SYNC_GAMEDATA` (default `ON`) runs
  `scripts/sync-gamedata.py` with the launcher manifest
  `scripts/manifests/metahook.json`. The synchronizer prunes each declared game
  version to the symbols the launcher resolves and publishes `index.json` plus
  one stable `<gameVersion>.json` per version (no more content-addressed names).
  Raw upstream snapshots and the last index are kept in the persistent cache
  `${CMAKE_BINARY_DIR}/gamedata-sync/raw`; later builds reuse them, and an
  unreachable index falls back to the cached index so the build can proceed
  offline. The cache is never cleaned between builds; only ephemeral staging
  directories are removed after publish. `OFF` installs existing data without
  downloading. `METAHOOK_GAMEDATA_DIR` points at an existing dataset and its
  path must end in `metahook/gamedata`. The release gate is
  `scripts/validate-gamedata.py <dir> --manifest scripts/manifests/metahook.json`
  (`--full-catalog` adds the external-plugin consumer gates, which require the
  complete upstream catalog and must not run on a pruned output).

## Dependency pinning

- Detours, Capstone, RapidJSON, Chocobo1Hash and Musa.Veil keep their
  MetaHookSv submodule URLs and exact commits. CMake initializes missing
  submodules without tracking remote branches or overwriting an existing
  working checkout.
- MemoryModulePP is a local component repository at `thirdparty/MemoryModulePP`
  (origin `https://github.com/MetaHookSv/MemoryModulePP`), copied unchanged from
  the baseline and given a CMake static-library target. Its initial commit
  `d3c042a` was local only at migration time and was published to the configured
  remote's `main` branch on 2026-10-02. A fresh shallow clone verified the exact
  pinned commit. No fallback to an unrelated upstream version is performed.
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

## GitHub Actions and dependency publication (2026-10-02)

- Trigger: add live builds and version-tag releases following BetterSpray's workflow behavior; publish MemoryModulePP with explicit user authorization.
- Implementation: `livebuild.yml` builds `main` pushes, pull requests and manual runs, then uploads a timestamped artifact. `msbuild.yml` builds `v*` tag pushes and creates a GitHub Release. Both run on `windows-2022` and share `.github/actions/build-windows-x86/action.yml` for the existing x86 Release script, installed manifest-mode gamedata validation and ZIP packaging of the full install tree. LiveBuild has `contents: read`; release publication has `contents: write`.
- Publication: MemoryModulePP was already committed and clean. `git push --set-upstream origin main` published `d3c042a2645b272aa9ec4b7118092dd989e3afa4`; a new remote shallow clone returned that exact HEAD. The parent's submodule pin did not change.
- Verification: actionlint 1.7.12 accepted both workflows. The normal Release entrypoint and the composite action's build step exited 0. A fresh install produced 11 snapshots that passed the action's manifest gate; its actual packaging step exited 0. ZIP CRC checks and comparison against the entire install tree verified all 208 files, including the launcher/PDB, SDL DLLs, SDK libraries/headers and gamedata. Logs are under `build/verification/ci-*.log`.
- Local install constraint: incremental CMake installation retains obsolete files. Validation of the existing install tree found 21 old content-addressed gamedata files, so packaging verification used a new install prefix under `build/verification/ci-workspace-*`, matching a fresh hosted runner. For release verification use a fresh staging prefix rather than assuming an old install tree is clean.
- Scope: no GitHub-hosted run, artifact upload, release publication or game startup was performed in this session.

## 7z build archives (2026-10-02)

- Trigger: use `MetaHook-windows-x86.7z` following MetaHookSv's Windows workflow.
- Constraint: 7-Zip preserves the relative input path in archive entries. Run the package step from `install/x86/Release` with `*` as input so the launcher and runtime files remain at the archive root.
- Implementation: the shared action uses `7z a -t7z ... * -r`, checks its exit code, then runs `7z t` before emitting outputs. Both callers consume `archive-path`, so live artifacts and tag-release assets use the new format. The Windows 2022 runner includes 7-Zip.
- Verification: executed the actual package step against a fresh install tree with official 7-Zip 26.03. Creation, integrity testing and extraction exited 0; all 208 extracted file paths and SHA-256 hashes matched the install tree. The archive was 2,406,066 bytes. Both workflows passed actionlint, and `git diff --check` reported no whitespace errors. Logs: `build/verification/ci-7z-package.log` and `ci-7z-extract.log`.
- Scope: packaging only; no launcher, SDL or gamedata behavior changed. The earlier ZIP verification above records the original workflow implementation.

## Direct 7z artifact downloads (2026-10-02)

- Trigger: the live artifact downloaded as a timestamped ZIP containing `MetaHook-windows-x86.7z`.
- Root cause: `actions/upload-artifact@v7` defaults `archive` to `true`. `compression-level: 0` disables ZIP compression but retains ZIP packaging.
- Implementation: LiveBuild sets `archive: false` for its single archive path. This mode uses the uploaded file's name as the artifact name, so the timestamped `name` and ZIP compression input were removed. Tag releases already upload the 7z file directly.
- Hosted verification before this fix: LiveBuild run `37023757187` at `be8a34b` completed successfully, including recursive dependency checkout, Release build, gamedata validation, 7z creation/testing and the ZIP-wrapped upload.
- Package scope: the user also restricted the archive to `MetaHook.exe`, `MetaHook.pdb`, `SDL2.dll`, `SDL3.dll` and the complete `svencoop/` subtree. The action selects those explicit inputs and removes a previous generated archive at its fixed `build/artifacts` path before creation, because `7z a` otherwise retains old entries when updating an archive.
- Local verification: executed the actual package step with official 7-Zip 26.03 after seeding the previous 208-file archive. Creation, integrity testing and extraction exited 0; exactly 16 requested files remained and all SHA-256 hashes matched. Both workflows passed actionlint and the diff whitespace check. Logs: `build/verification/ci-runtime-package.log` and `ci-runtime-extract.log`.
- Hosted verification after this fix: LiveBuild run `37024901218` at `05bfb5e` completed successfully. Artifact `11235166473` is named `MetaHook-windows-x86.7z` (1,675,860 bytes). Downloading its REST archive endpoint returned raw 7z bytes with signature `377abcaf271c`; official 7-Zip integrity testing and extraction exited 0, and all 16 files matched the requested path selection. Download and logs are under `build/verification/ci-runtime-download-37024901218/`.
- Scope: Actions artifact transport and packaged file selection. Previous artifacts retain their original format and contents.

## Blob launcher in the package (2026-10-04)

- Trigger: `MetaHook_blob.exe` is built and installed beside `MetaHook.exe`, but the 7z package omitted it, so CI artifacts and releases could not run blob engines.
- Implementation: the shared action's explicit 7z inputs add `MetaHook_blob.exe` and `MetaHook_blob.pdb`; the CI/CD docs list them.
- Verification: actionlint 1.7.12 accepted both workflows. The x86 Release entrypoint built and installed with exit 0, and the manifest-mode gamedata gate passed (11 snapshots). The action's packaging script, run verbatim from `install/x86/Release` with 7-Zip 24.07, exited 0 including `7z t`; the archive held 18 files (2,280,521 bytes), and every extracted file matched the install tree by SHA-256. Logs: `build/verification/blob-package-*.log`.
- Scope: packaging only; no hosted run, game startup or blob-engine smoke test was performed.

## Client gamedata aliases (2026-10-06, issue #903)

- Trigger/root cause: a proxy client and its renamed original have different file identities and image bases. Alias lookup must select both the catalog CRC64 and the address-owning loaded module.
- Implementation: optional snapshot client filenames survive pruning; the real client's missing symbols can resolve through same-directory, already-loaded modules with matching CRC64. Public CRC64 queries and mirror/Blob address semantics remain unchanged. See [[metahook/game-data]].
- Verification: standalone Win32 Debug and Release builds of both `MetaHook` and `MetaHook_blob` exited 0. Native tests in `tests/` passed in both configurations, including actual DLL load/unload, address ownership, candidate ordering, CRC mismatch, cross-directory rejection, scalar/member queries, bounds, mirror/Blob regression and merging two indexes.
- Script verification: the three new Python tests passed against all 14 component script pairs (42 executions); the existing BetterSpray and VGUI2Extension pruning tests also passed. All 14 manifest gates passed (171 snapshots total). Renderer initially failed against stale build output; normal synchronization regenerated its 21 snapshots and its gate then passed.
- Environment: the aggregator build could not regenerate because its existing LaunchGame target referenced missing `D:\CS3266\czero\liblist.gam`; the independent launcher build supplied the build evidence. Logs: `build/alias-standalone-debug.log`, `build/alias-standalone-release.log`, `build/alias-renderer-sync.log`.
- Scope: upstream alias publication is still required for production data to activate this feature. The user explicitly excluded real csldr verification; no game startup or gameplay claim is made.
- Upstream follow-up: `D:/GoldSrc_VibeSignatures` now declares the three filenames in 15 Windows client configs, freezes them into optional module `binary_aliases` metadata and exports `binaries.client.windows.alias`. A real local cstrike-10210 snapshot passed generation and both BulletPhysics/CaptionMod pruning/validation with the reported symbols present. Upstream changes remain local and require publication through the normal release pipeline.

## Runtime and verification limitations

- Build success does not validate game startup, plugin loading or gameplay.
- Game startup/plugin compatibility remains untested in this standalone checkout.
- The inherited `scripts/tests` suite was not migrated; do not report its
  results as current standalone results.

Related: [[metahook/project-overview]], [[metahook/suggested-commands]],
[[metahook/task-completion]].
