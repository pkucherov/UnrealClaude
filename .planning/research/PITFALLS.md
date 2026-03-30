# Domain Pitfalls

**Domain:** HTTP/SSE AI backend integration into UE5.7 editor plugin
**Researched:** 2026-03-30
**Confidence:** HIGH (based on UE5 HTTP module source analysis, OpenCode server source, and existing codebase patterns)

---

## Critical Pitfalls

Mistakes that cause rewrites, editor crashes, or major architectural rework.

### Pitfall 1: UE HTTP Module Silently Kills Long-Lived SSE Connections

**What goes wrong:** The UE `IHttpRequest` has a default timeout (configurable via `FHttpModule::SetHttpTimeout()` and per-request via `SetTimeout()`). An SSE connection to OpenCode's `/event` endpoint is meant to stay open indefinitely. If you create a standard `FHttpModule::Get().CreateRequest()` without explicitly clearing the timeout, UE will silently terminate the connection after ~30-60 seconds (the default varies by platform). The `OnProcessRequestComplete` fires with `Failed` status, and your SSE stream dies without any obvious error.

**Why it happens:** SSE is fundamentally different from request-response HTTP. UE's HTTP module was designed for request-response patterns. The `SetTimeout()` applies to the entire request lifecycle, not just the initial connection. There is no built-in SSE support in UE's HTTP module.

**Consequences:** Intermittent loss of all real-time events from OpenCode. The UI appears frozen — no streaming text, no tool call updates. If no reconnection logic exists, the session becomes permanently deaf to events.

**Prevention:**
- Call `Request->ClearTimeout()` or `Request->SetTimeout(0.0f)` on the SSE request before calling `ProcessRequest()`
- Alternatively, bypass `FHttpModule` entirely for the SSE connection and use a raw `FRunnable` thread with platform socket APIs or libcurl directly (similar to how `FClaudeCodeRunner` uses `FPlatformProcess` for subprocess I/O)
- Implement connection health monitoring: if no SSE event (including heartbeats) arrives within 15 seconds, consider the connection dead and reconnect

**Detection:** SSE events stop arriving but REST API calls still work. No error in UE log unless you log `OnProcessRequestComplete` failures. OpenCode server logs show the connection was cleanly closed.

**Phase:** Must be solved in the SSE client implementation phase. Validate with a 5-minute idle test before moving on.

---

### Pitfall 2: SSE Response Body Streaming Callback Fires on Non-Game Thread

**What goes wrong:** UE's `SetResponseBodyReceiveStreamDelegate` (the mechanism for reading a streaming HTTP response incrementally) explicitly documents that the delegate "will be called from another thread other than the game thread." If you parse SSE frames in this delegate and directly update Slate UI widgets, you get undefined behavior — most commonly a crash in `FSlateApplication::Tick()` or corrupted widget state.

**Why it happens:** UE's HTTP layer uses background threads (WinHTTP on Windows, libcurl on Linux/Mac). The streaming delegate fires from these threads. All Slate widget modification must happen on the game thread. The existing `FClaudeCodeRunner` already handles this correctly with `AsyncTask(ENamedThreads::GameThread, ...)`, but a new HTTP-based implementation could easily miss this because `FHttpModule` callbacks have a `SetDelegateThreadPolicy(CompleteOnGameThread)` option for the *completion* delegate but NOT for the streaming body delegate.

**Consequences:** Intermittent crashes during streaming responses. Race conditions that corrupt the `StreamingTextBlock`, `ToolCallStatusLabels`, or other Slate state in `SClaudeEditorWidget`. Crashes may appear unrelated (in Slate rendering code, not in your HTTP callback).

**Prevention:**
- Parse SSE frames in the streaming delegate (background thread) into `FClaudeStreamEvent` structs
- Marshal every UI-affecting event to the game thread via `AsyncTask(ENamedThreads::GameThread, [Event]() { ... })`
- Follow the exact pattern already established in `ClaudeCodeRunner.cpp` lines 689, 738, 751, 787, 875, 912 — the codebase already has a proven pattern for this
- Never capture raw `TSharedPtr<SWidget>` in background-thread lambdas; capture only value types or thread-safe shared pointers

**Detection:** Crash dumps pointing to `FSlateApplication` or `SWidget` destructors. Intermittent, hard to reproduce — only happens when SSE event arrives between game-thread ticks.

**Phase:** Must be addressed in the SSE parser design phase. Non-negotiable architectural constraint.

---

### Pitfall 3: Zombie OpenCode Server Processes After Editor Crash or Force-Close

**What goes wrong:** When the plugin spawns `opencode serve` via `FPlatformProcess::CreateProc()`, the child process outlives the editor if UE crashes, is force-killed via Task Manager, or the user closes the editor without the module's `ShutdownModule()` running. Zombie `opencode` processes accumulate, each holding port 4096 (or whatever port was assigned). Next editor launch fails to spawn a new server because the port is occupied, but the old server has no active sessions and stale MCP registrations.

**Why it happens:** `FUnrealClaudeModule::ShutdownModule()` is the designated cleanup point, but it doesn't run on crash or force-kill. Unlike subprocess stdout piping (where the pipe breaks and the child process gets SIGPIPE), an HTTP server process has no dependency on the parent — it runs independently.

**Consequences:** Port conflicts on next startup. Multiple `opencode` processes consuming memory. Stale MCP registrations pointing to a dead MCP bridge. Users must manually kill processes via Task Manager / `kill`.

**Prevention:**
- **Prefer "connect to existing" over "spawn new":** Always try `GET http://localhost:4096/` first. If a server is already running, reuse it. Only spawn if no server responds. This makes zombie servers *useful* rather than problematic.
- **On spawn, record the PID** in a file (e.g., `Saved/UnrealClaude/opencode_server.pid`). On startup, check if that PID is still alive and owns port 4096 before spawning a new one.
- **Use process groups on Windows** (`CREATE_NEW_PROCESS_GROUP` flag is NOT available via `FPlatformProcess::CreateProc` — you'd need to use `FPlatformProcess::CreateProc` with `bLaunchDetached=false` and carefully manage the handle) or **use Job Objects on Windows** to tie child process lifetime to the editor process. On Linux/Mac, set `PR_SET_PDEATHSIG` or use a parent-death pipe.
- **Graceful shutdown via API:** Call `POST /instance/dispose` on the OpenCode server during `ShutdownModule()` before terminating the process. This lets OpenCode clean up its own state.
- **Register an `FCoreDelegates::OnPreExit` callback** (fires before `ShutdownModule`) and a `FCoreDelegates::OnEnginePreExit` callback as belts-and-suspenders shutdown hooks.

**Detection:** Multiple `opencode` processes visible in Task Manager. Port 4096 already in use errors. `FPlatformProcess::IsProcRunning()` returns true for a PID you didn't spawn.

**Phase:** Must be solved in the process lifecycle management phase. Test by force-killing the editor mid-session.

---

### Pitfall 4: IClaudeRunner Interface Assumes Subprocess Semantics That Don't Map to HTTP

**What goes wrong:** The existing `IClaudeRunner` interface was designed around subprocess execution. Key assumptions baked into the interface:
1. `ExecuteAsync()` takes a `FClaudeRequestConfig` with `WorkingDirectory`, `AllowedTools`, `bSkipPermissions` — these are Claude CLI flags that have no HTTP equivalent
2. `ExecuteSync()` blocks until completion — trivial with a subprocess (wait on pipe), but with HTTP+SSE you must poll or spin-wait on an event, which risks game-thread deadlock
3. `Cancel()` maps to `FPlatformProcess::TerminateProc()` — for HTTP, cancellation requires `POST /:sessionID/abort` which is async and may not immediately stop the LLM
4. `IsAvailable()` checks if a CLI binary exists on disk — for HTTP, availability means "can I connect to the server," which is inherently async and may have transient failures
5. The `FOnClaudeStreamEvent` delegate system assumes events arrive via NDJSON lines — SSE events have a different framing (`data:` prefix, double-newline terminator, optional `event:` and `id:` fields)

**Why it happens:** The interface was a perfect abstraction for "run CLI as subprocess." HTTP is a fundamentally different communication model. Trying to force-fit HTTP into the subprocess interface creates leaky abstractions and edge-case bugs.

**Consequences:** If the interface isn't generalized, the `FOpenCodeRunner` implementation will have dead code paths, unused parameters, and semantic mismatches that confuse future maintainers. `ExecuteSync` becomes a footgun that can deadlock the editor. `Cancel()` appears to work but the OpenCode LLM continues generating for several seconds after.

**Prevention:**
- **Refactor `IClaudeRunner` into a backend-agnostic `IChatBackend` interface** before implementing the HTTP backend. Remove subprocess-specific concerns:
  - Replace `FClaudeRequestConfig` with a `FChatRequestConfig` that contains only backend-agnostic fields (prompt, system prompt, images, timeout)
  - Remove `ExecuteSync()` or make it optional — HTTP backends should not support synchronous execution
  - Make `Cancel()` async — return immediately but accept that cancellation is eventually consistent
  - Make `IsAvailable()` return a cached result updated by a background health check, not a synchronous check
- **Keep `FClaudeCodeRunner`'s subprocess-specific config** in a subclass or config adapter, not in the shared interface
- **Define `IChatStreamEvent`** as a backend-agnostic event model that both NDJSON and SSE parsers emit

**Detection:** Code review reveals parameters being ignored or hardcoded in the HTTP implementation. `ExecuteSync` is called from game thread and hangs.

**Phase:** Must be addressed as the FIRST phase — interface refactoring before any HTTP implementation begins. This is the architectural foundation.

---

### Pitfall 5: SSE Frame Parsing Across Chunked TCP Boundaries

**What goes wrong:** SSE events are delimited by double-newline (`\n\n`). HTTP chunked transfer encoding can split an SSE event across multiple TCP reads. The streaming body delegate fires per chunk, not per SSE event. A naive parser that splits on `\n\n` per callback invocation will:
1. Miss events that span two chunks
2. Parse half-events and produce corrupt JSON
3. Silently drop the tail of events that arrive at chunk boundaries

This is the SSE equivalent of the NDJSON line-buffer problem that `FClaudeCodeRunner` already solves with `NdjsonLineBuffer`.

**Why it happens:** TCP is a stream protocol. HTTP chunked encoding adds its own framing. SSE adds another layer of framing on top. The streaming delegate gives you raw bytes with no SSE-level framing guarantees.

**Consequences:** Intermittent corrupt events, missing events, or garbled JSON. Symptoms are sporadic and depend on network timing and payload sizes. Extremely hard to reproduce in testing because local connections rarely split small payloads across chunks.

**Prevention:**
- Implement an `FSSEParser` class with an internal `FString Buffer` that accumulates data across delegate calls
- On each delegate invocation, append to buffer, then scan for complete SSE events (terminated by `\n\n`)
- Extract complete events, leave any trailing partial data in the buffer
- Handle all SSE fields: `data:`, `event:`, `id:`, `retry:`, and comments (lines starting with `:`)
- Mirror the existing `NdjsonLineBuffer` pattern in `ClaudeCodeRunner.cpp` — the codebase already solved the analogous problem for NDJSON
- Unit test the parser with artificially split inputs: mid-field splits, mid-JSON splits, multiple events in one chunk, empty events

**Detection:** JSON parse errors in the stream event handler. Events that should have `data` field but it's empty or truncated. Missing events (event count doesn't match server-side count).

**Phase:** SSE parser implementation phase. Must be unit-tested in isolation before integration.

---

### Pitfall 6: OpenCode Event Bus Publishes ALL Events, Not Just Your Session's

**What goes wrong:** OpenCode's `/event` SSE endpoint subscribes to `Bus.subscribeAll()` — it publishes every event from every session, every MCP server, every permission request, instance lifecycle, etc. If the plugin doesn't filter events by session ID, it will:
1. Display text from other sessions running on the same server
2. Show tool calls from unrelated operations
3. React to permission requests that aren't for this plugin's session

**Why it happens:** OpenCode's architecture is designed for a single TUI or web client that manages all sessions. The event bus is intentionally global — the client is expected to filter. The Unreal plugin is a single-session client connecting to a multi-session server.

**Consequences:** Chat panel shows garbled content from multiple sessions. Tool call indicators appear for tools the user didn't invoke. Permission dialogs pop up for unrelated requests. If the user runs OpenCode's TUI alongside the editor, events cross-contaminate.

**Prevention:**
- After creating a session via `POST /session`, store the `sessionID`
- On every SSE event, parse the `properties.sessionID` field and discard events not matching the active session
- Also filter by event `type` — only process `session.*`, `message.*`, and `part.*` events that are relevant to the chat UI
- Ignore `server.heartbeat`, `server.connected` (use for connection health only), and events for other domains like `config.*`, `lsp.*`, `permission.*` (unless building permission UI)
- Consider that `prompt_async` fires-and-forgets the prompt — the *only* way to get streaming results is via the filtered event stream

**Detection:** Chat panel shows text the user didn't request. Event log shows events with wrong session IDs. Running OpenCode TUI alongside the plugin causes UI corruption.

**Phase:** SSE event handling phase. Implement filtering before any UI integration.

---

## Moderate Pitfalls

### Pitfall 7: MCP Bridge Registration Race on Server Startup

**What goes wrong:** The plugin spawns `opencode serve`, then immediately calls `POST /mcp` to register the MCP bridge. If the server hasn't finished initializing (Bun HTTP server bind is async), the registration request fails with `ECONNREFUSED`. The plugin falls back to "no MCP tools" mode and the user gets a degraded experience with no indication of why.

**Prevention:**
- After spawning `opencode serve`, poll `GET /` or `GET /path` with exponential backoff (100ms, 200ms, 400ms, ...) up to a maximum of 5 seconds
- Only after a successful health check, register MCP via `POST /mcp`
- If registration fails after the health check succeeds, retry 2-3 times before reporting an error to the user
- Log the MCP registration result (success/failure + tool count) to `LogUnrealClaude`
- Store MCP registration state and re-register if the SSE connection drops and reconnects (server may have restarted)

**Detection:** OpenCode responses don't use any editor tools. `GET /mcp` returns empty tool list. Log shows `ECONNREFUSED` during startup.

**Phase:** Process lifecycle / server connection phase.

---

### Pitfall 8: FClaudeCodeSubsystem Singleton Owns a Concrete FClaudeCodeRunner

**What goes wrong:** `FClaudeCodeSubsystem` currently has `TUniquePtr<FClaudeCodeRunner> Runner` — a concrete type, not the `IClaudeRunner` interface. To support backend switching at runtime, the subsystem must hold a pointer to the interface, not the concrete class. But simply changing the pointer type isn't enough: the subsystem also calls `FClaudeCodeSubsystem::BuildPromptWithHistory()` which prepends conversation history — this must work correctly regardless of which backend is active.

**Prevention:**
- Change `TUniquePtr<FClaudeCodeRunner> Runner` to `TUniquePtr<IClaudeRunner> ActiveRunner` (or the refactored `IChatBackend`)
- Add a `TMap<EBackendType, TUniquePtr<IChatBackend>> Backends` to hold both backends simultaneously
- Backend switching swaps the `ActiveRunner` pointer but does NOT destroy the old backend — it may have pending requests
- History from the session manager must be backend-agnostic (no Claude-specific formatting in stored history)
- When switching backends mid-conversation, the new backend doesn't have the old backend's conversation context server-side (OpenCode has its own session state). Decide whether to: (a) start a fresh OpenCode session with injected history, or (b) maintain separate server-side sessions and only unify the UI timeline

**Detection:** Compile errors when trying to assign `FOpenCodeRunner*` to `FClaudeCodeRunner*`. Runtime crash if switching backends destroys a runner with an active request.

**Phase:** Interface refactoring phase (same as Pitfall 4).

---

### Pitfall 9: Platform-Specific Process Spawning Differences

**What goes wrong:** `FPlatformProcess::CreateProc()` behaves differently across Win64, Linux, and macOS:
- **Windows:** `opencode.exe` may not be on `PATH` if installed via a package manager that modifies user PATH but UE was launched from a shortcut that doesn't inherit the updated environment
- **Linux:** `opencode` may be a shell script wrapper; `CreateProc` with `bLaunchHidden=true` may not work correctly for scripts
- **macOS (Apple Silicon):** ARM vs x86 Rosetta: if `opencode` is an x86 binary and UE is ARM (or vice versa), the process launches fine but may have different behavior or performance characteristics
- **All platforms:** `FPlatformProcess::CreateProc` doesn't support passing environment variables on all platforms consistently

**Prevention:**
- Use the same `GetClaudePath()`-style path resolution for `opencode`: check known install locations first, then fall back to PATH lookup
- On Windows, check `%USERPROFILE%\.local\bin\opencode.exe` (if opencode follows similar install patterns), npm global, and common package manager locations
- Test process spawning on all three platforms in CI, not just locally
- When spawning, use `FPlatformProcess::CreateProc` with `bLaunchDetached=false, bLaunchHidden=true` to maintain a process handle for lifecycle management
- Verify the spawned process is actually listening by checking the port, not just checking `IsProcRunning()`

**Detection:** Plugin works on developer's Windows machine but fails on CI Linux or Mac builds. `IsAvailable()` returns false despite `opencode` being installed.

**Phase:** Process lifecycle phase. Requires platform-specific testing.

---

### Pitfall 10: HTTP Request Concurrency Limits in UE's FHttpModule

**What goes wrong:** UE's `FHttpModule` has a configurable maximum concurrent request limit (default varies, typically 6-16 per domain). If the plugin makes many simultaneous REST calls to OpenCode (session create + message send + MCP status + event stream), it may hit the concurrent connection limit. Requests queue silently inside the HTTP manager, adding unexpected latency. The SSE connection counts as one persistent connection, permanently consuming a slot.

**Prevention:**
- Minimize concurrent requests: serialize session setup calls (create session → register MCP → start SSE → send first message)
- The SSE connection uses one permanent slot — account for this in connection budgeting
- Avoid polling patterns; use the SSE event stream for state updates instead of repeated REST calls
- If UE's HTTP module proves too restrictive for SSE (due to timeout issues or connection limits), implement the SSE connection using a raw `FRunnable` + platform sockets, separate from `FHttpModule`, while using `FHttpModule` for normal REST calls

**Detection:** REST calls to OpenCode take unexpectedly long. HTTP manager debug logging shows queued requests.

**Phase:** HTTP client implementation phase.

---

### Pitfall 11: Unified Session History With Two Different State Models

**What goes wrong:** Claude CLI subprocess has no server-side session state — all history is managed client-side in `FClaudeSessionManager`. OpenCode has its own server-side session state (`/session/:id/message`). If the unified history model tries to query OpenCode's server for history AND maintain local history, you get:
1. Duplicate messages (one from local store, one from server)
2. Ordering conflicts when messages from both backends are interleaved
3. Stale data if the server-side session is modified externally (e.g., via OpenCode TUI)

**Prevention:**
- Define the plugin's `FClaudeSessionManager` as the **single source of truth for UI display**
- For Claude CLI: continue current pattern (local-only history)
- For OpenCode: after each message exchange completes, store the result in the local session manager, NOT by querying the server
- Don't try to sync OpenCode's server-side message history back to the local store — treat OpenCode as a "stateless" backend from the plugin's perspective (fire message, get response, store locally)
- When switching backends mid-session, add a visual separator in the chat UI ("Switched to OpenCode" / "Switched to Claude")

**Detection:** Duplicate messages in chat panel. Message ordering doesn't match chronological order. Clearing local history doesn't clear OpenCode server history (and vice versa).

**Phase:** Session management phase. Design decision needed before implementation.

---

## Minor Pitfalls

### Pitfall 12: OpenCode Server Supports Optional Basic Auth

**What goes wrong:** OpenCode's server supports optional basic auth via `OPENCODE_SERVER_PASSWORD` / `OPENCODE_SERVER_USERNAME` environment variables. If the user has these set (e.g., for security when exposing the server beyond localhost), the plugin's HTTP requests fail with 401 without any helpful error message.

**Prevention:**
- Add optional username/password fields to plugin settings (alongside the port setting)
- If a request returns 401, check for `WWW-Authenticate: Basic` header and surface a clear error message: "OpenCode server requires authentication. Configure credentials in Plugin Settings."
- When spawning the server, propagate any configured credentials to the environment

**Detection:** All REST calls return 401. SSE connection fails immediately after connecting.

**Phase:** Settings/configuration phase.

---

### Pitfall 13: OpenCode's `prompt_async` Returns 204 But Streams via SSE

**What goes wrong:** The synchronous `POST /:sessionID/message` endpoint streams the response as JSON but blocks until the LLM finishes. The async `POST /:sessionID/prompt_async` returns 204 immediately and delivers results via the `/event` SSE stream. If the plugin uses the sync endpoint, it blocks an HTTP connection slot for the entire LLM response time (potentially minutes). If it uses async, it must have the SSE connection working first — a chicken-and-egg problem if the SSE connection isn't established yet.

**Prevention:**
- Always establish the SSE `/event` connection BEFORE sending any `prompt_async` request
- Use `prompt_async` (not the sync `message` endpoint) to avoid blocking HTTP connection slots
- Sequence: connect SSE → filter for `server.connected` event → create/reuse session → register MCP → send `prompt_async`
- Handle the edge case where the SSE connection drops between sending `prompt_async` and receiving the response events — the response is lost to the plugin but exists server-side in the session messages

**Detection:** Plugin sends a message but never receives a response. Or: one HTTP slot is blocked for minutes during LLM generation.

**Phase:** Core HTTP/SSE integration phase.

---

### Pitfall 14: OpenCode SSE Heartbeats Every 10 Seconds May Trigger False Reconnects

**What goes wrong:** OpenCode sends `server.heartbeat` events every 10 seconds. If the plugin's connection health check timeout is set to less than 10 seconds (e.g., matching the existing `FMCPTaskQueue` 10ms sleep pattern philosophy), it will continuously consider the connection dead and reconnect, creating a storm of reconnection attempts.

**Prevention:**
- Set the SSE connection health timeout to at least 30 seconds (3x the heartbeat interval) to account for heartbeat jitter
- Track the timestamp of the last received SSE event (any event, including heartbeat)
- Only trigger reconnection if no event has been received for 30+ seconds
- During reconnection, backoff exponentially to avoid hammering the server

**Detection:** Log shows repeated "SSE connected" / "SSE disconnected" messages. OpenCode server logs show many `/event` connections from the same client.

**Phase:** SSE connection management phase.

---

### Pitfall 15: Existing Game-Thread Deadlock Risk Amplified by HTTP Callbacks

**What goes wrong:** The codebase already has a documented game-thread deadlock risk (CONCERNS.md: MCPToolRegistry dispatches to game thread, tests are disabled because of it). Adding HTTP-based callbacks introduces another source of `AsyncTask(ENamedThreads::GameThread, ...)` calls. If an HTTP callback tries to dispatch to the game thread while the game thread is blocked waiting for an HTTP response (e.g., `ExecuteSync` or a synchronous health check), you get a deadlock.

**Prevention:**
- Never call any synchronous/blocking HTTP operation from the game thread
- All HTTP calls must be fire-and-forget with callbacks
- Health checks and availability checks must use cached results, not live HTTP calls
- If `ExecuteSync` is needed for backward compatibility, implement it by running the async path on a background thread and waiting on an `FEvent`, NOT by blocking the game thread directly
- Audit all call sites that currently call `IClaudeRunner::ExecuteSync()` — if any are on the game thread, they must be refactored before adding HTTP backend

**Detection:** Editor freezes completely (not just UI lag). Call stack shows game thread waiting for HTTP and HTTP callback waiting for game thread.

**Phase:** Interface refactoring phase. Must audit existing `ExecuteSync` call sites early.

---

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Mitigation |
|-------------|---------------|------------|
| Interface refactoring | Pitfall 4 (subprocess assumptions), Pitfall 8 (concrete type ownership) | Design `IChatBackend` as the first task; review all `IClaudeRunner` call sites |
| Process lifecycle | Pitfall 3 (zombies), Pitfall 9 (platform differences), Pitfall 7 (startup race) | PID file + health-check polling + `OnPreExit` hook; test on all 3 platforms |
| SSE client | Pitfall 1 (timeout kills SSE), Pitfall 2 (non-game-thread callbacks), Pitfall 5 (frame boundaries), Pitfall 10 (connection limits) | Clear timeout; thread-marshal all events; buffer-based parser; consider raw sockets for SSE |
| SSE event handling | Pitfall 6 (global event bus), Pitfall 14 (heartbeat false alarms), Pitfall 13 (async message flow) | Session-ID filtering; 30s health timeout; SSE-before-message sequencing |
| Session management | Pitfall 11 (dual state models) | Local-only source of truth; no server-side sync |
| Settings & auth | Pitfall 12 (basic auth) | Optional credential fields in plugin settings |
| Game-thread safety | Pitfall 15 (deadlock amplification) | Zero synchronous HTTP from game thread; audit `ExecuteSync` callers |

---

## Sources

- UE 5.3 HTTP module source: `IHttpRequest.h` — `SetResponseBodyReceiveStreamDelegate`, `SetTimeout`, `ClearTimeout`, `SetDelegateThreadPolicy` API analysis (5.7 API is a superset; patterns verified against 5.3 source available locally)
- OpenCode server source (dev branch): `server.ts`, `routes/event.ts`, `routes/session.ts`, `routes/mcp.ts` — SSE implementation uses `streamSSE` from Hono with `Bus.subscribeAll()`, heartbeat at 10s intervals
- OpenCode session routes: `prompt_async` returns 204, `message` blocks and streams JSON
- Existing codebase: `ClaudeCodeRunner.cpp` (1251 lines, `FRunnable` + `AsyncTask` pattern), `IClaudeRunner.h` (interface design), `ClaudeSubsystem.h` (concrete `FClaudeCodeRunner` ownership), `UnrealClaudeConstants.h` (timeout values)
- Existing concerns: `CONCERNS.md` — game-thread deadlock in task queue (tests disabled), `ProjectContext` reentrant lock risk, MCP server no-auth
- Confidence: HIGH — based on source-level analysis of both UE HTTP internals and OpenCode server implementation

---

*Pitfalls audit: 2026-03-30*
