---
phase: 03-sse-parser-server-lifecycle
plan: 02
subsystem: sse-parser
tags: [cpp, unreal-engine, opencode, tdd, sse, parser]

requires:
  - phase: 03-sse-parser-server-lifecycle
    plan: 01
    provides: ChatBackendTypes.h (EChatStreamEventType+EOpenCodeErrorType), FChatStreamEvent, IHttpClient, IProcessLauncher

provides:
  - FOpenCodeSSEParser class (stateful SSE chunk-to-event parser)
  - ProcessChunk() — accumulates partial chunks, emits events on blank-line boundaries
  - Reset() — discards buffer state for reconnect scenarios
  - GetLastRetryMs() — exposes SSE retry: field value
  - Full OpenCode event type mapping (10 type strings → EChatStreamEventType)
  - GlobalEvent wrapper extraction (payload field + flat fallback)
  - EOpenCodeErrorType classification for session.error events (401→AuthFailure, 429→RateLimit, 500→InternalError)
  - 21 TDD unit tests covering all behavior specifications

affects: [03-03, 03-04, 04-opencode-runner]

tech-stack:
  added: []
  patterns:
    - "Stateful SSE buffer pattern (mirrors FClaudeCodeRunner::NdjsonLineBuffer)"
    - "TDD: RED commit (tests) then GREEN commit (implementation)"
    - "FJsonSerializer::Deserialize for SSE data payload parsing"
    - "GlobalEvent wrapper extraction with flat-payload fallback"

key-files:
  created:
    - UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeSSEParser.h
    - UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeSSEParser.cpp
    - UnrealClaude/Source/UnrealClaude/Private/Tests/OpenCodeSSEParserTests.cpp

key-decisions:
  - "FlushEvent dispatches synchronously (no AsyncTask) — caller is responsible for thread dispatch; tests rely on synchronous firing per D-21 note in RESEARCH.md anti-patterns section"
  - "ToolPart state=completed check maps to ToolResult; all other states map to ToolUse"
  - "SSE event: field accepted but JSON type field always wins (event: field stored, not used in mapping)"
  - "session.error with code=0 → ServerUnreachable (no code field = network-level failure)"

requirements-completed: [COMM-02, COMM-03, TEST-02]

duration: 18min
completed: 2026-04-01
---

# Phase 3 Plan 02: FOpenCodeSSEParser (TDD)

**FOpenCodeSSEParser built and tested: stateful SSE chunk parser with 10 event type mappings, partial-chunk reassembly, malformed JSON handling, and 21 passing unit tests**

## Performance

- **Duration:** 18 min
- **Started:** 2026-04-01T20:20:00Z
- **Completed:** 2026-04-01T20:38:00Z
- **Tasks:** 2 (RED + GREEN)
- **Files created:** 3

## Accomplishments
- Created `OpenCodeSSEParser.h` — FOpenCodeSSEParser class declaration with ProcessChunk/Reset/GetLastRetryMs public API and 5 private mapping methods
- Created `OpenCodeSSEParser.cpp` — Full implementation: ProcessChunk buffer loop, FlushEvent, ParseEventPayload with GlobalEvent wrapper extraction, MapMessagePartUpdated (TextPart/ToolPart/StepPart/ReasoningPart), MapSessionStatus, MapSessionError
- Created `OpenCodeSSEParserTests.cpp` — 21 TDD unit tests covering: all 10 event types, malformed JSON, missing type field, comment lines, retry: field, multi-event batch, split-across-chunks, partial line, Reset(), event: field ignored, session.error 401/429/500, empty data line

## Task Commits

1. **RED — TDD test suite** — `7df1c8e` (test)
2. **GREEN — parser implementation** — `650696c` (feat)

## Files Created/Modified
- `Private/OpenCode/OpenCodeSSEParser.h` — FOpenCodeSSEParser class (90 lines)
- `Private/OpenCode/OpenCodeSSEParser.cpp` — Implementation (270 lines)
- `Private/Tests/OpenCodeSSEParserTests.cpp` — 21 tests (450 lines)

## Decisions Made
- FlushEvent fires OnEvent synchronously (no AsyncTask) — RESEARCH.md anti-patterns section warns that AsyncTask(GameThread) deadlocks in UE automation test context; the caller (FOpenCodeRunner, Phase 4) will wrap in AsyncTask for production use
- ToolPart detection priority: step type field checked first (step-start/step-finish/reasoning), then content/delta (TextPart), then callID/tool (ToolPart)
- JSON type field always wins over SSE event: field — event: value stored in CurrentEventName but not used in MapEventType routing
- session.error code=0 mapped to ServerUnreachable (missing code = network failure before HTTP response)

## Deviations from Plan
None.

## Issues Encountered
None.

## Next Phase Readiness
- Plan 03-03 (FOpenCodeHttpClient implementation) can proceed — no dependencies on SSEParser
- Plan 03-04 (FOpenCodeServerLifecycle TDD) depends on 03-03 completing first

---
*Phase: 03-sse-parser-server-lifecycle*
*Completed: 2026-04-01*
