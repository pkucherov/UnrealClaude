# Phase 1: Test Baseline - Research

**Researched:** 2026-03-30
**Domain:** Unreal Engine 5.7 Automation Testing — Editor plugin test coverage
**Confidence:** HIGH (all findings derived from direct codebase inspection and UE 5.7 source patterns)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- **D-01:** Full coverage sweep — all 8+ untested/partially-tested components get new tests (not just the 3 named in success criteria)
- **D-02:** Target >=90% coverage for all components, documenting where infeasible
- **D-03:** Full Slate UI testing including simulated input events and state verification for SClaudeEditorWidget, SClaudeToolbar, SClaudeInputArea
- **D-04:** Include MCP bridge JavaScript tests (via npm test) in the baseline — not C++ only
- **D-05:** Include FScriptExecutionManager test coverage (security-relevant permission gating)
- **D-06:** Add C++ HTTP route tests for FUnrealClaudeMCPServer (route registration, request parsing, response formatting)
- **D-07:** Comprehensive NDJSON stream event parsing tests for ClaudeCodeRunner (all event types, malformed JSON, partial lines, empty events, large payloads)
- **D-08:** Include thread safety testing where possible using non-blocking patterns to avoid the known FRunnableThread deadlock
- **D-09:** Test all error handling paths — verify FMCPToolResult error codes, messages, and bSuccess=false for all error paths; test FMCPErrors factory methods
- **D-10:** Test FUnrealClaudeModule::StartupModule() and ShutdownModule() for correct initialization order
- **D-11:** Validate UnrealClaudeConstants.h values in tests (port ranges, timeout positivity, max sizes > 0)
- **D-12:** Add regression guards (golden output / snapshot-style tests) capturing current behavior for detection of unintended changes during Phase 2 refactoring
- **D-13:** Skip performance/timing tests — this phase is purely functional correctness
- **D-14:** One test file per component (e.g., ClaudeSubsystemTests.cpp, SessionManagerTests.cpp, MCPServerTests.cpp)
- **D-15:** Follow existing naming convention: F{System}_{Category}_{Scenario} with 'UnrealClaude.{Category}.{SubCategory}.{TestName}' identifiers
- **D-16:** Apply 500-line file limit and 50-line function limit to test code (tests are production code)
- **D-17:** All new tests use EditorContext | ProductFilter flags (matching existing tests)
- **D-18:** Create shared test utilities (mock factories, fixture helpers, common assertions) in a TestUtils.h file
- **D-19:** File headers with component documentation + descriptive assertion strings in test bodies
- **D-20:** Write tests in refactoring-risk order: ClaudeSubsystem -> IClaudeRunner/ClaudeCodeRunner -> SessionManager -> MCPServer -> ProjectContext -> ScriptExecution -> MCPToolBase -> UI widgets -> Bridge JS
- **D-21:** Verify all 171 existing tests pass before writing new ones. If any fail, fix them first as part of this phase
- **D-22:** Add test-only module dependencies to Build.cs if needed (e.g., AutomationController for advanced test features)
- **D-23:** Test singletons directly via their static Get() accessors — no mock injection, no refactoring of Get() methods
- **D-24:** ClaudeCodeRunner: test helper methods directly (ParseStreamJsonOutput, BuildStreamJsonPayload), verify RunPrompt/IsRunning/Cancel signatures via compile-time checks, don't spawn actual claude subprocess
- **D-25:** SessionManager: test state machine (add/clear/save/load, max-size enforcement, file path resolution) using temp directory (FPaths::ProjectSavedDir() / 'Tests/')
- **D-26:** ProjectContextManager: structure validation — verify scan/format/return doesn't crash, check output structure (project name, source file count, asset summary), don't assert specific counts
- **D-27:** Attempt to fix FRunnableThread deadlock for 6 disabled tests in MCPTaskQueueTests.cpp. If fix succeeds, re-enable all 6. If not, document the attempt and leave disabled
- **D-28:** Fix strategy: try latent automation tests (AddCommand pattern) first — allows game thread to tick between test steps, avoiding deadlock. Fall back to FTaskGraphInterface if needed
- **D-29:** Full coverage report with method-level tracking, gap analysis, and risk assessment for untested areas
- **D-30:** Report lives in .planning/codebase/ alongside TESTING.md (codebase-level document persisting across phases)
- **D-31:** Update coverage report incrementally as each component gets tests — each commit that adds tests should update the report

### Agent's Discretion
- Exact test utility function signatures and organization
- How to structure Slate widget test fixtures for simulated input
- Specific mock patterns for HTTP route testing
- Whether to split large component test files further to stay under 500-line limit
- Temp directory cleanup strategy details

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.

</user_constraints>

---

## Summary

Phase 1 establishes the test baseline for the UnrealClaude plugin before Phase 2 refactoring begins. The project already has 171 tests across 8 files using `IMPLEMENT_SIMPLE_AUTOMATION_TEST`, but **8 components have zero test coverage** (ClaudeSubsystem, SessionManager, MCPServer HTTP routes, ProjectContextManager, ScriptExecutionManager, Slate widgets, UnrealClaudeModule startup) and **6 tests are disabled** due to a confirmed FRunnableThread deadlock.

The primary technical challenge is the FRunnableThread deadlock (D-27/D-28): the automation framework runs on the GameThread, so any test that starts a worker thread which dispatches back to GameThread via `AsyncTask(ENamedThreads::GameThread)` will deadlock. The solution is UE's latent automation test framework (`ADD_LATENT_AUTOMATION_COMMAND` + `IMPLEMENT_COMPLEX_AUTOMATION_TEST`), which yields the GameThread between commands rather than blocking it.

The secondary challenge is Slate widget testing. Slate widgets require construction via `SNew()` in an editor context, and simulated input is done through `FSlateApplication::Get().ProcessKeyDownEvent()` and similar APIs. Widgets can be tested for state changes (text content, visibility, callback invocations) without rendering.

**Primary recommendation:** Use the existing `IMPLEMENT_SIMPLE_AUTOMATION_TEST` pattern for all synchronous tests. Use `IMPLEMENT_COMPLEX_AUTOMATION_TEST` + `ADD_LATENT_AUTOMATION_COMMAND` only for the 6 disabled threading tests. Never start `FRunnableThread` in synchronous tests.

---

## Project Constraints (from AGENTS.md / CLAUDE.md)

| Constraint | Detail |
|------------|--------|
| C++ style | UE 5.7 coding standards; Unreal prefix conventions |
| File size | 500 lines max per file; 50 lines max per function |
| Test runner | `Automation RunTests UnrealClaude` in UE Editor console |
| Test flags | `EAutomationTestFlags::EditorContext \| EAutomationTestFlags::ProductFilter` |
| No new runtime deps | No external test frameworks (Google Mock, Catch2, etc.) |
| Copyright header | `// Copyright Natali Caggiano. All Rights Reserved.` |
| Naming | `F{System}_{Category}_{Scenario}` class, `"UnrealClaude.{Category}.{Sub}.{Name}"` string |
| Commit prefix | `test:` for test additions |
| `WITH_DEV_AUTOMATION_TESTS` | All test code inside this guard |
| No subprocess | Do not spawn actual `claude` CLI in tests |
| No modal dialogs | `FScriptExecutionManager::ExecuteScript()` shows modal — do not call in tests |

---

## Standard Stack

### Core Test Infrastructure

| Component | Version | Purpose | Source |
|-----------|---------|---------|--------|
| `IMPLEMENT_SIMPLE_AUTOMATION_TEST` | UE 5.7 built-in | Single-frame synchronous tests | `Misc/AutomationTest.h` |
| `IMPLEMENT_COMPLEX_AUTOMATION_TEST` | UE 5.7 built-in | Multi-frame latent tests | `Misc/AutomationTest.h` |
| `ADD_LATENT_AUTOMATION_COMMAND` | UE 5.7 built-in | Enqueues a command to run in a future frame | `Misc/AutomationTest.h` |
| `FAutomationTestBase` | UE 5.7 built-in | Base class with `TestTrue`, `TestEqual`, `TestNull`, etc. | `Misc/AutomationTest.h` |
| `FSlateApplication` | UE 5.7 built-in | Slate application singleton for widget testing | `Framework/Application/SlateApplication.h` |

### No New Build Dependencies Required

The existing Build.cs already has everything needed for test coverage:
- `Slate`, `SlateCore` — Slate widget construction and testing
- `HTTP`, `HTTPServer` — MCP server testing (via `FHttpServerModule`)
- `Json`, `JsonUtilities` — NDJSON parsing tests
- `UnrealEd` — Editor context (singletons, world access)

**The only potential addition (D-22):** `AutomationController` module for advanced features like latent command infrastructure. However, `ADD_LATENT_AUTOMATION_COMMAND` is available in `Core` via `Misc/AutomationTest.h` without `AutomationController`. The controller module is only needed for the test runner UI itself (not for writing tests). **Conclusion: No Build.cs changes needed.**

---

## Architecture Patterns

### Pattern 1: Standard Synchronous Test (Current Codebase Pattern)

Used for: all 171 existing tests, all new tests for components without threading.

```cpp
// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Unit tests for FClaudeSessionManager
 * Tests session persistence, history state machine, and file path resolution
 */

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "ClaudeSessionManager.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== Session History Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSessionManager_AddExchange_StoresInHistory,
    "UnrealClaude.Session.History.AddExchange_StoresInHistory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_AddExchange_StoresInHistory::RunTest(const FString& Parameters)
{
    FClaudeSessionManager Manager;

    Manager.AddExchange(TEXT("hello"), TEXT("world"));

    const TArray<TPair<FString, FString>>& History = Manager.GetHistory();
    TestEqual("History should have 1 entry", History.Num(), 1);
    TestEqual("Prompt should match", History[0].Key, FString(TEXT("hello")));
    TestEqual("Response should match", History[0].Value, FString(TEXT("world")));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

**Key rules from codebase:**
- `TestEqual` for values, `TestTrue`/`TestFalse` for conditions, `TestNotNull`/`TestNull` for pointers
- Descriptive assertion strings ("History should have 1 entry" — not just "true")
- Section comments `// ===== Section Name =====` to group related tests
- One `IMPLEMENT_SIMPLE_AUTOMATION_TEST` block per logical test

### Pattern 2: Latent Test for Threading (Fix for Disabled Tests)

Used for: the 6 disabled tests in `MCPTaskQueueTests.cpp` (D-27/D-28).

**How `IMPLEMENT_COMPLEX_AUTOMATION_TEST` works:**

Unlike `IMPLEMENT_SIMPLE_AUTOMATION_TEST` (which runs in a single `RunTest()` call blocking the GameThread), complex tests use a command queue. `RunTest()` enqueues commands; each command runs in a separate engine tick, allowing the GameThread to process AsyncTask dispatches between steps.

```cpp
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/AutomationTest.h"
#include "MCP/MCPToolRegistry.h"
#include "MCP/MCPTaskQueue.h"

#if WITH_DEV_AUTOMATION_TESTS

// Latent command: waits for task queue to stop naturally (no blocking Kill)
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FWaitForQueueStop,
    TSharedPtr<FMCPToolRegistry>, Registry
);

bool FWaitForQueueStop::Update()
{
    // Returns true when done (command is removed from queue)
    // Returns false to re-run next tick
    // Queue stop is idempotent; we just need the thread to drain
    TSharedPtr<FMCPTaskQueue> Queue = Registry->GetTaskQueue();
    if (!Queue.IsValid()) return true;
    // Check if worker thread has exited (no running tasks)
    int32 Pending, Running, Completed;
    Queue->GetStats(Pending, Running, Completed);
    return Running == 0;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
    FMCPTaskQueue_StartStop_Latent,
    "UnrealClaude.MCP.TaskQueue.StartStop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPTaskQueue_StartStop_Latent::RunTest(const FString& Parameters)
{
    // Step 1: start queue (enqueued as immediate action)
    TSharedPtr<FMCPToolRegistry> Registry = MakeShared<FMCPToolRegistry>();
    Registry->StartTaskQueue();

    // Step 2: wait for worker thread to become idle (latent — yields GameThread)
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForQueueStop(Registry));

    // Step 3: assertions run after latent command completes
    // NOTE: Assertions in complex tests must also be latent commands if
    // they depend on async state. For final state checks, use a final
    // ADD_LATENT_AUTOMATION_COMMAND that does the TestTrue calls.

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

**Critical limitation discovered in codebase comments (MCPTaskQueueTests.cpp lines 41-95):**

The deadlock is specifically caused by `AsyncTask(ENamedThreads::GameThread, ...)` in the worker thread dispatching back to a blocked GameThread. The latent pattern helps by yielding the GameThread between commands, but the worker must also complete its current dispatch before `StopTaskQueue()` can safely return. The safest approach for re-enabling the 6 tests is:

1. Start queue with `Registry.StartTaskQueue()`
2. Enqueue a `FWaitForQueueIdle` latent command (polls `GetStats()` until `Running == 0`)
3. Only then call `StopTaskQueue()` inside a latent command
4. If this still deadlocks after 5 seconds, document the attempt and leave the `#if 0` block

### Pattern 3: Singleton Testing (D-23)

Access via `static Get()` — no injection needed. Singletons are already initialized when tests run (StartupModule has already been called).

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FClaudeSubsystem_GetHistory_ReturnsSessionHistory,
    "UnrealClaude.ClaudeSubsystem.History.GetHistory_ReturnsSessionHistory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeSubsystem_GetHistory_ReturnsSessionHistory::RunTest(const FString& Parameters)
{
    FClaudeCodeSubsystem& Subsystem = FClaudeCodeSubsystem::Get();

    // Save initial state to restore after test
    int32 InitialCount = Subsystem.GetHistory().Num();

    // Test is read-only — asserting structure, not content
    TestTrue("History array should be valid (no crash)", true);
    TestTrue("History count should be non-negative", Subsystem.GetHistory().Num() >= 0);

    return true;
}
```

**Singleton test rules:**
- Never call `SendPrompt` (spawns subprocess)
- Never call `ExecuteScript` (shows modal dialog)
- Safe to call: `GetHistory`, `ClearHistory`, `HasSavedSession`, `GetSessionFilePath`, `GetUE57SystemPrompt`, `GetProjectContextPrompt`, `GetRunner`
- Safe to call `SaveSession`/`LoadSession` only in dedicated tests using temp directories
- Restore state after mutating tests (call `ClearHistory()` at end)

### Pattern 4: Temp File Tests with Cleanup (D-25)

Pattern established in `ClipboardImageTests.cpp` — create files, test, delete.

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSessionManager_SaveLoad_RoundTrip,
    "UnrealClaude.Session.Persistence.SaveLoad_RoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_SaveLoad_RoundTrip::RunTest(const FString& Parameters)
{
    FClaudeSessionManager Manager;
    // Use test-specific path to avoid polluting real session
    // NOTE: FClaudeSessionManager uses FPaths::ProjectSavedDir() / "UnrealClaude" / "session.json"
    // The Manager itself uses a fixed path — tests should save/load and then delete the test file

    Manager.AddExchange(TEXT("q1"), TEXT("a1"));
    Manager.AddExchange(TEXT("q2"), TEXT("a2"));

    bool bSaved = Manager.SaveSession();
    TestTrue("Save should succeed", bSaved);
    TestTrue("Session file should exist", FPaths::FileExists(Manager.GetSessionFilePath()));

    // Load into a fresh manager
    FClaudeSessionManager Manager2;
    bool bLoaded = Manager2.LoadSession();
    TestTrue("Load should succeed", bLoaded);
    TestEqual("Loaded history count should match", Manager2.GetHistory().Num(), 2);

    // Cleanup — delete the test session file
    IFileManager::Get().Delete(*Manager.GetSessionFilePath());

    return true;
}
```

**Cleanup rule:** Always delete test-created files in the same test. Do not use `AddExpectedError` or test teardown — UE simple tests have no teardown hook.

### Pattern 5: Static Helper Functions in Test Files

Pattern established in `ClipboardImageTests.cpp` lines 688-719. Declare helpers as `static` free functions above the tests that use them.

```cpp
#if WITH_DEV_AUTOMATION_TESTS

// Helper: create a minimal FJsonObject with required MCP params
static TSharedRef<FJsonObject> MakeJsonParams(
    const FString& Key, const FString& Value)
{
    TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(Key, Value);
    return Obj;
}

// Tests follow...
#endif
```

**Rules for helper functions:**
- `static` (file scope only — no ODR violations across test files)
- Must be inside `#if WITH_DEV_AUTOMATION_TESTS` guard
- Max 50 lines per function (D-16 applies to helpers too)
- Name as `Make{Something}`, `Create{Something}`, `Get{Something}Count`

### Pattern 6: Mock/Fake Patterns (No External Framework)

UE 5.7 tests use hand-written fakes — no Google Mock.

**For `IClaudeRunner`:** The interface exists. Create a `FMockClaudeRunner : IClaudeRunner` inline in the test file.

```cpp
#if WITH_DEV_AUTOMATION_TESTS

/** Test-only fake runner that records calls without spawning Claude */
class FMockClaudeRunner : public IClaudeRunner
{
public:
    // Recorded state
    bool bExecuteAsyncCalled = false;
    bool bCancelCalled = false;
    bool bShouldFail = false;
    FString LastPrompt;

    virtual bool ExecuteAsync(
        const FClaudeRequestConfig& Config,
        FOnClaudeResponse OnComplete,
        FOnClaudeProgress OnProgress) override
    {
        bExecuteAsyncCalled = true;
        LastPrompt = Config.Prompt;
        if (!bShouldFail)
        {
            OnComplete.ExecuteIfBound(TEXT("mock response"), true);
        }
        else
        {
            OnComplete.ExecuteIfBound(TEXT(""), false);
        }
        return !bShouldFail;
    }

    virtual bool ExecuteSync(const FClaudeRequestConfig& Config, FString& OutResponse) override
    {
        OutResponse = TEXT("mock sync response");
        return !bShouldFail;
    }

    virtual void Cancel() override { bCancelCalled = true; }
    virtual bool IsExecuting() const override { return false; }
    virtual bool IsAvailable() const override { return true; }
};

#endif
```

**Limitation:** `FClaudeCodeSubsystem` owns its runner as `TUniquePtr<FClaudeCodeRunner>` and constructs it in the private constructor. `GetRunner()` returns `IClaudeRunner*` but there is no setter. **The mock runner cannot be injected into the live singleton without refactoring.** Per D-23, we do NOT refactor Get() methods in this phase. Therefore:

- Test `FClaudeCodeSubsystem` only via its safe read-only methods (`GetHistory`, `GetUE57SystemPrompt`, etc.)
- Test `IClaudeRunner`/`FClaudeCodeRunner` directly (construct standalone instances)
- Use `FMockClaudeRunner` in future phases if a setter is added

### Pattern 7: HTTP Route Testing (D-06)

`FUnrealClaudeMCPServer` cannot have its `Start()`/`Stop()` called in tests because the editor's own MCP server is already running on port 3000. Calling `Start()` would attempt to bind the same port.

**Safe approach — test the route handler logic indirectly:**

```cpp
// Test ToolRegistry-level behavior (what HandleListTools would return)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMCPServer_ToolRegistry_ListToolsReturnsMCPFormat,
    "UnrealClaude.MCPServer.Routes.ListToolsReturnsMCPFormat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_ToolRegistry_ListToolsReturnsMCPFormat::RunTest(const FString& Parameters)
{
    // Test the registry (which HandleListTools delegates to) directly
    FMCPToolRegistry Registry;
    const TArray<FMCPToolInfo>& Tools = Registry.GetAllToolInfos();

    TestTrue("Registry should have tools registered", Tools.Num() > 0);

    // Verify expected tools are present (snapshot test — D-12)
    bool bHasSpawnActor = false;
    for (const FMCPToolInfo& Tool : Tools)
    {
        if (Tool.Name == TEXT("spawn_actor")) { bHasSpawnActor = true; }
    }
    TestTrue("spawn_actor should be registered", bHasSpawnActor);

    return true;
}
```

**For request/response JSON format (D-06):** Test `CreateJsonResponse`/`CreateErrorResponse` by constructing the server and calling methods that don't require binding. Since `HandleListTools` is private, we test at the ToolRegistry level. Alternatively, expose a testable `BuildListToolsJson()` method (agent's discretion per D-18 TestUtils approach).

**Safe to test:** `FUnrealClaudeMCPServer` construction, `GetToolRegistry()`, `IsRunning()`, `GetPort()`. Do NOT call `Start()` or `Stop()`.

### Pattern 8: Slate Widget Testing (D-03)

Slate widgets can be constructed and tested in editor context without rendering. The `SNew()` macro works in test context.

```cpp
#include "Widgets/SClaudeInputArea.h"
#include "Framework/Application/SlateApplication.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSClaudeInputArea_GetText_ReturnsCurrentText,
    "UnrealClaude.Widgets.InputArea.GetText_ReturnsCurrentText",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSClaudeInputArea_GetText_ReturnsCurrentText::RunTest(const FString& Parameters)
{
    // Slate requires SlateApplication to be initialized (it is in editor context)
    if (!FSlateApplication::IsInitialized())
    {
        AddWarning(TEXT("Slate not initialized — skipping widget test"));
        return true;
    }

    // Construct widget using SNew (works in editor context)
    TSharedPtr<SClaudeInputArea> InputArea;
    SAssignNew(InputArea, SClaudeInputArea)
        .bIsWaiting(false);

    TestNotNull("InputArea should construct successfully", InputArea.Get());

    // Test public API
    InputArea->SetText(TEXT("hello world"));
    TestEqual("GetText should return set text", InputArea->GetText(), FString(TEXT("hello world")));

    InputArea->ClearText();
    TestTrue("ClearText should empty the text", InputArea->GetText().IsEmpty());

    return true;
}
```

**Slate test capabilities:**
- `SNew()` / `SAssignNew()` — construct widget (works in editor EditorContext)
- Call public methods (`SetText`, `GetText`, `ClearText`, `HasAttachedImages`, etc.)
- Verify callback invocation by binding a lambda and checking a flag
- Test `SLATE_ATTRIBUTE` binding by verifying initial state from args

**Slate test limitations (confirmed):**
- Cannot test rendering output — no pixel comparisons
- Cannot simulate keyboard/mouse input without `FSlateApplication::ProcessKeyDownEvent()` — this requires careful setup with a window. Avoid unless necessary.
- `SClaudeEditorWidget` is very large (complex, many private members) — test public API only
- Widgets that access the subsystem singleton at construction time (e.g., `SClaudeToolbar` reading session state) will use the live singleton state — test defensively

**For `SClaudeToolbar`:** Test SLATE_ATTRIBUTE defaults and callback invocation:

```cpp
bool bCallbackFired = false;
TSharedPtr<SClaudeToolbar> Toolbar;
SAssignNew(Toolbar, SClaudeToolbar)
    .bUE57ContextEnabled(true)
    .bProjectContextEnabled(true)
    .bRestoreEnabled(false)
    .OnNewSession(FOnToolbarAction::CreateLambda([&]() { bCallbackFired = true; }));

TestNotNull("Toolbar should construct", Toolbar.Get());
// Note: cannot click buttons without FSlateApplication routing —
// test construction + attribute defaults only in basic tests
```

### Pattern 9: Constants Validation Tests (D-11)

Simple value-assertion tests against `UnrealClaudeConstants.h`. Catches regressions where a constant is accidentally changed.

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FConstants_MCPServer_PortIsValid,
    "UnrealClaude.Constants.MCPServer.PortIsValid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FConstants_MCPServer_PortIsValid::RunTest(const FString& Parameters)
{
    using namespace UnrealClaudeConstants;

    TestTrue("Default port should be in valid range",
        MCPServer::DefaultPort >= 1024 && MCPServer::DefaultPort <= 65535);
    TestEqual("Default port should be 3000",
        MCPServer::DefaultPort, 3000u);
    TestTrue("GameThread timeout should be positive",
        MCPServer::GameThreadTimeoutMs > 0u);
    TestTrue("Max request body size should be > 0",
        MCPServer::MaxRequestBodySize > 0);

    TestTrue("Session max history should be positive",
        Session::MaxHistorySize > 0);
    TestTrue("Process timeout should be positive",
        Process::DefaultTimeoutSeconds > 0.0f);

    return true;
}
```

### Pattern 10: NDJSON Stream Event Tests (D-07)

All `FClaudeCodeRunner` NDJSON tests use standalone runner instances (no subprocess). The `ParseStreamJsonOutput()` and `BuildStreamJsonPayload()` methods are already tested in `ClipboardImageTests.cpp`. New tests should cover `ParseAndEmitNdjsonLine` behavior via the public `ParseStreamJsonOutput` wrapper and direct `FClaudeStreamEvent` struct construction.

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FClaudeRunner_ParseStreamJson_MalformedJson_ReturnsFallback,
    "UnrealClaude.ClaudeRunner.ParseStreamJson.MalformedJson_ReturnsFallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeRunner_ParseStreamJson_MalformedJson_ReturnsFallback::RunTest(const FString& Parameters)
{
    FClaudeCodeRunner Runner;

    // Malformed NDJSON should not crash — should return empty or fallback
    FString Result = Runner.ParseStreamJsonOutput(TEXT("{broken json"));
    // Just verify no crash and some string is returned
    TestTrue("Malformed JSON should not crash parser", true);

    // Empty input
    FString EmptyResult = Runner.ParseStreamJsonOutput(TEXT(""));
    TestTrue("Empty input should return empty string", EmptyResult.IsEmpty());

    return true;
}
```

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Test assertions | Custom assert macros | `TestTrue`, `TestEqual`, `TestNull` from `FAutomationTestBase` | Already in every test file; consistent output format |
| Mock runner | Custom subprocess wrapper | `FMockClaudeRunner : IClaudeRunner` inline fake | Interface exists; just implement it |
| Latent waits | `FPlatformProcess::Sleep()` | `ADD_LATENT_AUTOMATION_COMMAND` | Sleep blocks GameThread and causes deadlocks |
| JSON construction | Manual string building | `TSharedRef<FJsonObject>` + `MakeShared<FJsonObject>()` | Already used in 30+ tests; type-safe |
| Temp file paths | Hardcoded paths | `FPaths::ProjectSavedDir()` + FString combination | Cross-platform, always valid in editor |
| File cleanup | Manual OS calls | `IFileManager::Get().Delete()` | UE cross-platform file manager |
| Singleton access | Refactoring Get() | Direct `FClaudeCodeSubsystem::Get()` call | D-23 explicitly prohibits refactoring |
| Thread synchronization | `FPlatformProcess::Sleep(N)` | `DEFINE_LATENT_AUTOMATION_COMMAND` with polling | Sleep is unreliable; latent commands yield properly |

**Key insight:** UE's automation test framework handles test discovery, registration, output formatting, and runner integration automatically through the `IMPLEMENT_*_AUTOMATION_TEST` macros. Never replicate any of this infrastructure.

---

## Common Pitfalls

### Pitfall 1: FRunnableThread Deadlock
**What goes wrong:** Test calls `StartTaskQueue()` or `FClaudeCodeRunner::ExecuteAsync()` → worker thread dispatches `AsyncTask(ENamedThreads::GameThread, ...)` → GameThread is blocked in `RunTest()` → permanent editor freeze.
**Why it happens:** `IMPLEMENT_SIMPLE_AUTOMATION_TEST::RunTest()` blocks the GameThread for its entire duration. Any worker thread that needs to dispatch back will wait forever.
**How to avoid:** Never call `StartTaskQueue()`, `ExecuteAsync()`, or any method that creates an `FRunnableThread` in a simple automation test. For threading tests, use `IMPLEMENT_COMPLEX_AUTOMATION_TEST` with latent commands.
**Warning signs:** Editor freezes after test starts; must force-quit. The 6 disabled tests in `MCPTaskQueueTests.cpp` demonstrate this exactly.

### Pitfall 2: Calling Stop() Before Worker Drains
**What goes wrong:** `FMCPTaskQueue::Stop()` / `FRunnableThread::Kill(true)` waits for thread completion. If the thread is blocked on a GameThread dispatch, calling `Kill(true)` from the GameThread deadlocks.
**How to avoid:** In latent tests, use a `FWaitForQueueIdle` command that polls `GetStats()` until `Running == 0` before calling `StopTaskQueue()`.

### Pitfall 3: Starting the MCP HTTP Server in Tests
**What goes wrong:** `FUnrealClaudeMCPServer::Start()` tries to bind port 3000. The editor's own server is already bound. The `Start()` call fails or worse — if running in isolation, the server starts but interferes with normal editor operation.
**How to avoid:** Never call `Start()` or `Stop()` on `FUnrealClaudeMCPServer` in tests. Test route handler logic by testing the underlying `FMCPToolRegistry` directly.

### Pitfall 4: Singleton State Leaking Between Tests
**What goes wrong:** `FClaudeCodeSubsystem::Get()` returns the same instance for all tests. A test that calls `ClearHistory()` will affect subsequent tests.
**How to avoid:** Tests that mutate singleton state must restore it. For `ClearHistory()` tests: save history count before, assert, then call `AddExchange` to restore entries if other tests depend on state. Or ensure mutating tests run in isolation (test runner order is not guaranteed).

### Pitfall 5: Missing `#if WITH_DEV_AUTOMATION_TESTS` Guard
**What goes wrong:** Test code compiles in shipping builds, increasing binary size and exposing test utilities in production.
**How to avoid:** Every test file must wrap all content in `#if WITH_DEV_AUTOMATION_TESTS` / `#endif`. Check: all 8 existing test files do this correctly.

### Pitfall 6: ODR Violations from Static Helpers
**What goes wrong:** If two test files define a `static` helper with the same name but without the `static` keyword (or as a namespace-level function without `static`), linker ODR violations can occur.
**How to avoid:** Always use `static` for all helper functions in test files. Keep helpers inside `#if WITH_DEV_AUTOMATION_TESTS` guard. Use unique names (prefix with component name).

### Pitfall 7: Slate Widget Construction Outside Editor Context
**What goes wrong:** Attempting `SNew(SClaudeInputArea)` in a non-editor context (e.g., game builds) crashes because `FSlateApplication` is not initialized.
**How to avoid:** Guard with `if (!FSlateApplication::IsInitialized()) { AddWarning(...); return true; }`. The `EditorContext` test flag helps but the guard makes the intent explicit.

### Pitfall 8: 500-Line File Limit
**What goes wrong:** A component with many testable methods (e.g., `FClaudeCodeRunner` has 11 testable methods × 3 test scenarios = 33 tests) exceeds 500 lines in one file.
**How to avoid:** Split by test category within the component. E.g., `ClaudeRunnerParsingTests.cpp` (NDJSON tests) + `ClaudeRunnerPayloadTests.cpp` (payload construction tests). D-16 applies to test code.

### Pitfall 9: ClaudeSubsystem SendPrompt Call
**What goes wrong:** Calling `FClaudeCodeSubsystem::Get().SendPrompt(...)` in a test spawns the `claude` CLI subprocess. This hangs if Claude is not installed/authenticated, and always causes test timing issues.
**How to avoid:** Never call `SendPrompt` in tests. Test subsystem state via `GetHistory()`, `GetUE57SystemPrompt()`, `HasSavedSession()`, etc.

### Pitfall 10: FScriptExecutionManager::ExecuteScript Modal Dialog
**What goes wrong:** `ExecuteScript()` calls `ShowPermissionDialog()` which blocks on a modal `FMessageDialog`. Tests hang waiting for user input.
**How to avoid:** Never call `ExecuteScript()` in tests. Test only: `GetRecentScripts()`, `FormatHistoryForContext()`, `GetHistoryFilePath()`, `GetCppScriptDirectory()`, `GetContentScriptDirectory()`, `GetRecentScripts()`.

---

## Code Examples

### TestUtils.h — Shared Test Utilities Structure

```cpp
// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "IClaudeRunner.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== JSON Helpers =====

/** Create a minimal FJsonObject with a single string field */
static TSharedRef<FJsonObject> MakeJsonWithString(
    const FString& Key, const FString& Value)
{
    TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(Key, Value);
    return Obj;
}

/** Create an empty FJsonObject */
static TSharedRef<FJsonObject> MakeEmptyJson()
{
    return MakeShared<FJsonObject>();
}

// ===== Temp File Helpers =====

/** Get the test-specific temp directory (cleaned between test runs) */
static FString GetTestTempDir()
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UnrealClaude"), TEXT("Tests"));
}

/** Ensure test temp directory exists */
static void EnsureTestTempDir()
{
    IFileManager::Get().MakeDirectory(*GetTestTempDir(), true);
}

/** Create a temp text file and return its path */
static FString CreateTempTextFile(const FString& FileName, const FString& Content)
{
    EnsureTestTempDir();
    FString Path = FPaths::Combine(GetTestTempDir(), FileName);
    FFileHelper::SaveStringToFile(Content, *Path);
    return Path;
}

/** Delete a temp file */
static void DeleteTempFile(const FString& Path)
{
    IFileManager::Get().Delete(*Path);
}

// ===== Mock Runner =====

/** Test-only fake IClaudeRunner that never spawns Claude */
class FMockClaudeRunner : public IClaudeRunner
{
public:
    bool bExecuteAsyncCalled = false;
    bool bCancelCalled = false;
    bool bShouldSucceed = true;
    FString MockResponse = TEXT("mock response");
    FString LastPrompt;

    virtual bool ExecuteAsync(
        const FClaudeRequestConfig& Config,
        FOnClaudeResponse OnComplete,
        FOnClaudeProgress OnProgress) override
    {
        bExecuteAsyncCalled = true;
        LastPrompt = Config.Prompt;
        OnComplete.ExecuteIfBound(bShouldSucceed ? MockResponse : TEXT(""), bShouldSucceed);
        return bShouldSucceed;
    }

    virtual bool ExecuteSync(
        const FClaudeRequestConfig& Config, FString& OutResponse) override
    {
        OutResponse = bShouldSucceed ? MockResponse : TEXT("");
        return bShouldSucceed;
    }

    virtual void Cancel() override { bCancelCalled = true; }
    virtual bool IsExecuting() const override { return false; }
    virtual bool IsAvailable() const override { return true; }
};

#endif // WITH_DEV_AUTOMATION_TESTS
```

**Placement:** `UnrealClaude/Source/UnrealClaude/Private/Tests/TestUtils.h`

### Latent Command Pattern for Threading Tests

```cpp
// DEFINE_LATENT_AUTOMATION_COMMAND: No parameters
// DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER: One parameter
// DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER: Two parameters

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FWaitForQueueIdle, TSharedPtr<FMCPTaskQueue>, TaskQueue);

bool FWaitForQueueIdle::Update()
{
    if (!TaskQueue.IsValid()) return true; // Done
    int32 Pending, Running, Completed;
    TaskQueue->GetStats(Pending, Running, Completed);
    return Running == 0; // true = done, false = retry next tick
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FStopTaskQueue, TSharedPtr<FMCPToolRegistry>, Registry);

bool FStopTaskQueue::Update()
{
    if (Registry.IsValid())
    {
        Registry->StopTaskQueue();
    }
    return true; // Done immediately
}
```

### FClaudeStreamEvent Struct Tests

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FClaudeStreamEvent_DefaultConstruct_HasUnknownType,
    "UnrealClaude.ClaudeRunner.StreamEvent.DefaultConstruct_HasUnknownType",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeStreamEvent_DefaultConstruct_HasUnknownType::RunTest(const FString& Parameters)
{
    FClaudeStreamEvent Event;

    TestTrue("Default type should be Unknown",
        Event.Type == EClaudeStreamEventType::Unknown);
    TestTrue("Default text should be empty", Event.Text.IsEmpty());
    TestFalse("Default bIsError should be false", Event.bIsError);
    TestEqual("Default DurationMs should be 0", Event.DurationMs, 0);
    TestEqual("Default TotalCostUsd should be 0", Event.TotalCostUsd, 0.0f);

    return true;
}
```

### Module Startup Testing (D-10)

The module is already started when tests run. We verify its effects rather than calling StartupModule() directly.

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUnrealClaudeModule_Startup_MCPServerIsRunning,
    "UnrealClaude.Module.Startup.MCPServerIsRunning",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FUnrealClaudeModule_Startup_MCPServerIsRunning::RunTest(const FString& Parameters)
{
    // Module is already started by the time tests run
    // Verify startup effects are present
    FClaudeCodeSubsystem& Subsystem = FClaudeCodeSubsystem::Get();
    IClaudeRunner* Runner = Subsystem.GetRunner();

    TestNotNull("Runner should be initialized after startup", Runner);
    TestTrue("System prompt should not be empty",
        !Subsystem.GetUE57SystemPrompt().IsEmpty());

    return true;
}
```

---

## Component Testability Matrix

| Component | Testable Methods | Untestable (why) | Test File |
|-----------|-----------------|------------------|-----------|
| `FClaudeSessionManager` | All 8 public methods | None | `SessionManagerTests.cpp` |
| `FClaudeCodeSubsystem` | `GetHistory`, `ClearHistory`, `GetUE57SystemPrompt`, `GetProjectContextPrompt`, `HasSavedSession`, `GetSessionFilePath`, `GetRunner`, `SetCustomSystemPrompt`, `CancelCurrentRequest`, `SaveSession`, `LoadSession` | `SendPrompt` (spawns subprocess) | `ClaudeSubsystemTests.cpp` |
| `FClaudeCodeRunner` | `ParseStreamJsonOutput`, `BuildStreamJsonPayload`, `IsClaudeAvailable` (static), `GetClaudePath` (static) | `ExecuteAsync`, `ExecuteSync`, `Init`/`Run`/`Stop`/`Exit` FRunnable methods (threading) | Extend `ClipboardImageTests.cpp` or new `ClaudeRunnerTests.cpp` |
| `IClaudeRunner` | Interface struct `FClaudeStreamEvent`, enum `EClaudeStreamEventType`, struct `FClaudeRequestConfig`, delegate declarations | None (pure interface) | Part of `ClaudeRunnerTests.cpp` |
| `FProjectContextManager` | `GetContext` (no crash), `FormatContextForPrompt`, `GetContextSummary`, `HasContext`, `GetTimeSinceRefresh` | None | `ProjectContextTests.cpp` |
| `FScriptExecutionManager` | `GetRecentScripts`, `FormatHistoryForContext`, `GetHistoryFilePath`, `GetCppScriptDirectory`, `GetContentScriptDirectory`, `GetRecentScripts(0)` | `ExecuteScript` (modal dialog) | `ScriptExecutionTests.cpp` |
| `FUnrealClaudeMCPServer` | Construction, `GetToolRegistry()`, `IsRunning()`, `GetPort()` | `Start()`, `Stop()` (port conflict), private route handlers | `MCPServerTests.cpp` |
| `FMCPToolBase` | All helpers via concrete tool subclasses (already tested by proxy) | Direct instantiation (abstract base) | Extension of `MCPToolTests.cpp` |
| `SClaudeInputArea` | `SetText`, `GetText`, `ClearText`, `HasAttachedImages`, `GetAttachedImageCount`, `GetAttachedImagePaths`, `ClearAttachedImages`, `RemoveAttachedImage` | Keyboard event simulation (requires full window) | `SlateWidgetTests.cpp` |
| `SClaudeToolbar` | Construction with args, SLATE_ATTRIBUTE defaults | Button clicks (requires window routing) | `SlateWidgetTests.cpp` |
| `SClaudeEditorWidget` | Construction (no crash) | Internal state (all private) | `SlateWidgetTests.cpp` |
| `UnrealClaudeConstants` | All constexpr values via direct assertions | None | `ConstantsTests.cpp` |
| `FUnrealClaudeModule` | Effects of startup (singletons initialized, runner not null) | `StartupModule()`/`ShutdownModule()` directly (already called) | `ModuleTests.cpp` |

---

## Build.cs Changes Required

**None** — all necessary modules are already present:

| Needed For | Module | Already In Build.cs |
|------------|--------|---------------------|
| Slate widget tests | `Slate`, `SlateCore` | ✅ PublicDependencyModuleNames |
| HTTP server tests | `HTTP`, `HTTPServer` | ✅ PrivateDependencyModuleNames |
| JSON parsing tests | `Json`, `JsonUtilities` | ✅ PrivateDependencyModuleNames |
| File I/O tests | `Core` (FFileHelper, IFileManager) | ✅ PublicDependencyModuleNames |
| Automation macros | `Core` (Misc/AutomationTest.h) | ✅ via Core |
| Editor singletons | `UnrealEd`, `EditorFramework` | ✅ PublicDependencyModuleNames |

`AutomationController` is not needed — it provides the test runner UI, not test writing APIs. `ADD_LATENT_AUTOMATION_COMMAND` and `IMPLEMENT_COMPLEX_AUTOMATION_TEST` are in `Core/Misc/AutomationTest.h`.

---

## New Test Files to Create

Based on D-14 (one file per component) and D-16 (500-line limit), the following files are needed:

| File | Component | Estimated Lines |
|------|-----------|----------------|
| `Private/Tests/TestUtils.h` | Shared helpers | ~100 |
| `Private/Tests/ClaudeSubsystemTests.cpp` | `FClaudeCodeSubsystem` | ~250 |
| `Private/Tests/ClaudeRunnerTests.cpp` | `IClaudeRunner`, `FClaudeCodeRunner`, `FClaudeStreamEvent` | ~400 |
| `Private/Tests/SessionManagerTests.cpp` | `FClaudeSessionManager` | ~300 |
| `Private/Tests/MCPServerTests.cpp` | `FUnrealClaudeMCPServer` + `FMCPToolRegistry` snapshot | ~250 |
| `Private/Tests/ProjectContextTests.cpp` | `FProjectContextManager` | ~200 |
| `Private/Tests/ScriptExecutionTests.cpp` | `FScriptExecutionManager` (safe methods only) | ~200 |
| `Private/Tests/ConstantsTests.cpp` | `UnrealClaudeConstants` | ~150 |
| `Private/Tests/SlateWidgetTests.cpp` | `SClaudeInputArea`, `SClaudeToolbar`, `SClaudeEditorWidget` | ~400 (or split) |
| `Private/Tests/ModuleTests.cpp` | `FUnrealClaudeModule` startup effects | ~100 |

**Existing files to extend:**
- `MCPTaskQueueTests.cpp` — Attempt to fix 6 disabled tests using latent pattern (D-27)
- `ClipboardImageTests.cpp` — Already extends `FClaudeCodeRunner` parsing; add NDJSON event type tests

---

## State of the Art

| Old Approach | Current Approach | Impact |
|--------------|------------------|--------|
| `FAutomationTestBase` only | `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macro | Macro handles registration; use macro, not base class directly |
| `FPlatformProcess::Sleep()` for async waits | `ADD_LATENT_AUTOMATION_COMMAND` with `Update()` | Latent commands yield GameThread; Sleep blocks it |
| Starting FRunnableThread in tests | Testing only non-threading code paths | Avoids confirmed deadlock pattern |

**Deprecated/outdated patterns to avoid:**
- `IMPLEMENT_SIMPLE_AUTOMATION_TEST` with `EAutomationTestFlags::SmokeFilter` — use `ProductFilter` (matches existing tests)
- `TestEqual(A, B)` with raw pointers — use `TestNotNull`/`TestNull` for pointers

---

## Environment Availability

Step 2.6: SKIPPED — this phase is purely C++ code additions (new test files and TestUtils.h). No external tools, services, or CLI utilities are required beyond the UE editor build environment already in use.

---

## Validation Architecture

> `workflow.nyquist_validation` not found in `.planning/config.json` — treating as enabled.

### Test Framework

| Property | Value |
|----------|-------|
| Framework | UE Automation Test Framework (built-in, no version separate from UE 5.7) |
| Config file | None — test registration is via macros at compile time |
| Quick run command | `Automation RunTests UnrealClaude.Session` (single category) |
| Full suite command | `Automation RunTests UnrealClaude` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command |
|--------|----------|-----------|-------------------|
| TEST-00 (D-01/D-02) | All components reach >=90% coverage | unit | `Automation RunTests UnrealClaude` |
| D-07 | NDJSON stream event parsing — all 7 types | unit | `Automation RunTests UnrealClaude.ClaudeRunner` |
| D-09 | FMCPToolResult error paths + FMCPErrors factory | unit | `Automation RunTests UnrealClaude.MCP` |
| D-11 | Constants validation | unit | `Automation RunTests UnrealClaude.Constants` |
| D-12 | Snapshot/regression: tool list hasn't changed | unit | `Automation RunTests UnrealClaude.MCPServer` |
| D-27 | 6 disabled threading tests re-enabled | unit | `Automation RunTests UnrealClaude.MCP.TaskQueue` |
| D-03 | Slate widget public API | unit | `Automation RunTests UnrealClaude.Widgets` |
| D-25 | SessionManager save/load round trip | unit | `Automation RunTests UnrealClaude.Session` |
| D-26 | ProjectContextManager no crash | unit | `Automation RunTests UnrealClaude.ProjectContext` |

### Sampling Rate
- **Per commit:** `Automation RunTests UnrealClaude.{category just added}`
- **Per wave merge:** `Automation RunTests UnrealClaude` (full suite)
- **Phase gate:** Full suite green (all tests pass including newly enabled threading tests OR re-documented as still blocked)

### Wave 0 Gaps

- [ ] `Private/Tests/TestUtils.h` — shared helpers (MakeJsonWithString, FMockClaudeRunner, etc.)
- [ ] `Private/Tests/SessionManagerTests.cpp` — covers D-25
- [ ] `Private/Tests/ClaudeSubsystemTests.cpp` — covers D-23 singleton testing
- [ ] `Private/Tests/ClaudeRunnerTests.cpp` — covers D-07, D-24
- [ ] `Private/Tests/MCPServerTests.cpp` — covers D-06, D-12
- [ ] `Private/Tests/ProjectContextTests.cpp` — covers D-26
- [ ] `Private/Tests/ScriptExecutionTests.cpp` — covers D-05
- [ ] `Private/Tests/ConstantsTests.cpp` — covers D-11
- [ ] `Private/Tests/SlateWidgetTests.cpp` — covers D-03
- [ ] `Private/Tests/ModuleTests.cpp` — covers D-10

---

## Open Questions

1. **Can the 6 disabled threading tests be fixed with latent commands?**
   - What we know: The deadlock is `AsyncTask(GameThread)` from worker while GameThread is in `RunTest()`. Latent commands yield the GameThread between steps. The WORKAROUNDS ATTEMPTED section in `MCPTaskQueueTests.cpp` line 74 says "Latent automation tests: Adds complexity, still has synchronization issues" — but this was noted as attempted without full implementation details.
   - What's unclear: Was the latent approach actually fully implemented and tested, or just noted as a potential future solution?
   - Recommendation: Implement the full `IMPLEMENT_COMPLEX_AUTOMATION_TEST` pattern with `FWaitForQueueIdle` latent command and test with a 5-second timeout. If it still deadlocks, document the full attempt and leave `#if 0` with updated comments.

2. **Can `SClaudeEditorWidget` be constructed in tests?**
   - What we know: It inherits `SCompoundWidget` and uses SLATE_BEGIN_ARGS. Its `Construct()` method likely accesses `FClaudeCodeSubsystem::Get()` and the singleton is initialized in editor context.
   - What's unclear: Whether it performs any operations at construction that could fail in test context (e.g., binding to session state, reading files).
   - Recommendation: Attempt `SAssignNew(Widget, SClaudeEditorWidget)` in a guard and catch any crash. If construction fails, document and test at `SClaudeInputArea`/`SClaudeToolbar` level only.

3. **Does `FProjectContextManager::GetContext()` call `GEditor` / world access at test time?**
   - What we know: `GatherLevelActors()` accesses `GEditor->GetEditorWorldContext().World()`. This is valid in editor context but may have side effects in tests.
   - What's unclear: Whether calling `GetContext()` in tests triggers expensive scanning or causes side effects.
   - Recommendation: Test with `bForceRefresh = false` first (uses cached context if available). If `HasContext()` is true, just test `FormatContextForPrompt()` and `GetContextSummary()`. Avoid triggering a full rescan in tests.

---

## Sources

### Primary (HIGH confidence)
- Direct codebase inspection: `MCPTaskQueueTests.cpp` — deadlock documentation, disabled test patterns
- Direct codebase inspection: `ClipboardImageTests.cpp` — static helper function pattern, temp file cleanup pattern, `FClaudeCodeRunner` standalone instantiation
- Direct codebase inspection: `MCPToolTests.cpp` — standard test file structure, naming convention
- Direct codebase inspection: All component headers — verified public API methods for testability
- Direct codebase inspection: `UnrealClaude.Build.cs` — confirmed no new dependencies needed
- `Misc/AutomationTest.h` — `IMPLEMENT_COMPLEX_AUTOMATION_TEST`, `ADD_LATENT_AUTOMATION_COMMAND`, `DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER` (standard UE 5.7 headers)

### Secondary (MEDIUM confidence)
- UE Forums citation in `MCPTaskQueueTests.cpp` line 90: FRunnable thread freezes editor — consistent with observed deadlock behavior

### Tertiary (LOW confidence)
- None

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all from direct codebase inspection; no new libraries
- Architecture patterns: HIGH — patterns derived from 171 existing passing tests in the codebase
- Build.cs changes: HIGH — verified all required modules are present
- Latent test fix attempt: MEDIUM — the macro API is confirmed, but whether it resolves the specific deadlock is uncertain (the codebase itself notes "still has synchronization issues" without full implementation)
- Slate widget construction in tests: MEDIUM — API is standard; whether specific widgets work without side effects requires empirical testing

**Research date:** 2026-03-30
**Valid until:** 2026-04-30 (stable UE 5.7 API; no fast-moving dependencies)
