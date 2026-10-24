---
title: Gamedata unversioned tags
type: note
permalink: metahook/gamedata-unversioned-tags
---

# Gamedata unversioned tags

- Trigger: GoldSrc_VibeSignatures v20261024a adds the standalone ModDB Decay client under the unversioned `decay` tag.
- Root cause: every component parses the entire upstream index before selecting its manifest entries. The old versioned-only regular expression rejects `decay` even when the component does not consume that identity.
- Correct handling: allow `decay` explicitly alongside the existing versioned syntax in each standalone synchronizer. Continue rejecting unknown unversioned tags. Keep component manifests scoped to their actual symbol consumption; accepting an index identity does not require selecting it.
- Verification (2026-10-24): the 15 affected synchronizers each reproduced rejection before the fix. All 16 consumers, including VGUI2Extension, then passed their Python suites (75 tests total), parsed the hash-verified official archive's 27-snapshot index, and published/pruned/validated their own manifests. The 200 previously selected component snapshots retained byte-identical pruned output. VGUI2Extension adds one Decay snapshot retaining only client/g_iVisibleMouse.
- Publication constraint: Pages still lacked `decay` at verification time. The 15 unchanged manifests can select their existing identities; VGUI2Extension's expanded manifest requires a Pages deployment containing Decay before default online sync can succeed. An available release archive alone does not update the mutable Pages endpoint.
- Scope: synchronizer/parser and catalog validation only. No native code changed and no native build or game run was performed. Temporary verification evidence lives under C:/Users/HZDEV/AppData/Local/Temp/vgui2extension-release-check-163b7a2af3ae46419e53b07bdac2423c/all-components.
