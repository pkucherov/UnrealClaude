# Codebase Concerns

**Analysis Date:** 2026-03-30

---

## Tech Debt

**Unimplemented stubs in ScriptExecutionManager:**
- Issue: `ExecuteEditorUtility()` returns "not yet implemented" — callers receive an error response for a documented feature
- Files: `Source/UnrealClaude/Private/ScriptExecutionManager.cpp` (line ~493–495)
- Impact: Any MCP client requesting EditorUtility script execution silently gets an error; no fallback
- Fix approach: Implement via `UEditorUtilityLibrary::SpawnEditorUtilityWidget` or document explicitly as out-of-scope and remove the dead code path

**Unimplemented BlendSpace parameter binding:**
- Issue: `TODO: Bind parameters to variables` in `AnimAssetManager.cpp` line 188 — BlendSpace parameters are read but never applied
- Files: `Source/UnrealClaude/Private/AnimAssetManager.cpp`
- Impact: Any animation asset operation targeting BlendSpace parameter binding silently does nothing
- Fix approach: Implement parameter-to-variable binding using `UBlendSpace::GetBlendParameters()` and apply to target variables, or surface an error to caller

**Unimplemented Montage-to-state assignment:**
- Issue: `AnimAssetManager.cpp` line 266 logs "not yet implemented" for Montage assignment to states
- Files: `Source/UnrealClaude/Private/AnimAssetManager.cpp`
- Impact: Montage-to-state workflow returns a misleading success without any effect
- Fix approach: Implement via `UAnimStateMachine` state entry transitions or return a proper error

**Disabled threading tests left in codebase:**
- Issue: Two test blocks wrapped in `#if 0` due to a documented game-thread deadlock, but the underlying issue is unresolved
- Files: `Source/UnrealClaude/Private/Tests/MCPTaskQueueTests.cpp` (line ~97), `Source/UnrealClaude/Private/Tests/MCPToolTests.cpp` (lines ~1416–1422)
- Impact: Task queue threading behaviour is untested; the deadlock risk remains latent in production
- Fix approach: Resolve the game-thread deadlock (likely by ensuring `FMCPTaskQueue::Run()` dispatches via `AsyncTask(ENamedThreads::GameThread, ...)` instead of blocking), then re-enable tests

**Dead `EMCPErrorCode` enum:**
- Issue: `MCPErrors.h` defines a rich `EMCPErrorCode` enum with ~15 values, but no tool or server code references it — all tools use plain string errors
- Files: `Source/UnrealClaude/Private/MCP/MCPErrors.h`
- Impact: Structured error codes were intended for client-side error handling but are never surfaced; clients cannot distinguish error types
- Fix approach: Either adopt the enum in `FMCPToolResult::Error()` overloads and serialize the code in JSON responses, or remove the enum entirely

**Build-stamp warning log at startup:**
- Issue: `UnrealClaudeModule.cpp` line 32 emits `UE_LOG(LogUnrealClaude, Warning, ...)` with a hardcoded build stamp `"BUILD 20260107-1450 THREAD_TESTS_DISABLED"` — a development artifact logged as `Warning` level every startup
- Files: `Source/UnrealClaude/Private/UnrealClaudeModule.cpp`
- Impact: Pollutes the Output Log with a misleading warning on every editor start; confuses users about plugin stability
- Fix approach: Remove the log line or change to `Display` level and remove the stale build stamp string

**Hardcoded version string in status endpoint:**
- Issue: `UnrealClaudeMCPServer.cpp` line 275 returns `"version": "1.0.0"` hardcoded, but the plugin is at version 1.4.1 (per `.uplugin`)
- Files: `Source/UnrealClaude/Private/MCP/UnrealClaudeMCPServer.cpp`
- Impact: MCP clients that check version for capability negotiation receive wrong information
- Fix approach: Read version from `IPluginManager::Get().FindPlugin("UnrealClaude")->GetDescriptor().VersionName` at runtime

---

## Known Bugs

**Game-thread blocking in `TriggerLiveCodingCompile()`:**
- Symptoms: Editor freezes for up to 60 seconds when a live-coding compile is triggered via MCP
- Files: `Source/UnrealClaude/Private/ScriptExecutionManager.cpp`
- Trigger: Any MCP tool that calls `TriggerLiveCodingCompile()` — currently `execute_script` after code changes
- Cause: A `FPlatformProcess::Sleep(0.5f)` polling loop runs on the game thread waiting for compile completion
- Workaround: None — editor is unresponsive during compile
- Fix approach: Move the polling loop to a background thread and use a delegate/event to signal completion back to the game thread

**`MCPTool_GetScriptHistory` ignores the `MaxHistoryQueryCount` constant:**
- Symptoms: History queries always cap at 50 regardless of `UnrealClaudeConstants::ScriptExecution::MaxHistoryQueryCount`
- Files: `Source/UnrealClaude/Private/MCP/Tools/MCPTool_GetScriptHistory.cpp`
- Trigger: Any call to `get_script_history` MCP tool
- Fix approach: Replace magic `50` with `UnrealClaudeConstants::ScriptExecution::MaxHistoryQueryCount`

**`MCPTool_BlueprintQueryList` uses magic number clamp:**
- Symptoms: List queries silently clamp at 1000 results with no constant backing
- Files: `Source/UnrealClaude/Private/MCP/Tools/MCPTool_BlueprintQueryList.cpp`
- Fix approach: Define `MaxBlueprintQueryResults` in `UnrealClaudeConstants` and reference it

---

## Security Considerations

**No authentication on MCP HTTP server:**
- Risk: Any process on the local machine can send arbitrary MCP tool calls — including `run_console_command` which can execute engine CVars
- Files: `Source/UnrealClaude/Private/MCP/UnrealClaudeMCPServer.cpp`
- Current mitigation: Server only binds to `127.0.0.1` (localhost), so remote access is blocked
- Recommendations: Add an optional API key header check (configurable via editor settings) to prevent accidental exposure if port forwarding is ever active; log all incoming tool invocations with source IP

**CORS header covers all localhost ports:**
- Risk: `Access-Control-Allow-Origin: http://localhost` without a port number — browsers interpret this as allowing any port on localhost, which is broader than intended
- Files: `Source/UnrealClaude/Private/MCP/UnrealClaudeMCPServer.cpp` (line ~310)
- Current mitigation: None beyond localhost binding
- Recommendations: Set the CORS header to `http://localhost:3000` (matching the server port) or make it configurable to match the actual Claude Desktop origin

**CVar command blocklist relies on prefix matching only:**
- Risk: `MCPParamValidator` blocks `r.` and `mem` prefixes but the blocklist is manually maintained — new dangerous CVars added in future UE versions are not automatically blocked
- Files: `Source/UnrealClaude/Private/MCP/MCPParamValidator.cpp`
- Current mitigation: Prefix blocklist covers common rendering and memory commands
- Recommendations: Consider an allowlist approach for permitted command categories, or add a review step in `MCPParamValidator` when upgrading UE versions

---

## Performance Bottlenecks

**Game-thread sleep loop in live-coding compile wait:**
- Problem: `ScriptExecutionManager::TriggerLiveCodingCompile()` polls with `FPlatformProcess::Sleep(0.5f)` in a loop on the game thread
- Files: `Source/UnrealClaude/Private/ScriptExecutionManager.cpp`
- Cause: No async compile-completion callback is used
- Improvement path: Subscribe to `ILiveCodingModule::GetOnPatchCompleteDelegate()` and signal a thread-safe event rather than polling

**`ProjectContext::RefreshContext()` re-entrant lock risk:**
- Problem: `RefreshContext()` acquires `FScopeLock Lock(&ContextLock)` — if any caller also holds `ContextLock`, this deadlocks
- Files: `Source/UnrealClaude/Private/ProjectContext.cpp`
- Cause: `ContextLock` is a `FCriticalSection`; re-entrancy will deadlock on Windows
- Improvement path: Audit all callers of `RefreshContext()` to ensure none hold `ContextLock`; or switch to `FRWLock` with read/write separation

**`FMCPTaskQueue::Run()` unconditional 10ms sleep:**
- Problem: Task queue worker thread sleeps 10ms on every iteration even when there are queued tasks, adding latency to every MCP tool call
- Files: `Source/UnrealClaude/Private/MCP/MCPTaskQueue.cpp`
- Cause: Fixed `FPlatformProcess::Sleep(0.010f)` rather than a condition variable wait
- Improvement path: Replace with `FEvent::Wait()` / `FEvent::Trigger()` so the thread wakes immediately when a task is enqueued

---

## Fragile Areas

**`AnimationBlueprintUtils.cpp` — 2400-line monolith:**
- Files: `Source/UnrealClaude/Private/AnimationBlueprintUtils.cpp`
- Why fragile: Single file handles Blueprint graph editing, state machine manipulation, node connection, and asset creation — far exceeds the 500-line project guideline; highly coupled to UE internal graph editor APIs that change between engine versions
- Safe modification: Any change should be preceded by a search for the specific function being modified; test manually against a known AnimBP fixture since there are no automated tests
- Test coverage: Zero automated test coverage for this file

**`MCPToolRegistry.cpp` — unsafe static_cast on shared pointer:**
- Files: `Source/UnrealClaude/Private/MCP/MCPToolRegistry.cpp` (line ~121)
- Why fragile: `static_cast<FMCPTool_ExecuteScript*>(Tool.Get())` downcasts a `TSharedPtr<IMCPTool>` without a runtime type check — if the wrong tool type is registered at that slot, this is undefined behaviour
- Safe modification: Replace with `StaticCastSharedPtr<FMCPTool_ExecuteScript>(Tool)` and add a `nullptr` check before use
- Test coverage: Not tested

**`ClaudeCodeRunner.cpp` — 1251-line subprocess manager:**
- Files: `Source/UnrealClaude/Private/ClaudeCodeRunner.cpp`
- Why fragile: Manages OS subprocess lifecycle, NDJSON stream parsing, and JSON payload construction in one class; platform-specific `FPlatformProcess` calls are hard to mock
- Safe modification: Changes to stream parsing logic risk breaking NDJSON frame boundaries — test with actual Claude CLI before merging
- Test coverage: Zero automated tests

**`MCPToolTests.cpp` — 2097 lines with disabled blocks:**
- Files: `Source/UnrealClaude/Private/Tests/MCPToolTests.cpp`
- Why fragile: Exceeds 500-line guideline; disabled `#if 0` blocks create confusion about which tests are active; large size makes it hard to find relevant test for a given tool
- Safe modification: When adding new tool tests, search for existing test patterns to avoid duplicating setup code
- Test coverage: Threading-related tests are disabled

---

## Scaling Limits

**Single-threaded MCP task queue:**
- Current capacity: One background thread processes all MCP tool calls sequentially
- Limit: Long-running tools (e.g., compile waits) block all subsequent MCP requests
- Scaling path: Allow configurable worker thread count; or implement a priority lane (fast/slow) so quick queries are not blocked by compile operations

**Output log scraping for tool results:**
- Current capacity: `MCPTool_GetOutputLog` scrapes the in-memory UE output log buffer
- Limit: The UE output log buffer is bounded; very high-frequency logging can cause older entries to be evicted before they are retrieved
- Scaling path: Buffer relevant log lines in a plugin-owned ring buffer keyed to session ID

---

## Dependencies at Risk

**Tight coupling to UE 5.7 internal graph editor APIs:**
- Risk: `AnimationBlueprintUtils.cpp` and `AnimStateMachineEditor.cpp` use non-public UE node graph APIs (`UK2Node_*`, `UAnimGraphNode_*`) that Epic changes between minor versions
- Impact: Upgrading to UE 5.8+ may break anim Blueprint editing tools entirely
- Files: `Source/UnrealClaude/Private/AnimationBlueprintUtils.cpp`, `Source/UnrealClaude/Private/AnimStateMachineEditor.cpp`
- Migration plan: Audit against UE 5.8 API diff when available; abstract internal node manipulation behind a version-shim layer

**Engine version mismatch between `.uplugin` and README:**
- Risk: `.uplugin` declares `"EngineVersion": "5.7.0"` but documentation references 5.7.2/5.7.3 — unclear which patch version the plugin is actually validated against
- Files: `UnrealClaude/UnrealClaude.uplugin`
- Migration plan: Pin to a specific validated patch version and update both `.uplugin` and README consistently

---

## Test Coverage Gaps

**`AnimationBlueprintUtils` — entirely untested:**
- What's not tested: All Blueprint graph modification, state machine editing, node connection, and asset creation functions
- Files: `Source/UnrealClaude/Private/AnimationBlueprintUtils.cpp` (2400 lines)
- Risk: Silent regressions when UE internal graph APIs change; currently the largest untested surface area in the plugin
- Priority: High

**`ClaudeCodeRunner` — entirely untested:**
- What's not tested: Subprocess spawning, NDJSON stream parsing, JSON payload construction, process lifecycle (kill/restart)
- Files: `Source/UnrealClaude/Private/ClaudeCodeRunner.cpp`
- Risk: Stream parsing bugs could cause garbled responses or deadlocks with no automated detection
- Priority: High

**`ScriptExecutionManager` — entirely untested:**
- What's not tested: Script queuing, result dispatch, compile triggering, history tracking
- Files: `Source/UnrealClaude/Private/ScriptExecutionManager.cpp`
- Risk: The most user-facing execution path has zero automated coverage
- Priority: High

**`ProjectContext` — entirely untested:**
- What's not tested: Source file scanning, context refresh, lock behaviour
- Files: `Source/UnrealClaude/Private/ProjectContext.cpp`
- Risk: Context data provided to Claude could be stale or incorrect with no detection
- Priority: Medium

**`ClaudeSessionManager` — entirely untested:**
- What's not tested: Session creation, state transitions, timeout/cleanup
- Files: `Source/UnrealClaude/Private/ClaudeSessionManager.cpp`
- Risk: Session lifecycle bugs (leaks, premature termination) are undetectable
- Priority: Medium

**Threading behaviour in `MCPTaskQueue` — tests disabled:**
- What's not tested: Concurrent enqueue/dequeue, shutdown-under-load, task ordering guarantees
- Files: `Source/UnrealClaude/Private/Tests/MCPTaskQueueTests.cpp`
- Risk: The underlying game-thread deadlock that caused tests to be disabled may still exist in production
- Priority: High

---

## Inconsistencies

**Mixed error return patterns across MCP tools:**
- Some tools use `FMCPErrors::InvalidParam(...)`, others call `FMCPToolResult::Error(text)` directly, others use `ExtractRequiredString` which handles errors internally — no consistent pattern
- Files: All files under `Source/UnrealClaude/Private/MCP/Tools/`
- Impact: Error messages have inconsistent structure and verbosity; harder to maintain
- Fix: Standardise on `FMCPErrors::*` factory methods throughout, then plumb `EMCPErrorCode` into JSON responses

**`IScriptExecutor.h` interface appears unused:**
- Files: `Source/UnrealClaude/Private/Scripting/IScriptExecutor.h`
- Only one file exists in `Private/Scripting/` — unclear if this interface is implemented anywhere or is a planned abstraction
- Fix: Either implement and wire up the interface, or remove it

---

*Concerns audit: 2026-03-30*
