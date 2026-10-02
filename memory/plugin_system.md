---
title: plugin_system
type: note
permalink: metahook/plugin-system
---

# MetaHook plugin host and lifecycle

## Scope

Migrated from MetaHookSv's `plugin_system` note, retaining the host/plugin contract. Plugin implementation directories, plugin build scripts and plugin resources are not part of this standalone repository.

## Entry points and runtime layout

- `src/metahook.cpp`: `MH_LoadPlugins`, `MH_LoadPlugin`, host API tables, lifecycle dispatch and hook transactions.
- `include/Interface/IPlugins.h`: plugin interface versions and callbacks.
- `include/metahook.h`: `metahook_api_t`, `mh_interface_t`, engine types and public helper contracts.
- `<game>/<mod>/metahook/configs/plugins.lst`: runtime load configuration.
- `<game>/<mod>/metahook/plugins/`: plugin DLLs; sibling `dlls/` supplies shared runtime dependencies.
- `<game>/<mod>/metahook/gamedata/`: shared catalog consumed by the host and compatible plugins.

## Lifecycle

1. The launcher loads an engine and initializes catalog/module identities in `MH_LoadEngine`.
2. Plugin loading negotiates V4, then V3/V2/V1; compatible plugin `Init` receives the host API/interfaces.
3. The host invokes `LoadEngine`; client initialization later invokes `LoadClient` and passes the client export table.
4. Plugins may replace exports such as `pExportFunc->HUD_Init` and use host inline/VFT/IAT/inline-patch capabilities.
5. Exit/shutdown dispatch and centralized hook cleanup accompany engine-session teardown. DLL callbacks also report load/unload events; callbacks in the Ldr critical region must avoid blocking and unsafe reentrancy.

`MH_LoadPlugin` inserts at the head of a linked list, so `LoadEngine` / `LoadClient` traversal is the reverse of the textual plugin list. Load order therefore matters for dependencies and hooks; see [[metahook/meta-hook]].

## Integration conventions

- External plugins use the public headers and their own build systems. Do not add nonexistent `Plugins/` or `PluginLibs/` targets to this project's CMake build to reproduce the old solution layout.
- Engine-specific behavior must check the host engine type. Catalog-backed addresses resolve against the real module base through the public API; see [[metahook/game-data]].
- Preserve older interface versions and API slot order. New symbol metadata respects the versioned `cbSize` contract.
- Build/install success validates the launcher artifact, not plugin loading or gameplay. Deploy the selected plugins, configuration and resources separately before runtime verification.

Related: [[metahook/project-overview]], [[metahook/privatevars/metahook-privatevars]].
