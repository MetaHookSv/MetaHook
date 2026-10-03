[返回 README](../../README.md) | [English](../en/compatibility.md)

# 兼容性

本页涵盖与旧版 MetaHook 插件的 ABI 兼容性，以及 MetaHookSv (V4) 相比 MetaHook (V2) 的行为变化。V4 新增的特性见[功能特性](features.md)；由 gamedata 支撑的符号 API 见[游戏符号 API](api.md)。

## ABI兼容性

MetaHookSv (V4) 导出的 `g_pMetaHookAPI` 接口从ABI层面完全兼容 MetaHook (V2) 时期的插件，所以加载 MetaHook (V2) 时期的插件也没有任何兼容性问题。唯一需要考虑的是与其他插件之间的兼容性。

## MetaHookSv (V4) 相比 MetaHook (V2) 的行为变化

### API行为改变：g_pMetaHookAPI->GetVideoMode

该API用于获取VideoMode相关信息，如游戏分辨率、色深、是否窗口模式等信息。

在MetaHook (V2)中该API只会从注册表中获取信息，得到的信息很不准确，基本上没有参考价值

MetaHookSv (V4)在引擎中的`IVideoMode`接口可用的情况下，会先尝试使用`IVideoMode`接口获取相关信息，如果失败才会fallback到以前的获取方式。

### API行为改变：g_pMetaHookAPI->GetEngineBase

MetaHook (V2)版本的 `g_pMetaHookAPI->GetEngineBase()` 对BLOB加密版本的引擎(如3266)会错误返回 0x1D01000 而非 0x1D00000。

然而实际上 0x1D01000 是代码段起始地址而非引擎基址。因此某些依赖V2版本API的插件会依赖错误的引擎基址而硬编码一个错误的偏移（这些插件使用正确偏移-0x1000来达到抵消V2API错误行为的目的，但是这种抵消手法如果遇上返回正确结果的API就会导致算出的最终偏移比正常的大0x1000）。

如果插件使用新的`IPlugins`接口（`METAHOOK_PLUGIN_API_VERSION003`或`METAHOOK_PLUGIN_API_VERSION004`）则默认对BLOB加密版本的引擎返回正确的0x1D00000。否则该插件仍会像MetaHook (V2)时期那样对Blob引擎获取到“错误”的返回值。

### 自动忽略重复插件

如果一个插件在加载列表里出现了两次，或者被别的dll以其他方式加载进来了，MetaHookSv不会重复加载该插件。

* 重复加载可能会导致插件替换函数指针后出现自己调用自己的情况，引发无限递归。

### Hook的"事务"支持

引擎调用所有插件的 `LoadEngine` 期间会对所有 `InlineHook`, `VFTHook` 和 `IATHook` 请求开启"事务"。

直到所有插件的 `LoadEngine` 接口调用结束才会让hook真正生效

这样就可以允许不同插件同时 `SearchPattern` 和 hook 同一个函数，避免了因为前一个插件提前hook修改了引擎代码导致后一个插件搜索特征码失败等插件之间互相冲突的问题。

事务开启时机：引擎调用所有插件的`LoadEngine`和`LoadClient`期间、引擎调用客户端的 `HUD_GetStudioModelInterface` 期间以及DllLoadNotification期间。
