[Back to README](../../README.md) | [中文](../zh-CN/compatibility.md)

# Compatibility

This page covers ABI compatibility with older MetaHook plugins and the behavioral changes introduced by MetaHookSv (V4) compared to MetaHook (V2). For the features added in V4, see [Features](features.md); for the gamedata-backed symbol API, see [Game Symbol API](api.md).

## ABI Compatibility

The `g_pMetaHookAPI` interface exported by MetaHookSv (V4) is fully compatible with plugins from the MetaHook (V2) era at the ABI level, so there are no compatibility issues when loading plugins from the MetaHook (V2) period. The only compatibility consideration is between different plugins.

## Behavioral Changes of MetaHookSv (V4) Compared to MetaHook (V2)

### API Behavior Change: g_pMetaHookAPI->GetVideoMode

This API is used to obtain information related to VideoMode, such as game resolution, color depth, and whether it is in windowed mode. In MetaHook (V2), this API only retrieved information from the registry, resulting in inaccurate information with little reference value. In MetaHookSv (V4), if the `IVideoMode` interface is available in the engine, it will first attempt to use the `IVideoMode` interface to obtain relevant information; if that fails, it will fallback to the previous method of retrieval.

### API Behavior Change: g_pMetaHookAPI->GetEngineBase

In the MetaHook (V2) version, `g_pMetaHookAPI->GetEngineBase()` incorrectly returned 0x1D01000 instead of 0x1D00000 for BLOB encrypted versions of the engine (e.g., 3266). However, 0x1D01000 is actually the starting address of the code segment, not the engine base address. As a result, some plugins that relied on the V2 API would hard-code an incorrect offset based on this erroneous engine base (these plugins would use the correct offset of -0x1000 to counteract the V2 API's incorrect behavior, but this workaround could lead to an incorrect final offset if it encountered an API that returned the correct result). If a plugin uses the new `IPlugins` interface (`METAHOOK_PLUGIN_API_VERSION003` or `METAHOOK_PLUGIN_API_VERSION004`), it will return the correct 0x1D00000 for BLOB encrypted versions of the engine by default. Otherwise, the plugin will still receive the "incorrect" return value for the Blob engine as it did in the MetaHook (V2) era.

### Automatic Ignoring of Duplicate Plugins

If a plugin appears twice in the loading list or is loaded by another DLL in some other way, MetaHookSv will not load that plugin again.
* Duplicate loading may lead to situations where a plugin replaces function pointers and ends up calling itself, resulting in infinite recursion.

### Support for Hook "Transactions"

During the engine's call to all plugins' `LoadEngine`, a "transaction" will be opened for all `InlineHook`, `VFTHook`, and `IATHook` requests. The hooks will only take effect after all plugins' `LoadEngine` interface calls have completed. This allows different plugins to simultaneously `SearchPattern` and hook the same function, avoiding conflicts where the previous plugin's early hook modifies the engine code, causing the subsequent plugin's pattern search to fail.

The transaction opening timing includes: during the engine's calls to all plugins' `LoadEngine` and `LoadClient`, during the engine's call to the client's `HUD_GetStudioModelInterface`, and during the DllLoadNotification period.
