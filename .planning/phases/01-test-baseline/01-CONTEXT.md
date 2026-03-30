# Phase 1: Test Baseline - Context

**Gathered:** 2026-03-30
**Status:** Ready for planning

<domain>
## Phase Boundary

Establish existing test coverage baseline before any code changes. Measure current test coverage, document it, fill gaps in all components to >=90% where feasible, and produce a comprehensive coverage report. This provides a safety net before Phase 2 refactoring begins.

</domain>

<decisions>
## Implementation Decisions

### Coverage Target Scope
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

### Test Organization
- **D-14:** One test file per component (e.g., ClaudeSubsystemTests.cpp, SessionManagerTests.cpp, MCPServerTests.cpp)
- **D-15:** Follow existing naming convention: F{System}_{Category}_{Scenario} with 'UnrealClaude.{Category}.{SubCategory}.{TestName}' identifiers
- **D-16:** Apply 500-line file limit and 50-line function limit to test code (tests are production code)
- **D-17:** All new tests use EditorContext | ProductFilter flags (matching existing tests)
- **D-18:** Create shared test utilities (mock factories, fixture helpers, common assertions) in a TestUtils.h file
- **D-19:** File headers with component documentation + descriptive assertion strings in test bodies

### Priority Order
- **D-20:** Write tests in refactoring-risk order: ClaudeSubsystem -> IClaudeRunner/ClaudeCodeRunner -> SessionManager -> MCPServer -> ProjectContext -> ScriptExecution -> MCPToolBase -> UI widgets -> Bridge JS

### Existing Test Verification
- **D-21:** Verify all 171 existing tests pass before writing new ones. If any fail, fix them first as part of this phase
- **D-22:** Add test-only module dependencies to Build.cs if needed (e.g., AutomationController for advanced test features)

### Singleton Testing Approach
- **D-23:** Test singletons directly via their static Get() accessors — no mock injection, no refactoring of Get() methods
- **D-24:** ClaudeCodeRunner: test helper methods directly (ParseStreamJsonOutput, BuildStreamJsonPayload), verify RunPrompt/IsRunning/Cancel signatures via compile-time checks, don't spawn actual claude subprocess
- **D-25:** SessionManager: test state machine (add/clear/save/load, max-size enforcement, file path resolution) using temp directory (FPaths::ProjectSavedDir() / 'Tests/')
- **D-26:** ProjectContextManager: structure validation — verify scan/format/return doesn't crash, check output structure (project name, source file count, asset summary), don't assert specific counts

### Disabled Tests
- **D-27:** Attempt to fix FRunnableThread deadlock for 6 disabled tests in MCPTaskQueueTests.cpp. If fix succeeds, re-enable all 6. If not, document the attempt and leave disabled
- **D-28:** Fix strategy: try latent automation tests (AddCommand pattern) first — allows game thread to tick between test steps, avoiding deadlock. Fall back to FTaskGraphInterface if needed

### Coverage Documentation
- **D-29:** Full coverage report with method-level tracking, gap analysis, and risk assessment for untested areas
- **D-30:** Report lives in .planning/codebase/ alongside TESTING.md (codebase-level document persisting across phases)
- **D-31:** Update coverage report incrementally as each component gets tests — each commit that adds tests should update the report

### Agent's Discretion
- Exact test utility function signatures and organization
- How to structure Slate widget test fixtures for simulated input
- Specific mock patterns for HTTP route testing
- Whether to split large component test files further to stay under 500-line limit
- Temp directory cleanup strategy details

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Testing infrastructure
- `.planning/codebase/TESTING.md` — Current test framework, runner, organization, known limitations (FRunnableThread deadlock), and coverage conventions
- `.planning/codebase/STRUCTURE.md` — Project structure including test file locations and naming conventions
- `.planning/codebase/CONVENTIONS.md` — Code style, naming, documentation, and file size limits

### Existing test files (read to understand patterns)
- `UnrealClaude/Source/UnrealClaude/Private/Tests/MCPToolTests.cpp` — Largest test file (2097 lines, 64 tests), core MCP tool patterns
- `UnrealClaude/Source/UnrealClaude/Private/Tests/MCPTaskQueueTests.cpp` — Contains 6 disabled tests with detailed deadlock documentation (lines 41-119)
- `UnrealClaude/Source/UnrealClaude/Private/Tests/ClipboardImageTests.cpp` — ClaudeCodeRunner helper tests and FClaudePromptOptions tests

### Components to test
- `UnrealClaude/Source/UnrealClaude/Public/IClaudeRunner.h` — IClaudeRunner interface, FClaudeStreamEvent, delegates
- `UnrealClaude/Source/UnrealClaude/Public/ClaudeSubsystem.h` — FClaudeCodeSubsystem public API
- `UnrealClaude/Source/UnrealClaude/Public/ClaudeSessionManager.h` — Session persistence API
- `UnrealClaude/Source/UnrealClaude/Public/ClaudeCodeRunner.h` — FClaudeCodeRunner : IClaudeRunner
- `UnrealClaude/Source/UnrealClaude/Private/ProjectContext.h` — FProjectContextManager singleton
- `UnrealClaude/Source/UnrealClaude/Public/ScriptExecutionManager.h` — FScriptExecutionManager with permission gating
- `UnrealClaude/Source/UnrealClaude/Private/MCP/UnrealClaudeMCPServer.h` — HTTP server routes
- `UnrealClaude/Source/UnrealClaude/Private/MCP/MCPToolBase.h` — Shared tool helper base class
- `UnrealClaude/Source/UnrealClaude/Private/Widgets/SClaudeInputArea.h` — Input widget
- `UnrealClaude/Source/UnrealClaude/Private/Widgets/SClaudeToolbar.h` — Toolbar widget
- `UnrealClaude/Source/UnrealClaude/Public/ClaudeEditorWidget.h` — Main chat panel widget
- `UnrealClaude/Source/UnrealClaude/Public/UnrealClaudeConstants.h` — Constants to validate
- `UnrealClaude/Source/UnrealClaude/Private/UnrealClaudeModule.cpp` — Module startup/shutdown

### Build configuration
- `UnrealClaude/Source/UnrealClaude/UnrealClaude.Build.cs` — Module dependencies (may need test-only additions)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **IMPLEMENT_SIMPLE_AUTOMATION_TEST macro**: All 171 existing tests use this pattern consistently — new tests should follow the same pattern
- **FMCPToolBase helpers**: ExtractRequiredString, ValidateBlueprintPathParam, ValidateEditorContext — these are indirectly tested through tool tests but could benefit from direct unit tests
- **FMCPParamValidator**: Already has 11 comprehensive validation tests — pattern to replicate for new validator coverage
- **ClipboardImageTests.cpp**: Contains the only existing ClaudeCodeRunner tests (ParseStreamJsonOutput, BuildStreamJsonPayload) — extend this pattern for NDJSON stream event testing

### Established Patterns
- **Test file structure**: Copyright header, #if WITH_DEV_AUTOMATION_TESTS guard, IMPLEMENT_SIMPLE_AUTOMATION_TEST macros, descriptive assertion strings
- **Test categorization**: "UnrealClaude.{Category}.{SubCategory}.{TestName}" hierarchy for test runner filtering
- **Known issue documentation**: Detailed inline comments with root cause, attempted workarounds, and references (see MCPTaskQueueTests.cpp lines 41-119)
- **Tool testing pattern**: GetInfo metadata check + Execute with bad params for error path validation

### Integration Points
- **Test runner**: `Automation RunTests UnrealClaude` in UE Editor console — all new tests must appear under this category
- **Build.cs**: May need additional module dependencies for HTTP testing, Slate widget testing, or AutomationController
- **MCP bridge**: JavaScript tests run via `cd Resources/mcp-bridge && npm test` — separate from C++ test runner
- **Module initialization**: Tests run after StartupModule() — singletons are already initialized when tests execute

</code_context>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches within the decisions captured above.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 01-test-baseline*
*Context gathered: 2026-03-30*
