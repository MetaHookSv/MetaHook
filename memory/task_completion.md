---
title: task_completion
type: note
permalink: metahook/task-completion
---

# MetaHook completion and verification

Adapted from MetaHookSv's completion note to the standalone launcher and the current session's verification rules. Scale validation to the behavior changed; do not impose a game run or compiler build on a documentation-only edit.

## Code and build changes

- Confirm scope, root cause for fixes, engine-specific boundaries, memory ownership and hook cleanup.
- Preserve public header/API compatibility and the GameData versioned `cbSize` contract unless an interface change is explicitly authorized.
- Run the affected Debug/Release build entrypoints for build or launcher changes; report commands, results and material warnings.
- Check dependent static-library settings, installation output and failure propagation when changing build preparation.
- For gamedata changes distinguish synchronization/integrity checks from the full consumer gate. Report unavailable tests; the old source workspace's test suite is not present here.
- Runtime behavior changes need relevant game/plugin smoke verification when available. Build success alone does not certify engine or plugin compatibility; explicitly record skipped runtime checks.

## Documentation changes

- Keep notes, public usage and source paths consistent with the current checkout.
- Verify links, note scope, unique `metahook/` permalinks and source provenance.
- Do not report historical source-workspace test results as current standalone results.
- Do not write documentation/configuration snapshot tests or rebuild solely for prose changes.
- Use a Basic Memory project only after confirming its path is this checkout's `memory/`; otherwise edit local notes without touching `metahooksv`.

## Delivery

- Inspect the diff and working tree; exclude generated output, caches and unrelated changes.
- Report actual verification and any remaining limitations before declaring completion.
- Commit only when requested, using `<type>(scope): <summary>` and `Co-Authored-By: Codex <codex@openai.com>`.
- Do not push or publish local component commits without authorization. A clean local build does not prove remote recursive clone availability.

Related: [[metahook/code-styles]], [[metahook/suggested-commands]], [[metahook/game-data]].
