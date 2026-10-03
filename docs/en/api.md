[Back to README](../../README.md) | [中文](../zh-CN/api.md)

# Game Symbol API (API 109, extended through API 114)

MetaHookSv API version 109 adds a public game symbol query/resolution API. It is backed by a local gamedata catalog that is synchronized at build time (see `scripts/sync-gamedata.py`) and read at runtime from `<game>\<mod>\metahook\gamedata\`.

API version 110 appends `MH_GAMESYMBOL_KIND_PATCH` and the `IsGameSymbolAvailable` slot without moving any existing slot.

API version 111 appends `MH_GAMESYMBOL_KIND_SCALAR` and the `QueryGameSymbolScalar` slot without moving any existing slot. The gamedata contract moves to dataset schema 5 / source snapshot contract 8 / analysis output contract 3, which is the only accepted generation.

API version 112 appends `MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION`. It adds no function slot: the kind is consumed through the existing `ResolveGameSymbol` (and reported by `QueryGameSymbol`).

API version 113 appends `MH_GAMESYMBOL_KIND_VTABLE`; it uses the existing `ResolveGameSymbol` slot.

API version 114 appends `MH_GAMESYMBOL_KIND_STRUCT_MEMBER` and `QueryGameSymbolStructMember` after all existing function slots. Existing slot offsets remain unchanged.

API version 115 appends `DWORD vfuncIndex` to `mh_gamesymbol_t`, reporting a `VIRTUAL_FUNCTION` record's owning vtable slot. It adds no function slot. `mh_gamesymbol_t` is a versioned struct: a module built against an earlier header passes its own smaller `cbSize` and simply does not receive the new field, so this addition stays source- and binary-compatible in both directions.

Plugins must check `g_pInterface->MetaHookAPIVersion` against the version that introduced each function, kind or struct field (114 for `QueryGameSymbolStructMember`, 115 for `mh_gamesymbol_t::vfuncIndex`). All returned string/pattern pointers are owned by MetaHook and remain valid until process exit; do not free or modify them.

## Types

### `mh_gamesymbol_kind_t`

```cpp
typedef enum mh_gamesymbol_kind_e
{
	MH_GAMESYMBOL_KIND_UNKNOWN = 0,
	MH_GAMESYMBOL_KIND_FUNCTION = 1,
	MH_GAMESYMBOL_KIND_GLOBAL = 2,
	MH_GAMESYMBOL_KIND_PATCH = 3,
	MH_GAMESYMBOL_KIND_SCALAR = 4,
	MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION = 5,
	MH_GAMESYMBOL_KIND_VTABLE = 6,
	MH_GAMESYMBOL_KIND_STRUCT_MEMBER = 7
} mh_gamesymbol_kind_t;
```

`PATCH` denotes a single call/jump instruction address (`patch_rva`) that can be redirected; its signature is metadata only and is never interpreted as a function body or length.

`SCALAR` (API 111) denotes a plain `uint32` value tied to the matched binary identity, not an address. It is never resolved by `ResolveGameSymbol` and its value must not have an image base added or be dereferenced; query it with `QueryGameSymbolScalar`.

`VIRTUAL_FUNCTION` (API 112) denotes a function entry recovered from its owning vtable slot (`func_rva`). It is address-bearing and resolved by `ResolveGameSymbol` exactly like `FUNCTION` (`moduleBase + rva`). Its owning slot index is reported through `vfuncIndex` (API 115).

`VTABLE` (API 113) denotes the address of a virtual function table array.

`STRUCT_MEMBER` (API 114) denotes a `uint32` byte offset from the beginning of an object. Query it with `QueryGameSymbolStructMember`; it is never an address.

### `mh_gamesymbol_status_t`

```cpp
typedef enum mh_gamesymbol_status_e
{
	MH_GAMESYMBOL_OK = 0,
	MH_GAMESYMBOL_INVALID_ARGUMENT = 1,
	MH_GAMESYMBOL_OUTPUT_TOO_SMALL = 2,
	MH_GAMESYMBOL_GAMEDATA_UNAVAILABLE = 3,
	MH_GAMESYMBOL_MODULE_PATH_UNAVAILABLE = 4,
	MH_GAMESYMBOL_MODULE_HASH_FAILED = 5,
	MH_GAMESYMBOL_MODULE_NOT_FOUND = 6,
	MH_GAMESYMBOL_SYMBOL_NOT_FOUND = 7,
	MH_GAMESYMBOL_UNSUPPORTED_KIND = 8,
	MH_GAMESYMBOL_KIND_MISMATCH = 9,
	MH_GAMESYMBOL_RVA_OUT_OF_RANGE = 10,
	MH_GAMESYMBOL_CATALOG_CONFLICT = 11
} mh_gamesymbol_status_t;
```

### `mh_pattern_t` and `mh_gamesymbol_t`

```cpp
typedef struct mh_pattern_s
{
	const char* text;          // canonical signature text
	const BYTE* bytes;         // parsed bytes (authoritative, lossless)
	const BYTE* mask;          // 0 == wildcard, non-zero == match bytes[i]
	const char* legacyPattern; // wildcards encoded as 0x2A; NULL if a literal 0x2A is present
	DWORD length;
} mh_pattern_t;

typedef struct mh_gamesymbol_s
{
	DWORD cbSize;              // set to sizeof(mh_gamesymbol_t) before calling
	mh_gamesymbol_kind_t kind;
	DWORD flags;
	uint64_t moduleCRC64;

	DWORD rva;
	DWORD symbolSize;
	DWORD signatureRva;
	mh_pattern_t signature;

	DWORD instructionOffset;   // global only
	DWORD operandOffset;       // global only
	DWORD instructionLength;   // global only

	DWORD vfuncIndex;          // virtualFunction only (API 115)
} mh_gamesymbol_t;
```

## Functions

| Function | Purpose |
| --- | --- |
| `GetModuleCRC64(moduleBase, &crc64)` | Lazily compute and cache the CRC-64/XZ of the original module file backing `moduleBase`. |
| `QueryGameSymbolByCRC64(crc64, name, &symbol)` | Query normalized metadata by module CRC64 + canonical (case-sensitive) symbol name. |
| `QueryGameSymbol(moduleBase, name, &symbol)` | Hash the module, then query by CRC64. Does not convert RVA to VA. |
| `ResolveGameSymbol(moduleBase, name, expectedKind, &address)` | Resolve to `moduleBase + rva`; `expectedKind` must be FUNCTION, GLOBAL, PATCH, VIRTUAL_FUNCTION or VTABLE, and a mismatch returns `MH_GAMESYMBOL_KIND_MISMATCH`. |
| `SearchPatternMasked(base, len, bytes, mask, plen)` | Search with an explicit mask (literal `0x2A` has no special meaning). |
| `GetGameSymbolStatusString(status)` | Return a static, MetaHook-owned English string for a status. |
| `IsGameSymbolAvailable(moduleBase, name)` (API 110) | Return `MH_GAMESYMBOL_OK` when the exact, case-sensitive name exists, `MH_GAMESYMBOL_SYMBOL_NOT_FOUND` when it does not, and the original status for every other failure (never converted to "not found"). It does not return an address. |
| `QueryGameSymbolScalar(moduleBase, name, &value)` (API 111) | Return the `uint32` value of a scalar record. Returns `MH_GAMESYMBOL_KIND_MISMATCH` when the symbol exists with a non-scalar kind. The value is consumed verbatim: no image base, no dereference. |
| `QueryGameSymbolStructMember(moduleBase, name, &offset)` (API 114) | Return the `uint32` byte offset of a structMember record. Returns `MH_GAMESYMBOL_KIND_MISMATCH` for another kind. The offset is relative to an object; no image base is added. |

`QueryGameSymbol` / `QueryGameSymbolByCRC64` are versioned through `cbSize`: the caller initializes `outSymbol->cbSize` to `sizeof(mh_gamesymbol_t)` as it was compiled, and only `min(cbSize, sizeof(mh_gamesymbol_t))` bytes are written. A caller built against an older, smaller shape therefore keeps working and receives every field it can hold; a `cbSize` below the versioned prefix (everything through `signature`) returns `MH_GAMESYMBOL_OUTPUT_TOO_SMALL`. On failure the output fields are zeroed while `cbSize` is preserved. Scalar and structMember records report their kind with zero address fields; obtain the value or offset from their dedicated query function. A virtualFunction record reports its owning vtable slot in `vfuncIndex`, and zero for every other kind.

`IsGameSymbolAvailable` accepts no wildcard or numeric-range syntax. Callers that need a contiguous family of numbered records, such as `Cvar_Set_to_Cvar_DirectSet_callsite_0..N`, build each exact name themselves, probe with `IsGameSymbolAvailable`, and call `ResolveGameSymbol` only for names that exist.

Scalars and struct members use the same `(moduleCRC64, symbolName)` catalog key as address records. The two value-query functions return `MH_GAMESYMBOL_KIND_MISMATCH` for another kind; `ResolveGameSymbol` accepts only address-bearing kinds. A catalog built from an unsupported dataset generation (anything other than dataset schema 5 / source snapshot contract 8 / analysis output contract 3) is rejected per snapshot and recorded as a diagnostic.

APIs that accept `moduleBase` require the module to remain loaded for the duration of the call. MetaHook invalidates the module CRC cache and any mirror aliases when the module unloads; an address returned by `ResolveGameSymbol` is valid only until that module instance unloads.

For the features added in V4, see [Features](features.md); for ABI compatibility and behavioral changes, see [Compatibility](compatibility.md).
