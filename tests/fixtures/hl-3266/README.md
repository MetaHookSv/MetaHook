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
  the dump's thread list. This initially suggested an abandoned CRT lock; the
  follow-up below identifies the responsible exit path in another reproduction.
  Do not count this restart scenario as passed.

The binary was not changed. Temporary game files and configuration were restored.

## Follow-up: GameUI socket-thread termination

A debugger trace and full dump from the same real-engine restart scenario
identified an abandoned UCRT lock on 2026-10-06:

- Main TID `0x35dc` called `GameUI.dll+0x29afe -> TerminateThread(0x1004, 0)`
  during `FreeLibrary` / DLL process detach. The handle identified TID `0xa2fc`,
  starting at `GameUI.dll+0x29ce0` (`SocketThreadFunc`). The debugger then recorded
  that thread's exit.
- The subsequent dump retained TID `0xa2fc` as owner of the UCRT critical section
  at `0x768b4448`, with recursion count 1. That TID was absent from the live list.
- Engine network TID `0x7678` had reached ThreadGuard's safe sleep boundary. Its
  `ExitThread -> LdrShutdownThread -> RtlpFlsDataCleanup -> destroy_fls ->
  destroy_ptd` path blocked acquiring that same lock. This is the UCRT multibyte
  data lock; the earlier dump blocked on the adjacent locale lock instead.

Thus the follow-up demonstrates a GameUI worker killed while owning a CRT lock,
which prevents the engine network worker from completing normal thread cleanup.
It does not justify bypassing thread cleanup or resuming engine resource cleanup
before the worker is signaled. The earlier dump's particular owner TID was not
traced, and other restart failures must not automatically be assigned this cause.

The local evidence is under ThreadGuard's ignored
`build/issue4/runtime-investigate-blob-mintrace/` (`0-debugger.log`,
`0-restart.dmp`), with the decoded worker stack in MetaHook's ignored
`build/issue4-tests/real-blob-mintrace-stacks.txt`.

### Binary/source correspondence

Compared the original
`D:/GoldSrc_VibeSignatures/bin/hl-3266/serverbrowser/ServerBrowser.dll.i64` with
`D:/HLND2T-DiligentGraphics/Tracker/common/Socket.cpp`. The installed ServerBrowser
binary matches the upstream binary byte-for-byte (SHA-256
`32aa55403b7f5aa3e816ae05b1ea7624865a46800931e9483b9feafc841b62d7`).
The installed GameUI SHA-256 is
`384449b55753370586619b6908d178b4151bbef86eda0833310dea6aa1384c6a`.

| Function | ServerBrowser RVA | GameUI RVA |
| --- | --- | --- |
| `SocketThreadFunc` | `0x13b40` | `0x29ce0` |
| `CSocketThread` destructor | `0x13930` | `0x29ad0` |
| Destructor's `TerminateThread` call | `0x1395e` | `0x29afe` |

RTTI identifies `CSocketThread`. Both binaries match the source's shutdown-event
poll at the unlocked loop head, 100 ms select timeout, and destructor sequence:
lock the socket list, set the shutdown event, sleep 2 ms, forcibly terminate the
worker, unlock, close handles, and delete the critical section.

The source is a guide, not an exact replacement for binary evidence: these DLLs
have no loop-tail `Sleep(1)`, drain `ReceiveData()` repeatedly until false, and
create the worker lazily in `AddSocketToThread`. The supplied source has a sleep,
one receive call per socket per iteration, and creation in the constructor.

The required repair is to request and join each identified socket worker before
`FreeLibrary`, outside both the loader lock and socket-list lock. Use the existing
unlocked shutdown-event check as the exit boundary, and make continuous receive
traffic observe the stop request. Replacing `TerminateThread` with a blocking
wait inside the destructor would deadlock. At diagnosis, ThreadGuard waited before
ServerBrowser unload but had no corresponding GameUI pre-unload wait, and its
zero-timeout wait override did not address receive-loop starvation.

### Working-tree repair verification

ThreadGuard now joins GameUI before engine FreeLibrary on PE and BLOB engines,
and hooks GameUI/ServerBrowser select and recvfrom to bound idle waits and stop
nonblocking receive draining. In `runtime-gameui-heap-trace/0-debugger.log`,
GameUI worker `0x70f4` exits before the pre-unload join message and before the
destructor's TerminateThread targeting that same, already-dead worker. Both
engine generations finish their network joins; the restart/quit run exits 0.
Another run (`runtime-gameui-prejoin/`) also exits 0.

The full installation is still not a consistently passing restart test:
`runtime-gameui-final-blob/` exits with `0xc0000374` during reload, and
`runtime-gameui-heap-capture/` hangs after both engine generations have completed
their network joins. Its main-thread stack is `doexit -> sub_1E0B1E0 ->
sub_1E32C40 -> Sleep(128)`, in the engine's small-block allocator spin loop,
not WaitForNetworkThread. Access violations in the embedded Steam networking
code include `0x1e17856`, also observed before this repair. The allocator/reload
failure's complete cause remains unresolved; these runs are not passes.

ThreadGuard Debug/Release builds and both CTests pass, including real UDP idle
and continuous-receive shutdown, main-thread receive forwarding, and unload
routing/engine-state gates. HL 10210 and CoF 5936 exit tests and Sven 10257's
map/restart/quit test exit 0 with the repair. The 11-snapshot catalog validates;
the other engine snapshots were not individually launched in this follow-up.
