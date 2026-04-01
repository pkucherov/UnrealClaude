---
phase: 03-sse-parser-server-lifecycle
plan: 04
subsystem: server-lifecycle
tags: [cpp, unreal-engine, opencode, state-machine, process-management, tdd]

requires:
  - phase: 03-sse-parser-server-lifecycle
    plan: 01
    provides: IProcessLauncher, IHttpClient interfaces, EOpenCodeConnectionState, FOnConnectionStateChanged, backend constants, FMockProcessLauncher, FMockHttpClient
  - phase: 03-sse-parser-server-lifecycle
    plan: 03
    provides: FOpenCodeHttpClient (IHttpClient production implementation)

provides:
  - FOpenCodeServerLifecycle: six-state connection state machine (OpenCodeServerLifecycle.h / .cpp)
  - EnsureServerReady(): Disconnected → Detecting → Connecting/Spawning → Connecting
  - DetectExistingServer(): PID file read + IsApplicationRunning + health GET /global/health
  - TrySpawnServer(): port retry loop (4096→4099), PID file write, bIsManagedServer tracking
  - Shutdown(): managed-only termination + PID file cleanup
  - Exponential backoff: 1→2→4→8→16→30s, reset on MarkConnected()
  - ResolvePort(): ConfiguredPort → env OPENCODE_PORT → DefaultOpenCodePort(4096)
  - 19 TDD unit tests (OpenCodeServerLifecycleTests.cpp)

affects: [04-opencode-runner]

tech-stack:
  added: []
  patterns:
    - "Spawn success via OutProcessID != 0 (not FProcHandle::IsValid() — mock returns invalid handle)"
    - "PID file at <Project>/Saved/UnrealClaude/opencode-server.pid"
    - "Constructor-injected IProcessLauncher + IHttpClient for full testability"
    - "SetState() fires OnStateChanged delegate on every transition"

key-files:
  created:
    - UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeServerLifecycle.h
    - UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeServerLifecycle.cpp
    - UnrealClaude/Source/UnrealClaude/Private/Tests/OpenCodeServerLifecycleTests.cpp

key-decisions:
  - "Spawn success determined by OutProcessID != 0 rather than FProcHandle::IsValid() — mock returns default-constructed invalid handle per UE convention"
  - "bIsManagedServer=true set when PID file found (detection path) OR when we spawned the process — allows Shutdown to clean up either way"
  - "EnsureServerReady() is idempotent: returns immediately if CurrentState != Disconnected"
  - "MarkConnected() calls ResetBackoff() internally — single method, atomic guarantee"
  - "IncrementBackoff() doubles and caps at SSEReconnectBackoffMaxSeconds (30s); GetNextBackoffSeconds() returns BEFORE any increment"

requirements-completed: [SRVR-01, SRVR-02, SRVR-03, SRVR-04, SRVR-05, TEST-03]

duration: 15min
completed: 2026-04-01
---

# Phase 3 Plan 04: FOpenCodeServerLifecycle

**FOpenCodeServerLifecycle six-state server lifecycle manager implemented with 19 TDD tests**

## Performance

- **Duration:** 15 min
- **Started:** 2026-04-01T21:00:00Z
- **Completed:** 2026-04-01T21:15:00Z
- **Tasks:** 3 (RED tests, header, implementation)
- **Files created:** 3

## Accomplishments

- Created `OpenCodeServerLifecycleTests.cpp` — 19 RED tests covering all state transitions and behaviors
- Created `OpenCodeServerLifecycle.h` — Full class declaration with 6-state machine, public API, and private helpers
- Created `OpenCodeServerLifecycle.cpp` — Complete implementation:
  - **EnsureServerReady()**: idempotent guard → Detecting → DetectExistingServer / TrySpawnServer → Connecting or Disconnected
  - **DetectExistingServer()**: reads PID file, checks IsApplicationRunning, calls GET /global/health, handles stale PID cleanup and unhealthy-but-alive kill
  - **TrySpawnServer()**: loops MaxPortRetryCount (4) times, checks OutPID != 0 for success, writes PID file, fires ServerUnreachable on exhaustion
  - **Shutdown()**: TerminateProc only if bIsManagedServer + ManagedPID > 0; deletes PID file; resets to Disconnected
  - **ResolvePort()**: ConfiguredPort → OPENCODE_PORT env var → DefaultOpenCodePort (4096)
  - **Backoff**: GetNextBackoffSeconds / IncrementBackoff (doubles, cap 30s) / ResetBackoff (1s) / MarkConnected calls ResetBackoff
  - **PID file I/O**: WritePidFile / ReadPidFile / DeletePidFile using FFileHelper + IFileManager

## Task Commits

1. **RED tests** — `593238e` (test)
2. **GREEN implementation** — `39efbe0` (feat)

## Files Created

- `Private/OpenCode/OpenCodeServerLifecycle.h` — Class declaration (158 lines)
- `Private/OpenCode/OpenCodeServerLifecycle.cpp` — Full implementation (246 lines)
- `Private/Tests/OpenCodeServerLifecycleTests.cpp` — 19 TDD test cases (642 lines)

## Test Coverage (19 tests)

| Area | Tests |
|------|-------|
| State machine basics | InitialState_IsDisconnected, OnStateChanged_FiredOnTransition, GetConnectionState_ReturnsCurrentState |
| Detection | NoPidFile_ProceedsToSpawning, ValidPidAndHealthy_SkipsSpawning, StalePid_ProceedsToSpawning |
| Spawning | DefaultPortSpawns_WritesPidFile, SpawnParams_UseCorrectFormat, PortRetry_TriesNextPort, AllPortsFail_DisconnectedWithError |
| Shutdown | Shutdown_TerminatesManagedServer, Shutdown_LeavesUnmanagedServer, PidFile_DeletedOnShutdown |
| Backoff | Backoff_InitialValue, Backoff_DoublesAndCaps, Backoff_ResetReturnsToInitial, MarkConnected_ResetsBackoff, MarkReconnecting_SetsState |
| Configuration | ConfiguredPort_OverridesDefault, GetServerBaseUrl_ReturnsCorrectUrl, EnsureServerReady_IdempotentWhenNotDisconnected |

## Decisions Made

- Spawn success determined by `OutProcessID != 0` — FMockProcessLauncher returns default-constructed invalid FProcHandle per UE convention; lifecycle never calls `Handle.IsValid()` for success detection
- `bIsManagedServer=true` on both detection (existing PID file) and spawn paths so Shutdown correctly terminates either way
- `MarkConnected()` atomically resets backoff to prevent caller needing two separate calls

## Deviations from Plan

None — all 19 behaviors from the spec tested and implemented.

## Issues Encountered

None.

## Next Phase Readiness

- Phase 3 is COMPLETE — all 4 plans executed
- Phase 4 (FOpenCodeRunner) can now proceed — depends on all Phase 3 artifacts:
  - `FOpenCodeSSEParser` (03-02)
  - `FOpenCodeHttpClient` (03-03)
  - `FOpenCodeServerLifecycle` (03-04)

---
*Phase: 03-sse-parser-server-lifecycle*
*Completed: 2026-04-01*
