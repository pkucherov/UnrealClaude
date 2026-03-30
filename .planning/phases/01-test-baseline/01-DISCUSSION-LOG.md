# Phase 1: Test Baseline - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-03-30
**Phase:** 01-test-baseline
**Areas discussed:** Coverage target scope, Testing approach for singletons, Disabled tests handling, Coverage documentation

---

## Coverage Target Scope

| Option | Description | Selected |
|--------|-------------|----------|
| Core + soon-to-refactor | Test the 3 named + ProjectContext and MCPToolBase | |
| Success criteria only (3) | Strictly IClaudeRunner, ClaudeSubsystem, SessionManager | |
| Full coverage sweep | All 8+ untested/partially-tested components | ✓ |

**User's choice:** Full coverage sweep
**Notes:** User wants comprehensive baseline before any changes.

---

| Option | Description | Selected |
|--------|-------------|----------|
| >=90% everywhere | Target >=90% for all components, document where infeasible | ✓ |
| >=90% core, best-effort rest | Hard target for 3 core, best-effort for others | |
| Function coverage | Every public method has at least one test | |

**User's choice:** >=90% everywhere
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Skip UI widgets | Skip Slate UI tests entirely | |
| Construction-only UI tests | Basic widget instantiation tests only | |
| Full UI testing | Full Slate widget testing including simulated input events | ✓ |

**User's choice:** Full UI testing
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| One file per component | Matches existing pattern | ✓ |
| Grouped by domain | Fewer files | |
| Extend existing files | Add to existing test files where possible | |

**User's choice:** One file per component
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Match existing convention | F{System}_{Category}_{Scenario} naming | ✓ |
| New component-first scheme | UnrealClaude.Subsystem.SendPrompt.EmptyInput style | |

**User's choice:** Match existing convention
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Fix first | Verify all 171 pass, fix failures before adding new tests | ✓ |
| Document only | Document failures but don't fix | |
| Skip verification | Focus on new tests only | |

**User's choice:** Fix first
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Apply to tests too | 500-line file, 50-line function limits for test code | ✓ |
| Relaxed file limit | 1000-line limit for test files | |
| No limits for tests | Focus on coverage, not style | |

**User's choice:** Apply to tests too
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Include bridge tests | Add tests for MCP bridge JavaScript side too | ✓ |
| C++ only | Only test C++ plugin code | |
| Verify only | Confirm npm test passes but don't add new JS tests | |

**User's choice:** Include bridge tests
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Include script execution | FScriptExecutionManager gets test coverage | ✓ |
| Skip for now | Defer to later phase | |

**User's choice:** Include script execution
**Notes:** Security-relevant permission gating is a good reason to include.

---

| Option | Description | Selected |
|--------|-------------|----------|
| Skip HTTP tests | Rely on external bridge tests | |
| Add HTTP route tests | Test route registration, request parsing, response formatting | ✓ |
| Registration only | Only test route wiring | |

**User's choice:** Add HTTP route tests
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| By refactoring risk | ClaudeSubsystem -> Runner -> SessionManager -> ... | ✓ |
| Easiest first | Build momentum with simpler components | |
| Bottom-up by dependency | Lower-level first, then mid, then top | |

**User's choice:** By refactoring risk
**Notes:** Ensures most-refactored code gets tests first.

---

| Option | Description | Selected |
|--------|-------------|----------|
| Comprehensive edge cases | Error paths, boundary conditions, malformed inputs | ✓ |
| Happy paths + security edges | Golden paths plus security-critical edge cases | |
| Happy paths only | Just verify golden path works | |

**User's choice:** Comprehensive edge cases
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Create test utilities | Shared TestUtils.h with mock factories, fixture helpers | ✓ |
| Self-contained tests | Each file standalone | |
| Extract on duplication | Extract only when duplicated 3+ times | |

**User's choice:** Create test utilities
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Add regression guards | Golden output / snapshot-style tests for Phase 2 safety | ✓ |
| Unit tests sufficient | Existing tests serve as regression guards | |
| Guards for runner + subsystem only | Only the most-refactored components | |

**User's choice:** Add regression guards
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| No Build.cs changes | Existing dependencies sufficient | |
| Add test-only dependencies | Add AutomationController or other test modules | ✓ |

**User's choice:** Add test-only dependencies
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Match existing flags | EditorContext, ProductFilter | ✓ |
| Minimize editor dependency | ApplicationContextMask where possible | |
| Add smoke test tier | SmokeTest flag for quick sanity checks | |

**User's choice:** Match existing flags
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Test constants | Validate port ranges, timeout positivity, max sizes | ✓ |
| Skip constants | Compile-time constexpr, not worth testing | |
| Security constants only | Only security-boundary constants | |

**User's choice:** Test constants
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Comprehensive stream tests | All event types, malformed JSON, partial lines, large payloads | ✓ |
| Extend existing tests | Add to ClipboardImageTests.cpp | |
| Skip (will change Phase 2) | Don't invest in tests that may be rewritten | |

**User's choice:** Comprehensive stream tests
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Test thread safety | Use non-blocking patterns to test concurrency | ✓ |
| Skip all threading tests | Known UE limitation, avoid entirely | |
| Data-level thread safety only | Concurrent reads/writes without FRunnableThread | |

**User's choice:** Test thread safety
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Skip perf tests | Purely functional correctness | ✓ |
| Basic timing guards | Timing assertions for critical paths | |
| Full benchmarks | Statistical performance analysis | |

**User's choice:** Skip perf tests
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| File headers + descriptive assertions | Component docs + descriptive TestTrue strings | ✓ |
| Self-documenting names only | No extra comments | |
| Full Doxygen per test | Complete setup/action/expected comments | |

**User's choice:** File headers + descriptive assertions
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Temp directory | FPaths::ProjectSavedDir() / 'Tests/' | ✓ |
| In-memory mocking | Mock file I/O | |
| Real directory, test prefix | Real Saved/UnrealClaude/ with test_ prefix | |

**User's choice:** Temp directory
**Notes:** Avoids polluting the real session directory.

---

| Option | Description | Selected |
|--------|-------------|----------|
| Test all error paths | Verify all FMCPToolResult error codes and messages | ✓ |
| Refactored tools only | Only tools touched by Phase 2 | |
| Existing coverage sufficient | Already well-tested | |

**User's choice:** Test all error paths
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Test module startup | Verify StartupModule() initializes singletons, menus, MCP server | ✓ |
| Skip (implicit) | Module loads for any test to run | |
| Singleton accessibility only | Just verify Get() returns non-null | |

**User's choice:** Test module startup
**Notes:** None

---

## Testing Approach for Singletons

| Option | Description | Selected |
|--------|-------------|----------|
| Direct singleton testing | Test via static Get() accessors, no mock injection | ✓ |
| Mock injection | Create mock/stub versions (requires refactoring) | |
| Adapter pattern wrapping | Thin adapters that can be swapped for mocks | |

**User's choice:** Direct singleton testing
**Notes:** Matches existing test patterns, avoids premature refactoring.

---

| Option | Description | Selected |
|--------|-------------|----------|
| Helpers + signature verification | Test helper methods directly, compile-time signature checks | ✓ |
| Mock subprocess | Pre-recorded NDJSON streams | |
| Real subprocess testing | Actually spawn claude CLI | |

**User's choice:** Helpers + signature verification
**Notes:** Avoids external dependency on Claude CLI being installed.

---

| Option | Description | Selected |
|--------|-------------|----------|
| State machine + temp dir | Test add/clear/save/load + max-size using temp directory | ✓ |
| Full save/load roundtrip | Full lifecycle including disk roundtrip | |
| In-memory operations only | Skip file I/O testing | |

**User's choice:** State machine + temp dir
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Structure validation | Verify scan/format/return without asserting specific counts | ✓ |
| Fixture-based exact assertions | Known project state with exact count assertions | |
| Existence checks only | Just verify Get() non-null and HasContext() | |

**User's choice:** Structure validation
**Notes:** Output depends on actual project state — asserting specific counts would be brittle.

---

## Disabled Tests Handling

| Option | Description | Selected |
|--------|-------------|----------|
| Attempt fix, document if fails | Try to resolve deadlock, re-enable if successful | ✓ |
| Leave disabled, document | Known UE limitation, just document | |
| Rewrite as non-threaded | Replace with equivalent non-threading tests | |
| You decide | Agent's discretion on approach | |

**User's choice:** Attempt fix, document if fails
**Notes:** 6 disabled tests represent real functionality gaps worth addressing.

---

| Option | Description | Selected |
|--------|-------------|----------|
| Latent tests first | AddCommand pattern allows game thread to tick between steps | ✓ |
| Event wait with timeout | FEvent/FPlatformProcess::Sleep with timeout | |
| AsyncTask yield pattern | Use AsyncTask(GameThread) to yield from test | |

**User's choice:** Latent tests first
**Notes:** None

---

## Coverage Documentation

| Option | Description | Selected |
|--------|-------------|----------|
| Component coverage table | Table per component with method-level tracking | |
| Automated from test results | Parse UE automation test results | |
| Before/after counts only | Simple '171 before, N after' | |
| Full coverage report | Method-level tracking, gap analysis, risk assessment | ✓ |

**User's choice:** Full coverage report
**Notes:** None

---

| Option | Description | Selected |
|--------|-------------|----------|
| Phase directory | .planning/phases/01-test-baseline/ | |
| Codebase docs directory | .planning/codebase/ alongside TESTING.md | ✓ |
| Tests directory | Alongside test files | |

**User's choice:** Codebase docs directory
**Notes:** Persists across phases as a codebase-level document.

---

| Option | Description | Selected |
|--------|-------------|----------|
| Update incrementally | Each commit with tests updates the report | ✓ |
| Generate at end | Single final report | |
| Baseline + final snapshots | Two snapshots: before and after | |

**User's choice:** Update incrementally
**Notes:** None

---

## Agent's Discretion

- Exact test utility function signatures and organization
- Slate widget test fixture structure for simulated input
- Specific mock patterns for HTTP route testing
- Whether to split large test files to stay under 500-line limit
- Temp directory cleanup strategy details

## Deferred Ideas

None — discussion stayed within phase scope.
