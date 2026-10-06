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
failure was unresolved at that point; the spin-loop diagnosis below now identifies
the skipped engine-worker join. These runs are not passes.

ThreadGuard Debug/Release builds and both CTests pass, including real UDP idle
and continuous-receive shutdown, main-thread receive forwarding, and unload
routing/engine-state gates. HL 10210 and CoF 5936 exit tests and Sven 10257's
map/restart/quit test exit 0 with the repair. The 11-snapshot catalog validates;
the other engine snapshots were not individually launched in this follow-up.

## Follow-up: engine exit gate skips the old Steam workers

The 2026-10-06/07 investigation reproduced the spin-loop hang with the same
unmodified fixture and the merged ThreadGuard repair. The failure is a separate
lifecycle gap in ThreadGuard's general engine-worker wait:

1. In the real HL 3266 binary, `CEngine::Unload` at `0x1dbbd80` calls
   `Sys_ShutdownGame`, then writes zero to `this + 8` at `0x1dbbd88`.
   `CEngine::GetState` at `0x1dbbf20` reads that field. The state is therefore
   already `DLL_INACTIVE` when `EngineAPI->Run` returns to the launcher.
2. `launcher.cpp` calls `MH_ExitGame` after `Run` returns and before
   `FreeBlobModule` invokes the engine's CRT detach. ThreadGuard's
   `IPluginsV4::ExitGame` calls `Engine_WaitForShutdown`, but that function
   accepts only `DLL_CLOSE` or `DLL_RESTART`. It skips both `StartTermination`
   and `WaitForAliveThreadsToShutdown` when the state is zero.
3. The runtime breakpoint in `runtime-allocator-exitgate-v2/0-debugger.log`
   confirms `g_Engine + 8` (`0x02492030`) is zero on both restart and final exit.
   The callback arguments are respectively `iResult = 1` and `iResult = 0`.
   GameUI/ServerBrowser joins execute earlier, but the engine-manager join does
   not execute at either callback. The early `NET_Shutdown` hook still joins
   NET_ThreadFunc correctly; its successful trace does not cover Steam workers.
4. The old Steam worker can consequently outlive the engine's CRT heap and
   continue during the next load into the same BLOB address range. Its stack
   retains pointers into the previous engine lifetime's allocations.

### Captured access-violation chain

`runtime-allocator-python/` reproduced the hang and contains first-chance full
dumps for all three exceptions, on the same worker TID `0x8520`:

- At `0x1e17856`, the GeneralDirectoryServer connection routine's exception
  cleanup executes `mov [eax+8], ecx`, with `eax = 0x071e1910`. This WSABUFInfo
  storage is no longer readable in the dump. The wrapper itself remains on the
  worker's stack at `0x1ff2fa98`; its vector is `[0x071e1910, 0x071e1920)`.
  The allocator lock at `0x024f83a8` is still zero at this first exception.
- At `0x1e18243`, the wrapper destructor encounters the same invalid storage.
- During exception unwinding, the destructor calls `sub_1E32C40` with
  `(0x071e1910, 0x10)`. At `0x1e32d3d`, `mov [eax], edx` faults while returning
  that block to the 16-byte free list. The allocator has already exchanged its
  lock to 1. The release instruction at `0x1e32d41` is not reached.
- This worker is the embedded Steam discovery routine `sub_1E0A650`, parameter
  1, entered through the engine's `_beginthread` wrapper at `0x1e67817`.
  It installs the SEH-to-C++ exception translator `sub_1E19C50`. At the third
  exception the main thread is already in the next engine's Renderer `GL_Init`.
- Later main-thread CRT cleanup enters the same allocator with its lock left
  at 1. Its exponential-backoff wait saturates at `Sleep(128)`. This explains
  the hang without attributing it to another forced termination of a lock owner.

Decoded evidence is in MetaHook's ignored `build/issue4-tests/`:
`allocator-av-first.txt`, `allocator-av-chain.txt`, `allocator-main.txt`, and
`allocator-old-dump.txt`. The last file independently confirms lock value 1 in
the earlier `runtime-gameui-heap-capture/0-restart.dmp`.

### Repair direction and diagnostic verification

The general engine-manager join must run at the actual `ExitGame` lifecycle
boundary, even after `CEngine::Unload` has reset the runtime state. Keep the
earlier GameUI/ServerBrowser gates at their respective FreeLibrary boundaries
and keep the early NET_Shutdown join. Do not clear the allocator lock, skip CRT
destructors, or terminate the Steam worker while it may own resources.

As a diagnostic only, a CDB breakpoint set `g_Engine + 8` to `DLL_RESTART`
immediately before ThreadGuard's ExitGame callback. In
`runtime-allocator-forcegate/0-debugger.log`, the engine manager at `0x075b8128`
now enters its join, and worker `0x91e0` exits before the next engine load.
Three real-fixture map/restart/quit runs with this temporary gate override and
a 1-second RCON reply timeout exited 0. This is evidence for the repair
direction, not a production implementation or a full engine-family regression.
The production repair should use the lifecycle boundary, not write engine state.

Unmodified runs under CDB also sometimes pass: six initial runs with its
5-second RCON reply timeout passed, while the lightweight debugger with a
1-second reply timeout reproduced the failure immediately. Timing differs, so
passing retries alone cannot establish correctness. An initial gate-breakpoint
attempt stopped on an unresolved inline symbol and timed out before startup;
that harness failure is excluded from game results. Temporary game files were
restored after each experiment. No production source or fixture binary was
changed in this investigation. The separate `0xc0000374` reload crash has not
been independently proven to share this cause.

### Production fix verification

[ThreadGuard PR #7](https://github.com/MetaHookSv/ThreadGuard/pull/7) removes the runtime-state gate from
`Engine_WaitForShutdown`, so the existing ExitGame callback always requests and
joins its manager before engine CRT detach. The module FreeLibrary gates remain
intact. Its real-thread regression first failed on the old inactive-state path,
then passed with the repair; it also covers close/restart/active states, a missing
engine pointer, repeated calls, and a missing manager.

With the repaired Release plugin and no debugger state override, five fresh
real-fixture `de_dust2 -> _restart -> quit` runs in `runtime-engine-exit-fixed/`
all exited 0. None recorded the three access violations or a heap-corruption
exception. These used the lightweight debugger and 1-second RCON reply timeout
that reproduced the original failure. Debug/Release plugin builds and both
CTests passed, and all 11 gamedata snapshots validated. Temporary game files
matched their saved backups after restoration. This does not establish that
every other HL 3266 reload failure has the same cause.

Cross-family checks with the same plugin also exited 0: HL 10210 and CoF 5936
startup/quit, and Sven 10257 `osprey -> _restart -> quit`. Their artifacts are
`runtime-engine-exit-hl/`, `runtime-engine-exit-cof/`, and
`runtime-engine-exit-sven/`. The other seven catalog snapshots were not launched.
