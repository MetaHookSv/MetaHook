#if defined(METAHOOK_BLOB_SUPPORT)

// This buffer is only a placeholder that materializes the ".blob" section; no
// code references it. C linkage gives it a stable name so the blob target can
// force it in with /INCLUDE, which keeps the section from being dropped by
// LTCG and /OPT:REF (Release) while a blob engine is loaded into it at runtime.
extern "C" {
#pragma bss_seg(".blob")
__declspec(allocate(".blob"))
unsigned char g_pBlobBuffer[0x3000000];
}

#endif