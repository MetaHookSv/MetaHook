---
title: GameData
type: note
permalink: metahook/game-data
tags:
- metahook
- gamedata
- symbol-catalog
- api-115
- crc64
---

# GameData

## 概览与迁移范围

GameData 是 launcher 与插件共享的本地符号 catalog，从 `<game>/<mod>/metahook/gamedata/index.json` 以及所有嵌套的 `gamedata/**/index.json` 读取并冻结只读符号表，按 `(moduleCRC64, symbolName)` 查询。根 index.json 必须存在且合法；嵌套 index.json 为尽力合并，单个失败只记入 diagnostics，不使整份 catalog 失败。模块 CRC-64/XZ 从原始二进制文件懒计算，不由游戏名或当前内存镜像代替。

本笔记从 MetaHookSv 的 [GameData 原笔记](https://github.com/hzqst/MetaHookSv/blob/11a852774b1725d02735aeb348c32a7bf454507c/memory/GameData.md) 提取本体架构、API 契约与通用经验；逐插件迁移日志、旧 MSBuild 验证记录保留在原笔记。这里描述移植基线 API 115 的最终状态，不沿用原笔记中已被后续条目取代的 API 112、vtable unsupported 或严格 `cbSize` 规则。

## 职责与源码入口

- `src/GameData.h`：初始化、查询、模块来源注册、镜像 alias 和失效入口。
- `src/GameData.cpp`：catalog 解析/冻结、各 kind 规范化、签名解析、模块哈希状态机和公共 `MH_*` 查询实现。
- `include/metahook.h`：`METAHOOK_API_VERSION 115`、`mh_gamesymbol_t`、kind/status 枚举与尾部追加的 API 槽位。
- `src/metahook.cpp`：初始化 catalog、注册 PE/blob/mirror 身份，按数据决定引擎类型并解析本体所需地址。
- `src/LoadDllNotification.cpp`：加载/卸载事件触发模块身份失效，传递 loader-critical-region 状态。
- `scripts/sync-gamedata.py`：读取 manifest，HTTPS index 下载、原始快照持久缓存、按 manifest 裁剪（symbols + 字段）、稳定命名发布；`--validate-only` 离线校验并核对 manifest 覆盖。
- `scripts/manifests/metahook.json`：launcher 的 manifest（支持的 gameVersions、必需/可选 symbols、编号 patch set、替代组、条件组、字段裁剪表）。symbols 支持按 module 分组（`{engine:{名:kind},client:{...}}`）与条件组 `conditionalGroups`（按 gameVersion 应用）。插件可提供同 schema 的 manifest 复用同步器，并发布到嵌套的 `<mod>/metahook/gamedata/<plugin>/`（launcher 会与其 catalog 取并集）。
- `scripts/validate-gamedata.py`：manifest 模式的发布门禁（`--manifest`）；`--full-catalog` 保留外部插件完整消费者门禁，需完整上游 catalog。
- `CMakeLists.txt`：构建前同步目标、配置选项和安装路径；RapidJSON 与 Chocobo1Hash 由固定 submodule 提供。

## 长期架构约定

- **触发信号**：新增或迁移 catalog 消费、定位引擎符号、审查相关 API 或门禁。
- **根因/约束**：上游负责符号数据正确性；消费端统一承担查询和地址解析，不重复实现定位体系。
- **正确做法**：信任上游，复用现有解析、查询和发布检查。已发布地址通过 `MH_LoadEngine_ResolveSymbol` / `MH_ResolveGameSymbol` 获取；插件使用 `ResolveGameSymbol`，不存在 signature、字符串、反查或函数体扫描 fallback。已提供地址的 call-site 同样直接 Resolve。
- **失败处理**：缺少必需数据沿用诊断并终止相应加载；补数由上游完成。存在性探测与元数据查询不能替代最终地址 Resolve。
- **验证方式**：核对最终地址来源、旧 fallback 是否清除及真实消费者；执行与行为有关的验证和现有门禁，不为假设的坏 RVA、长度或跨 snapshot 数据另建防御体系。
- **适用范围**：launcher、公共 API 与外部消费者。原决策于 2026-09-08、issue #850 确认。

## Catalog 数据流与生命周期

1. `GameData::Initialize(const char* const* gamedataRoots, size_t gamedataRootCount)` 接收一组 gamedata 根目录：`gamedataRoots[0]` 为主根，其 index.json 缺失/非法直接返回 false；其余根按尽力合并。metahook.cpp 的 `MH_LoadEngine_CollectGamedataRoot` 负责递归发现 `gamedata/**/index.json`（主根恒为首项，子目录按名排序、深度优先，跳过 reparse point）。
2. 每个根读取自己的 `<root>/index.json` 并校验 index schema 4；随后按该根目录解析 snapshot url，校验路径安全、大小和 SHA-256，只提取 Windows records。snapshot 校验 snapshot dataset schema 5、source snapshot contract 8、analysis contract 3。单个 snapshot 失败记入 diagnostics，不破坏其余 catalog。
3. 将记录规范化为 `GameSymbolRecord`，按 CRC64 和大小写敏感的 symbolName 建表；完全一致的重复记录去重，内容不同返回 `CATALOG_CONFLICT`。多个 index 声明同一 gameVersion 时**取并集**：每个不同的 (url, sha256) 声明都会被加载，其符号并入同一张表；只有 url+sha256 完全相同的声明才作为重复跳过。这是插件自带 catalog（如 `metahook/gamedata/renderer/`）能与 launcher catalog 合并的前提。
4. catalog 冻结后不重载；返回的签名文本、bytes、mask、legacyPattern 指针有效至进程退出。
5. 按 `moduleBase` 管理 PE/blob/None 来源。`RegisterModuleFileSource` 注册 blob 原始文件，`RegisterMirrorAlias` 关联镜像与真实模块。

模块哈希状态从 `Uncomputed` 到 `Computing` 再到 Ready/Failed。首次查询在锁外做文件 I/O；其它线程通过条件变量等待。文件读取期间大小/mtime 变化返回 `MODULE_HASH_FAILED`；无可用来源返回 `MODULE_PATH_UNAVAILABLE`。

loader 临界区内用 lock-free 原子队列 `g_pendingModuleInvalidations[64]` 和 `g_resetModuleIdentitiesPending` 排队失效，溢出时保守全量失效；在下一个安全查询点 `DrainPendingModuleInvalidations`。每个 engine session 的 `ResetModuleIdentities` 提供兜底，不能在 loader lock 内添加阻塞哈希操作。

## 当前 kind 与 API 契约

| Kind | 规范化与消费方式 |
| --- | --- |
| FUNCTION | `func_rva` / `func_size`；`func_sig` 可省略，存在则必须是格式合法的字符串。Resolve 返回模块基址加 RVA。 |
| GLOBAL | 直接全局地址以及已发布的引用指令元数据；按全局实际类型决定是否解引用，不能统一多解引用一次。 |
| PATCH | 消费 `patch_rva` 指向的目标指令；不搜索 signature，不把 signature 当函数大小。 |
| SCALAR | 仅 uint32 `scalar_value`，地址字段为 0；经 `QueryGameSymbolScalar` 返回，不加 image base。 |
| VIRTUAL_FUNCTION | 有 `func_rva` 和 `func_size` 时存储地址及 `vfuncIndex`；签名优先 `vfunc_sig`，其次 `func_sig`，也可省略。 |
| VTABLE | 消费 `vtable_rva` / `vtable_size`，Resolve 得到虚表数组地址，不是虚函数地址。 |
| STRUCT_MEMBER | 消费对象内 uint32 offset，经 `QueryGameSymbolStructMember` 获取，不作为地址 Resolve。 |

- `virtualFunction` 必须有合法 `vfunc_index` 和非空 `vtable_name`。地址字段必须同时有或同时无；slot-only 声明无地址时接受但**不入符号表**，查询仍返回 NOT_FOUND。不能把它解析成 `moduleBase + 0`。这是一种有意的数据形态，不应要求上游拆 kind 或补地址；运行时按槽解析仍待真实消费者需求。
- `MH_ResolveGameSymbol` 接受 FUNCTION/GLOBAL/PATCH/VIRTUAL_FUNCTION/VTABLE，保留既有 kind、RVA/size 溢出和映像边界检查；不校验内存签名、不跨版本扫描。
- `IsGameSymbolAvailable` 是精确名字的存在性探测，不返回地址或检查 expected kind。除了 OK/NOT_FOUND，其余失败状态原样保留。
- 连续编号 PATCH 集由调用方从 `_0` 起自行探测；API 不提供枚举。首项或中途异常的处理由既有消费者契约决定。
- scalar/structMember 的专用查询遇到其它 kind 返回 KIND_MISMATCH，普通失败时清零输出。
- 函数缺省签名时 `signatureText` 为空、`signatureRva = 0`；地址型 virtualFunction 仅在有签名时设置 signature RVA。
- signature 接受空格分隔的两位十六进制字节和 `??` 通配。`bytes + mask` 是权威表示；有字面 `0x2A` 时不生成旧式星号通配字符串。

### API 版本演进与 cbSize

- 109：六个 catalog/CRC64/Resolve/masked search/status API 槽。
- 110：PATCH 和 `IsGameSymbolAvailable`（第七槽）。
- 111：SCALAR 和 `QueryGameSymbolScalar`（第八槽）。
- 112：地址型 VIRTUAL_FUNCTION；113：VTABLE，均未新增槽。
- 114：STRUCT_MEMBER 和尾部 `QueryGameSymbolStructMember` 槽。
- 115：`mh_gamesymbol_t` 末尾追加 `DWORD vfuncIndex`，不增加函数槽；仅 VIRTUAL_FUNCTION 使用，其它 kind 为 0。

API 115 把旧的 `cbSize < sizeof(mh_gamesymbol_t)` 拒绝规则改成版本化有界填充。下界为 `offsetof(mh_gamesymbol_t, instructionOffset)`，低于此前缀才 OUTPUT_TOO_SMALL。`FillSymbolBounded` 先构造完整本地结果，再拷贝 `min(cbSize, sizeof(mh_gamesymbol_t))` 字节，保留调用方原 cbSize；失败清零也有界。旧头调用方仍可查询，只是看不到新尾字段。不得恢复按当前结构体大小整体写入或严格拒绝旧大小的实现。

## Launcher 集成

- `MH_LoadEngine` 初始化 catalog 并注册来源/alias。
- `MH_LoadEngine_DetermineEngineType` 按引擎模块 CRC64 → `GetGameVersion` → 前缀/版本阈值分类；`build_number` 保留给既有 API 消费者，不再决定引擎身份。
- `MH_LoadEngine_ResolveGlobalOperand` 是从发布的 `signatureRva + instructionOffset + operandOffset` 求引用指令操作数的特殊路径，不改变 GLOBAL 直接地址契约。
- cvar callback 先探测原生 `cvar_hooks`；不存在时解析编号 `Cvar_Set_to_Cvar_DirectSet_callsite_N` PATCH 并转到 managed callback 链表。
- DLL 通知通过 `InvalidateModule` 使缓存失效。详见 [[metahook/privatevars/metahook-privatevars]]。

## 构建、安装与验证边界

`METAHOOK_SYNC_GAMEDATA=ON` 时，CMake 每次构建调用同步器（带 `--manifest scripts/manifests/metahook.json`），输出默认在 `build/x86/<configuration>/assets/svencoop/metahook/gamedata`：`index.json` 加每个版本的稳定 `<gameVersion>.json`（如 `hl-4554.json`），只保留 manifest 声明的 launcher symbols 与 GameData 实际读取的字段。原始上游快照与最后一次 index 持久缓存在 `build/x86/<configuration>/gamedata-sync/raw`，后续构建复用；index 不可达时回退缓存 index 以支持离线构建，缓存不在构建间清理。安装到 `install/x86/<configuration>/svencoop/metahook/gamedata`。OFF 仅安装已有数据，不触发下载。

发布门禁为 `scripts/validate-gamedata.py <dir> --manifest scripts/manifests/metahook.json`（校验 manifest 覆盖）；`--full-catalog` 才运行面向外部插件的完整消费者门禁，需要完整上游 catalog，不能用于裁剪输出。插件可提供自己的同 schema manifest 与本插件 gamedata 目录（launcher 会合并嵌套 `metahook/gamedata/**/index.json`）。

完整校验器保留源仓库的消费者门禁：按 `(module, name)` 建键，允许同名跨模块，不允许同一模块名字的冲突记录。运行时则按 `(moduleCRC64, name)` 建键；共享 DLL 可使不同游戏使用同一 catalog 身份。上游有记录不等于每个消费者都需要它：新增必需门禁前先确认真实读取/调用点、可达游戏集合及模块身份。

迁移时已有的同步校验、构建证据见 [[metahook/build-and-verification]]；原笔记的 pytest、插件构建和发布验证是**源仓库历史结果**，不是本仓库已执行结果。`scripts/tests` 未迁入；本体运行、旧 cbSize 调用方和插件加载尚未在游戏内复核。命令见 [[metahook/suggested-commands]]。
