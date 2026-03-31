---
phase: 02-backend-abstraction
plan: 01
subsystem: api
tags: [abstraction-layer, interface, polymorphism, factory-pattern, unreal-engine, c++]

# Dependency graph
requires:
  - phase: 01-test-baseline
    provides: 281 baseline tests across 14 components ensuring no regressions during refactor
provides:
  - IChatBackend 9-method pure virtual interface (backend-agnostic)
  - EChatBackendType enum with Claude/OpenCode values and string serialization
  - FChatRequestConfig base config with FClaudeRequestConfig derived specialization
  - FChatBackendFactory with type dispatch, binary detection, and auto-detect
  - FChatSubsystem with runtime backend swapping via SetActiveBackend()
  - FChatSessionManager, SChatEditorWidget, SChatInputArea, SChatToolbar (all renamed from Claude-prefixed)
  - FMockChatBackend test utility implementing all 9 IChatBackend methods
  - 19 new abstraction tests (enum serialization, config hierarchy, factory, subsystem routing)
affects: [03-sse-parser-server-lifecycle, 04-opencode-runner-sessions, 05-chat-ui-backend-switching]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "IChatBackend pure virtual interface pattern for backend polymorphism"
    - "FChatBackendFactory with DetectPreferredBackend/Create/IsBinaryAvailable"
    - "FChatRequestConfig base + FClaudeRequestConfig derived for backend-specific config"
    - "static_cast<const FClaudeRequestConfig&> inside FClaudeCodeRunner for Claude-specific field access"
    - "Backend swap guard: IsExecuting() blocks SetActiveBackend() during active requests"

key-files:
  created:
    - UnrealClaude/Source/UnrealClaude/Public/ChatBackendTypes.h
    - UnrealClaude/Source/UnrealClaude/Public/IChatBackend.h
    - UnrealClaude/Source/UnrealClaude/Public/ClaudeRequestConfig.h
    - UnrealClaude/Source/UnrealClaude/Private/ChatBackendFactory.h
    - UnrealClaude/Source/UnrealClaude/Private/ChatBackendFactory.cpp
    - UnrealClaude/Source/UnrealClaude/Public/ChatSubsystem.h
    - UnrealClaude/Source/UnrealClaude/Private/Tests/ChatBackendAbstractionTests.cpp
  modified:
    - UnrealClaude/Source/UnrealClaude/Public/ClaudeCodeRunner.h
    - UnrealClaude/Source/UnrealClaude/Private/ClaudeCodeRunner.cpp
    - UnrealClaude/Source/UnrealClaude/Private/UnrealClaudeModule.cpp
    - UnrealClaude/Source/UnrealClaude/Public/UnrealClaudeConstants.h
    - UnrealClaude/Source/UnrealClaude/Private/Tests/TestUtils.h

key-decisions:
  - "FClaudeCodeRunner keeps its name (D-06) since it remains Claude-specific"
  - "Module name UnrealClaude, log category LogUnrealClaude, tab name ClaudeAssistant all preserved (D-04/D-05)"
  - "FChatRequestConfig is base config; FClaudeRequestConfig derives with Claude-specific fields (D-11)"
  - "Backend swap blocked during active requests via IsExecuting() guard (D-14)"
  - "Session history shared across backend swaps - FChatSessionManager not destroyed on swap (D-19)"
  - "SChatMessage role label uses GetBackendDisplayName() instead of hardcoded Claude (D-08)"
  - "ChatBackendAbstractionTests.cpp split from ChatBackendTests.cpp to keep both under 500-line convention"

patterns-established:
  - "IChatBackend interface: all backends implement 9 pure virtual methods"
  - "Factory pattern: FChatBackendFactory::Create(EChatBackendType) returns TUniquePtr<IChatBackend>"
  - "Config hierarchy: FChatRequestConfig (base) -> FClaudeRequestConfig (derived)"
  - "Backend availability: check via subsystem (IsBackendAvailable) not static runner methods"
  - "FMockChatBackend: test utility with controllable responses for all 9 interface methods"

requirements-completed: [ABST-01, ABST-02, ABST-03]

# Metrics
duration: ~90min
completed: 2026-03-31
---

# Phase 2 Plan 1: Backend Abstraction Summary

**IClaudeRunner replaced with 9-method IChatBackend interface, 13 files renamed to backend-agnostic names, FChatBackendFactory with type dispatch, FChatSubsystem with runtime backend swapping, 19 new abstraction tests**

## Performance

- **Duration:** ~90 min (across 2 executor sessions)
- **Started:** 2026-03-31T21:18:19Z
- **Completed:** 2026-03-31T23:55:00Z
- **Tasks:** 9/9
- **Files changed:** 29 (created: 7, renamed: 13, modified: 7, deleted: 2)

## Accomplishments

- Created `IChatBackend` pure virtual interface with 9 methods (5 carried from IClaudeRunner + 4 new: GetDisplayName, GetBackendType, FormatPrompt, CreateConfig) — zero Claude-specific types in signatures
- Created `EChatBackendType` enum with Claude/OpenCode values plus `ChatBackendUtils::EnumToString`/`StringToEnum` for config persistence
- Created `FChatRequestConfig` base struct and `FClaudeRequestConfig` derived struct for polymorphic config
- Renamed 13 files from Claude-prefixed to backend-agnostic names (ClaudeSubsystem -> ChatSubsystem, ClaudeSessionManager -> ChatSessionManager, ClaudeEditorWidget -> ChatEditorWidget, etc.)
- Created `FChatBackendFactory` with `Create()`, `DetectPreferredBackend()`, and `IsBinaryAvailable()` — ready for OpenCode backend registration
- Updated `FChatSubsystem` with `SetActiveBackend()` for runtime backend swapping, guarded by `IsExecuting()` check
- Added 19 new abstraction tests covering enum serialization, config hierarchy, factory dispatch, and subsystem routing
- Verified zero stale references to 17+ old type names across entire Source/ directory

## Task Commits

Each task was committed atomically:

1. **Task 1: Create ChatBackendTypes.h** - `535644b` (feat)
2. **Task 2: Create IChatBackend.h** - `b6f03d9` (feat)
3. **Task 3: Create ClaudeRequestConfig.h** - `eb5de33` (feat)
4. **Task 4: Delete IClaudeRunner.h, update FClaudeCodeRunner** - `b01808d` (refactor)
5. **Task 5: Rename session manager and subsystem** - `be19895` (refactor)
6. **Task 6: Create ChatBackendFactory** - `9c22103` (feat)
7. **Task 7: Rename widget files and update module** - `f072b06` (refactor)
8. **Task 8: Rename test files and update all test references** - `5af41f6` (refactor)
9. **Task 9: Add 19 new abstraction tests** - `d15c3f0` (test)

## Files Created/Modified

### Created
- `Public/ChatBackendTypes.h` — EChatBackendType enum, FChatStreamEvent, EChatStreamEventType, all renamed delegates, base FChatRequestConfig, FChatPromptOptions (197 lines)
- `Public/IChatBackend.h` — 9-method pure virtual interface with Doxygen docs (89 lines)
- `Public/ClaudeRequestConfig.h` — FClaudeRequestConfig extending FChatRequestConfig (23 lines)
- `Public/ChatSubsystem.h` — FChatSubsystem with SetActiveBackend, IsBackendAvailable, GetActiveBackendType (96 lines)
- `Private/ChatBackendFactory.h` — Factory class declaration (29 lines)
- `Private/ChatBackendFactory.cpp` — Factory implementation with OpenCode binary detection (114 lines)
- `Private/Tests/ChatBackendAbstractionTests.cpp` — 19 new abstraction tests (378 lines)

### Renamed (git mv)
- `Public/ClaudeSubsystem.h` -> deleted (replaced by `ChatSubsystem.h`)
- `Public/ClaudeSessionManager.h` -> `Public/ChatSessionManager.h`
- `Public/ClaudeEditorWidget.h` -> `Public/ChatEditorWidget.h`
- `Public/IClaudeRunner.h` -> deleted (replaced by `IChatBackend.h`)
- `Private/ClaudeSubsystem.cpp` -> `Private/ChatSubsystem.cpp`
- `Private/ClaudeSessionManager.cpp` -> `Private/ChatSessionManager.cpp`
- `Private/ClaudeEditorWidget.cpp` -> `Private/ChatEditorWidget.cpp`
- `Private/Widgets/SClaudeInputArea.h/.cpp` -> `Private/Widgets/SChatInputArea.h/.cpp`
- `Private/Widgets/SClaudeToolbar.h/.cpp` -> `Private/Widgets/SChatToolbar.h/.cpp`
- `Private/Tests/ClaudeRunnerTests.cpp` -> `Private/Tests/ChatBackendTests.cpp`
- `Private/Tests/ClaudeSubsystemTests.cpp` -> `Private/Tests/ChatSubsystemTests.cpp`

### Modified
- `Public/ClaudeCodeRunner.h` — implements IChatBackend instead of IClaudeRunner, added 4 new methods
- `Private/ClaudeCodeRunner.cpp` — all type references updated, FormatPrompt() extracted from subsystem
- `Private/UnrealClaudeModule.cpp` — updated includes and type references to backend-agnostic names
- `Public/UnrealClaudeConstants.h` — added Backend namespace with DefaultType, OpenCodeBinaryName, ClaudeBinaryName
- `Private/Tests/TestUtils.h` — FMockChatBackend replacing FMockClaudeRunner with 9 IChatBackend methods
- All 7 remaining test files — type references updated (SessionManagerTests, SlateWidgetTests, ModuleTests, ClipboardImageTests, etc.)

## Decisions Made

1. **FClaudeCodeRunner keeps its name (D-06)** — it remains the Claude-specific backend implementation; renaming would lose semantic meaning
2. **Module/log/tab names preserved (D-04/D-05)** — UnrealClaude module name, LogUnrealClaude category, ClaudeAssistant tab name all stay as-is to avoid breaking UE module registration
3. **Config hierarchy (D-11)** — FChatRequestConfig as base with FChatPromptOptions; FClaudeRequestConfig derives adding Claude-specific fields (SystemPrompt, AllowedTools, McpConfigPath). OpenCode will add its own derived config.
4. **Backend swap guard (D-14)** — SetActiveBackend() checks IsExecuting() and returns false if a request is in-flight, preventing mid-stream corruption
5. **Session preservation on swap (D-19)** — FChatSessionManager is NOT destroyed when switching backends; conversation history persists across swaps
6. **Dynamic display name (D-08)** — SChatMessage role label calls GetBackendDisplayName() instead of hardcoded "Claude" text
7. **Test file split** — ChatBackendAbstractionTests.cpp created as separate file from ChatBackendTests.cpp to maintain ≤500 line convention

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Split ChatBackendAbstractionTests.cpp from ChatBackendTests.cpp**
- **Found during:** Task 9 (abstraction tests)
- **Issue:** ChatBackendTests.cpp was already 502 lines after Task 8; adding 19 new tests would exceed 500-line convention
- **Fix:** Created separate ChatBackendAbstractionTests.cpp (378 lines) for the new abstraction tests
- **Files created:** `Private/Tests/ChatBackendAbstractionTests.cpp`
- **Committed in:** `d15c3f0` (Task 9 commit)

---

**Total deviations:** 1 auto-fixed (Rule 2 - convention enforcement)
**Impact on plan:** Minimal — test split maintains codebase conventions. No scope creep.

## Issues Encountered

- **ClaudeCodeRunner.cpp exceeds 500-line convention** (1251 lines pre-existing, ~1290 after changes) — this was pre-existing and the plan didn't ask for a split, only type updates. Documented but not addressed per scope boundary rules.
- **ChatEditorWidget.cpp exceeds 500-line convention** (1389 lines pre-existing) — same situation, pre-existing, only type renames performed.
- **ripgrep (rg) not available on Windows** — used PowerShell `Select-String` and the Grep tool for verification instead.
- **LSP errors about CoreMinimal.h** — expected in UE5.7 projects without local engine headers; ignored throughout.

## Known Stubs

None — all interfaces are fully wired. FChatBackendFactory::Create() returns nullptr for EChatBackendType::OpenCode (intentional — OpenCode runner doesn't exist yet, will be implemented in Phase 4).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- **IChatBackend interface is ready** for FOpenCodeRunner implementation in Phase 4
- **FChatBackendFactory::Create()** has the OpenCode case placeholder ready for wiring
- **FChatBackendFactory::IsBinaryAvailable()** already checks for "opencode" binary on PATH
- **EChatBackendType::OpenCode** enum value exists and serializes correctly
- **FChatRequestConfig** base struct ready for FOpenCodeRequestConfig derivation
- **Phase 3 can proceed** — SSE parser and server lifecycle are independent components that only need to know about IChatBackend (which now exists)
- **No blockers** — all 9 tasks complete, zero stale references, abstraction layer is clean

## Self-Check: PASSED

All 9 task commits verified present in git history. All 8 key created files exist on disk. All 3 deleted files confirmed removed. SUMMARY.md exists at expected path.

---
*Phase: 02-backend-abstraction*
*Completed: 2026-03-31*
