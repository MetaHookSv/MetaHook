// Exercise the production catalog and resolver with real, independently loaded
// PE modules; direct catalog construction keeps test data independent of releases.
#include "../src/GameData.cpp"
#include <cassert>

bool MH_IsInLdrCriticalRegion() { return false; }

static void AddSymbol(uint64_t crc, const char* name, DWORD rva, mh_gamesymbol_kind_t kind = MH_GAMESYMBOL_KIND_FUNCTION)
{
    auto& module = g_catalog.modules[crc];
    if (!module) module = std::make_unique<ModuleCatalog>();
    auto record           = std::make_unique<GameSymbolRecord>();
    record->kind          = kind;
    record->rva           = rva;
    record->symbolSize    = 1;
    record->scalarValue   = 42;
    record->memberOffset  = 12;
    module->symbols[name] = std::move(record);
}

static void WriteCatalog(const std::string& root, const char* name, uint64_t crc)
{
    char hex[17];
    snprintf(hex, sizeof(hex), "%016llx", (unsigned long long)crc);
    std::string snapshot = R"({"schemaVersion":5,"source":{"snapshotSchemaVersion":8,"analysisOutputContractVersion":3},"binaries":{"client":{"windows":{"crc64":")";
    snapshot += hex;
    snapshot += R"(","alias":["client_orig.dll"]}}},"records":[{"platform":"windows","module":"client","symbolName":")";
    snapshot += name;
    snapshot += R"(","kind":"function","payload":{"func_rva":"0x1000","func_size":"0x1"}}]})";
    std::ofstream(root + "/snapshot.json", std::ios::binary) << snapshot;
    std::string index = R"({"schemaVersion":4,"versions":[{"gameVersion":"test-1","url":"snapshot.json","snapshotSchemaVersion":8,"sha256":")";
    index += ComputeSha256Hex(snapshot.data(), snapshot.size());
    index += "\",\"size\":" + std::to_string(snapshot.size()) + "}]}";
    std::ofstream(root + "/index.json", std::ios::binary) << index;
}

int main()
{
    std::vector<ClientAlias> aliases;
    rapidjson::Document      declaration;
    declaration.Parse(R"({"alias":["client_orig.dll","CLIENT_ORIG.DLL","client_org.dll"]})");
    assert(ReadClientAliases(declaration, 1, aliases));
    assert(2 == aliases.size());
    assert("client_orig.dll" == aliases[0].filename);
    declaration.Parse(R"({"alias":["valid.dll","../invalid.dll"]})");
    assert(!ReadClientAliases(declaration, 1, aliases));
    assert(2 == aliases.size());
    for (const char* invalid : {"", ".", "..", "a/b", "a\\b", "C:a", "a*", "a.", "a "})
        assert(!IsAliasFilename(invalid));
    auto         proxy = GetModuleHandleW(nullptr);
    std::wstring path;
    assert(GetModuleFilePathW(proxy, path));
    auto     directory    = path.substr(0, path.find_last_of(L"\\/") + 1);
    auto     originalPath = directory + L"client_orig.dll";
    uint64_t originalCRC = 0, proxyCRC = 0;
    assert(MH_GAMESYMBOL_OK == ComputeCrc64FromFile(originalPath, originalCRC));
    assert(MH_GAMESYMBOL_OK == GameData::GetModuleCRC64(proxy, &proxyCRC));
    g_catalog.available = true;
    g_catalog.clientAliases.push_back({"client_orig.dll", originalCRC});
    AddSymbol(originalCRC, "function", 0x1000);
    AddSymbol(originalCRC, "scalar", 0, MH_GAMESYMBOL_KIND_SCALAR);
    AddSymbol(originalCRC, "member", 0, MH_GAMESYMBOL_KIND_STRUCT_MEMBER);
    GameData::RegisterClientModule(proxy);
    PVOID address = nullptr;
    assert(MH_GAMESYMBOL_MODULE_NOT_FOUND == MH_ResolveGameSymbol(proxy, "function", MH_GAMESYMBOL_KIND_FUNCTION, &address));
    auto original = LoadLibraryW(originalPath.c_str());
    assert(original);
    assert(MH_GAMESYMBOL_OK == MH_ResolveGameSymbol(proxy, "function", MH_GAMESYMBOL_KIND_FUNCTION, &address));
    assert((BYTE*)original + 0x1000 == address);
    uint64_t crc = 0;
    assert(MH_GAMESYMBOL_OK == MH_GetModuleCRC64(proxy, &crc));
    assert(proxyCRC == crc);
    mh_gamesymbol_t metadata{};
    metadata.cbSize = sizeof(metadata);
    assert(MH_GAMESYMBOL_MODULE_NOT_FOUND == MH_QueryGameSymbolByCRC64(proxyCRC, "function", &metadata));
    assert(MH_GAMESYMBOL_OK == MH_QueryGameSymbol(proxy, "function", &metadata));
    assert(originalCRC == metadata.moduleCRC64);
    uint32_t value = 0;
    assert(MH_GAMESYMBOL_OK == MH_QueryGameSymbolScalar(proxy, "scalar", &value));
    assert(42 == value);
    assert(MH_GAMESYMBOL_OK == MH_QueryGameSymbolStructMember(proxy, "member", &value));
    assert(12 == value);
    assert(MH_GAMESYMBOL_OK == MH_IsGameSymbolAvailable(proxy, "function"));
    assert(MH_GAMESYMBOL_KIND_MISMATCH == MH_ResolveGameSymbol(proxy, "scalar", MH_GAMESYMBOL_KIND_FUNCTION, &address));
    AddSymbol(proxyCRC, "function", 0x1100);
    assert(MH_GAMESYMBOL_OK == MH_ResolveGameSymbol(proxy, "function", MH_GAMESYMBOL_KIND_FUNCTION, &address));
    assert((BYTE*)proxy + 0x1100 == address);
    g_catalog.modules[proxyCRC]->symbols["function"]->conflicted = true;
    assert(MH_GAMESYMBOL_CATALOG_CONFLICT == MH_IsGameSymbolAvailable(proxy, "function"));
    g_catalog.modules[proxyCRC]->symbols.clear();
    auto secondPath = directory + L"client_org.dll";
    auto second     = LoadLibraryW(secondPath.c_str());
    assert(second);
    uint64_t secondCRC = 0;
    assert(MH_GAMESYMBOL_OK == GameData::GetModuleCRC64(second, &secondCRC));
    assert(originalCRC != secondCRC);
    g_catalog.clientAliases.push_back({"client_org.dll", secondCRC});
    AddSymbol(secondCRC, "function", 0x1100);
    assert(MH_GAMESYMBOL_OK == MH_ResolveGameSymbol(proxy, "function", MH_GAMESYMBOL_KIND_FUNCTION, &address));
    assert((BYTE*)original + 0x1000 == address);
    g_catalog.modules[originalCRC]->symbols["function"]->rva = UINT32_MAX;
    assert(MH_GAMESYMBOL_RVA_OUT_OF_RANGE == MH_ResolveGameSymbol(proxy, "function", MH_GAMESYMBOL_KIND_FUNCTION, &address));
    g_catalog.modules[originalCRC]->symbols["function"]->rva = 0x1000;
    auto saved                                               = std::move(g_catalog.modules[originalCRC]->symbols["function"]);
    g_catalog.modules[originalCRC]->symbols.erase("function");
    assert(MH_GAMESYMBOL_OK == MH_ResolveGameSymbol(proxy, "function", MH_GAMESYMBOL_KIND_FUNCTION, &address));
    assert((BYTE*)second + 0x1100 == address);
    g_catalog.modules[originalCRC]->symbols["function"] = std::move(saved);
    g_catalog.clientAliases.erase(g_catalog.clientAliases.begin() + 1);
    FreeLibrary(second);
    g_catalog.clientAliases[0].crc64 ^= 1;
    assert(MH_GAMESYMBOL_SYMBOL_NOT_FOUND == MH_IsGameSymbolAvailable(proxy, "function"));
    g_catalog.clientAliases[0].crc64 = originalCRC;
    GameData::InvalidateModule(original, true);
    FreeLibrary(original);
    assert(MH_GAMESYMBOL_SYMBOL_NOT_FOUND == MH_IsGameSymbolAvailable(proxy, "function"));
    auto otherDirectory = directory + L"alias-other-" + std::to_wstring(GetCurrentProcessId());
    assert(CreateDirectoryW(otherDirectory.c_str(), nullptr));
    auto otherPath = otherDirectory + L"\\client_orig.dll";
    assert(CopyFileW(originalPath.c_str(), otherPath.c_str(), TRUE));
    auto other = LoadLibraryW(otherPath.c_str());
    assert(other);
    assert(MH_GAMESYMBOL_SYMBOL_NOT_FOUND == MH_IsGameSymbolAvailable(proxy, "function"));
    FreeLibrary(other);
    assert(DeleteFileW(otherPath.c_str()));
    assert(RemoveDirectoryW(otherDirectory.c_str()));
    original = LoadLibraryW(originalPath.c_str());
    assert(original);
    assert(MH_GAMESYMBOL_OK == MH_ResolveGameSymbol(proxy, "function", MH_GAMESYMBOL_KIND_FUNCTION, &address));
    assert((BYTE*)original + 0x1000 == address);
    std::vector<BYTE> mirror(GetPeImageSize(original));
    GameData::RegisterMirrorAlias(mirror.data(), original);
    assert(MH_GAMESYMBOL_OK == MH_ResolveGameSymbol(mirror.data(), "function", MH_GAMESYMBOL_KIND_FUNCTION, &address));
    assert(mirror.data() + 0x1000 == address);
    std::vector<BYTE> blob(mirror.size());
    int               ansiLength = WideCharToMultiByte(CP_ACP, 0, originalPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string       ansiPath(ansiLength, '\0');
    WideCharToMultiByte(CP_ACP, 0, originalPath.c_str(), -1, &ansiPath[0], ansiLength, nullptr, nullptr);
    GameData::RegisterModuleFileSource(blob.data(), ansiPath.c_str(), (ULONG)blob.size());
    assert(MH_GAMESYMBOL_OK == MH_ResolveGameSymbol(blob.data(), "function", MH_GAMESYMBOL_KIND_FUNCTION, &address));
    assert(blob.data() + 0x1000 == address);
    auto diagnostic = GameData::GetDiagnostics();
    assert(std::string::npos != diagnostic.find("not loaded"));
    assert(std::string::npos != diagnostic.find("CRC64 does not match"));
    GameData::ResetModuleIdentities();
    assert(MH_GAMESYMBOL_SYMBOL_NOT_FOUND == MH_IsGameSymbolAvailable(proxy, "function"));

    // Separate plugin indexes contribute symbols and deduplicate alias declarations.
    auto catalogRoot = ansiPath.substr(0, ansiPath.find_last_of("\\/") + 1) + "alias-catalog-" + std::to_string(GetCurrentProcessId());
    auto nestedRoot  = catalogRoot + "/plugin";
    assert(CreateDirectoryA(catalogRoot.c_str(), nullptr));
    assert(CreateDirectoryA(nestedRoot.c_str(), nullptr));
    WriteCatalog(catalogRoot, "root_symbol", originalCRC);
    WriteCatalog(nestedRoot, "plugin_symbol", originalCRC);
    const char* roots[] = {catalogRoot.c_str(), nestedRoot.c_str()};
    assert(GameData::Initialize(roots, 2));
    assert(1 == g_catalog.clientAliases.size());
    GameData::RegisterClientModule(proxy);
    for (auto name : {"root_symbol", "plugin_symbol"})
    {
        assert(MH_GAMESYMBOL_OK == MH_ResolveGameSymbol(proxy, name, MH_GAMESYMBOL_KIND_FUNCTION, &address));
        assert((BYTE*)original + 0x1000 == address);
    }
    for (const auto& root : {nestedRoot, catalogRoot})
    {
        assert(DeleteFileA((root + "/snapshot.json").c_str()));
        assert(DeleteFileA((root + "/index.json").c_str()));
        assert(RemoveDirectoryA(root.c_str()));
    }
    FreeLibrary(original);
    puts("gamedata alias tests passed");
}
