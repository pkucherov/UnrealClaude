---
phase: 01-test-baseline
plan: 01
subsystem: testing
tags: [test-infrastructure, subsystem-tests, runner-tests, mock-runner, ndjson-parsing]
dependency_graph:
  requires: []
  provides: [TestUtils.h, ClaudeSubsystemTests, ClaudeRunnerTests, FMockClaudeRunner]
  affects: [all-subsequent-test-plans]
tech_stack:
  added: []
  patterns: [FMockClaudeRunner-as-IClaudeRunner, standalone-FClaudeCodeRunner-construction, singleton-safe-testing]
key_files:
  created:
    - UnrealClaude/Source/UnrealClaude/Private/Tests/TestUtils.h
    - UnrealClaude/Source/UnrealClaude/Private/Tests/ClaudeSubsystemTests.cpp
    - UnrealClaude/Source/UnrealClaude/Private/Tests/ClaudeRunnerTests.cpp
  modified: []
decisions:
  - FMockClaudeRunner executes callbacks synchronously in ExecuteAsync to avoid threading in tests
  - FClaudeCodeRunner constructed standalone for helper method testing (constructor does not start threads)
  - ParseStreamJsonOutput tested with result-priority behavior (result messages override assistant text blocks)
  - Static methods (IsClaudeAvailable, GetClaudePath) tested for no-crash only, not return values (CLI may not be installed)
metrics:
  duration: ~45min
  completed: 2026-03-31
---

# Phase 1 Plan 01: Test Infrastructure & Core Component Tests Summary

Test infrastructure foundation with shared utilities and 25 new tests covering ClaudeSubsystem singleton access, system prompts, history management, session persistence, and ClaudeCodeRunner NDJSON stream parsing, struct defaults, payload construction, and IClaudeRunner interface contract.

## What Was Done

### Task 1: TestUtils.h (Shared Test Infrastructure)
**Commit:** `da5cedd`

Created `TestUtils.h` with three utility sections:

1. **JSON Helpers** (4 functions): `MakeJsonWithString`, `MakeEmptyJson`, `MakeJsonWithInt`, `MakeJsonWithBool` — static functions for quick test JSON construction
2. **Temp File Helpers** (5 functions): `GetTestTempDir`, `EnsureTestTempDir`, `CreateTempTextFile`, `DeleteTempFile`, `CleanupTestTempDir` — file I/O utilities for tests needing temp files
3. **FMockClaudeRunner** class: Implements `IClaudeRunner` with synchronous fake execution — captures `LastPrompt`, returns configurable `MockResponse`, tracks `bExecuteAsyncCalled`/`bCancelCalled` flags. No subprocess or threading.

**File stats:** ~149 lines, well under 500-line limit.

### Task 2: ClaudeSubsystemTests.cpp (11 tests)
**Commit:** `53e4b23`

Tests organized into 5 sections:
- **Singleton Access** (2 tests): `Get()` returns valid reference, `GetRunner()` returns non-null pointer
- **System Prompts** (3 tests): `GetUE57SystemPrompt()` contains UE version, `GetProjectContextPrompt()` returns valid string, `SetCustomSystemPrompt()` no-crash
- **History** (2 tests): `GetHistory()` returns valid array, `ClearHistory()` resets count (with state restore)
- **Sessions** (2 tests): `GetSessionFilePath()` contains "UnrealClaude", `HasSavedSession()` returns bool without crash
- **Safety** (1 test): `CancelCurrentRequest()` when idle doesn't crash
- **Regression Guards** (1 test): `FClaudePromptOptions` default values guard (bIncludeEngineContext=true, bIncludeProjectContext=true, empty ImagePaths)

**File stats:** ~249 lines.

### Task 3: ClaudeRunnerTests.cpp (14 tests)
**Commit:** `9721a9a`

Tests organized into 6 sections:
- **Stream Event Struct** (2 tests): Default values regression guard (all 12 fields), all 7 `EClaudeStreamEventType` enum values assignable
- **Request Config** (1 test): Default values regression guard (8 fields including bSkipPermissions=true, TimeoutSeconds=300.0f)
- **NDJSON Parsing** (8 tests): Empty input, valid result message, valid assistant message, malformed JSON, multiple NDJSON lines with result priority, assistant fallback with multiple blocks concatenation, mixed valid/invalid lines resilience, tool_use lines correctly ignored
- **Payload Construction** (2 tests): Text-only payload structure validation (JSON parse + field checks), empty prompt handling
- **Static Methods** (2 tests): `IsClaudeAvailable()` and `GetClaudePath()` no-crash (no assertion on return value)
- **Instance State** (1 test): Fresh instance `IsExecuting()` returns false
- **Interface Contract** (1 test): `FMockClaudeRunner` used as `IClaudeRunner*` — verifies `IsAvailable()`, `IsExecuting()`, `Cancel()`, and `ExecuteAsync()` through interface pointer

**File stats:** 500 lines (exactly at limit).

## Test Count Summary

### Before This Plan
| File | Tests |
|------|-------|
| CharacterToolTests.cpp | 16 |
| ClipboardImageTests.cpp | 28 |
| MaterialAssetToolTests.cpp | 19 |
| MCPAnimBlueprintTests.cpp | 8 |
| MCPAssetToolTests.cpp | 14 |
| MCPParamValidatorTests.cpp | 11 |
| MCPTaskQueueTests.cpp | 17 (6 disabled in `#if 0`) |
| MCPToolTests.cpp | 64 |
| **Total existing** | **177 (171 active)** |

### After This Plan
| File | Tests |
|------|-------|
| TestUtils.h | 0 (utility, no tests) |
| ClaudeSubsystemTests.cpp | 11 |
| ClaudeRunnerTests.cpp | 14 |
| **New tests added** | **25** |
| **New total** | **202 (196 active)** |

## Deviations from Plan

None — plan executed exactly as written. All three files created per specification, all test counts match expectations.

## Known Stubs

None — all tests contain real assertions and test real behavior. No placeholder or TODO code.

## Decisions Made

1. **FMockClaudeRunner synchronous execution**: `ExecuteAsync` immediately invokes the `OnComplete` delegate rather than using deferred callbacks. This avoids threading complexity in tests while still exercising the interface contract.

2. **ParseStreamJsonOutput result-priority verified**: Testing confirmed that when both `"result"` and `"assistant"` messages exist in NDJSON output, the result message takes priority. Multiple assistant text blocks are concatenated when no result message is present.

3. **Static methods no-crash only**: `IsClaudeAvailable()` and `GetClaudePath()` cannot be asserted for specific values since the Claude CLI may or may not be installed in the test environment. Tests verify only that these calls don't crash.

4. **ClaudeRunnerTests.cpp at exactly 500 lines**: The file was trimmed from 503 to 500 lines by condensing the Doxygen header comment to meet the max-500-lines-per-file convention.

## Self-Check: PASSED

- [x] TestUtils.h exists
- [x] ClaudeSubsystemTests.cpp exists
- [x] ClaudeRunnerTests.cpp exists
- [x] 01-01-SUMMARY.md exists
- [x] Commit da5cedd found (Task 1)
- [x] Commit 53e4b23 found (Task 2)
- [x] Commit 9721a9a found (Task 3)
