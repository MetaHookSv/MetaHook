# AGENTS.md

This file provides guidance and important rules working with code in this repository.

## When coding / building plan

- Use a progressive disclosure approach for agent coding in this repository: start from high-level information in the Basic Memory knowledge base first, and only locate/read specific files or symbols when necessary, instead of expanding a large amount of context at once.

### Basic Memory knowledge base (project-scoped, `memory/`)

- Notes live in `memory/` (markdown with YAML frontmatter: `title`/`type`/`permalink`), tracked in git.
- Notes use the `metahook/` permalink prefix to distinguish them from the source repository.

### High-level information in this repository (read corresponding notes first)

- Project overview, launcher architecture/lifecycle and codebase entry points: `project_overview`
- Plugin system and development workflow: `plugin_system`
- Symbol catalog and public API contracts: `GameData`
- Engine-private symbol inventory: `PrivateSymbols`
- Build commands, conventions, and verification: `suggested_commands`, `CodeStyles`, `task_completion`
- Build internals, dependency pinning, migration verification: `build_and_verification`

### When notes are insufficient: source entry points (query and read on demand)

- Build: `CMakeLists.txt`, `cmake/Dependencies.cmake`, `scripts/build-MetaHook-x86-Debug.bat`, `scripts/build-MetaHook-x86-Release.bat`
- Loader and core logic: `src/`
- Public API / interfaces: `include/metahook.h`, `include/Interface/`
- Dependencies: `thirdparty/`; source dependencies are fixed submodules, while VC-LTL is a verified binary download prepared by CMake.
- SDL runtime/SDK packaging: `cmake/SDL.cmake`, enabled by `METAHOOK_BUILD_SDL` (default ON). MetaHook builds and installs SDL3 plus sdl2-compat; external plugins consume the installed headers rather than rebuilding SDL.
- Plugins and PluginLibs are external consumers, not directories built by this checkout.
- Runtime plugin configuration: `<game>/<mod>/metahook/configs/plugins.lst` (not a file shipped in this repository).
- Build output: `build/x86/<configuration>/`; install output: `install/x86/<configuration>/`. Neither is tracked.
