---
title: CodeStyles
type: note
permalink: metahook/code-styles
---

# MetaHook code conventions

Adapted from MetaHookSv's `CodeStyles.md`. Follow the exact local file style; the inherited source mixes conventions, so these are guidance rather than reasons for broad formatting changes.

## Naming and organization

- Existing globals commonly use `g_` or legacy names; pointers use `p`, members `m_`, and Hungarian type indicators such as `i`, `fl`, and `b` where already established.
- Public host functions use the `MH_` prefix; class methods/types use their existing PascalCase forms. Preserve exported names and ABI.
- Constants/macros use uppercase names with underscores. Prefer named constants over new unexplained numbers.
- Keep existing source filenames (`metahook.cpp`, `GameData.cpp`, `LoadBlob.cpp`, etc.); do not impose lowercase naming on the migrated files.
- Preserve each header's guard style (`#pragma once` or traditional include guards). Keep related declarations together and use forward declarations where practical.

## Formatting and comments

- Existing C++ mostly uses tabs and local brace conventions; new CMake uses four-space indentation.
- Match the surrounding comment language and style. Explain non-obvious behavior and ownership/lifetime constraints.
- Include order can be semantic: Chocobo1Hash headers precede Windows headers in `GameData.cpp` to avoid `min`/`max` macro conflicts. Do not mechanically reorder includes.
- Keep changes local; source migration is not a reason to reformat unrelated code or third-party sources.

## Architecture and correctness

- Keep catalog querying, address resolution, hook management and plugin lifecycle responsibilities separate.
- Follow [[metahook/game-data]] for upstream trust, gamedata-only address resolution and versioned API contracts.
- Make engine-specific paths explicit; use the correct real module identity for resolution.
- Account for ownership, hook cleanup, thread synchronization and loader-critical-region restrictions.
- Verify behavior with appropriate builds or focused tests; do not add tests that pin mutable documentation/configuration text.
