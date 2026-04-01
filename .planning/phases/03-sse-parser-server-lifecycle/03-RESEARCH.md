# Phase 3: SSE Parser & Server Lifecycle - Research

**Researched:** 2026-04-01
**Domain:** UE 5.7 C++ — Server-Sent Events parsing, HTTP streaming, FRunnable subprocess lifecycle, OpenCode REST API
**Confidence:** HIGH (core transport, API contract, UE patterns) / MEDIUM (OpenCode event exhaustiveness)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**SSE Event Mapping**
- D-01: Extend EChatStreamEventType enum with new values: minimum PermissionRequest, StatusUpdate, AgentThinking. Researcher may identify additional types.
- D-02: FOpenCodeSSEParser is a stateful parser with internal buffer — accumulates partial SSE chunks across ProcessChunk() calls, emits via FOnChatStreamEvent delegate. Mirrors FClaudeCodeRunner NdjsonLineBuffer pattern.
- D-03: Malformed SSE data emitted as FChatStreamEvent with bIsError=true and raw data in RawJson. Consumer decides display.
- D-04: Last-Event-ID tracking — **SKIP** (OpenCode SSE endpoint does not document or use Last-Event-ID).

**Server Lifecycle Behavior**
- D-05: Detection strategy: PID file first → health endpoint check. Read PID → check if process alive → hit GET /global/health → if healthy, connect. If stale/no PID, spawn.
- D-06: PID file at `<Project>/Saved/UnrealClaude/opencode-server.pid`
- D-07: Spawn failure: try ports 4096, 4097, 4098, 4099 before reporting error.
- D-08: Only kill managed servers (PID-file-tracked). User-started servers left running.
- D-09: Stale PID cleanup on startup. Process alive but health fails → kill and respawn.
- D-10: FOpenCodeServerLifecycle is a standalone class, owned by FOpenCodeRunner (Phase 4). Not a singleton.
- D-11: Periodic health polling every 10 seconds while connected.
- D-12: Lazy startup — detect/spawn on first switch to OpenCode backend.
- D-13: Port priority: config JSON → OPENCODE_PORT env var → DefaultOpenCodePort (4096).
- D-14: Graceful shutdown timeout: 10 seconds, then force-kill.

**Disconnect/Reconnect UX**
- D-15: Input blocked during SSE disconnect.
- D-16: Exponential backoff: 1s → 2s → 4s → max 30s. Reset on any success.
- D-17: Infinite retries, backoff capped at 30s.
- D-18: Mid-stream disconnect: show partial response with `[disconnected]` marker.

**SSE Transport Mechanism**
- D-19: **Use FHttpModule with SetResponseBodyReceiveStreamDelegate** for SSE. Raw FSocket NOT needed.
- D-20: FHttpModule for all non-streaming REST calls.
- D-21: SSE read loop on dedicated FRunnable thread. Events parsed on background thread, dispatched to game thread via AsyncTask(ENamedThreads::GameThread).
- D-22: Full FOpenCodeHttpClient wrapper with request/response logging, retry logic, timeout handling.
- D-23: One SSE connection per FOpenCodeRunner instance.

**Testing Strategy**
- D-24: SSE parser test data from recorded captures of real OpenCode SSE output.
- D-25: Server lifecycle tests fully mocked — mock HTTP, mock process spawning, mock file I/O.
- D-26: Coverage target ≥90% method-level for FOpenCodeSSEParser, FOpenCodeServerLifecycle, FOpenCodeHttpClient.
- D-27: Testability via interface injection — constructor takes IHttpClient and IProcessLauncher.

**Error Classification**
- D-28: EOpenCodeErrorType enum: ServerUnreachable, AuthFailure, RateLimit, BadRequest, InternalError, Unknown.
- D-29: Network-level errors surfaced as FChatStreamEvent with bIsError and typed enum.

**File Organization**
- D-30: All OpenCode-specific files in `Private/OpenCode/` subdirectory.
- D-31: Minimal public surface — IChatBackend.h, ChatBackendTypes.h, ChatBackendFactory.h only. All OpenCode internals stay Private.

**Server Spawn Arguments**
- D-32: Exact command-line flags — researcher determines (see Standard Stack section below).
- D-33: Working directory: FPaths::ProjectDir().

**Connection State Machine**
- D-34: Six states: Disconnected, Detecting, Spawning, Connecting, Connected, Reconnecting.
- D-35: FOpenCodeServerLifecycle owns the state machine and fires delegate on state changes.

**API Contract**
- D-36: Build against documented behavior.
- D-37: Target latest stable OpenCode release at research time.

### Agent's Discretion
- Additional EChatStreamEventType values beyond PermissionRequest, StatusUpdate, AgentThinking
- Internal organization of FOpenCodeHttpClient (retry counts, timeout values, logging format)
- Exact health polling interval (10s starting point)
- How to handle SSE `retry:` field from server

### Deferred Ideas (OUT OF SCOPE)
- None — discussion stayed within phase scope.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| COMM-02 | FOpenCodeSSEParser connects to `/event` SSE endpoint, parses streaming events, dispatches to chat UI in real time | SSE wire format documented; SetResponseBodyReceiveStreamDelegate confirmed; event-type mapping table provided |
| COMM-03 | SSE events mapped to existing chat UI message model (assistant text, tool calls, tool results, errors) with no UI-layer changes | Full OpenCode→FChatStreamEvent mapping table provided; new enum values identified |
| SRVR-01 | On backend switch, plugin auto-detects running `opencode serve` via GET /global/health | Health endpoint contract confirmed; detection sequence documented |
| SRVR-02 | If no server detected, spawn `opencode serve` as managed child process on configured port | FPlatformProcess::CreateProc pattern documented; exact spawn flags confirmed |
| SRVR-03 | On editor shutdown, managed process gracefully terminated (SIGTERM then force-kill) | FPlatformProcess::TerminateProc pattern documented; 10s timeout pattern |
| SRVR-04 | PID tracked in temp file to prevent orphan processes on crash recovery | PID file pattern with FFileHelper documented; Windows OutProcessID caveat documented |
| SRVR-05 | SSE connection auto-reconnects with exponential backoff (1s → 2s → 4s → max 30s) | Backoff algorithm and FRunnable timer pattern documented |
| TEST-02 | SSE parser tested against malformed streams, partial chunks, reconnect tokens, multi-event batches | Test fixture categories defined; parser unit test patterns provided |
| TEST-03 | Server lifecycle tested: spawn, health-check, reconnect, graceful shutdown, crash recovery, PID file cleanup | Mock interface pattern (IHttpClient, IProcessLauncher) documented; FRunnableThread deadlock workaround documented |
</phase_requirements>

---

## Summary

Phase 3 builds two isolated components — FOpenCodeSSEParser and FOpenCodeServerLifecycle — plus the shared FOpenCodeHttpClient utility. All three must work within UE 5.7's existing HTTP and process modules without adding external dependencies.

**Critical finding resolved:** UE 5.7's `IHttpRequest::SetResponseBodyReceiveStreamDelegate` fires per-chunk from the HTTP thread, making FHttpModule viable for SSE without raw socket code. The delegate signature is `bool(void* Ptr, int64 Length)`. Once set, `IHttpResponse::GetContent()` returns empty — all SSE data must be consumed via this delegate. Call `SetTimeout(0.0f)` to prevent UE from closing the long-lived connection.

**OpenCode API is well-documented.** The `/event` endpoint streams `GlobalEvent` objects (discriminated union on `type`) as standard SSE. The health endpoint is `GET /global/health`. Process spawn uses `opencode serve [--port N] [--hostname H]`. The abort endpoint is `POST /session/:id/abort` — NOT `/cancel` as REQUIREMENTS.md says.

**Primary recommendation:** Implement FOpenCodeSSEParser as a pure stateful parser (no FRunnable — just buffer + ProcessChunk method), driven by the HTTP stream delegate on its background thread. FOpenCodeServerLifecycle manages the full connection state machine using FPlatformProcess APIs. Both accept constructor-injected mock interfaces for ≥90% testability.

---

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| FHttpModule (IHttpRequest) | UE 5.7 built-in | SSE streaming + REST calls | Already in Build.cs; SetResponseBodyReceiveStreamDelegate confirmed for chunk streaming |
| FPlatformProcess | UE 5.7 built-in | Spawn/monitor/kill `opencode serve` subprocess | Used by FClaudeCodeRunner; same API, same platform coverage |
| FRunnable / FRunnableThread | UE 5.7 built-in | Background SSE dispatcher thread | Established pattern in FClaudeCodeRunner and FMCPTaskQueue |
| FFileHelper | UE 5.7 built-in | PID file read/write | Used throughout plugin for Saved/ file I/O |
| TSharedRef\<FJsonObject\> | UE 5.7 built-in | JSON parsing of SSE event payloads | Already in plugin's Json + JsonUtilities modules |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| AsyncTask(ENamedThreads::GameThread) | UE 5.7 built-in | Dispatch parsed events to game thread | Every SSE event callback from HTTP background thread |
| FPlatformMisc::GetEnvironmentVariable | UE 5.7 built-in | Read OPENCODE_PORT env var | Port resolution in FOpenCodeServerLifecycle |
| FPaths::ProjectDir() | UE 5.7 built-in | Working directory for spawned server | D-33 requirement |
| FTimerManager (game thread) | UE 5.7 built-in | 10-second health poll timer | Periodic health check (D-11); use GEditor->GetTimerManager() from game thread |

### No New Build.cs Dependencies Required
All required modules are already declared in `UnrealClaude.Build.cs`: `HTTP`, `HTTPServer`, `Sockets`, `Networking`, `Json`, `JsonUtilities`, `Core`, `CoreUObject`. No new `PublicDependencyModuleNames` entries needed.

---

## OpenCode API Contract

### Resolved Endpoints (HIGH confidence — from official docs + types.gen.ts)

| Endpoint | Method | Purpose | Response |
|----------|--------|---------|----------|
| `/global/health` | GET | Health detection (SRVR-01) | `{ healthy: true, version: string }` |
| `/event` | GET | SSE stream — session events | `GlobalEvent` NDJSON lines wrapped in SSE |
| `/global/event` | GET | SSE stream — all sessions (wraps with `directory` field) | `GlobalEvent` objects |
| `/session` | POST | Create session (Phase 4) | Session object |
| `/session/:id/message` | POST | Send message (Phase 4) | — |
| `/session/:id/abort` | POST | Cancel in-flight request | — |

> ⚠️ **Abort endpoint:** REQUIREMENTS.md (CHUI-03) says `POST /session/:id/cancel` but the actual OpenCode API uses `POST /session/:id/abort`. Use `/abort`.

### Server Spawn Command (D-32 resolved — HIGH confidence)

```
opencode serve [--port <number>] [--hostname <string>] [--cors <origin>] [--mdns] [--mdns-domain <string>]
```

**Recommended spawn invocation:**
```
opencode serve --port 4096 --hostname 127.0.0.1
```

- Default port: **4096**
- Default hostname: **127.0.0.1** (localhost only — correct for local editor use)
- No `--version` flag; version is retrieved from health endpoint response

### SSE Wire Format (standard SSE per RFC)

```
data: {"type":"message.part.updated","properties":{...}}\n\n
```

Or with event field:
```
event: message.part.updated\ndata: {...}\n\n
```

Rules for the parser:
- Lines starting with `data:` carry the JSON payload (strip `data: ` prefix)
- Lines starting with `event:` carry the event name (may or may not match `type` inside JSON)
- Lines starting with `:` are comments — ignore
- Lines starting with `retry:` carry reconnect interval in ms — update backoff if present
- Blank line `\n\n` terminates an event — emit accumulated fields
- All fields accumulate until blank line; JSON in `data:` may NOT itself contain bare newlines

### GlobalEvent Shape

```typescript
// From OpenCode types.gen.ts (verified)
type GlobalEvent = {
  directory: string;   // project directory the event belongs to
  payload: Event;      // discriminated union on payload.type
}
```

### Critical Event Types → FChatStreamEvent Mapping

| OpenCode `type` | Trigger | Maps To (EChatStreamEventType) | Key Fields |
|-----------------|---------|-------------------------------|------------|
| `server.connected` | First event on SSE connect | `SessionInit` | — |
| `message.part.updated` with TextPart | Streaming assistant text | `TextContent` | `part.content` + `delta` |
| `message.part.updated` with ToolPart | Tool call initiated/updated | `ToolUse` | `part.callID`, `part.tool`, `part.state` |
| `message.part.updated` with ToolPart (state=completed) | Tool result received | `ToolResult` | `part.output` |
| `message.part.updated` with StepStartPart/StepFinishPart | Agent thinking/step lifecycle | `AgentThinking` (**NEW**) | step metadata |
| `message.part.updated` with ReasoningPart | Model reasoning | `AgentThinking` (**NEW**) | reasoning text |
| `message.updated` | Final message state | `AssistantMessage` | full message object |
| `permission.updated` | Permission request from server | `PermissionRequest` (**NEW**) | permission object |
| `session.status` | Session idle/busy/retry state | `StatusUpdate` (**NEW**) | `status: "idle"\|"retry"\|"busy"` |
| `session.error` | Session-level error | maps to `bIsError=true` + `EOpenCodeErrorType` | typed error |

### New EChatStreamEventType Values Required (D-01)

```cpp
// Add to EChatStreamEventType in ChatBackendTypes.h:
PermissionRequest,   // from permission.updated
StatusUpdate,        // from session.status (idle/busy/retry)
AgentThinking,       // from step-start, step-finish, reasoning parts
```

`ConnectionEstablished` is NOT needed as a new type — reuse `SessionInit` for `server.connected`.

### New EOpenCodeErrorType Enum Required (D-28)

```cpp
// Add to ChatBackendTypes.h:
enum class EOpenCodeErrorType : uint8
{
    None,               // Not an error
    ServerUnreachable,  // Connection refused / timeout / DNS failure
    AuthFailure,        // 401/403 from OpenCode
    RateLimit,          // 429 from OpenCode
    BadRequest,         // 400 — malformed request
    InternalError,      // 500 from OpenCode
    Unknown             // Unclassified error
};
```

Add `EOpenCodeErrorType ErrorType = EOpenCodeErrorType::None;` to `FChatStreamEvent`.

### New Constants Required in UnrealClaudeConstants.h

```cpp
namespace Backend
{
    // Existing:
    constexpr uint32 DefaultOpenCodePort = 4096;  // Already present

    // New constants to add:
    constexpr float HealthPollIntervalSeconds = 10.0f;
    constexpr float GracefulShutdownTimeoutSeconds = 10.0f;
    constexpr float SSEReconnectBackoffInitialSeconds = 1.0f;
    constexpr float SSEReconnectBackoffMaxSeconds = 30.0f;
    constexpr int32 MaxPortRetryCount = 4;  // 4096-4099
    constexpr const TCHAR* OpenCodePidFilename = TEXT("opencode-server.pid");
    constexpr const TCHAR* OpenCodePortEnvVar = TEXT("OPENCODE_PORT");
}
```

---

## Architecture Patterns

### Recommended Project Structure

```
Private/OpenCode/
├── OpenCodeSSEParser.h         # Stateful SSE parser (no FRunnable — pure buffer/callback)
├── OpenCodeSSEParser.cpp
├── OpenCodeServerLifecycle.h   # State machine: Detect → Spawn → Connect → Reconnect
├── OpenCodeServerLifecycle.cpp
├── OpenCodeHttpClient.h        # IHttpClient interface + FOpenCodeHttpClient impl
├── OpenCodeHttpClient.cpp
├── IProcessLauncher.h          # IProcessLauncher interface for test injection
└── OpenCodeTypes.h             # EOpenCodeConnectionState, any OpenCode-only types

Private/Tests/
├── OpenCodeSSEParserTests.cpp  # Parser unit tests (no threading)
├── OpenCodeServerLifecycleTests.cpp  # Lifecycle tests (mocked IHttpClient + IProcessLauncher)
└── OpenCodeHttpClientTests.cpp # HTTP client tests
```

### Pattern 1: SSE Streaming with SetResponseBodyReceiveStreamDelegate

**What:** Use IHttpRequest's streaming delegate to receive SSE chunks on the HTTP thread. Pass chunks directly to FOpenCodeSSEParser. Dispatch complete events to game thread.

**When to use:** The only correct approach for SSE in UE 5.7 without raw sockets.

```cpp
// Source: UE 5.7 IHttpRequest.h (confirmed in UE 5.3 — same API)
TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
Request->SetURL(TEXT("http://127.0.0.1:4096/event"));
Request->SetVerb(TEXT("GET"));
Request->SetTimeout(0.0f);  // CRITICAL: prevent UE from closing long-lived connection

// Stream delegate fires per-chunk on HTTP thread
Request->SetResponseBodyReceiveStreamDelegate(
    FHttpRequestStreamDelegate::CreateLambda(
        [this](void* Ptr, int64 Length) -> bool
        {
            // Called on HTTP thread — NOT game thread
            FString Chunk = FString(Length, (const UTF8CHAR*)Ptr);
            Parser.ProcessChunk(Chunk);  // Parser emits events via AsyncTask internally
            return true;  // return false to abort
        }
    )
);
Request->ProcessRequest();
```

> ⚠️ **After SetResponseBodyReceiveStreamDelegate is set, `IHttpResponse::GetContent()` returns empty.** All data must be consumed via the delegate.

> ⚠️ **Chunk encoding:** The `void* Ptr` buffer is raw bytes from the HTTP response body — may be UTF-8. Cast to `UTF8CHAR*` and convert via `FString(Length, (const UTF8CHAR*)Ptr)`.

### Pattern 2: FOpenCodeSSEParser — Stateful Buffer

**What:** Pure stateful parser — no FRunnable. Takes raw chunk bytes, accumulates in FString buffer, splits on `\n\n`, parses SSE fields, emits FChatStreamEvent via delegate.

**When to use:** Called from HTTP stream delegate on background thread. Fire-and-forget per chunk.

```cpp
// OpenCodeSSEParser.h (prescriptive interface)
class FOpenCodeSSEParser
{
public:
    /** Delegate fires once per complete SSE event. Called from HTTP thread. */
    FOnChatStreamEvent OnEvent;

    /** Feed raw bytes from HTTP stream delegate. Thread-safe if OnEvent dispatches via AsyncTask. */
    void ProcessChunk(const FString& RawChunk);

    /** Reset buffer state (e.g. on reconnect). */
    void Reset();

private:
    FString Buffer;          // Mirrors FClaudeCodeRunner::NdjsonLineBuffer pattern
    FString CurrentEventName; // From "event:" field
    FString CurrentData;     // Accumulated "data:" content
    uint32  CurrentRetryMs = 0; // From "retry:" field

    void FlushEvent();
    FChatStreamEvent ParseEventPayload(const FString& EventName, const FString& Data);
    EChatStreamEventType MapEventType(const FString& OcType, const TSharedPtr<FJsonObject>& Payload);
};
```

**ProcessChunk algorithm:**
```
Append RawChunk to Buffer
While Buffer contains "\n":
  Extract line up to first "\n"
  Remove line + "\n" from Buffer
  If line is empty:
    FlushEvent()  (blank line = event boundary)
  Else if line starts with "data:":
    Append stripped data to CurrentData (with "\n" if CurrentData non-empty)
  Else if line starts with "event:":
    CurrentEventName = stripped value
  Else if line starts with "retry:":
    Parse ms value, store CurrentRetryMs
  Else if line starts with ":":
    // comment — ignore
```

### Pattern 3: FOpenCodeServerLifecycle — State Machine

**What:** Six-state enum drives all lifecycle transitions. Each state has entry/exit logic. Health poll timer runs in Connected state.

**State transitions:**
```
Disconnected → [EnsureServerReady()] → Detecting
Detecting → [PID found, process alive, health ok] → Connecting
Detecting → [PID stale OR no PID] → Spawning
Spawning → [spawn success] → Connecting
Spawning → [all ports exhausted] → Disconnected (error)
Connecting → [SSE established] → Connected
Connecting → [SSE failed] → Reconnecting
Connected → [health poll fails] → Reconnecting
Connected → [SSE disconnect] → Reconnecting
Reconnecting → [backoff expired] → Connecting (retry SSE)
```

```cpp
enum class EOpenCodeConnectionState : uint8
{
    Disconnected,
    Detecting,
    Spawning,
    Connecting,
    Connected,
    Reconnecting
};

DECLARE_DELEGATE_OneParam(FOnConnectionStateChanged, EOpenCodeConnectionState);
```

### Pattern 4: Interface Injection for Testability (D-27)

**What:** FOpenCodeServerLifecycle takes IHttpClient and IProcessLauncher in constructor. Tests inject synchronous mocks. Production injects real implementations.

```cpp
// IProcessLauncher.h
class IProcessLauncher
{
public:
    virtual ~IProcessLauncher() = default;
    virtual FProcHandle LaunchProcess(
        const FString& Url, const FString* Args, int32 ArgCount,
        bool bLaunchDetached, bool bLaunchHidden, uint32* OutProcessID) = 0;
    virtual bool IsProcRunning(FProcHandle& Handle) = 0;
    virtual void TerminateProc(FProcHandle& Handle, bool bKillTree) = 0;
    virtual bool IsApplicationRunning(uint32 PID) = 0;
};

// IHttpClient.h (in OpenCodeHttpClient.h)
class IHttpClient
{
public:
    virtual ~IHttpClient() = default;
    /** Synchronous GET — returns HTTP response code and body. */
    virtual int32 Get(const FString& Url, FString& OutBody) = 0;
    /** Synchronous POST — returns HTTP response code. */
    virtual int32 Post(const FString& Url, const FString& Body, FString& OutResponse) = 0;
    /** Begin SSE stream. StreamDelegate fires per-chunk. */
    virtual void BeginSSEStream(const FString& Url,
        FHttpRequestStreamDelegate StreamDelegate,
        FSimpleDelegate OnDisconnected) = 0;
    virtual void AbortSSEStream() = 0;
};
```

### Pattern 5: PID File Management

**What:** Write PID to file at spawn time. Read on startup to detect orphan recovery. Use FFileHelper for all I/O.

**Windows caveat:** `FProcHandle` does not expose an integer PID. Capture via `OutProcessID` parameter of `FPlatformProcess::CreateProc`.

```cpp
// Write PID file after successful spawn
uint32 OutPID = 0;
FProcHandle Handle = FPlatformProcess::CreateProc(
    *OpenCodeExePath, *Args, /*bLaunchDetached=*/true,
    /*bLaunchHidden=*/true, /*bLaunchReallyHidden=*/false,
    &OutPID,  // CRITICAL: capture PID here on Windows
    0, *FPaths::ProjectDir(), nullptr
);
FString PidFilePath = FPaths::ProjectDir() / TEXT("Saved/UnrealClaude/opencode-server.pid");
FFileHelper::SaveStringToFile(FString::FromInt((int32)OutPID), *PidFilePath);

// Read PID on startup (crash recovery)
FString PidStr;
if (FFileHelper::LoadFileToString(PidStr, *PidFilePath))
{
    uint32 SavedPID = (uint32)FCString::Atoi(*PidStr);
    if (FPlatformProcess::IsApplicationRunning(SavedPID))
    {
        // Process still alive — check health endpoint before connecting
    }
    else
    {
        // Stale PID — delete file, spawn fresh
        IFileManager::Get().Delete(*PidFilePath);
    }
}
```

### Pattern 6: Exponential Backoff on FRunnable

**What:** Background reconnect thread sleeps with backoff, attempts reconnect, doubles delay up to max.

```cpp
// In FOpenCodeServerLifecycle reconnect loop (FRunnable::Run())
float BackoffSeconds = UnrealClaudeConstants::Backend::SSEReconnectBackoffInitialSeconds; // 1.0f
while (bReconnecting)
{
    FPlatformProcess::Sleep(BackoffSeconds);
    if (TryReconnect())
    {
        BackoffSeconds = UnrealClaudeConstants::Backend::SSEReconnectBackoffInitialSeconds; // reset
        break;
    }
    BackoffSeconds = FMath::Min(BackoffSeconds * 2.0f,
                                UnrealClaudeConstants::Backend::SSEReconnectBackoffMaxSeconds); // cap 30s
}
```

### Anti-Patterns to Avoid

- **Calling GetContent() after SetResponseBodyReceiveStreamDelegate:** Returns empty. All SSE data must come through the stream delegate.
- **Blocking game thread for HTTP:** All HTTP calls must use async pattern or run on background FRunnable. Never `FPlatformProcess::Sleep` on game thread.
- **Storing FProcHandle across module reload:** FProcHandle is a raw OS handle. Serialize PID to file instead for crash recovery.
- **Directly calling AsyncTask from test:** FRunnableThread + AsyncTask(GameThread) deadlocks in UE automation test context (documented in TESTING.md:115). Use mock IHttpClient that executes synchronously.
- **Single-point port bind:** Always try port range 4096–4099 before reporting spawn failure (D-07).
- **Forgetting SetTimeout(0.0f):** UE's default HTTP timeout will close long-lived SSE connections.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| HTTP streaming | Custom FSocket read loop | `IHttpRequest::SetResponseBodyReceiveStreamDelegate` | Already in FHttpModule; handles TLS, redirects, chunked encoding |
| JSON parsing | Manual string parsing of SSE payload | `FJsonSerializer::Deserialize` + `TSharedRef<FJsonObject>` | Handles escaping, Unicode, nested objects; already in Json module |
| Background thread | Custom thread wrapper | `FRunnable` + `FRunnableThread::Create` | UE-managed thread lifecycle; already established in FClaudeCodeRunner |
| Game thread dispatch | Thread-unsafe direct callback | `AsyncTask(ENamedThreads::GameThread, ...)` | Safe cross-thread dispatch; already used throughout plugin |
| Process management | Platform-specific spawn/kill | `FPlatformProcess::CreateProc` + `TerminateProc` | Cross-platform (Win64/Linux/Mac); same API FClaudeCodeRunner uses |
| File I/O for PID | Platform-specific file write | `FFileHelper::SaveStringToFile` / `LoadFileToString` | Cross-platform; UTF-8 safe; already used for session.json |
| Timer/polling | Manual sleep loop on game thread | `FTimerManager` (GEditor->GetTimerManager()) | Integrates with UE tick; cancellable; no busy-wait |

**Key insight:** Every required capability already exists in UE 5.7's built-in modules, all of which are already declared in Build.cs. Phase 3 is pure glue code — no new library surface.

---

## Common Pitfalls

### Pitfall 1: GetContent() Returns Empty After Streaming
**What goes wrong:** Developer calls `Response->GetContent()` in `OnProcessRequestComplete` to get SSE body, gets empty array.
**Why it happens:** Once `SetResponseBodyReceiveStreamDelegate` is set, UE stops buffering the body. All data was delivered via the stream delegate.
**How to avoid:** Consume all SSE data in the stream delegate. In `OnProcessRequestComplete`, only inspect the status code.
**Warning signs:** Empty `GetContentAsString()` despite confirmed SSE traffic.

### Pitfall 2: FRunnableThread Deadlock in Automation Tests
**What goes wrong:** Tests that instantiate FOpenCodeServerLifecycle and trigger reconnect logic hang indefinitely.
**Why it happens:** `AsyncTask(ENamedThreads::GameThread, ...)` waits for game thread, but game thread is blocked running the test. Documented in TESTING.md:115.
**How to avoid:** Inject `IHttpClient` mock that invokes the stream delegate synchronously (no FRunnable). Test the parser and state machine logic without real threading.
**Warning signs:** Test hangs, never reaches `return true`.

### Pitfall 3: Forgetting SetTimeout(0.0f) on SSE Request
**What goes wrong:** SSE connection is closed by UE after its default timeout (typically 30–300s), triggering unnecessary reconnect loops.
**Why it happens:** UE's HTTP module applies a timeout to all requests by default. SSE connections are intentionally indefinite.
**How to avoid:** Always call `Request->SetTimeout(0.0f)` before `ProcessRequest()` for SSE requests.
**Warning signs:** Periodic disconnects at regular intervals; backoff resets unnecessarily.

### Pitfall 4: Windows FProcHandle PID Extraction
**What goes wrong:** PID file contains 0 on Windows; crash recovery always spawns a new server, leaving orphans.
**Why it happens:** `FProcHandle` on Windows does not expose an integer PID through public API. Developers assume handle == PID.
**How to avoid:** Always pass a `uint32* OutProcessID` to `FPlatformProcess::CreateProc` and use that value for the PID file.
**Warning signs:** PID file contains "0"; `FPlatformProcess::IsApplicationRunning(0)` returns unexpected results.

### Pitfall 5: SSE Chunk Boundaries Split Multi-Byte UTF-8
**What goes wrong:** Parser emits malformed JSON because a multi-byte Unicode codepoint is split across two chunks.
**Why it happens:** HTTP chunks are arbitrary byte boundaries; a 3-byte UTF-8 sequence can be split 1+2 bytes across chunks.
**How to avoid:** Buffer at the `FString` level (after UTF-8 → FString conversion). UE's `FString(Length, (UTF8CHAR*)Ptr)` constructor handles incomplete sequences internally. Alternatively, buffer raw bytes and only convert after a complete `\n\n` boundary is detected.
**Warning signs:** JSON parse errors on events with non-ASCII content.

### Pitfall 6: Port Exhaustion Without Proper Error Propagation
**What goes wrong:** All four ports (4096–4099) fail, but lifecycle transitions to Connecting anyway and silently fails later.
**Why it happens:** Port retry loop exits without setting error state.
**How to avoid:** If all ports exhausted, transition to `Disconnected` with a `FChatStreamEvent` containing `bIsError=true`, `ErrorType=EOpenCodeErrorType::ServerUnreachable`.
**Warning signs:** State machine shows `Connecting` but no SSE events arrive; no error displayed to user.

### Pitfall 7: Abort Endpoint Name Mismatch
**What goes wrong:** POST to `/session/:id/cancel` returns 404; in-flight request is never cancelled.
**Why it happens:** REQUIREMENTS.md says "cancel" but OpenCode API uses "abort".
**How to avoid:** Use `POST /session/:id/abort`. Note this in code with a comment referencing the discrepancy.
**Warning signs:** 404 response when cancelling; user sees no cancellation feedback.

---

## Code Examples

### SSE Stream Setup (FOpenCodeHttpClient)
```cpp
// Source: UE 5.7 IHttpRequest.h — SetResponseBodyReceiveStreamDelegate
// Called from game thread; delegate fires on HTTP thread

void FOpenCodeHttpClient::BeginSSEStream(
    const FString& Url,
    FHttpRequestStreamDelegate StreamDelegate,
    FSimpleDelegate OnDisconnected)
{
    SSERequest = FHttpModule::Get().CreateRequest();
    SSERequest->SetURL(Url);
    SSERequest->SetVerb(TEXT("GET"));
    SSERequest->SetHeader(TEXT("Accept"), TEXT("text/event-stream"));
    SSERequest->SetHeader(TEXT("Cache-Control"), TEXT("no-cache"));
    SSERequest->SetTimeout(0.0f);  // Never timeout SSE connections
    SSERequest->SetResponseBodyReceiveStreamDelegate(StreamDelegate);
    SSERequest->OnProcessRequestComplete().BindLambda(
        [OnDisconnected](FHttpRequestPtr, FHttpResponsePtr, bool)
        {
            // Connection closed — fire disconnect on game thread
            AsyncTask(ENamedThreads::GameThread,
                [OnDisconnected]() { OnDisconnected.ExecuteIfBound(); });
        });
    SSERequest->ProcessRequest();
}
```

### SSE Parser — ProcessChunk Core Logic
```cpp
// Source: Adapted from FClaudeCodeRunner::ParseAndEmitNdjsonLine pattern
// Called from HTTP thread; dispatches events via AsyncTask

void FOpenCodeSSEParser::ProcessChunk(const FString& RawChunk)
{
    Buffer += RawChunk;

    int32 LineEnd;
    while (Buffer.FindChar(TEXT('\n'), LineEnd))
    {
        FString Line = Buffer.Left(LineEnd);
        Buffer.RemoveAt(0, LineEnd + 1);

        if (Line.IsEmpty())
        {
            // Blank line = event boundary
            FlushEvent();
        }
        else if (Line.StartsWith(TEXT("data:")))
        {
            FString Data = Line.Mid(5).TrimStart();
            if (!CurrentData.IsEmpty()) CurrentData += TEXT("\n");
            CurrentData += Data;
        }
        else if (Line.StartsWith(TEXT("event:")))
        {
            CurrentEventName = Line.Mid(6).TrimStart();
        }
        else if (Line.StartsWith(TEXT("retry:")))
        {
            CurrentRetryMs = (uint32)FCString::Atoi(*Line.Mid(6).TrimStart());
        }
        // Lines starting with ":" are comments — skip
    }
}

void FOpenCodeSSEParser::FlushEvent()
{
    if (CurrentData.IsEmpty()) { CurrentEventName.Reset(); return; }
    FChatStreamEvent Event = ParseEventPayload(CurrentEventName, CurrentData);
    CurrentData.Reset();
    CurrentEventName.Reset();

    // Dispatch to game thread
    FOnChatStreamEvent LocalDelegate = OnEvent;
    AsyncTask(ENamedThreads::GameThread,
        [LocalDelegate, Event]() { LocalDelegate.ExecuteIfBound(Event); });
}
```

### Health Check (FOpenCodeHttpClient)
```cpp
// Synchronous health check — call from background thread only
bool FOpenCodeHttpClient::CheckHealth(const FString& BaseUrl, FString& OutVersion)
{
    FString Body;
    int32 Code = Get(BaseUrl / TEXT("global/health"), Body);
    if (Code != 200) return false;

    TSharedPtr<FJsonObject> Json;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
    if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid()) return false;

    bool bHealthy = false;
    Json->TryGetBoolField(TEXT("healthy"), bHealthy);
    Json->TryGetStringField(TEXT("version"), OutVersion);
    return bHealthy;
}
```

### Process Spawn with Port Retry (FOpenCodeServerLifecycle)
```cpp
// Source: FPlatformProcess::CreateProc pattern from FClaudeCodeRunner
bool FOpenCodeServerLifecycle::TrySpawnServer(uint32& OutPort)
{
    const uint32 BasePort = ResolvePort(); // config → env var → DefaultOpenCodePort
    const int32 MaxRetry = UnrealClaudeConstants::Backend::MaxPortRetryCount; // 4

    for (int32 i = 0; i < MaxRetry; ++i)
    {
        uint32 TryPort = BasePort + (uint32)i;
        FString Args = FString::Printf(TEXT("serve --port %u --hostname 127.0.0.1"), TryPort);
        uint32 OutPID = 0;

        FProcHandle Handle = ProcessLauncher->LaunchProcess(
            *OpenCodePath, *Args, true, true, false, &OutPID, 0,
            *FPaths::ProjectDir(), nullptr);

        if (Handle.IsValid())
        {
            ManagedHandle = Handle;
            ManagedPID = OutPID;
            OutPort = TryPort;

            // Write PID file
            FString PidPath = GetPidFilePath();
            FFileHelper::SaveStringToFile(FString::FromInt((int32)OutPID), *PidPath);
            return true;
        }
    }
    return false; // All ports exhausted
}
```

### Event Type Mapping (SSE payload → EChatStreamEventType)
```cpp
// Source: OpenCode types.gen.ts — verified event type strings
FChatStreamEvent FOpenCodeSSEParser::ParseEventPayload(
    const FString& EventName, const FString& DataJson)
{
    FChatStreamEvent Event;
    Event.RawJson = DataJson;

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DataJson);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        Event.bIsError = true;
        Event.Type = EChatStreamEventType::Unknown;
        return Event;
    }

    // OpenCode wraps payload in { directory, payload: { type, ... } }
    TSharedPtr<FJsonObject> Payload = Root->GetObjectField(TEXT("payload"));
    if (!Payload.IsValid()) Payload = Root; // fallback: some events are flat

    FString TypeStr;
    Payload->TryGetStringField(TEXT("type"), TypeStr);

    if (TypeStr == TEXT("server.connected"))
        Event.Type = EChatStreamEventType::SessionInit;
    else if (TypeStr == TEXT("message.part.updated"))
        Event = MapMessagePartUpdated(Payload);
    else if (TypeStr == TEXT("message.updated"))
        Event.Type = EChatStreamEventType::AssistantMessage;
    else if (TypeStr == TEXT("permission.updated"))
        Event.Type = EChatStreamEventType::PermissionRequest;
    else if (TypeStr == TEXT("session.status"))
        Event = MapSessionStatus(Payload);
    else if (TypeStr == TEXT("session.error"))
        { Event.bIsError = true; Event.Type = EChatStreamEventType::Unknown; }
    else
        Event.Type = EChatStreamEventType::Unknown;

    return Event;
}
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Raw FSocket for SSE | `IHttpRequest::SetResponseBodyReceiveStreamDelegate` | UE 5.3+ | No socket management code needed; handles TLS automatically |
| FClaudeCodeRunner::NdjsonLineBuffer (manual) | Same pattern reused for SSE buffer | Already in codebase | Proven, tested — direct copy with SSE field parsing added |
| Singleton subsystem | Non-singleton FOpenCodeServerLifecycle (D-10) | Phase 3 decision | Better testability, cleaner ownership, no global state |

**Not deprecated:** `FPlatformProcess::CreateProc` remains the standard cross-platform subprocess API in UE 5.7. No alternatives.

---

## Open Questions

1. **SSE `event:` field vs JSON `type` field**
   - What we know: OpenCode docs show SSE events have `data:` field with JSON containing `type`. The `event:` SSE field may duplicate this.
   - What's unclear: Whether OpenCode always sets the SSE `event:` header or only the JSON `type`. The parser should prefer the JSON `type` field and use SSE `event:` only as fallback.
   - Recommendation: Parse both; JSON `type` wins. Log a warning if they differ.

2. **`message.part.updated` with multiple simultaneous part types**
   - What we know: A single `message.part.updated` carries one `Part` object.
   - What's unclear: Whether a batch event can contain multiple parts, or if each part update is a separate SSE event.
   - Recommendation: Treat each SSE event as a single-part update per the schema. If a batch variant exists (not documented), emit one FChatStreamEvent per part.

3. **Auth mechanism for OpenCode server**
   - What we know: `GET /global/health` is unauthenticated. Local server defaults to 127.0.0.1.
   - What's unclear: Whether any OpenCode endpoints require auth tokens when running locally.
   - Recommendation: Start with no auth headers. Map 401/403 responses to `EOpenCodeErrorType::AuthFailure`. Phase 4 can add auth header injection if needed.

---

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| UE HTTP Module | SSE + REST | ✓ (Build.cs) | UE 5.7 built-in | — |
| UE Json Module | SSE parsing | ✓ (Build.cs) | UE 5.7 built-in | — |
| FPlatformProcess | Process spawn/kill | ✓ (Core) | UE 5.7 built-in | — |
| opencode CLI | Runtime server spawn | ✗ (user-installed) | — | Tests mock; runtime docs user to install |
| FFileHelper | PID file I/O | ✓ (Core) | UE 5.7 built-in | — |

**Missing dependencies with no fallback:**
- `opencode` CLI binary — required at runtime for SRVR-02. Not bundled; user must install. Tests are fully mocked (IProcessLauncher) so tests do not require it.

**Missing dependencies with fallback:**
- None — all build-time dependencies are available.

---

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | UE Automation Test Framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`) |
| Config file | None — run from editor console: `Automation RunTests UnrealClaude` |
| Quick run command | `Automation RunTests UnrealClaude.OpenCode` |
| Full suite command | `Automation RunTests UnrealClaude` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| COMM-02 | ProcessChunk accumulates partial SSE chunks | unit | `Automation RunTests UnrealClaude.OpenCode.SSEParser` | ❌ Wave 0 |
| COMM-02 | Blank-line boundary emits complete event | unit | `Automation RunTests UnrealClaude.OpenCode.SSEParser` | ❌ Wave 0 |
| COMM-03 | `message.part.updated` TextPart → TextContent | unit | `Automation RunTests UnrealClaude.OpenCode.SSEParser` | ❌ Wave 0 |
| COMM-03 | `permission.updated` → PermissionRequest | unit | `Automation RunTests UnrealClaude.OpenCode.SSEParser` | ❌ Wave 0 |
| COMM-03 | Malformed JSON → bIsError=true | unit | `Automation RunTests UnrealClaude.OpenCode.SSEParser` | ❌ Wave 0 |
| SRVR-01 | PID file found + process alive + health ok → Connected | unit (mocked) | `Automation RunTests UnrealClaude.OpenCode.ServerLifecycle` | ❌ Wave 0 |
| SRVR-01 | No PID file → proceeds to Spawning | unit (mocked) | `Automation RunTests UnrealClaude.OpenCode.ServerLifecycle` | ❌ Wave 0 |
| SRVR-02 | Port 4096 busy → retry 4097 | unit (mocked) | `Automation RunTests UnrealClaude.OpenCode.ServerLifecycle` | ❌ Wave 0 |
| SRVR-02 | All 4 ports fail → Disconnected + error event | unit (mocked) | `Automation RunTests UnrealClaude.OpenCode.ServerLifecycle` | ❌ Wave 0 |
| SRVR-03 | Shutdown: TerminateProc called within 10s | unit (mocked) | `Automation RunTests UnrealClaude.OpenCode.ServerLifecycle` | ❌ Wave 0 |
| SRVR-04 | PID written to file after spawn | unit (mocked) | `Automation RunTests UnrealClaude.OpenCode.ServerLifecycle` | ❌ Wave 0 |
| SRVR-04 | Stale PID file cleaned up on startup | unit (mocked) | `Automation RunTests UnrealClaude.OpenCode.ServerLifecycle` | ❌ Wave 0 |
| SRVR-05 | Backoff doubles: 1s → 2s → 4s → 8s → 16s → 30s cap | unit | `Automation RunTests UnrealClaude.OpenCode.ServerLifecycle` | ❌ Wave 0 |
| SRVR-05 | Backoff resets to 1s on successful reconnect | unit | `Automation RunTests UnrealClaude.OpenCode.ServerLifecycle` | ❌ Wave 0 |
| TEST-02 | Multi-event batch in single chunk | unit | `Automation RunTests UnrealClaude.OpenCode.SSEParser` | ❌ Wave 0 |
| TEST-02 | Event split across two chunks | unit | `Automation RunTests UnrealClaude.OpenCode.SSEParser` | ❌ Wave 0 |
| TEST-02 | `retry:` field updates backoff | unit | `Automation RunTests UnrealClaude.OpenCode.SSEParser` | ❌ Wave 0 |
| TEST-03 | Process crash (IsProcRunning=false) → clean up + respawn | unit (mocked) | `Automation RunTests UnrealClaude.OpenCode.ServerLifecycle` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `Automation RunTests UnrealClaude.OpenCode`
- **Per wave merge:** `Automation RunTests UnrealClaude`
- **Phase gate:** Full suite green before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `Private/Tests/OpenCodeSSEParserTests.cpp` — covers COMM-02, COMM-03, TEST-02
- [ ] `Private/Tests/OpenCodeServerLifecycleTests.cpp` — covers SRVR-01 through SRVR-05, TEST-03
- [ ] `Private/Tests/OpenCodeHttpClientTests.cpp` — covers FOpenCodeHttpClient retry + logging
- [ ] `Private/OpenCode/IProcessLauncher.h` — mock interface (add FMockProcessLauncher to TestUtils.h)
- [ ] `Private/OpenCode/OpenCodeHttpClient.h` — IHttpClient interface (add FMockHttpClient to TestUtils.h)

---

## Sources

### Primary (HIGH confidence)
- UE 5.3 `IHttpRequest.h` — `SetResponseBodyReceiveStreamDelegate` signature, `SetTimeout`, delegate type
- `https://opencode.ai/docs/server/` — endpoint paths, health contract, spawn flags
- `https://raw.githubusercontent.com/anomalyco/opencode/dev/packages/sdk/js/src/gen/types.gen.ts` — complete TypeScript discriminated union definitions for all OpenCode SSE event types
- `UnrealClaude/Source/UnrealClaude/Public/ChatBackendTypes.h` — existing EChatStreamEventType enum, FChatStreamEvent fields to extend
- `UnrealClaude/Source/UnrealClaude/Public/UnrealClaudeConstants.h` — existing Backend namespace, constants to add
- `UnrealClaude/Source/UnrealClaude/Private/ClaudeCodeRunner.cpp` — FPlatformProcess::CreateProc pattern, NdjsonLineBuffer pattern, AsyncTask dispatch pattern

### Secondary (MEDIUM confidence)
- `https://opencode.ai/docs/sdk/` — SDK type overview (cross-referenced with types.gen.ts)
- `.planning/codebase/TESTING.md` line 115 — FRunnableThread deadlock in automation test context

### Tertiary (LOW confidence)
- OpenCode SSE `event:` field behavior — not explicitly documented; inferred from standard SSE RFC + types.gen.ts structure. Flag for validation against live instance.

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all UE APIs verified in engine source headers
- Architecture patterns: HIGH — directly derived from existing FClaudeCodeRunner patterns in codebase
- OpenCode API contract: HIGH for endpoints, MEDIUM for exhaustiveness of event types
- Pitfalls: HIGH — each sourced from either UE docs, codebase TESTING.md, or verified behavior
- Test map: HIGH — maps directly to locked requirements

**Research date:** 2026-04-01
**Valid until:** 2026-05-01 (OpenCode API may change; verify types.gen.ts against current release before implementation)
