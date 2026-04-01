# Phase 3: SSE Parser & Server Lifecycle - Context

**Gathered:** 2026-04-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Build and thoroughly test the two most technically complex OpenCode components in isolation, before any integration with existing code: (1) FOpenCodeSSEParser — parses Server-Sent Events from OpenCode's `/event` endpoint into the existing FChatStreamEvent model, and (2) FOpenCodeServerLifecycle — auto-detects a running `opencode serve` instance, spawns one if none found, manages graceful shutdown, and tracks PID files for crash recovery. Also includes SSE reconnection with exponential backoff and a shared HTTP client utility for REST API calls.

</domain>

<decisions>
## Implementation Decisions

### SSE Event Mapping
- **D-01:** Extend EChatStreamEventType enum with new values for OpenCode-specific events that don't map to existing types. Minimum new types: PermissionRequest, StatusUpdate, AgentThinking. Researcher may identify additional types needed based on live OpenCode SSE output
- **D-02:** FOpenCodeSSEParser is a stateful parser with internal buffer — accumulates partial SSE chunks across ProcessChunk() calls, emits complete events via FOnChatStreamEvent delegate callback. Mirrors FClaudeCodeRunner's NdjsonLineBuffer pattern
- **D-03:** Malformed SSE data (bad JSON, missing fields, unknown event types) emitted as FChatStreamEvent with bIsError=true and raw data in RawJson field. Consumer decides display treatment
- **D-04:** Last-Event-ID tracking: agent's discretion after research — implement if OpenCode's SSE endpoint supports it, skip if it doesn't

### Server Lifecycle Behavior
- **D-05:** Detection strategy: PID file first, then health endpoint check. Read PID file → check if process alive → if alive, hit `GET /global/health` → if healthy, connect. If PID stale or no PID file, proceed to spawn
- **D-06:** PID file stored at `<Project>/Saved/UnrealClaude/opencode-server.pid` — consistent with existing config/session files, per-project isolation
- **D-07:** Spawn failure: fail fast + try alternate ports. If default port (4096) fails, try 4097, 4098, 4099 before reporting error. Handles port conflicts automatically
- **D-08:** Shutdown: only kill managed servers (spawned by the plugin, tracked via PID file). If user started `opencode serve` manually, leave it running on editor shutdown
- **D-09:** Crash recovery: stale PID cleanup + kill zombies + reconnect. On startup, if PID file exists but process is dead, clean up stale file and start fresh. If process alive but not responding to health checks, kill it and spawn fresh
- **D-10:** FOpenCodeServerLifecycle is a standalone class, owned by FOpenCodeRunner (Phase 4) as a member. Not a singleton. Clean separation of concerns, easy to test
- **D-11:** Periodic health polling every 10 seconds while connected. If health check fails, trigger reconnect flow. Catches server crashes between SSE events
- **D-12:** Lazy startup on first use — detect/spawn server when user first switches to OpenCode backend, not at editor startup. Faster editor launch, slight delay on first OpenCode use
- **D-13:** Port configuration: config JSON field takes priority, environment variable (OPENCODE_PORT) as fallback, UnrealClaudeConstants::Backend::DefaultOpenCodePort (4096) as default
- **D-14:** Graceful shutdown timeout: 10 seconds graceful termination signal, then force-kill if still running. Gives OpenCode time to save state

### Disconnect/Reconnect UX
- **D-15:** Input blocked during SSE disconnect — user sees reconnection status but cannot send messages until connection is restored. Prevents confusion about message delivery
- **D-16:** Exponential backoff: 1s → 2s → 4s → max 30s. Reset backoff to 1s on any success (SSE reconnect OR health check success). Ensures fast recovery after brief blips
- **D-17:** Infinite retries with backoff capped at 30s. Never stops trying. User can manually switch backends or close panel if they give up
- **D-18:** Mid-stream disconnect: show partial response with a '[disconnected]' marker. On reconnect, user can re-send the prompt. Partial content preserved for context

### SSE Transport Mechanism
- **D-19:** Try FHttpModule first for SSE connection. If streaming doesn't work (no callback-per-chunk support), fall back to raw FSocket + FRunnable. Researcher verifies which approach works with UE 5.7
- **D-20:** FHttpModule for all non-streaming REST calls (health checks, session creation, message sending). SSE is the only transport with uncertainty
- **D-21:** SSE read loop runs on dedicated FRunnable thread. Events parsed on background thread, dispatched to game thread via AsyncTask(ENamedThreads::GameThread). Matches FClaudeCodeRunner threading pattern
- **D-22:** Full FOpenCodeHttpClient wrapper with request/response logging, retry logic, and timeout handling. Shared across Phase 3 and Phase 4 components. Production-ready from the start
- **D-23:** One SSE connection per FOpenCodeRunner instance. Since Phase 2 established one backend at a time (D-15), only one SSE connection will ever be active

### Testing Strategy
- **D-24:** SSE parser test data from recorded captures of real OpenCode SSE output. Capture well-formed, malformed, partial, and multi-event batches from a live `opencode serve` instance
- **D-25:** Server lifecycle tests fully mocked — mock HTTP calls, mock process spawning, mock file I/O. Tests run fast with no OpenCode binary required
- **D-26:** Coverage target: >=90% method-level for FOpenCodeSSEParser, FOpenCodeServerLifecycle, FOpenCodeHttpClient. Document any methods that can't be tested with rationale
- **D-27:** Testability via interface injection — FOpenCodeServerLifecycle takes IHttpClient and IProcessLauncher in constructor. Tests inject mocks. Production injects real implementations

### Error Classification
- **D-28:** EOpenCodeErrorType enum with values: ServerUnreachable, AuthFailure, RateLimit, BadRequest, InternalError, Unknown. Stored in FChatStreamEvent alongside bIsError. Phase 5 uses this for targeted error UI (CHUI-04)
- **D-29:** Network-level errors (connection refused, timeout, DNS failure) surfaced as FChatStreamEvent with bIsError and the typed error enum. Same event model for transport and application errors — downstream handles display uniformly

### File Organization
- **D-30:** All OpenCode-specific files in `Private/OpenCode/` subdirectory. Matches `Private/MCP/` pattern for grouped components
- **D-31:** Minimal public surface — only IChatBackend.h, ChatBackendTypes.h, ChatBackendFactory.h are public. All OpenCode internals (SSEParser, ServerLifecycle, HttpClient) stay in Private/OpenCode/

### Server Spawn Arguments
- **D-32:** Exact command-line flags for `opencode serve`: agent's discretion after research. Researcher determines available flags from OpenCode CLI help
- **D-33:** Working directory for spawned server: UE project root (FPaths::ProjectDir()). Consistent with how Claude CLI is invoked

### Connection State Machine
- **D-34:** Six-state enum: Disconnected, Detecting, Spawning, Connecting, Connected, Reconnecting. Exposed via delegate that fires on state changes. Gives Phase 5's health indicator (CHUI-07) granular status info
- **D-35:** FOpenCodeServerLifecycle owns the connection state machine. State changes fire through its delegate. FOpenCodeRunner (Phase 4) subscribes to it

### API Contract
- **D-36:** Research OpenCode GitHub repo and official docs first, validate key endpoints against live instance if available. Build against documented behavior, not implementation details
- **D-37:** Target latest stable OpenCode release at research time. Document exact version. Breaking changes handled in a patch phase if needed

### Agent's Discretion
- Last-Event-ID tracking (D-04) — depends on OpenCode SSE support
- Exact command-line flags for `opencode serve` (D-32) — researcher determines
- Additional EChatStreamEventType values beyond PermissionRequest, StatusUpdate, AgentThinking — researcher may find more needed
- Internal organization of FOpenCodeHttpClient (retry counts, timeout values, logging format)
- Exact health polling interval (10s is starting point, may adjust based on testing)
- How to handle the SSE `retry:` field from the server (adjust reconnect timing if present)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Core interface (Phase 2 output)
- `UnrealClaude/Source/UnrealClaude/Public/IChatBackend.h` — 9-method interface that FOpenCodeRunner (Phase 4) will implement. Phase 3 components support this interface
- `UnrealClaude/Source/UnrealClaude/Public/ChatBackendTypes.h` — EChatStreamEventType enum to extend, FChatStreamEvent flat struct, FChatRequestConfig base, all delegates
- `UnrealClaude/Source/UnrealClaude/Private/ChatBackendFactory.h` — Factory with GetOpenCodePath() and OpenCode stub to wire in Phase 4

### Subprocess management reference
- `UnrealClaude/Source/UnrealClaude/Public/ClaudeCodeRunner.h` — FRunnable + FProcHandle + pipe pattern. Reference for threading, process spawning, NDJSON buffering, game thread dispatch
- `UnrealClaude/Source/UnrealClaude/Private/ClaudeCodeRunner.cpp` — GetClaudePath() binary search pattern. Reference for OpenCode binary detection and process launch

### Constants
- `UnrealClaude/Source/UnrealClaude/Public/UnrealClaudeConstants.h` — Backend::DefaultOpenCodePort (4096), Process namespace for buffer sizes/timeouts

### Testing infrastructure
- `UnrealClaude/Source/UnrealClaude/Private/Tests/TestUtils.h` — FMockChatBackend, test helpers, JSON utilities. Extend with IHttpClient/IProcessLauncher mocks
- `.planning/codebase/TESTING.md` — Test framework, runner, organization, known limitations

### Codebase analysis
- `.planning/codebase/ARCHITECTURE.md` — Component map, data flow, entry points
- `.planning/codebase/STRUCTURE.md` — File locations, naming conventions
- `.planning/codebase/CONVENTIONS.md` — Code patterns, file size limits, documentation requirements

### Phase requirements
- `.planning/REQUIREMENTS.md` — COMM-02, COMM-03, SRVR-01 through SRVR-05, TEST-02, TEST-03

### Prior phase decisions
- `.planning/phases/02-backend-abstraction/02-CONTEXT.md` — All rename decisions, interface design, backend swap mechanics

### OpenCode API (researcher must validate)
- OpenCode GitHub repository — primary API documentation source
- `opencode serve --help` — available command-line flags for server spawn
- `GET /global/health` endpoint — health check contract
- `GET /event` endpoint — SSE event stream contract and event types

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **FClaudeCodeRunner threading pattern** (`Public/ClaudeCodeRunner.h`): FRunnable + FRunnableThread + game thread dispatch via AsyncTask. Directly applicable to SSE read loop
- **FClaudeCodeRunner::NdjsonLineBuffer** (`Public/ClaudeCodeRunner.h:71`): Incomplete line buffering pattern. SSE parser uses identical approach for partial chunk accumulation
- **FClaudeCodeRunner::GetClaudePath()** (`Public/ClaudeCodeRunner.h:48`): Platform-specific binary search. Reference for OpenCode binary detection
- **FChatBackendFactory::GetOpenCodePath()** (`Private/ChatBackendFactory.h:28`): Already implemented OpenCode binary search on PATH
- **FMCPTaskQueue** (`Private/MCP/MCPTaskQueue.h`): Another FRunnable example with thread-safe task queue pattern
- **UnrealClaudeConstants::Backend::DefaultOpenCodePort** (`Public/UnrealClaudeConstants.h:241`): Port constant already defined

### Established Patterns
- **FRunnable for background I/O**: Both FClaudeCodeRunner and FMCPTaskQueue use this. SSE read loop follows the same pattern
- **Game thread dispatch**: `AsyncTask(ENamedThreads::GameThread, [=]() { ... })` for callbacks from background threads
- **FProcHandle for subprocess management**: FClaudeCodeRunner manages Claude CLI lifecycle. Server lifecycle manager uses the same UE process API
- **IMPLEMENT_SIMPLE_AUTOMATION_TEST**: All tests use this macro with EditorContext|ProductFilter flags
- **Private/MCP/ subdirectory grouping**: Precedent for grouping related files. Private/OpenCode/ follows this pattern

### Integration Points
- **ChatBackendTypes.h**: Where EChatStreamEventType and EOpenCodeErrorType enums will be added
- **UnrealClaudeConstants.h**: Where new OpenCode constants (health poll interval, shutdown timeout, backoff values) will be added
- **Private/Tests/**: Where SSEParserTests.cpp, ServerLifecycleTests.cpp, HttpClientTests.cpp will live
- **TestUtils.h**: Where IHttpClient and IProcessLauncher mock interfaces will be added
- **Build.cs**: May need no new dependencies — HTTP, HTTPServer, Sockets, Networking already included

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

*Phase: 03-sse-parser-server-lifecycle*
*Context gathered: 2026-04-01*
