# Phase 4: OpenCode Runner & Sessions - Context

**Gathered:** 2026-04-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Compose Phase 3 components (FOpenCodeSSEParser, FOpenCodeServerLifecycle, FOpenCodeHttpClient) into a working FOpenCodeRunner that implements IChatBackend. Add session management via OpenCode's REST API, dynamic MCP bridge registration, and permission prompt handling. This phase delivers a fully functional OpenCode backend ready for UI wiring in Phase 5 — it must be testable end-to-end without any UI changes.

</domain>

<decisions>
## Implementation Decisions

### Session Lifecycle
- **D-01:** Local-primary session model — FChatSessionManager remains the single display/persistence layer for the chat panel UI. OpenCode server sessions are an implementation detail of FOpenCodeRunner, not exposed to the subsystem or UI
- **D-02:** One OpenCode session per editor session — FOpenCodeRunner creates an OpenCode session (POST /session) during initialization (after server connection succeeds) and reuses it for all messages within that editor session
- **D-03:** Session ID stored as a member of FOpenCodeRunner. If the session becomes invalid (server restart, error), create a new one transparently on the next prompt
- **D-04:** Conversation history passed to OpenCode via FormatPrompt() — the local FChatSessionManager history is formatted into the prompt, same pattern as FClaudeCodeRunner. OpenCode's server-side session also accumulates context, providing double-coverage

### Permission Handling
- **D-05:** Modal dialog for permission prompts — when FOpenCodeRunner receives a `permission.updated` SSE event, it shows a modal dialog (similar to FScriptExecutionManager's existing pattern) with the permission description, and sends approval/denial back to OpenCode via REST
- **D-06:** Event buffering during permission dialog — SSE stream stays open, FOpenCodeRunner buffers incoming events while the modal is shown. Once the user responds, buffered events are processed in order. No data loss, no stream interruption
- **D-07:** Permission response endpoint: use the appropriate OpenCode REST endpoint to send approval/denial (researcher to determine exact API)

### MCP Registration
- **D-08:** Register MCP bridge immediately after server connection succeeds (health check passes), before creating the OpenCode session. Ensures all 30+ editor tools are available from the first prompt
- **D-09:** Re-register MCP bridge on every reconnect (Reconnecting -> Connected state transition). Covers server restarts that may lose registration state
- **D-10:** Graceful degradation on MCP registration failure — log warning, continue with chat-only mode (no editor tools). User sees a log message. Retry happens automatically on next reconnect
- **D-11:** MCP bridge URL is `http://localhost:3000` (the existing MCP server port). Construct from UnrealClaudeConstants::MCPServer::DefaultPort

### Agent & Model Configuration
- **D-12:** Hardcoded defaults in Phase 4 — use sensible defaults for agent type and model (e.g., default agent, default model from OpenCode config). No user-facing settings. FOpenCodeRequestConfig stores the values but they're set to defaults
- **D-13:** FOpenCodeRequestConfig : FChatRequestConfig — derived config class with OpenCode-specific fields (AgentType, ModelId, SessionId). Matches FClaudeRequestConfig pattern from Phase 2 (D-11/D-12)
- **D-14:** FOpenCodeRunner::CreateConfig() returns TUniquePtr<FChatRequestConfig> pointing to FOpenCodeRequestConfig. Subsystem populates common fields, runner populates OpenCode-specific fields

### Component Composition
- **D-15:** FOpenCodeRunner owns Phase 3 components as members: TUniquePtr<FOpenCodeServerLifecycle>, FOpenCodeSSEParser (value member), TSharedPtr<FOpenCodeHttpClient> (shared with lifecycle manager)
- **D-16:** FOpenCodeRunner subscribes to FOpenCodeServerLifecycle's connection state delegate. On Connected: register MCP, create session, start SSE stream. On Reconnecting: buffer state. On Disconnected: stop SSE, cleanup
- **D-17:** FOpenCodeRunner::IsAvailable() delegates to FOpenCodeServerLifecycle connection state (Connected = available, anything else = unavailable)
- **D-18:** FOpenCodeRunner::ExecuteAsync() sends prompt via POST /session/:id/message (or prompt_async), SSE events routed through FOpenCodeSSEParser, parsed events dispatched to game thread via AsyncTask

### Factory Integration
- **D-19:** Replace the stub in ChatBackendFactory.cpp (case EChatBackendType::OpenCode) with MakeUnique<FOpenCodeRunner>(). This is the single integration point that activates the entire OpenCode pipeline

### Testing
- **D-20:** >= 90% unit test coverage for FOpenCodeRunner. Mock all dependencies (IHttpClient, IProcessLauncher, server lifecycle) via constructor injection or member injection
- **D-21:** Test session creation, prompt sending, SSE event routing, permission dialog flow, MCP registration, error handling, cancel flow
- **D-22:** Integration-style tests with FMockHttpClient simulating OpenCode API responses. No real OpenCode binary needed for tests

### Agent's Discretion
- Exact OpenCode REST API endpoint names (researcher validates against live docs)
- Internal state management within FOpenCodeRunner (how to track in-flight requests)
- Event buffering implementation details during permission prompts
- Exact permission dialog widget reuse vs. new dialog
- Thread synchronization between SSE events and game thread permission dialog
- How to handle OpenCode session expiration/invalidation edge cases

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase 3 output (components to compose)
- `UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeSSEParser.h` — Stateful SSE chunk-to-event parser. FOpenCodeRunner feeds HTTP chunks into this
- `UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeSSEParser.cpp` — Parser implementation (272 lines, 10 event type mappings)
- `UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeServerLifecycle.h` — Six-state server lifecycle manager. FOpenCodeRunner owns this as member
- `UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeServerLifecycle.cpp` — Lifecycle implementation (288 lines)
- `UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeHttpClient.h` — IHttpClient interface + production FOpenCodeHttpClient. Shared between lifecycle and runner
- `UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeHttpClient.cpp` — HTTP client implementation (193 lines)
- `UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeTypes.h` — EOpenCodeConnectionState, FOnConnectionStateChanged delegate
- `UnrealClaude/Source/UnrealClaude/Private/OpenCode/IProcessLauncher.h` — Process abstraction interface

### Phase 2 output (interface to implement)
- `UnrealClaude/Source/UnrealClaude/Public/IChatBackend.h` — 9-method interface FOpenCodeRunner must implement
- `UnrealClaude/Source/UnrealClaude/Public/ChatBackendTypes.h` — EChatStreamEventType, FChatStreamEvent, FChatRequestConfig, all delegates
- `UnrealClaude/Source/UnrealClaude/Private/ChatBackendFactory.h` — Factory with OpenCode stub to replace
- `UnrealClaude/Source/UnrealClaude/Private/ChatBackendFactory.cpp` — Line 18: case EChatBackendType::OpenCode stub

### Reference implementation (Claude backend pattern)
- `UnrealClaude/Source/UnrealClaude/Public/ClaudeCodeRunner.h` — FClaudeCodeRunner : IChatBackend. Reference for ExecuteAsync/Cancel/IsExecuting patterns
- `UnrealClaude/Source/UnrealClaude/Private/ClaudeCodeRunner.cpp` — Subprocess spawning, NDJSON parsing, game thread dispatch. Reference for threading pattern

### Subsystem (consumer of IChatBackend)
- `UnrealClaude/Source/UnrealClaude/Public/ChatSubsystem.h` — FChatSubsystem that owns the backend, calls ExecuteAsync/FormatPrompt/CreateConfig
- `UnrealClaude/Source/UnrealClaude/Private/ChatSubsystem.cpp` — BuildPromptWithHistory(), backend swap logic

### Session persistence
- `UnrealClaude/Source/UnrealClaude/Public/ChatSessionManager.h` — FChatSessionManager for local history. FOpenCodeRunner writes to this after each exchange

### Testing infrastructure
- `UnrealClaude/Source/UnrealClaude/Private/Tests/TestUtils.h` — FMockChatBackend, FMockHttpClient, FMockProcessLauncher, test helpers
- `UnrealClaude/Source/UnrealClaude/Private/Tests/OpenCodeSSEParserTests.cpp` — 22 parser tests. Reference for OpenCode test patterns
- `UnrealClaude/Source/UnrealClaude/Private/Tests/OpenCodeServerLifecycleTests.cpp` — 19 lifecycle tests. Reference for mock injection
- `UnrealClaude/Source/UnrealClaude/Private/Tests/OpenCodeHttpClientTests.cpp` — 9 HTTP client tests

### Constants
- `UnrealClaude/Source/UnrealClaude/Public/UnrealClaudeConstants.h` — Backend namespace with OpenCode port, health poll, shutdown timeout, backoff values

### MCP server (to register with OpenCode)
- `UnrealClaude/Source/UnrealClaude/Private/MCP/UnrealClaudeMCPServer.h` — HTTP server on port 3000 that MCP bridge connects to

### Phase requirements
- `.planning/REQUIREMENTS.md` — COMM-01, COMM-04, COMM-05, COMM-06, MCPI-01, TEST-01

### Prior phase decisions
- `.planning/phases/02-backend-abstraction/02-CONTEXT.md` — Interface design, backend swap mechanics, request config hierarchy
- `.planning/phases/03-sse-parser-server-lifecycle/03-CONTEXT.md` — Component design, testing strategy, connection state machine

### OpenCode API (researcher must validate)
- OpenCode GitHub repository — primary API documentation source
- `opencode serve --help` — available flags and endpoints
- OpenCode REST API endpoints: /session, /session/:id/message, /session/:id/prompt_async, /mcp, /event

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **FClaudeCodeRunner** (`Public/ClaudeCodeRunner.h`): Direct reference implementation for IChatBackend. FOpenCodeRunner follows the same ExecuteAsync/Cancel/IsExecuting pattern but uses HTTP instead of subprocess
- **FOpenCodeSSEParser** (`Private/OpenCode/OpenCodeSSEParser.h`): Ready to use — ProcessChunk() takes raw SSE data, emits FChatStreamEvent via delegate
- **FOpenCodeServerLifecycle** (`Private/OpenCode/OpenCodeServerLifecycle.h`): Ready to use — EnsureServerReady(), Shutdown(), connection state machine
- **FOpenCodeHttpClient** (`Private/OpenCode/OpenCodeHttpClient.h`): Ready to use — Get(), Post(), BeginSSEStream(), CheckHealth()
- **FMockHttpClient / FMockProcessLauncher** (`Private/Tests/TestUtils.h`): Test doubles for dependency injection
- **FChatBackendFactory::GetOpenCodePath()** (`Private/ChatBackendFactory.h`): Already searches PATH for opencode binary

### Established Patterns
- **IChatBackend implementation**: FClaudeCodeRunner shows the exact method signatures, threading model, and callback dispatch pattern. FOpenCodeRunner mirrors this
- **FRunnable for background I/O**: SSE read loop uses FRunnable + AsyncTask(GameThread) dispatch, same as FClaudeCodeRunner's NDJSON reader
- **TUniquePtr ownership**: Subsystem owns backend via TUniquePtr. Runner owns lifecycle manager via TUniquePtr
- **Constructor injection for testability**: FOpenCodeServerLifecycle takes IHttpClient + IProcessLauncher. FOpenCodeRunner should follow this pattern
- **FOpenCodeRequestConfig : FChatRequestConfig**: Matches FClaudeRequestConfig derivation pattern from Phase 2

### Integration Points
- **ChatBackendFactory.cpp line 18**: case EChatBackendType::OpenCode — replace stub with MakeUnique<FOpenCodeRunner>()
- **FChatSubsystem::SetActiveBackend()**: Calls factory, swaps TUniquePtr<IChatBackend>. No changes needed — just wire the factory
- **Private/OpenCode/**: All new files go here alongside existing Phase 3 files
- **Private/Tests/**: OpenCodeRunnerTests.cpp goes here alongside existing Phase 3 test files

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

*Phase: 04-opencode-runner-sessions*
*Context gathered: 2026-04-01*
