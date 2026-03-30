# Testing

**Analysis Date:** 2026-03-30

## Test Framework

**C++ (Plugin):**
- **Unreal Engine Automation Test Framework** — built into UE5, no external dependencies
- Macro: `IMPLEMENT_SIMPLE_AUTOMATION_TEST`
- Assertion macros: `TestTrue`, `TestFalse`, `TestEqual`, `TestNotNull`, `TestNull`
- Guard: All test code wrapped in `#if WITH_DEV_AUTOMATION_TESTS`

**JavaScript (MCP Bridge — git submodule, currently empty/uninitialised):**
- `Resources/mcp-bridge/` is a git submodule pointing to `Natfii/ue5-mcp-bridge`
- Bridge test runner: **npm test** (framework not inspectable — submodule is empty in this clone)

## Test Runner

**C++ tests:**
```
Automation RunTests UnrealClaude
```
Run from the Unreal Editor console (not from CLI). No standalone test runner or CI config detected.

**JavaScript tests (mcp-bridge submodule):**
```bash
cd UnrealClaude/Resources/mcp-bridge && npm test
```

## Test Organization

**Location:**
- All C++ test files live in `Source/UnrealClaude/Private/Tests/`
- Tests are co-located with implementation code in a dedicated `Tests/` subdirectory, not mirrored in a separate tree

**Naming convention:**
- Test files: `{Area}Tests.cpp` (e.g., `MCPToolTests.cpp`, `MCPParamValidatorTests.cpp`, `MCPTaskQueueTests.cpp`)
- Test class names: `F{System}_{Category}_{Scenario}` (e.g., `FMCPParamValidator_ValidateActorName_ValidNames`)
- Test string identifiers: `"UnrealClaude.{Category}.{SubCategory}.{TestName}"` (e.g., `"UnrealClaude.MCP.Tools.SpawnActor.GetInfo"`)

**Current test files:**
| File | Coverage Area |
|------|---------------|
| `Private/Tests/MCPToolTests.cpp` | Core actor tools, asset tools, task tools, blueprint query tools |
| `Private/Tests/MCPParamValidatorTests.cpp` | Security validation logic |
| `Private/Tests/MCPTaskQueueTests.cpp` | Task queue data structures (threading tests disabled) |
| `Private/Tests/CharacterToolTests.cpp` | Character MCP tool |
| `Private/Tests/ClipboardImageTests.cpp` | Clipboard image utilities, multimodal payloads |
| `Private/Tests/MaterialAssetToolTests.cpp` | Material and Asset MCP tools |
| `Private/Tests/MCPAnimBlueprintTests.cpp` | Animation blueprint tool |
| `Private/Tests/MCPAssetToolTests.cpp` | Asset tool operations |

**MCP bridge test structure (from CLAUDE.md — submodule not locally available):**
```
tests/unit/        — Schema conversion, HTTP client, context loader, async execution
tests/integration/ — Tool listing, tool execution with mocked Unreal server
tests/helpers/     — Mock fetch, fixtures
```

## Test Types

### Unit Tests

The majority of tests are unit tests that run in-process against tool/validator objects directly — no Unreal Editor world state is required.

**Pattern:**
```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMCPTool_SpawnActor_GetInfo,
    "UnrealClaude.MCP.Tools.SpawnActor.GetInfo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPTool_SpawnActor_GetInfo::RunTest(const FString& Parameters)
{
    FMCPTool_SpawnActor Tool;
    FMCPToolInfo Info = Tool.GetInfo();

    TestEqual("Tool name should be spawn_actor", Info.Name, TEXT("spawn_actor"));
    TestTrue("Description should not be empty", !Info.Description.IsEmpty());
    TestTrue("Should have parameters", Info.Parameters.Num() > 0);

    return true;
}
```

**Categories covered by unit tests:**
- `UnrealClaude.MCP.Tools.*` — Tool metadata (`GetInfo`) and parameter validation (`Execute` with bad params)
- `UnrealClaude.MCP.ParamValidator.*` — Actor name, blueprint path, property path, console command, numeric validation
- `UnrealClaude.MCP.Registry.*` — Tool registration, lookup, task queue data structure
- `UnrealClaude.ClipboardImage.*` — Path utilities, base64 encoding, JSON payload format

### Integration Tests

The task queue tests (`MCPTaskQueueTests.cpp`) test multiple interacting components (registry + task queue + tool lifecycle) but avoid starting the worker thread (see known issue below).

MCP bridge integration tests (in submodule) test tool listing and execution against a **mocked Unreal HTTP server** — they do not require a running UE instance.

### E2E Tests

No automated E2E tests in this repository. End-to-end verification is done by:
1. Running `Automation RunTests UnrealClaude` in the live Unreal Editor
2. Manual testing of MCP bridge calls against a live editor instance

## Test Flags

All C++ tests use:
```cpp
EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
```
This means tests run only in the UE Editor context (not in game builds).

## Known Test Limitations

**FRunnableThread deadlock in automation context** (documented in `MCPTaskQueueTests.cpp` lines 41–119 and `MCPToolTests.cpp` lines 1375–1425):

Threading tests are **intentionally disabled** due to a deadlock when `FRunnableThread` dispatches back to the game thread via `AsyncTask(GameThread)` while the game thread is synchronously blocked executing the automation test. Affected tests are commented out with detailed explanations.

Unaffected tests (data structure tests, tool validation tests) are fully active and cover the same code paths without requiring thread execution.

## Coverage

**No coverage tooling configured** — no `.lcov`, `.gcov`, coverage flags in `Build.cs`, or CI coverage reporting detected.

Coverage is enforced by convention:
- `CLAUDE.md` mandates: "Add tests in `Private/Tests/MCPToolTests.cpp`" for every new MCP tool
- Test additions must accompany: new tools, `context-loader.js` changes, bridge bug fixes, HTTP format changes

## CI Integration

**No CI/CD configuration detected** — no `.github/workflows/`, no `Jenkinsfile`, no Azure Pipelines, no CircleCI config in the repository.

Tests must be run manually before committing per `CLAUDE.md`:
```
Automation RunTests UnrealClaude  # Run in Unreal Editor console
```

JavaScript bridge tests:
```bash
cd Resources/mcp-bridge && npm test
```

---

*Testing analysis: 2026-03-30*
