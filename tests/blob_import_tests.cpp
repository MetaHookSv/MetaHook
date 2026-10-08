// Exercise the real BLOB loader with an in-memory fixture and real Win32
// ordinal/name exports. Only the hook allocation sink and unused host services
// are stubbed; this test does not duplicate the loader's matching algorithm.
#include "../src/LoadBlob.cpp"
#include <cstdio>
#include <cstdlib>

#define CHECK(x)                                                      \
    do                                                                \
    {                                                                 \
        if (!(x))                                                     \
        {                                                             \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); \
            std::abort();                                             \
        }                                                             \
    } while (0)

IFileSystem*      g_pFileSystem      = NULL;
IFileSystem_HL25* g_pFileSystem_HL25 = NULL;
PVOID             MH_GetSectionByName(PVOID, const char*, ULONG*) { return NULL; }
void*             MH_SearchPattern(void*, DWORD, const char*, DWORD) { return NULL; }
void*             MH_SearchPatternNoWildCard(void*, DWORD, const char*, DWORD) { return NULL; }
void              MH_SysError(const char*, ...) { std::abort(); }
static ULONG_PTR* hookedSlot = NULL;
hook_t*           MH_CreateIATHook(HMODULE, BlobHandle_t, const char*, const char*, void* replacement, void** original, ULONG_PTR* slot)
{
    hookedSlot = slot;
    if (original) *original = (void*)*slot;
    *slot = (ULONG_PTR)replacement;
    return (hook_t*)slot;
}
static BOOL WINAPI FixtureDllMain(HINSTANCE, DWORD, void*) { return TRUE; }
static void        Replacement() {}

int main()
{
    const DWORD imageSize = 4096;
    auto        image     = (BYTE*)VirtualAlloc(NULL, imageSize * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    CHECK(image != NULL);
    const DWORD       importOffset = 256, ordinalThunkOffset = 384, namedThunkOffset = 400;
    const DWORD       socketNameOffset = 512, kernelNameOffset = 544, sleepNameOffset = 576;
    const size_t      dataOffset = sizeof(BlobInfo_t) + sizeof(BlobHeader_t) + sizeof(BlobSection_t);
    std::vector<BYTE> fixture(dataOffset + imageSize, 0);
    auto              header = (BlobHeader_t*)(fixture.data() + sizeof(BlobInfo_t));
    header->m_dwImageBase    = (DWORD)image ^ 0x49C042D1;
    header->m_dwImportTable  = ((DWORD)image + importOffset) ^ 0x872C3D47;
    header->m_dwEntryPoint   = (DWORD)FixtureDllMain + 12;
    header->m_dwExportPoint  = 0x7A32BC85;
    // The BLOB format's section count is the last section index.
    header->m_wSectionCount     = 0;
    auto section                = (BlobSection_t*)(fixture.data() + sizeof(BlobInfo_t) + sizeof(BlobHeader_t));
    section->m_dwVirtualAddress = (DWORD)image;
    section->m_dwVirtualSize    = imageSize;
    section->m_dwDataSize       = imageSize;
    section->m_dwDataAddress    = (DWORD)dataOffset;
    auto data                   = fixture.data() + dataOffset;
    auto imports                = (IMAGE_IMPORT_DESCRIPTOR*)(data + importOffset);
    imports[0].Name             = socketNameOffset;
    imports[0].FirstThunk       = ordinalThunkOffset;
    imports[1].Name             = kernelNameOffset;
    imports[1].FirstThunk       = namedThunkOffset;
    std::memcpy(data + socketNameOffset, "wsock32.dll", sizeof("wsock32.dll"));
    std::memcpy(data + kernelNameOffset, "kernel32.dll", sizeof("kernel32.dll"));
    std::memcpy(data + sleepNameOffset + sizeof(WORD), "Sleep", sizeof("Sleep"));
    // Winsock's documented ordinal for select, deliberately loaded without a name.
    *(DWORD*)(data + ordinalThunkOffset) = IMAGE_ORDINAL_FLAG32 | 18;
    *(DWORD*)(data + namedThunkOffset)   = sleepNameOffset;
    BYTE key                             = 0x57;
    for (size_t i = sizeof(BlobInfo_t); i < fixture.size(); ++i)
    {
        const BYTE plain = fixture[i];
        fixture[i]       = plain ^ key;
        key += plain + 0x57;
    }
    auto blob = LoadBlobFromBuffer(fixture.data(), (DWORD)fixture.size(), image, imageSize * 2);
    CHECK(blob != NULL);
    CHECK(MH_BlobHasImport(blob, "WSOCK32.dll"));
    CHECK(MH_BlobHasImportEx(blob, "wsock32.dll", "select"));
    CHECK(MH_BlobHasImportEx(blob, "kernel32.dll", "Sleep"));
    CHECK(!MH_BlobHasImportEx(blob, "kernel32.dll", "select"));
    CHECK(!MH_BlobHasImportEx(blob, "wsock32.dll", "not_an_export"));
    void* original = NULL;
    CHECK(MH_BlobIATHook(blob, "wsock32.dll", "select", Replacement, &original));
    CHECK(hookedSlot == (ULONG_PTR*)(image + ordinalThunkOffset));
    CHECK(original == (void*)GetProcAddress(GetModuleHandleA("wsock32.dll"), "select"));
    // Matching is based on the retained import, even after its slot is hooked.
    CHECK(MH_BlobHasImportEx(blob, "wsock32.dll", "select"));
    CHECK(MH_BlobIATHook(blob, "wsock32.dll", "select", original, NULL));
    CHECK(*(void**)(image + ordinalThunkOffset) == original);
    CHECK(MH_BlobIATHook(blob, "kernel32.dll", "Sleep", Replacement, &original));
    CHECK(hookedSlot == (ULONG_PTR*)(image + namedThunkOffset));
    CHECK(MH_BlobHasImportEx(blob, "kernel32.dll", "Sleep"));
    FreeBlobModule(blob);
    CHECK(VirtualFree(image, 0, MEM_RELEASE));
    std::puts("PASS real BLOB loader: ordinal/name discovery, hook routing, retained identity and cleanup");
}
