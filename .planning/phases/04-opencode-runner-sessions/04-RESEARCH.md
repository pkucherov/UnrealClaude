# Phase 4: OpenCode Runner & Sessions - Research

**Researched:** 2026-04-01
**Domain:** OpenCode REST API integration, IChatBackend composition, FRunnable threading, permission handling
**Confidence:** HIGH

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Session Lifecycle
- **D-01:** Local-primary session model — FChatSessionManager remains the single display/persistence layer for the chat panel UI. OpenCode server sessions are an implementation detail of FOpenCodeRunner, not exposed to the subsystem or UI
- **D-02:** One OpenCode session per editor session — FOpenCodeRunner creates an OpenCode session (`POST /session`) during initialization (after server connection succeeds) and reuses it for all messages within that editor session
- **D-03:** Session ID stored as a member of FOpenCodeRunner. If the session becomes invalid (server restart, error), create a new one transparently on the next prompt
- **D-04:** Conversation history passed to OpenCode via FormatPrompt() — the local FChatSessionManager history is formatted into the prompt, same pattern as FClaudeCodeRunner. OpenCode's server-side session also accumulates context, providing double-coverage

#### Permission Handling
- **D-05:** Modal dialog for permission prompts — when FOpenCodeRunner receives a `permission.updated` SSE event, it shows a modal dialog (similar to FScriptExecutionManager's existing pattern) with the permission description, and sends approval/denial back to OpenCode via REST
- **D-06:** Event buffering during permission dialog — SSE stream stays open, FOpenCodeRunner buffers incoming events while the modal is shown. Once the user responds, buffered events are processed in order. No data loss, no stream interruption
- **D-07:** Permission response endpoint: `POST /session/:id/permissions/:permissionID` (validated against live OpenCode API docs)

#### MCP Registration
- **D-08:** Register MCP bridge immediately after server connection succeeds (health check passes), before creating the OpenCode session
- **D-09:** Re-register MCP bridge on every reconnect (Reconnecting → Connected transition)
- **D-10:** Graceful degradation on MCP registration failure — log warning, continue with chat-only mode
- **D-11:** MCP bridge URL is `http://localhost:3000` constructed from `UnrealClaudeConstants::MCPServer::DefaultPort`

#### Agent & Model Configuration
- **D-12:** Hardcoded defaults in Phase 4 — sensible defaults for agent type and model; no user-facing settings
- **D-13:** `FOpenCodeRequestConfig : FChatRequestConfig` — derived config class with AgentType, ModelId, SessionId fields
- **D-14:** `FOpenCodeRunner::CreateConfig()` returns `TUniquePtr<FChatRequestConfig>` pointing to `FOpenCodeRequestConfig`

#### Component Composition
- **D-15:** FOpenCodeRunner owns Phase 3 components as members: `TUniquePtr<FOpenCodeServerLifecycle>`, `FOpenCodeSSEParser` (value member), `TSharedPtr<FOpenCodeHttpClient>` (shared with lifecycle)
- **D-16:** FOpenCodeRunner subscribes to `FOpenCodeServerLifecycle::OnStateChanged`. On Connected: register MCP, create session, start SSE stream. On Reconnecting: buffer state. On Disconnected: stop SSE, cleanup
- **D-17:** `FOpenCodeRunner::IsAvailable()` delegates to `FOpenCodeServerLifecycle` connection state (Connected = available)
- **D-18:** `ExecuteAsync()` sends prompt via `POST /session/:id/message`, SSE events routed through `FOpenCodeSSEParser`, dispatched to game thread via `AsyncTask`

#### Factory Integration
- **D-19:** Replace stub in `ChatBackendFactory.cpp` case `EChatBackendType::OpenCode` with `MakeUnique<FOpenCodeRunner>()`

#### Testing
- **D-20:** ≥ 90% unit test coverage for FOpenCodeRunner; mock all dependencies via constructor injection
- **D-21:** Test session creation, prompt sending, SSE event routing, permission dialog flow, MCP registration, error handling, cancel flow
- **D-22:** Integration-style tests with `FMockHttpClient` simulating OpenCode API responses; no real OpenCode binary needed

### Agent's Discretion
- Exact internal state management within FOpenCodeRunner (how to track in-flight requests)
- Event buffering implementation details during permission prompts
- Exact permission dialog widget reuse vs. new dialog
- Thread synchronization between SSE events and game thread permission dialog
- How to handle OpenCode session expiration/invalidation edge cases

### Deferred Ideas (OUT OF SCOPE)
- None — discussion stayed within phase scope
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| COMM-01 | FOpenCodeRunner implements IChatBackend, sending prompts via REST POST to `opencode serve` | IChatBackend interface verified (9 methods); `POST /session/:id/message` endpoint confirmed |
| COMM-04 | Session lifecycle managed via REST API — create session, send messages, retrieve history | `POST /session`, `POST /session/:id/message`, `GET /session/:id/message` all confirmed |
| COMM-05 | OpenCode permission prompts surfaced to user through existing permission dialog flow | `permission.updated` SSE event → `POST /session/:id/permissions/:permissionID` confirmed |
| COMM-06 | Agent type and model selection forwarded to OpenCode via prompt payload | `model` and `agent` fields in `POST /session/:id/message` body confirmed |
| MCPI-01 | Existing MCP bridge URL dynamically registered via `POST /mcp` | `McpRemoteConfig` type confirmed; registration before session creation decided |
| TEST-01 | All new classes have ≥90% unit test coverage | FMockHttpClient + FMockProcessLauncher pattern established from Phase 3 |
</phase_requirements>

---

## Summary

Phase 4 composes three existing Phase 3 components (`FOpenCodeSSEParser`, `FOpenCodeServerLifecycle`, `FOpenCodeHttpClient`) into a single `FOpenCodeRunner` that fully implements the 9-method `IChatBackend` interface. The runner communicates with `opencode serve` exclusively via HTTP REST — no subprocess spawning. It follows the same `FRunnable` + `AsyncTask(GameThread)` threading pattern established by `FClaudeCodeRunner`, replacing subprocess pipe I/O with HTTP calls and SSE stream polling.

All OpenCode REST endpoints have been validated against live documentation. The `POST /session/:id/message` endpoint is **synchronous** (waits for full response); `POST /session/:id/prompt_async` returns 204 immediately and delivers results via SSE — the async variant is preferred for streaming. The SSE stream at `GET /global/event` carries all in-flight events wrapped in a `GlobalEvent` object (`{ directory, payload }`), which `FOpenCodeSSEParser` already handles. Permission events arrive on the same SSE stream as `permission.updated` with permission metadata in `payload.properties`.

The single integration point that activates the entire pipeline is replacing the 3-line stub at `ChatBackendFactory.cpp:18` with `MakeUnique<FOpenCodeRunner>()`. Everything else flows from the `FOpenCodeServerLifecycle::OnStateChanged` delegate that triggers MCP registration → session creation → SSE stream start in sequence.

**Primary recommendation:** Implement `FOpenCodeRunner` as an `IChatBackend` + `FRunnable` that owns lifecycle, parser, and HTTP client; wire the `OnStateChanged` delegate to drive the MCP registration → session → SSE startup sequence; buffer permission events in a `TArray<FChatStreamEvent>` behind an `FCriticalSection`; replace the factory stub.

---

## Standard Stack

### Core (no new libraries — all already in plugin)

| Component | Source | Purpose | Why Standard |
|-----------|--------|---------|--------------|
| `FOpenCodeSSEParser` | `Private/OpenCode/OpenCodeSSEParser.h` | Parse SSE chunks → `FChatStreamEvent` | Built and tested in Phase 3 |
| `FOpenCodeServerLifecycle` | `Private/OpenCode/OpenCodeServerLifecycle.h` | 6-state server detect/spawn/connect machine | Built and tested in Phase 3 |
| `FOpenCodeHttpClient` / `IHttpClient` | `Private/OpenCode/OpenCodeHttpClient.h` | `Get()`, `Post()`, `BeginSSEStream()`, `AbortSSEStream()` | Built and tested in Phase 3 |
| `IChatBackend` | `Public/IChatBackend.h` | 9-method interface FOpenCodeRunner must implement | Defined in Phase 2 |
| `FChatRequestConfig` / `FOpenCodeRequestConfig` | `Public/ChatBackendTypes.h` + new derived struct | Config hierarchy for polymorphic config creation | Phase 2 pattern; `FClaudeRequestConfig` is reference |
| `FRunnable` / `FRunnableThread` | UE HAL | Background thread for HTTP I/O; same pattern as `FClaudeCodeRunner` | UE standard for non-blocking I/O |
| `AsyncTask(ENamedThreads::GameThread, ...)` | UE async | Dispatch SSE events and callbacks to game thread | Required by UE Slate — UI only on game thread |
| `TAtomic<bool>` | UE HAL | `bIsExecuting` flag thread-safe across FRunnable and game thread | UE cross-platform atomic |
| `FCriticalSection` + `FScopeLock` | UE HAL | Protect permission event buffer shared between SSE thread and game thread | Standard UE mutex |

### Supporting

| Component | Purpose | When to Use |
|-----------|---------|-------------|
| `FMockHttpClient` (TestUtils.h) | Simulate OpenCode REST responses in tests | Every FOpenCodeRunner unit test |
| `FMockProcessLauncher` (TestUtils.h) | Prevent real process spawning in lifecycle tests | Tests that construct FOpenCodeRunner |
| `SGenericDialogWidget` or inline modal via `FMessageDialog` | Permission approval dialog | D-05: modal gate on `permission.updated` SSE event |

### No New Packages

No `npm install` or new UE module dependencies. All needed components exist in the plugin already.

---

## Architecture Patterns

### Recommended File Structure

```
Private/OpenCode/
├── OpenCodeRunner.h           # FOpenCodeRunner : IChatBackend, FRunnable (new)
├── OpenCodeRunner.cpp         # Implementation (new)
├── OpenCodeRequestConfig.h    # FOpenCodeRequestConfig : FChatRequestConfig (new)
├── OpenCodeSSEParser.h        # Existing (Phase 3)
├── OpenCodeSSEParser.cpp      # Existing (Phase 3)
├── OpenCodeServerLifecycle.h  # Existing (Phase 3)
├── OpenCodeServerLifecycle.cpp # Existing (Phase 3)
├── OpenCodeHttpClient.h       # Existing (Phase 3)
├── OpenCodeHttpClient.cpp     # Existing (Phase 3)
├── OpenCodeTypes.h            # Existing (Phase 3)
└── IProcessLauncher.h         # Existing (Phase 3)

Private/Tests/
└── OpenCodeRunnerTests.cpp    # New — ≥90% coverage (D-20, D-21, D-22)

Private/
└── ChatBackendFactory.cpp     # Modify line 18: replace stub with MakeUnique<FOpenCodeRunner>()
```

### Pattern 1: FOpenCodeRunner Class Shape

**What:** `FOpenCodeRunner` inherits `IChatBackend` and `FRunnable`, owns Phase 3 components, follows `FClaudeCodeRunner` structure exactly but replaces subprocess I/O with HTTP calls.

```cpp
// Source: ClaudeCodeRunner.h (reference) + IChatBackend.h
class FOpenCodeRunner : public IChatBackend, public FRunnable
{
public:
    // Constructor injection for testability (D-15, D-20)
    FOpenCodeRunner(
        TSharedPtr<IHttpClient> InHttpClient = nullptr,
        TSharedPtr<IProcessLauncher> InProcessLauncher = nullptr);
    virtual ~FOpenCodeRunner();

    // IChatBackend — 9 methods
    virtual bool ExecuteAsync(const FChatRequestConfig& Config,
        FOnChatResponse OnComplete, FOnChatProgress OnProgress) override;
    virtual bool ExecuteSync(const FChatRequestConfig& Config,
        FString& OutResponse) override;
    virtual void Cancel() override;
    virtual bool IsExecuting() const override { return bIsExecuting; }
    virtual bool IsAvailable() const override;
    virtual FString GetDisplayName() const override { return TEXT("OpenCode"); }
    virtual EChatBackendType GetBackendType() const override
        { return EChatBackendType::OpenCode; }
    virtual FString FormatPrompt(...) const override;
    virtual TUniquePtr<FChatRequestConfig> CreateConfig() const override;

    // FRunnable — worker thread executes HTTP POST + SSE routing
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

private:
    TSharedPtr<IHttpClient> HttpClient;
    TUniquePtr<FOpenCodeServerLifecycle> Lifecycle;
    FOpenCodeSSEParser SSEParser;          // value member

    FString ActiveSessionId;              // D-02, D-03
    TAtomic<bool> bIsExecuting{ false };
    FThreadSafeCounter StopTaskCounter;
    FRunnableThread* Thread = nullptr;

    // Permission event buffering (D-06)
    TArray<FChatStreamEvent> PendingPermissionBuffer;
    FCriticalSection PermissionBufferLock;
    TAtomic<bool> bPermissionDialogActive{ false };

    // Current request state
    FOpenCodeRequestConfig CurrentConfig;
    FOnChatResponse OnCompleteDelegate;
    FOnChatProgress OnProgressDelegate;
};
```

### Pattern 2: OnStateChanged → Startup Sequence (D-16)

**What:** Subscribe to `FOpenCodeServerLifecycle::OnStateChanged` in constructor. On `Connected` transition: register MCP bridge, create OpenCode session, start SSE stream. On `Reconnecting`: set reconnect flag. On `Disconnected`: abort SSE, clear session.

```cpp
// In FOpenCodeRunner constructor:
Lifecycle->OnStateChanged.BindLambda(
    [this](EOpenCodeConnectionState OldState,
           EOpenCodeConnectionState NewState)
    {
        if (NewState == EOpenCodeConnectionState::Connected)
        {
            RegisterMcpBridge();     // POST /mcp
            CreateOrReuseSession(); // POST /session (or reuse ActiveSessionId)
            StartSSEStream();       // BeginSSEStream on /global/event
        }
        else if (NewState == EOpenCodeConnectionState::Disconnected)
        {
            HttpClient->AbortSSEStream();
            ActiveSessionId.Empty();
            SSEParser.Reset();
        }
    });
```

### Pattern 3: MCP Bridge Registration (D-08, D-09, D-10, D-11, MCPI-01)

**What:** Register the local MCP HTTP server with OpenCode so all 30+ editor tools are available. Must happen before creating a session. On failure, log and continue (graceful degradation).

```cpp
// POST /mcp body (McpRemoteConfig shape — confirmed against OpenCode SDK)
void FOpenCodeRunner::RegisterMcpBridge()
{
    FString McpUrl = FString::Printf(
        TEXT("http://localhost:%d"),
        UnrealClaudeConstants::MCPServer::DefaultPort);  // port 3000

    FString Body = FString::Printf(
        TEXT("{\"name\":\"unrealclaude\","
             "\"config\":{\"type\":\"remote\","
             "\"url\":\"%s\","
             "\"enabled\":true}}"),
        *McpUrl);

    FString Response;
    int32 Status = HttpClient->Post(
        Lifecycle->GetServerBaseUrl() + TEXT("/mcp"),
        Body, Response);

    if (Status != 200 && Status != 201)
    {
        UE_LOG(LogUnrealClaude, Warning,
            TEXT("MCP bridge registration failed (HTTP %d) — "
                 "continuing without editor tools"), Status);
        // D-10: graceful degradation — do NOT abort
    }
}
```

### Pattern 4: Session Creation (D-02, D-03)

**What:** `POST /session` → store ID in `ActiveSessionId`. If session already exists (re-use after reconnect), skip creation. If session invalid on next prompt, create new one transparently.

```cpp
// POST /session — confirmed body schema
void FOpenCodeRunner::CreateOrReuseSession()
{
    if (!ActiveSessionId.IsEmpty()) return; // D-03: reuse existing

    FString Response;
    // title is optional — use plugin name for traceability
    int32 Status = HttpClient->Post(
        Lifecycle->GetServerBaseUrl() + TEXT("/session"),
        TEXT("{\"title\":\"UnrealClaude\"}"),
        Response);

    if (Status == 200 || Status == 201)
    {
        // Parse "id" from response JSON
        TSharedPtr<FJsonObject> Json;
        TSharedRef<TJsonReader<>> Reader =
            TJsonReaderFactory<>::Create(Response);
        if (FJsonSerializer::Deserialize(Reader, Json) && Json.IsValid())
        {
            ActiveSessionId = Json->GetStringField(TEXT("id"));
        }
    }
}
```

### Pattern 5: ExecuteAsync → prompt_async + SSE (D-18, COMM-04, COMM-06)

**What:** `ExecuteAsync` stores config, sets `bIsExecuting`, creates `FRunnableThread`. Worker thread posts to `POST /session/:id/message` with `parts` array. SSE events from ongoing stream are routed to game thread via `AsyncTask`.

```cpp
// POST /session/:id/message confirmed body schema (synchronous variant)
// For async: POST /session/:id/prompt_async (returns 204, results come via SSE)
FString FOpenCodeRunner::BuildMessageBody(const FOpenCodeRequestConfig& Config)
{
    // parts array — TextPartInput shape: { type: "text", text: "..." }
    // model format: "providerID/modelID" e.g. "anthropic/claude-opus-4-5"
    // agent: agent name string e.g. "build"
    return FString::Printf(
        TEXT("{\"parts\":[{\"type\":\"text\",\"text\":\"%s\"}],"
             "\"model\":\"%s\","
             "\"agent\":\"%s\"}"),
        *EscapeJsonString(Config.Prompt),
        *Config.ModelId,        // empty string = use OpenCode default
        *Config.AgentType);     // empty string = use OpenCode default
}
```

### Pattern 6: Permission Handling (D-05, D-06, D-07, COMM-05)

**What:** SSE `permission.updated` event → extract `permissionID` from `payload.properties.id` → buffer all subsequent SSE events → dispatch modal to game thread → on user response → `POST /session/:id/permissions/:permissionID` → drain buffer.

```cpp
// In SSE stream callback (runs on HTTP thread):
void FOpenCodeRunner::HandleSSEEvent(const FChatStreamEvent& Event)
{
    if (Event.Type == EChatStreamEventType::PermissionRequest)
    {
        // Extract permissionID from Event.RawJson
        // (payload.properties.id — confirmed from SDK types)
        FString PermId = ExtractPermissionId(Event.RawJson);

        bPermissionDialogActive = true;
        AsyncTask(ENamedThreads::GameThread, [this, PermId, Event]()
        {
            bool bApproved = ShowPermissionDialog(Event);
            FString Body = bApproved
                ? TEXT("{\"response\":\"allow\"}")
                : TEXT("{\"response\":\"deny\"}");

            FString Resp;
            HttpClient->Post(
                Lifecycle->GetServerBaseUrl()
                + TEXT("/session/") + ActiveSessionId
                + TEXT("/permissions/") + PermId,
                Body, Resp);

            // Drain buffered events
            {
                FScopeLock Lock(&PermissionBufferLock);
                bPermissionDialogActive = false;
                for (const FChatStreamEvent& Buffered : PendingPermissionBuffer)
                    DispatchEventToUI(Buffered);
                PendingPermissionBuffer.Empty();
            }
        });
        return;
    }

    // While modal is active, buffer all other events
    if (bPermissionDialogActive)
    {
        FScopeLock Lock(&PermissionBufferLock);
        PendingPermissionBuffer.Add(Event);
        return;
    }

    DispatchEventToUI(Event);
}
```

### Pattern 7: Cancel Flow (CHUI-03)

**What:** `Cancel()` calls `POST /session/:id/abort` (confirmed endpoint — NOT `/cancel`), then `AbortSSEStream()`, then sets `StopTaskCounter`.

```cpp
void FOpenCodeRunner::Cancel()
{
    if (!ActiveSessionId.IsEmpty())
    {
        FString Resp;
        HttpClient->Post(
            Lifecycle->GetServerBaseUrl()
            + TEXT("/session/") + ActiveSessionId + TEXT("/abort"),
            TEXT("{}"), Resp);
    }
    HttpClient->AbortSSEStream();
    StopTaskCounter.Increment();
}
```

### Pattern 8: Game Thread Dispatch

**What:** All callbacks and UI updates MUST arrive on game thread. Pattern identical to `FClaudeCodeRunner::ReportCompletion`.

```cpp
// Standard UE game thread dispatch pattern (from FClaudeCodeRunner reference)
void FOpenCodeRunner::DispatchEventToUI(const FChatStreamEvent& Event)
{
    FOnChatStreamEvent EventDelegate = CurrentConfig.OnStreamEvent;
    FOnChatProgress ProgressDelegate = OnProgressDelegate;

    AsyncTask(ENamedThreads::GameThread, [Event, EventDelegate, ProgressDelegate]()
    {
        EventDelegate.ExecuteIfBound(Event);
        if (Event.Type == EChatStreamEventType::TextContent)
        {
            ProgressDelegate.ExecuteIfBound(Event.Text);
        }
    });
}
```

### Pattern 9: FormatPrompt (D-04)

**What:** Incorporate local history into the prompt text, identical to `FClaudeCodeRunner`. OpenCode also accumulates server-side session context (double coverage).

```cpp
// Same Human:/Assistant: format as FClaudeCodeRunner
FString FOpenCodeRunner::FormatPrompt(
    const TArray<TPair<FString, FString>>& History,
    const FString& SystemPrompt,
    const FString& ProjectContext) const
{
    FString Formatted;
    for (const auto& Pair : History)
    {
        Formatted += TEXT("Human: ") + Pair.Key + TEXT("\n\n");
        Formatted += TEXT("Assistant: ") + Pair.Value + TEXT("\n\n");
    }
    // System context prepended separately via FChatRequestConfig.SystemPrompt
    return Formatted;
}
```

### Anti-Patterns to Avoid

- **Exposing `ActiveSessionId` to subsystem or UI:** Session ID is FOpenCodeRunner's private implementation detail (D-01). Never put it in `FChatRequestConfig` base class.
- **Showing permission dialog from HTTP thread:** Always `AsyncTask(GameThread)` before touching Slate. Modal shown on game thread only.
- **Calling `HttpClient->Post()` from game thread in Cancel():** Cancel() is called from game thread — use a fire-and-forget approach or accept the brief blocking (abort is fast).
- **Creating a new OpenCode session on every `ExecuteAsync` call:** Violates D-02. One session per editor session. Only re-create if `ActiveSessionId` is empty (D-03).
- **Forgetting to call `SSEParser.Reset()` on reconnect:** Stale buffer will inject corrupt events from the previous connection.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| SSE chunk parsing | Custom split/parse logic | `FOpenCodeSSEParser::ProcessChunk()` | Already handles partial chunks, blank-line boundaries, retry: fields — 22 tests passing |
| Server detect/spawn | Custom process management | `FOpenCodeServerLifecycle::EnsureServerReady()` | Handles PID file, port resolution, backoff, 6-state machine — 19 tests passing |
| HTTP requests | `FHttpModule` directly | `FOpenCodeHttpClient` (or `IHttpClient` interface) | Abstracts FEvent blocking for synchronous-style calls; mock-able via `IHttpClient` |
| Mock HTTP in tests | Real HTTP calls | `FMockHttpClient` (TestUtils.h) | Has `SimulateSSEData()` / `SimulateSSEDisconnect()` helpers; `PostCallCount` / `LastPostBody` for assertions |
| Game thread dispatch boilerplate | Custom ticker/delegate | `AsyncTask(ENamedThreads::GameThread, lambda)` | Standard UE pattern; used by `FClaudeCodeRunner` — identical approach |
| Binary detection / PATH search | Custom file search | `FChatBackendFactory::GetOpenCodePath()` | Already searches PATH + npm global locations on all 3 platforms |

**Key insight:** The value of Phase 4 is composition, not construction. Every non-trivial piece already exists. The main work is wiring them together with correct sequencing (lifecycle → MCP → session → SSE).

---

## Common Pitfalls

### Pitfall 1: Wrong Abort Endpoint
**What goes wrong:** Calling `POST /session/:id/cancel` — this endpoint does not exist in the OpenCode API.
**Why it happens:** REQUIREMENTS.md line 41 says `POST /session/:id/cancel` (written before API validation).
**How to avoid:** Use `POST /session/:id/abort` (confirmed from live docs and SDK types).
**Warning signs:** HTTP 404 on cancel; OpenCode server logs show unknown route.

### Pitfall 2: SSE Stream at Wrong URL
**What goes wrong:** Connecting SSE stream to `/event` instead of `/global/event`.
**Why it happens:** The CONTEXT.md (line 116) lists `/event` as a candidate. The actual endpoint is `/global/event`.
**How to avoid:** Use `Lifecycle->GetServerBaseUrl() + TEXT("/global/event")` (confirmed from live docs).
**Warning signs:** No SSE events received; HTTP 404 on stream connect.

### Pitfall 3: Session Error Code Misread
**What goes wrong:** `FOpenCodeSSEParser::MapSessionError` reads `Payload->GetIntegerField("code")` — but the `EventSessionError` type has no top-level `code` integer. Error details are nested in `payload.properties.error` (a typed union: ProviderAuthError, ApiError, UnknownError).
**Why it happens:** `OpenCodeSSEParser.cpp` `MapSessionError` method was written before live API validation.
**How to avoid:** In Phase 4, when consuming `session.error` events, parse `properties.error` object rather than a flat `code` field. Log `RawJson` for diagnosis. File a note on the parser for a cleanup pass.
**Warning signs:** `ErrorType` always maps to `Unknown` even for auth failures.

### Pitfall 4: Permission Event Property Nesting
**What goes wrong:** Extracting `permissionID` from the top level of the SSE payload JSON — it's actually at `payload.properties.id`.
**Why it happens:** The SSE parser sets `Event.RawJson` to the full payload JSON, not the properties sub-object.
**How to avoid:** When processing `PermissionRequest` events in FOpenCodeRunner, parse `RawJson` → get `"properties"` object → get `"id"` string.
**Warning signs:** Empty `permissionID`; `POST /session/:id/permissions/` returns 404.

### Pitfall 5: SSE Events After Permission Modal Opens
**What goes wrong:** `session.status: idle` arrives while permission modal is blocking game thread — if not buffered, it fires completion callback prematurely.
**Why it happens:** SSE stream continues receiving events regardless of UI state.
**How to avoid:** Check `bPermissionDialogActive` before dispatching any event (D-06). Buffer behind `FCriticalSection`. Drain buffer after modal resolves.
**Warning signs:** Chat UI shows "complete" before user has approved the permission.

### Pitfall 6: FRunnable Thread Lifetime vs. SSE Stream
**What goes wrong:** `FRunnableThread` destructor runs before `AbortSSEStream()` completes — use-after-free on the `FOpenCodeRunner` members the SSE callback captures.
**Why it happens:** The SSE callback lambda captures `this`. If the runner is destroyed while the stream is still running, the callback fires into freed memory.
**How to avoid:** In destructor: (1) call `HttpClient->AbortSSEStream()`, (2) increment `StopTaskCounter`, (3) `Thread->WaitForCompletion()`, (4) `delete Thread`. Same order as `FClaudeCodeRunner`.
**Warning signs:** Intermittent crash on editor shutdown; Address Sanitizer reports heap-use-after-free.

### Pitfall 7: Blocking Game Thread in Constructor
**What goes wrong:** Calling `Lifecycle->EnsureServerReady()` directly in `FOpenCodeRunner` constructor — this performs health-check HTTP calls that block.
**Why it happens:** Eager initialization is tempting.
**How to avoid:** Call `EnsureServerReady()` from `ExecuteAsync()` on the first request (lazy init), or kick it off on a background thread from the constructor. Never block the game thread.
**Warning signs:** Editor freezes for 1-5s on backend switch.

---

## Code Examples

### Full SSE Stream Wire Format (Verified against live OpenCode API)

```
data: {"directory":"/project","payload":{"type":"server.connected","properties":{}}}

data: {"directory":"/project","payload":{"type":"session.status","properties":{"sessionID":"abc123","status":{"type":"busy"}}}}

data: {"directory":"/project","payload":{"type":"message.part.updated","properties":{"part":{"type":"text","text":"Hello"}}}}

data: {"directory":"/project","payload":{"type":"session.status","properties":{"sessionID":"abc123","status":{"type":"idle"}}}}

```

Note: Each event ends with a blank line. The outer object is `GlobalEvent: { directory: string, payload: Event }`. `FOpenCodeSSEParser` already handles this wrapper.

### POST /session Request (Verified)

```
POST /session
Content-Type: application/json

{"title":"UnrealClaude"}

Response 200:
{"id":"ses_abc123","title":"UnrealClaude","created":1234567890}
```

### POST /session/:id/message Request (Verified — synchronous)

```
POST /session/ses_abc123/message
Content-Type: application/json

{
  "parts": [{"type":"text","text":"Create a blueprint actor"}],
  "model": "",
  "agent": ""
}
```

Empty strings for `model` and `agent` use OpenCode server defaults. Set to `"anthropic/claude-opus-4-5"` / `"build"` for explicit control.

### POST /mcp Request — Remote Config (Verified)

```
POST /mcp
Content-Type: application/json

{
  "name": "unrealclaude",
  "config": {
    "type": "remote",
    "url": "http://localhost:3000",
    "enabled": true
  }
}
```

### POST /session/:id/permissions/:permissionID (Verified)

```
POST /session/ses_abc123/permissions/perm_xyz789
Content-Type: application/json

{"response":"allow"}
-- or --
{"response":"deny"}

Response: true (boolean)
```

Optional `"remember": true` field to persist the decision.

### FOpenCodeRequestConfig (New Derived Type)

```cpp
// Source: FClaudeRequestConfig pattern (Public/ClaudeRequestConfig.h)
// File: Private/OpenCode/OpenCodeRequestConfig.h

struct FOpenCodeRequestConfig : public FChatRequestConfig
{
    /** OpenCode agent name (empty = use server default) */
    FString AgentType;

    /** Model string "providerID/modelID" (empty = use server default) */
    FString ModelId;

    /** Active OpenCode session ID (populated by runner before use) */
    FString SessionId;

    virtual ~FOpenCodeRequestConfig() = default;
};
```

### Test Pattern — Mock HTTP Simulation

```cpp
// Source: OpenCodeSSEParserTests.cpp + OpenCodeServerLifecycleTests.cpp patterns
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOpenCodeRunner_SendPromptCallsPost,
    "UnrealClaude.OpenCode.Runner.SendPromptCallsPost",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeRunner_SendPromptCallsPost::RunTest(const FString& Parameters)
{
    auto MockHttp = MakeShared<FMockHttpClient>();
    auto MockProc = MakeShared<FMockProcessLauncher>();
    MockHttp->MockStatusCode = 200;
    MockHttp->MockResponseBody = TEXT("{\"id\":\"ses_test\"}");

    FOpenCodeRunner Runner(MockHttp, MockProc);
    // ... trigger session creation sequence ...

    TestEqual("POST /session called once", MockHttp->PostCallCount, 1);
    TestTrue("Session URL used", MockHttp->LastPostUrl.Contains(TEXT("/session")));
    return true;
}
```

---

## Runtime State Inventory

> This is a composition/integration phase — not a rename/refactor. No runtime state is being renamed. Omitted per instructions.

---

## Environment Availability

> Phase 4 adds no new external tools. All runtime dependencies (`opencode` binary, Node.js MCP bridge on port 3000) are handled by existing infrastructure (Phase 3 lifecycle manager, Phase 1 MCP server). No new probes needed.

| Dependency | Required By | Available | Fallback |
|------------|------------|-----------|----------|
| `opencode` CLI binary | FOpenCodeServerLifecycle | Detected via `FChatBackendFactory::GetOpenCodePath()` | Graceful: IsAvailable() returns false; factory already logs warning |
| MCP HTTP server (port 3000) | MCP bridge registration | Started by `FUnrealClaudeMCPServer` at module load | Graceful: D-10 — log + continue without editor tools |
| UE HTTP module | FOpenCodeHttpClient | ✓ — declared in `Build.cs` | — |
| UE Automation Test framework | OpenCodeRunnerTests.cpp | ✓ — `IMPLEMENT_SIMPLE_AUTOMATION_TEST` used in Phase 3 | — |

---

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | UE Automation Test (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`) |
| Config file | None — runs via Editor console `Automation RunTests UnrealClaude` |
| Quick run command | `Automation RunTests UnrealClaude.OpenCode.Runner` |
| Full suite command | `Automation RunTests UnrealClaude` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| COMM-01 | FOpenCodeRunner implements IChatBackend; ExecuteAsync sends REST POST | unit | `Automation RunTests UnrealClaude.OpenCode.Runner` | ❌ Wave 0 |
| COMM-04 | Session created via POST /session; ID stored; reused across prompts | unit | `Automation RunTests UnrealClaude.OpenCode.Runner.Session` | ❌ Wave 0 |
| COMM-05 | permission.updated event → modal → POST /permissions/:id | unit | `Automation RunTests UnrealClaude.OpenCode.Runner.Permission` | ❌ Wave 0 |
| COMM-06 | model + agent fields forwarded in message body | unit | `Automation RunTests UnrealClaude.OpenCode.Runner.Config` | ❌ Wave 0 |
| MCPI-01 | POST /mcp called with remote config on Connected state | unit | `Automation RunTests UnrealClaude.OpenCode.Runner.MCP` | ❌ Wave 0 |
| TEST-01 | ≥90% coverage for FOpenCodeRunner | unit | `Automation RunTests UnrealClaude.OpenCode.Runner` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `Automation RunTests UnrealClaude.OpenCode.Runner`
- **Per wave merge:** `Automation RunTests UnrealClaude`
- **Phase gate:** Full suite green before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `Private/Tests/OpenCodeRunnerTests.cpp` — all COMM-01/04/05/06, MCPI-01, TEST-01 coverage
- [ ] `Private/OpenCode/OpenCodeRequestConfig.h` — `FOpenCodeRequestConfig` derived struct
- [ ] `Private/OpenCode/OpenCodeRunner.h` — class declaration
- [ ] `Private/OpenCode/OpenCodeRunner.cpp` — implementation

*(No framework gaps — existing infrastructure covers all test patterns)*

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `IClaudeRunner` (Claude-only interface) | `IChatBackend` (9-method backend-agnostic interface) | Phase 2 | FOpenCodeRunner implements IChatBackend, not the old interface |
| `ClaudeSubsystem` with hardcoded Claude | `FChatSubsystem` with `TUniquePtr<IChatBackend>` | Phase 2 | Factory produces FOpenCodeRunner; subsystem unchanged |
| `FOpenCodeRunner` stub in factory (line 18) | Real `MakeUnique<FOpenCodeRunner>()` | **Phase 4 target** | Single line change activates entire pipeline |

---

## Open Questions

1. **`MapSessionError` Code Field Mismatch**
   - What we know: `OpenCodeSSEParser.cpp::MapSessionError` reads a flat `code` integer from the payload. The live API has `properties.error` as a typed union with no flat `code`.
   - What's unclear: Is the existing code silently tolerating this (no-op on missing field) or producing wrong `ErrorType` values?
   - Recommendation: In Phase 4, add a note to the OpenCodeSSEParser fix in the implementation task. The runner should fall back to `EOpenCodeErrorType::Unknown` on session.error and log `RawJson`. A parser fix can be a separate task within Phase 4 since the runner depends on correct error classification.

2. **`POST /session/:id/prompt_async` vs synchronous `/message`**
   - What we know: `/prompt_async` returns 204 immediately and delivers all results via SSE. `/message` waits for completion. D-18 says "POST /session/:id/message (or prompt_async)".
   - What's unclear: For Phase 4's FRunnable worker thread, either approach works. `prompt_async` is more natural for streaming (results flow through the already-open SSE stream). `/message` requires a long blocking POST.
   - Recommendation: Use `prompt_async` in `ExecuteAsync` — it aligns with the SSE-first design. Use `/message` only in `ExecuteSync`.

3. **Permission Dialog Widget**
   - What we know: D-05 says "similar to FScriptExecutionManager's existing pattern." The existing permission UI uses `FMessageDialog::Open` for simple yes/no.
   - What's unclear: Whether the permission description from OpenCode (title + type + metadata) needs a richer dialog than `FMessageDialog`.
   - Recommendation: Use `FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(PermissionTitle))` for Phase 4. A richer dialog is Phase 5 territory if needed.

---

## Sources

### Primary (HIGH confidence)
- Live OpenCode API documentation at `https://opencode.ai/docs/server/` — all endpoint names, request/response shapes
- Live OpenCode TypeScript SDK `types.gen.ts` — `Session`, `EventPermissionUpdated`, `EventSessionStatus`, `EventSessionError`, `McpRemoteConfig`, `TextPartInput` type shapes
- `UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeSSEParser.h/.cpp` — Phase 3 output, confirmed interface
- `UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeServerLifecycle.h` — Phase 3 output, confirmed interface
- `UnrealClaude/Source/UnrealClaude/Private/OpenCode/OpenCodeHttpClient.h` — Phase 3 output, confirmed interface
- `UnrealClaude/Source/UnrealClaude/Public/IChatBackend.h` — Phase 2 output, all 9 method signatures
- `UnrealClaude/Source/UnrealClaude/Public/ChatBackendTypes.h` — all types/enums/delegates
- `UnrealClaude/Source/UnrealClaude/Public/ClaudeCodeRunner.h` — threading pattern reference
- `UnrealClaude/Source/UnrealClaude/Private/ChatBackendFactory.cpp` — factory stub to replace (line 18)
- `UnrealClaude/Source/UnrealClaude/Private/Tests/TestUtils.h` — FMockHttpClient, FMockProcessLauncher confirmed shapes

### Secondary (MEDIUM confidence)
- `.planning/phases/04-opencode-runner-sessions/04-CONTEXT.md` — all D-01 through D-22 locked decisions
- `.planning/REQUIREMENTS.md` — COMM-01/04/05/06, MCPI-01, TEST-01 requirement text

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all components are existing Phase 3 code, confirmed interfaces
- Architecture patterns: HIGH — directly derived from FClaudeCodeRunner reference + confirmed OpenCode API
- Pitfalls: HIGH — endpoints verified against live docs; error schema mismatch confirmed by reading actual SDK types
- Open questions: MEDIUM — implementation choices within agent's discretion; both paths are viable

**Research date:** 2026-04-01
**Valid until:** 2026-05-01 (OpenCode API may change; re-verify endpoints if > 30 days pass)
