---
phase: 03-sse-parser-server-lifecycle
plan: 01
subsystem: infrastructure
tags: [cpp, unreal-engine, opencode, interfaces, enums, mocks]

requires:
  - phase: 02-backend-abstraction
    provides: IChatBackend, ChatBackendTypes.h, TestUtils.h FMockChatBackend

provides:
  - EOpenCodeErrorType enum (7 values) in ChatBackendTypes.h
  - EChatStreamEventType extended with PermissionRequest, StatusUpdate, AgentThinking
  - FChatStreamEvent.ErrorType field (EOpenCodeErrorType, backwards-compatible)
  - UnrealClaudeConstants::Backend - 8 new constants (health poll, shutdown timeout, backoff, ports, PID, env var, binary name)
  - EOpenCodeConnectionState enum (6 states) in Private/OpenCode/OpenCodeTypes.h
  - FOnConnectionStateChanged delegate
  - IProcessLauncher pure virtual interface
  - IHttpClient pure virtual interface + FOpenCodeHttpClient class declaration
  - FMockProcessLauncher and FMockHttpClient in TestUtils.h

affects: [03-02, 03-03, 03-04, 04-opencode-runner]

tech-stack:
  added: []
  patterns:
    - "Constructor-injectable interfaces (IProcessLauncher, IHttpClient) for test isolation"
    - "Mock pattern: bLaunchShouldSucceed flag + call counters + SimulateSSE helpers"

key-files:
  created:
    - UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeTypes.h
    - UnrealClaude/Source/UnrealClaude/Private/OpenCode/IProcessLauncher.h
    - UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeHttpClient.h
  modified:
    - UnrealClaude/Source/UnrealClaude/Public/ChatBackendTypes.h
    - UnrealClaude/Source/UnrealClaude/Public/UnrealClaudeConstants.h
    - UnrealClaude/Source/UnrealClaude/Private/Tests/TestUtils.h

key-decisions:
  - "EOpenCodeErrorType placed BEFORE FChatStreamEvent (not after, as originally noted) — required by C++ declaration order"
  - "FMockProcessLauncher exposes bLaunchShouldSucceed + OutPID != 0 as spawn success indicator since UE FProcHandle default-constructs as invalid"
  - "OpenCode headers in TestUtils.h wrapped in #if WITH_DEV_AUTOMATION_TESTS guard to avoid pulling OpenCode headers into non-test builds"

requirements-completed: [COMM-03, SRVR-01, SRVR-05, TEST-02, TEST-03]

duration: 12min
completed: 2026-04-01
---

# Phase 3 Plan 01: Foundation Types, Interfaces, Constants, and Mock Implementations

**Shared OpenCode type contracts established: EChatStreamEventType extended, EOpenCodeErrorType/EOpenCodeConnectionState enums defined, IProcessLauncher/IHttpClient interfaces created, and FMockProcessLauncher/FMockHttpClient test utilities added**

## Performance

- **Duration:** 12 min
- **Started:** 2026-04-01T20:00:00Z
- **Completed:** 2026-04-01T20:12:00Z
- **Tasks:** 2
- **Files modified:** 6 (2 modified, 4 created)

## Accomplishments
- Extended `EChatStreamEventType` with `PermissionRequest`, `StatusUpdate`, `AgentThinking` (backwards-compatible additions before `Unknown`)
- Created `EOpenCodeErrorType` (7 values) and added `ErrorType` field to `FChatStreamEvent` with default `None`
- Added 8 Backend constants to `UnrealClaudeConstants` (health poll interval, shutdown timeout, SSE backoff, port retry count, PID filename, env var, binary name)
- Created `Private/OpenCode/` directory with `OpenCodeTypes.h`, `IProcessLauncher.h`, `OpenCodeHttpClient.h`
- Extended `TestUtils.h` with `FMockProcessLauncher` and `FMockHttpClient` inside `WITH_DEV_AUTOMATION_TESTS` guard

## Task Commits

1. **Task 1: Extend ChatBackendTypes.h and UnrealClaudeConstants.h** — `6af85e4` (feat)
2. **Task 2: Create OpenCode types, interfaces, and mock implementations** — `7be2bf1` (feat)

## Files Created/Modified
- `Public/ChatBackendTypes.h` — EOpenCodeErrorType enum, 3 new EChatStreamEventType values, ErrorType field on FChatStreamEvent
- `Public/UnrealClaudeConstants.h` — 8 new Backend namespace constants
- `Private/OpenCode/OpenCodeTypes.h` — EOpenCodeConnectionState (6 states), FOnConnectionStateChanged delegate
- `Private/OpenCode/IProcessLauncher.h` — Pure virtual process management interface (4 methods)
- `Private/OpenCode/OpenCodeHttpClient.h` — IHttpClient interface (4 methods) + FOpenCodeHttpClient class declaration
- `Private/Tests/TestUtils.h` — FMockProcessLauncher + FMockHttpClient with SSE simulation helpers

## Decisions Made
- `EOpenCodeErrorType` placed BEFORE `FChatStreamEvent` struct (C++ declaration order requirement — plan's note about "after struct" was incorrect)
- `FMockProcessLauncher::LaunchProcess` returns default-constructed (invalid) `FProcHandle`; spawn success determined by `bLaunchShouldSucceed` flag and `OutProcessID != 0` pattern
- OpenCode includes in TestUtils.h wrapped inside `#if WITH_DEV_AUTOMATION_TESTS` to prevent header pollution in non-test builds

## Deviations from Plan

**1. [Rule 1 - Bug] EOpenCodeErrorType placed before FChatStreamEvent**
- **Found during:** Task 1
- **Issue:** Plan specified adding enum after FChatStreamEvent, but struct uses the type — forward declaration order violation
- **Fix:** Placed EOpenCodeErrorType definition between EChatStreamEventType and FChatStreamEvent
- **Verification:** EOpenCodeErrorType referenced correctly in ErrorType field
- **Committed in:** 6af85e4

---
**Total deviations:** 1 auto-fixed (1 declaration order bug)
**Impact on plan:** Required correction — C++ cannot use a type before it's declared. No scope creep.

## Issues Encountered
None.

## Next Phase Readiness
- Plans 03-02 (SSE Parser), 03-03 (HTTP Client), and 03-04 (Server Lifecycle) can all proceed — all shared types and interfaces are in place
- FMockProcessLauncher and FMockHttpClient ready for 03-04's TDD test suite

---
*Phase: 03-sse-parser-server-lifecycle*
*Completed: 2026-04-01*
