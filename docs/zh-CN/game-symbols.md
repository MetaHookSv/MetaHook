[返回 README](../../README.md) | [English](../en/game-symbols.md)

# 游戏符号 API（API 109，扩展至 API 114）

MetaHookSv API 版本 109 新增了一套公开的游戏符号查询/解析 API。它由本地 gamedata catalog 支撑：catalog 在构建期同步（见 `scripts/sync-gamedata.py`），运行时从 `<game>\<mod>\metahook\gamedata\` 读取。

API 版本 110 在末尾追加 `MH_GAMESYMBOL_KIND_PATCH` 与 `IsGameSymbolAvailable` 槽位，不移动任何已有槽位。

API 版本 111 在末尾追加 `MH_GAMESYMBOL_KIND_SCALAR` 与 `QueryGameSymbolScalar` 槽位，不移动任何已有槽位。gamedata 契约同步升级为 dataset schema 5 / source snapshot contract 8 / analysis output contract 3，这是唯一接受的代际。

API 版本 112 追加 `MH_GAMESYMBOL_KIND_VIRTUAL_FUNCTION`。它不新增函数槽位：该 kind 通过现有的 `ResolveGameSymbol` 消费（`QueryGameSymbol` 也会如实返回该 kind）。

API 版本 113 追加 `MH_GAMESYMBOL_KIND_VTABLE`，沿用 `ResolveGameSymbol` 槽位。

API 版本 114 追加 `MH_GAMESYMBOL_KIND_STRUCT_MEMBER` 与 `QueryGameSymbolStructMember` 槽位，不移动已有槽位。

API 版本 115 在 `mh_gamesymbol_t` 末尾追加 `DWORD vfuncIndex`，报告 `VIRTUAL_FUNCTION` 记录所属的 vtable 槽位，不新增函数槽位。`mh_gamesymbol_t` 是版本化结构体：用更早的头编译的模块按自身较小的 `cbSize` 调用，只是拿不到新字段，因此本次追加在源码与二进制两个方向上都是兼容的。

插件调用前须检查所用函数、kind 或结构体字段对应的 `MetaHookAPIVersion`（`QueryGameSymbolStructMember` 要求 114，`mh_gamesymbol_t::vfuncIndex` 要求 115）。所有返回的字符串/pattern 指针都由 MetaHook 持有、进程退出前有效；请勿释放或修改。

## 类型

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

`PATCH` 表示可被重定向的单条 call/jump 指令地址（`patch_rva`）；其 signature 仅为元数据，不会被当作函数体或长度解释。

`SCALAR`（API 111）表示按 binary identity 绑定的纯 `uint32` 数值，而非地址。它不会被 `ResolveGameSymbol` 解析，其数值不得加 image base、不得解引用，请使用 `QueryGameSymbolScalar` 获取。

`VIRTUAL_FUNCTION`（API 112）表示从所属 vtable 槽位恢复的函数入口（`func_rva`）。它是地址型记录，由 `ResolveGameSymbol` 按与 `FUNCTION` 相同的方式解析为 `moduleBase + rva`。其所属槽位索引由 `vfuncIndex` 报告（API 115）。

`VTABLE`（API 113）表示虚函数表数组的地址。

`STRUCT_MEMBER`（API 114）表示对象起始位置起算的 `uint32` 字节偏移。使用 `QueryGameSymbolStructMember` 查询，不能当作地址解析。

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

### `mh_pattern_t` 与 `mh_gamesymbol_t`

```cpp
typedef struct mh_pattern_s
{
	const char* text;          // canonical signature 文本
	const BYTE* bytes;         // 解析后的字节（权威、无损）
	const BYTE* mask;          // 0 == 通配，非 0 == 精确匹配 bytes[i]
	const char* legacyPattern; // 通配编码为 0x2A；若存在字面 0x2A 则为 NULL
	DWORD length;
} mh_pattern_t;

typedef struct mh_gamesymbol_s
{
	DWORD cbSize;              // 调用前设为 sizeof(mh_gamesymbol_t)，或旧形态的大小
	mh_gamesymbol_kind_t kind;
	DWORD flags;
	uint64_t moduleCRC64;

	DWORD rva;
	DWORD symbolSize;
	DWORD signatureRva;
	mh_pattern_t signature;

	DWORD instructionOffset;   // 仅 global
	DWORD operandOffset;       // 仅 global
	DWORD instructionLength;   // 仅 global

	DWORD vfuncIndex;          // 仅 virtualFunction（API 115）
} mh_gamesymbol_t;
```

## 函数

| 函数 | 用途 |
| --- | --- |
| `GetModuleCRC64(moduleBase, &crc64)` | 惰性计算并缓存 `moduleBase` 对应原始模块文件的 CRC-64/XZ。 |
| `QueryGameSymbolByCRC64(crc64, name, &symbol)` | 按模块 CRC64 + canonical（区分大小写）符号名查询规范化元数据。 |
| `QueryGameSymbol(moduleBase, name, &symbol)` | 先计算模块哈希，再按 CRC64 查询；不把 RVA 转成 VA。 |
| `ResolveGameSymbol(moduleBase, name, expectedKind, &address)` | 解析为 `moduleBase + rva`；`expectedKind` 必须为 FUNCTION / GLOBAL / PATCH / VIRTUAL_FUNCTION / VTABLE，kind 不符时返回 `MH_GAMESYMBOL_KIND_MISMATCH`。 |
| `SearchPatternMasked(base, len, bytes, mask, plen)` | 用显式 mask 搜索（字面 `0x2A` 无特殊含义）。 |
| `GetGameSymbolStatusString(status)` | 返回 MetaHook 持有的静态英文字符串。 |
| `IsGameSymbolAvailable(moduleBase, name)`（API 110） | 精确、区分大小写的符号名存在时返回 `MH_GAMESYMBOL_OK`，不存在时返回 `MH_GAMESYMBOL_SYMBOL_NOT_FOUND`，其它失败保留原状态码（绝不转换为“不存在”）。不返回地址。 |
| `QueryGameSymbolScalar(moduleBase, name, &value)`（API 111） | 返回 scalar 记录的 `uint32` 数值；符号存在但 kind 非 scalar 时返回 `MH_GAMESYMBOL_KIND_MISMATCH`。数值按原样消费：不加 image base、不解引用。 |
| `QueryGameSymbolStructMember(moduleBase, name, &offset)`（API 114） | 返回 structMember 记录的 `uint32` 字节偏移；kind 不符时返回 `MH_GAMESYMBOL_KIND_MISMATCH`。偏移相对于对象，不加 image base。 |

`QueryGameSymbol` / `QueryGameSymbolByCRC64` 通过 `cbSize` 做版本化：调用方将 `outSymbol->cbSize` 设为其编译时已知的 `sizeof(mh_gamesymbol_t)`，实现只写入 `min(cbSize, sizeof(mh_gamesymbol_t))` 字节。因此用更早、更小的形态编译的调用方仍可正常工作，并拿到它能容纳的全部字段；`cbSize` 小于版本化前缀（直到 `signature` 为止）时返回 `MH_GAMESYMBOL_OUTPUT_TOO_SMALL`。失败时输出字段会被清零，同时保留 `cbSize`。scalar 与 structMember 记录会返回对应 kind，地址字段为 0；数值或偏移通过各自专用接口获取。virtualFunction 记录在 `vfuncIndex` 中报告所属 vtable 槽位，其它 kind 恒为 0。

`IsGameSymbolAvailable` 不支持通配符或数字区间语法。需要连续编号记录族（如 `Cvar_Set_to_Cvar_DirectSet_callsite_0..N`）的调用方自行拼接精确名字，先用 `IsGameSymbolAvailable` 探测存在性，仅对存在的名字调用 `ResolveGameSymbol` 取地址。

scalar、structMember 与地址型记录共用同一 catalog 与 `(moduleCRC64, symbolName)` identity。两个数值查询接口遇到其它 kind 返回 `MH_GAMESYMBOL_KIND_MISMATCH`；`ResolveGameSymbol` 只接受地址型 kind。由不支持的 dataset 代际（非 dataset schema 5 / source snapshot contract 8 / analysis output contract 3）构建的 snapshot 会被逐个拒绝并记为诊断。

接受 `moduleBase` 的 API 要求模块在调用期间保持已加载。模块卸载时 MetaHook 会失效其 CRC 缓存及全部 mirror aliases；`ResolveGameSymbol` 返回的地址仅在该模块加载实例卸载前有效。

V4 新增的特性见[功能特性](metahook-api.md)；ABI 兼容性与行为变化见[兼容性](compatibility.md)。
