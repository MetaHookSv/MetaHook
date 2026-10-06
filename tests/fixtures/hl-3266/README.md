# Original HL 3266 BLOB engine

`hw.dll` is a byte-for-byte copy of the original engine supplied for integration
testing at `D:/GoldSrc_VibeSignatures/bin/hl-3266/engine/hw.dll`. It is not the
decrypted PE and is not a generated BLOB fixture.

SHA-256: `e3faf0b5f1f694b02208ee905148d7febcde9eb8ea093c1251ce6a3dda1bb65e`

Use this file with the real `MetaHook_blob.exe` launcher and a compatible game
installation. Keep the complete game assets and dependencies outside this
fixture directory. The isolated `blob_import_tests` CTest remains a separate
synthetic unit test; passing it does not constitute testing this engine.

## Local integration verification

Tested with MetaHook main `c2281f6d74c60106f8efd968a883866f32dd560b`,
the published ThreadGuard gamedata and the existing CS 3266 installation:

- Release and Debug launchers loaded this exact fixture, reported engine build
  3266, installed ThreadGuard's named `select` hook and exited with code 0.
- The extended `de_dust2 -> _restart -> quit` run hung. The dump shows the main
  thread in `ThreadGuard!WaitForNetworkThread`, and the network worker in
  `ExitThread -> LdrShutdownThread -> ucrtbase -> RtlEnterCriticalSection`.
  The critical section at `0x768b4430` records owner TID `0x99e8`, absent from
  the dump's thread list. This suggests an abandoned CRT lock; the responsible
  exit path has not been identified. Do not count this restart scenario as passed.

The binary was not changed. Temporary game files and configuration were restored.
