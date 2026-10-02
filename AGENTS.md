# AGENTS.md

This file provides guidance and important rules working with code in this repository.

## When coding / building plan

- Use a progressive disclosure approach for agent coding in this repository: start from high-level information in the Basic Memory knowledge base first, and only locate/read specific files or symbols when necessary, instead of expanding a large amount of context at once.

#### Basic Memory knowledge base (project-scoped, `memory/`)

- Notes live in `memory/` (markdown with YAML frontmatter: `title`/`type`/`permalink`), tracked in git.
- This repository contains the standalone MetaHook launcher. Its notes were migrated from MetaHookSv and adapted to the CMake workspace; see `memory/project_overview.md` for scope and provenance.
- Prefer Basic Memory MCP tools (`search_notes` / `read_note` / `write_note` / `edit_note`) only when their project resolves to this repository's `memory/` directory. Verify the project binding before writing.
- No Basic Memory project or project-level `.mcp.json` has been configured for this checkout by this migration. A server pinned to `metahooksv` reads/writes the original repository, not this one. Until a matching project is available, read and edit the local markdown files directly.
- Notes use the `metahook/` permalink prefix to distinguish them from the source repository.

#### High-level information in this repository (read corresponding notes first)

- Project overview and codebase entry points: `project_overview`
- Plugin system and development workflow: `plugin_system`
- Launcher architecture and lifecycle: `MetaHook`
- Symbol catalog and public API contracts: `GameData`
- Engine-private symbol inventory: `privatevars/metahook-privatevars.md`
- Build commands, conventions, and verification: `suggested_commands`, `CodeStyles`, `task_completion`

#### When notes are insufficient: source entry points (query and read on demand)

- Build: `CMakeLists.txt`, `cmake/Dependencies.cmake`, `scripts/build-MetaHook-x86-Debug.bat`, `scripts/build-MetaHook-x86-Release.bat`
- Loader and core logic: `src/`
- Public API / interfaces: `include/metahook.h`, `include/Interface/`
- Dependencies: `thirdparty/`; source dependencies are fixed submodules, while VC-LTL is a verified binary download prepared by CMake.
- Plugins and PluginLibs are external consumers, not directories built by this checkout.
- Runtime plugin configuration: `<game>/<mod>/metahook/configs/plugins.lst` (not a file shipped in this repository).
- Build output: `build/x86/<configuration>/`; install output: `install/x86/<configuration>/`. Neither is tracked.

#### Progressive disclosure key points

- Read notes first, then locate a single file/symbol; do not read the whole repository at once.
- Prefer correctly scoped Basic Memory MCP tools for knowledge retrieval; otherwise use the local notes before reading source.
- Prefer Context7 for external dependency/library usage (query on demand).

## Explore SKILLs

- Project-level skills, when present, live in `.claude/skills` no matter what harness tool is being used. Skills from the old workspace were not copied as part of the notes migration.
