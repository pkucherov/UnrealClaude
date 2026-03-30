# Architecture Patterns

**Domain:** OpenCode HTTP backend integration into existing UE5.7 AI chat plugin
**Researched:** 2026-03-30

## Recommended Architecture

The integration adds OpenCode as a second backend behind a generalized runner interface. The key insight from reading the codebase: the existing architecture is almost ready for multi-backend — `IClaudeRunner` already abstracts execution — but three coupling points need refactoring, and three entirely new components are needed.

### High-Level Component Diagram

```
┌─────────────────────────────────────────────────────────┐
│  SClaudeEditorWidget (Chat UI)                          │
│  ┌─────────────┐  ┌──────────────────┐                  │
│  │SClaudeToolbar│  │SClaudeInputArea  │                  │
│  │+ Backend     │  │                  │                  │
│  │  Switcher    │  │                  │                  │
│  └──────┬───────┘  └────────┬─────────┘                  │
│         └────────┬──────────┘                            │
│                  ▼                                       │
│         FClaudeCodeSubsystem (Orchestrator)              │
│         ┌──────────────────────────────┐                 │
│         │ - ActiveRunner: IClaudeRunner*│                │
│         │ - Runners: TMap<EBackend,...> │                │
│         │ - SessionManager             │                │
│         │ - BackendState               │                │
│         └──────────┬───────────────────┘                │
│                    │ delegates to ActiveRunner           │
│         ┌──────────┴───────────┐                        │
│         ▼                      ▼                         │
│  ┌──────────────┐   ┌───────────────────┐               │
│  │FClaudeCode   │   │FOpenCodeRunner    │               │
│  │Runner        │   │: IClaudeRunner    │               │
│  │(subprocess)  │   │(HTTP+SSE client)  │               │
│  │              │   │                   │               │
│  │claude -p     │   │  ┌──────────────┐ │               │
│  │--stream-json │   │  │FOpenCodeSSE  │ │               │
│  │   ▼ NDJSON   │   │  │EventParser   │ │               │
│  └──────────────┘   │  └──────────────┘ │               │
│                     │  ┌──────────────┐ │               │
│                     │  │FOpenCodeServer│ │               │
│                     │  │Lifecycle     │ │               │
│                     │  └──────────────┘ │               │
│                     └───────────────────┘               │
└─────────────────────────────────────────────────────────┘
                     Both emit FClaudeStreamEvent
                     to the same UI event handler
```

### Component Classification: Modify vs. New vs. Unchanged

| Component | Action | Scope of Change |
|-----------|--------|-----------------|
| `IClaudeRunner` | **Rename/Generalize** | Rename to `IChatRunner` (or keep name, just add `IsAvailable()` semantics for OpenCode). Add `GetBackendName()` pure virtual. Small. |
| `FClaudeCodeSubsystem` | **Modify (significant)** | Replace `TUniquePtr<FClaudeCodeRunner>` with a runner registry (`TMap` or pair). Add `SetActiveBackend()`, backend switching. Route `SendPrompt()` to active runner. |
| `SClaudeEditorWidget` | **Modify (moderate)** | Remove direct `FClaudeCodeRunner::IsClaudeAvailable()` call (line 528). Use `Subsystem.GetActiveRunner()->IsAvailable()`. Update status text to show active backend name. |
| `SClaudeToolbar` | **Modify (moderate)** | Add backend selector (combo box or toggle button). Wire to subsystem's `SetActiveBackend()`. |
| `FClaudeCodeRunner` | **Unchanged** | No changes needed. Continues implementing `IClaudeRunner`. |
| `FClaudeSessionManager` | **Modify (minimal)** | Optionally tag exchanges with backend source (enum field). Otherwise unchanged — both backends write to same history. |
| `FClaudeStreamEvent` / `EClaudeStreamEventType` | **Modify (minimal)** | Add optional `BackendSource` field. Event types themselves already cover what OpenCode emits (text, tool use, tool result, completion). Possibly rename to `FChatStreamEvent`. |
| `FClaudeRequestConfig` | **Modify (minimal)** | Some fields are Claude-specific (`bSkipPermissions`, `AllowedTools`). Add `bIsOpenCode` flag or make fields optional. Alternatively, keep as-is — OpenCode runner ignores inapplicable fields. |
| `FOpenCodeRunner` | **NEW** | Full HTTP client + SSE parser implementing `IClaudeRunner`. |
| `FOpenCodeSSEParser` | **NEW** | SSE text stream parser: `data:`, `event:`, `id:` fields → `FClaudeStreamEvent`. |
| `FOpenCodeServerLifecycle` | **NEW** | Auto-detect running `opencode serve` instance, spawn if absent, health check, shutdown. |
| `UnrealClaudeConstants::OpenCode` | **NEW (additive)** | New namespace in existing constants file: default port (4096), health check interval, spawn timeout. |

---

## Component Boundaries

### FOpenCodeRunner (NEW — the core new component)

**Responsibility:** Implement `IClaudeRunner` by communicating with `opencode serve` HTTP API.

**Communicates with:**
- `FClaudeCodeSubsystem` — via `IClaudeRunner` interface (receives `ExecuteAsync` calls)
- `opencode serve` REST API — outbound HTTP requests
- `opencode serve` SSE endpoint — persistent HTTP connection for streaming events
- `FOpenCodeServerLifecycle` — to ensure server is running before requests

**Does NOT communicate with:**
- UI layer (no Slate references)
- MCP server/tools (those are independent; OpenCode connects to MCP bridge separately)
- `FClaudeCodeRunner` (no inter-runner communication)

**Key methods:**
```cpp
class FOpenCodeRunner : public IClaudeRunner
{
public:
    // IClaudeRunner interface
    bool ExecuteAsync(const FClaudeRequestConfig& Config,
                      FOnClaudeResponse OnComplete,
                      FOnClaudeProgress OnProgress) override;
    bool ExecuteSync(const FClaudeRequestConfig& Config,
                     FString& OutResponse) override;
    void Cancel() override;
    bool IsExecuting() const override;
    bool IsAvailable() const override;

    // OpenCode-specific
    FString GetBackendName() const { return TEXT("OpenCode"); }

private:
    // HTTP session state
    FString OpenCodeSessionId;
    FString BaseUrl;  // e.g. "http://localhost:4096"

    // SSE connection
    TSharedPtr<IHttpRequest> ActiveSSERequest;

    // Server lifecycle (owned or referenced)
    TSharedPtr<FOpenCodeServerLifecycle> ServerLifecycle;

    // SSE parser
    FOpenCodeSSEParser SSEParser;
};
```

### FOpenCodeSSEParser (NEW)

**Responsibility:** Parse raw SSE byte stream into structured events. SSE format: `data: {json}\n\n` lines. Map OpenCode event types to `EClaudeStreamEventType`.

**Communicates with:**
- `FOpenCodeRunner` — receives raw HTTP response chunks, emits `FClaudeStreamEvent` via delegate

**Does NOT communicate with:**
- Anything else. Pure data transformation.

**Key concern:** SSE data arrives in arbitrary chunk boundaries. Parser must buffer incomplete lines across `OnProcessRequestComplete`/`OnHeaderReceived` callbacks from UE's `FHttpModule`.

```cpp
class FOpenCodeSSEParser
{
public:
    /** Feed raw bytes from HTTP stream. Emits events via delegate. */
    void ProcessChunk(const FString& RawChunk);

    /** Reset parser state for a new stream */
    void Reset();

    /** Delegate fired for each complete event */
    FOnClaudeStreamEvent OnEvent;

private:
    FString LineBuffer;
    FString CurrentEventType;
    FString CurrentEventData;

    /** Map OpenCode event JSON to FClaudeStreamEvent */
    FClaudeStreamEvent MapToStreamEvent(const FString& EventType,
                                         const TSharedRef<FJsonObject>& Data);
};
```

### FOpenCodeServerLifecycle (NEW)

**Responsibility:** Ensure `opencode serve` is running. Try connecting first (health check), spawn if not found, track process handle for cleanup on plugin shutdown.

**Communicates with:**
- `FOpenCodeRunner` — provides `IsServerReady()` and `EnsureServerRunning()` 
- OS process layer — `FPlatformProcess::CreateProc()` for spawning
- `opencode serve` health endpoint — HTTP GET for readiness check

**Does NOT communicate with:**
- UI, MCP tools, session manager

```cpp
class FOpenCodeServerLifecycle
{
public:
    /** Check if opencode serve is reachable */
    bool IsServerReady() const;

    /** Try connect, spawn if needed. Returns false if unable. */
    bool EnsureServerRunning();

    /** Shutdown spawned server (if we spawned it) */
    void Shutdown();

    /** Check if opencode CLI is installed */
    static bool IsOpenCodeInstalled();

    /** Get opencode CLI path */
    static FString GetOpenCodePath();

private:
    FProcHandle ServerProcess;
    bool bWeSpawnedServer = false;
    FString BaseUrl;
    uint16 Port;
};
```

### FClaudeCodeSubsystem (MODIFIED)

**Current state:** Hardcoded `TUniquePtr<FClaudeCodeRunner> Runner`. Calls `Runner->ExecuteAsync()` directly.

**Target state:** Holds multiple runners, routes to active one.

**Key changes:**
```cpp
// Current (line 110-111 of ClaudeSubsystem.h):
TUniquePtr<FClaudeCodeRunner> Runner;

// Becomes:
TUniquePtr<FClaudeCodeRunner> ClaudeRunner;
TUniquePtr<FOpenCodeRunner> OpenCodeRunner;
IClaudeRunner* ActiveRunner;  // Points to one of the above
EBackendType ActiveBackendType = EBackendType::Claude;
```

**Critical change in `SendPrompt()`:** Currently calls `Runner->ExecuteAsync(Config, WrappedComplete, Options.OnProgress)` on line 136. Must change to `ActiveRunner->ExecuteAsync(...)`.

**Critical change in constructor:** Currently `Runner = MakeUnique<FClaudeCodeRunner>()` (line 76). Must also construct `OpenCodeRunner` and set initial `ActiveRunner`.

**New methods:**
- `SetActiveBackend(EBackendType)` — switch active runner, called from toolbar
- `GetActiveBackendType()` — for UI display
- `GetActiveBackendName()` — "Claude" or "OpenCode"
- `IsBackendAvailable(EBackendType)` — check if specific backend is ready
- `RegisterMCPWithOpenCode()` — POST to OpenCode's `/mcp` endpoint to register the MCP bridge

### SClaudeToolbar (MODIFIED)

**Current state:** Context toggles and session management buttons only.

**Target state:** Add a backend selector widget.

**Addition:** A `SComboBox<TSharedPtr<FString>>` or two-state toggle button in the toolbar args:
```cpp
SLATE_EVENT(FOnBackendChanged, OnBackendChanged)  // New
SLATE_ATTRIBUTE(FText, ActiveBackendName)          // New
```

### SClaudeEditorWidget (MODIFIED)

**Coupling points that need fixing (3 total):**

1. **Line 528:** `return FClaudeCodeRunner::IsClaudeAvailable();` — static call to concrete class. Must become `FClaudeCodeSubsystem::Get().GetActiveRunner()->IsAvailable()`.

2. **Line 330/547/567:** Multiple `IsClaudeAvailable()` calls that gate functionality. All route through the same method, so fixing `IsClaudeAvailable()` fixes all.

3. **Line 556:** Status text shows "Claude CLI not found". Must become backend-aware: "Claude CLI not found" vs "OpenCode server not available".

4. **Line 56:** Role label says "Claude". Should say active backend name.

---

## Data Flow

### Current Data Flow (Claude CLI Path)

```
User types prompt
  → SClaudeEditorWidget::SendMessage()
    → FClaudeCodeSubsystem::SendPrompt()
      → builds prompt with history + context
      → FClaudeCodeRunner::ExecuteAsync(Config)
        → spawns `claude -p --stream-json` subprocess
        → worker thread reads NDJSON from stdout
        → ParseAndEmitNdjsonLine() per line
          → fires Config.OnStreamEvent delegate (FClaudeStreamEvent)
          → fires OnProgress delegate (raw text)
        → fires OnComplete delegate (FOnClaudeResponse)
      → SClaudeEditorWidget::OnClaudeStreamEvent() handles events
      → SClaudeEditorWidget::OnClaudeResponse() finalizes
    → FClaudeSessionManager::AddExchange() saves history
```

### New Data Flow (OpenCode HTTP Path)

```
User types prompt (backend = OpenCode)
  → SClaudeEditorWidget::SendMessage()
    → FClaudeCodeSubsystem::SendPrompt()
      → builds prompt with history + context
      → FOpenCodeRunner::ExecuteAsync(Config)
        → FOpenCodeServerLifecycle::EnsureServerRunning()
          → health check GET http://localhost:4096/
          → if not running: spawn `opencode serve --port 4096`
          → wait for readiness
        → POST http://localhost:4096/session
          → creates session, stores OpenCodeSessionId
        → POST http://localhost:4096/mcp (if not yet registered)
          → registers MCP bridge URL with OpenCode
        → POST http://localhost:4096/session/{id}/message
          → body: { content: "prompt text" }
        → GET http://localhost:4096/session/{id}/event (SSE)
          → persistent HTTP connection, chunked transfer
          → FOpenCodeSSEParser::ProcessChunk() per HTTP chunk
            → parses `data: {json}\n\n` boundaries
            → maps to FClaudeStreamEvent
            → fires Config.OnStreamEvent delegate
          → on SSE "done" event:
            → fires OnComplete delegate (FOnClaudeResponse)
      → SClaudeEditorWidget::OnClaudeStreamEvent() handles events
        (SAME handler as Claude path — events are identical type)
      → SClaudeEditorWidget::OnClaudeResponse() finalizes
    → FClaudeSessionManager::AddExchange() saves history
```

**Critical observation:** The UI layer (`SClaudeEditorWidget`) is completely unaware of which backend produced the events. Both paths emit `FClaudeStreamEvent` through the same delegate. This is the key architectural win — the event model is the adapter boundary.

### OpenCode SSE → FClaudeStreamEvent Mapping

| OpenCode SSE Event | FClaudeStreamEvent Type | Notes |
|---------------------|--------------------------|-------|
| `event.message.start` | `SessionInit` | Session/message ID |
| `event.content.delta` (text) | `TextContent` | Streaming text chunks |
| `event.tool.start` | `ToolUse` | Tool name, input, call ID |
| `event.tool.end` | `ToolResult` | Tool result content |
| `event.message.end` | `Result` | Cost, duration, turns |
| (any error) | Error flag on `TextContent` | `bIsError = true` |

**Confidence: MEDIUM** — Exact OpenCode SSE event names need verification against actual `opencode serve` output. The mapping structure is sound but event name strings may differ.

---

## Patterns to Follow

### Pattern 1: Runner Interface Polymorphism (existing, extend)

**What:** Both backends implement `IClaudeRunner`. Subsystem routes through interface pointer.
**When:** All prompt execution.
**Why:** Zero UI changes needed for the execution path. Same delegates, same event types.

```cpp
// In FClaudeCodeSubsystem::SendPrompt():
// Before: Runner->ExecuteAsync(Config, WrappedComplete, Options.OnProgress);
// After:
ActiveRunner->ExecuteAsync(Config, WrappedComplete, Options.OnProgress);
```

### Pattern 2: SSE Chunked Parsing with Line Buffer

**What:** SSE streams arrive in arbitrary TCP chunks. Buffer incomplete lines, emit complete events.
**When:** Parsing OpenCode SSE stream.
**Why:** UE's HTTP module delivers data in chunks that don't respect SSE message boundaries.

```cpp
void FOpenCodeSSEParser::ProcessChunk(const FString& RawChunk)
{
    LineBuffer += RawChunk;
    
    // SSE messages are separated by double newlines
    int32 DoubleNewline;
    while (LineBuffer.FindChar(TEXT('\n'), DoubleNewline))
    {
        // Find complete SSE message block (ends with \n\n)
        int32 MsgEnd = LineBuffer.Find(TEXT("\n\n"));
        if (MsgEnd == INDEX_NONE) break;
        
        FString MessageBlock = LineBuffer.Left(MsgEnd);
        LineBuffer = LineBuffer.Mid(MsgEnd + 2);
        
        // Parse SSE fields from message block
        ParseSSEMessage(MessageBlock);
    }
}
```

### Pattern 3: Backend-Agnostic Config Consumption

**What:** `FClaudeRequestConfig` contains fields applicable to one backend or both. Each runner takes what it needs and ignores the rest.
**When:** Building config in subsystem, consuming in runner.
**Why:** Avoids config proliferation. Claude runner uses `bSkipPermissions` and `AllowedTools`. OpenCode runner ignores those, uses `Prompt` and `SystemPrompt`.

```cpp
// FOpenCodeRunner::ExecuteAsync just uses what it needs:
bool FOpenCodeRunner::ExecuteAsync(const FClaudeRequestConfig& Config, ...)
{
    // Uses: Config.Prompt, Config.SystemPrompt, Config.OnStreamEvent,
    //       Config.TimeoutSeconds, Config.AttachedImagePaths
    // Ignores: Config.bSkipPermissions, Config.AllowedTools,
    //          Config.bUseJsonOutput, Config.WorkingDirectory
}
```

### Pattern 4: Lifecycle Management with Auto-Detect

**What:** Try connecting to existing `opencode serve` first; spawn only if not found.
**When:** First OpenCode request or backend switch.
**Why:** Best UX — no manual server management. Respects users who already run `opencode serve`.

```cpp
bool FOpenCodeServerLifecycle::EnsureServerRunning()
{
    if (IsServerReady()) return true;  // Already running
    
    if (!IsOpenCodeInstalled()) return false;  // CLI not installed
    
    // Spawn: opencode serve --port 4096
    ServerProcess = FPlatformProcess::CreateProc(
        *GetOpenCodePath(), TEXT("serve --port 4096"),
        true, false, false, nullptr, 0, nullptr, nullptr);
    bWeSpawnedServer = true;
    
    // Poll for readiness (max 10 seconds)
    for (int i = 0; i < 100; i++)
    {
        FPlatformProcess::Sleep(0.1f);
        if (IsServerReady()) return true;
    }
    return false;
}
```

---

## Anti-Patterns to Avoid

### Anti-Pattern 1: Separate Event Models Per Backend

**What:** Creating `FOpenCodeStreamEvent` alongside `FClaudeStreamEvent`.
**Why bad:** Doubles the UI handling code. Widget would need two `OnStreamEvent` handlers.
**Instead:** Map OpenCode SSE events to the existing `FClaudeStreamEvent` model inside `FOpenCodeSSEParser`. The event model is the common language.

### Anti-Pattern 2: Backend Logic in the Widget

**What:** Adding `if (bIsOpenCode) { ... } else { ... }` branches in `SClaudeEditorWidget`.
**Why bad:** UI shouldn't know about backend internals. Violates the existing clean separation.
**Instead:** All backend differences are encapsulated below `IClaudeRunner`. Widget only sees `FClaudeStreamEvent` and calls `Subsystem.SendPrompt()`.

### Anti-Pattern 3: Abstract Factory Over-Engineering

**What:** Creating `IBackendFactory`, `IBackendConfig`, `IBackendLifecycleManager` hierarchies.
**Why bad:** Two backends. Not twenty. YAGNI. The codebase convention is direct, pragmatic code.
**Instead:** Two concrete runner members in the subsystem. An enum to switch. Simple.

### Anti-Pattern 4: Blocking HTTP in ExecuteAsync

**What:** Using UE's synchronous HTTP request API (`ProcessRequest()` + wait loop) in `ExecuteAsync`.
**Why bad:** Blocks a thread unnecessarily. SSE is long-lived — could be minutes.
**Instead:** Use UE's `FHttpModule::CreateRequest()` with async completion delegates. SSE parsing happens in `OnProcessRequestComplete` or via streaming body delegate.

### Anti-Pattern 5: Modifying FClaudeCodeRunner

**What:** Adding OpenCode support to the existing runner class.
**Why bad:** Violates SRP. Claude runner does subprocess + NDJSON. OpenCode runner does HTTP + SSE. Completely different I/O paths.
**Instead:** New class `FOpenCodeRunner` implementing the same `IClaudeRunner` interface.

---

## Refactoring Scope Assessment

### Minimal Refactoring (do first)

| File | Change | Lines Affected (est.) |
|------|--------|----------------------|
| `ClaudeSubsystem.h` | Add second runner, ActiveRunner pointer, backend enum, switching methods | ~15 new lines |
| `ClaudeSubsystem.cpp` | Constructor creates both runners, `SendPrompt` uses `ActiveRunner`, new `SetActiveBackend()` method | ~30 changed/new lines |
| `ClaudeEditorWidget.cpp` | `IsClaudeAvailable()` → `IsBackendAvailable()`, status text, role label | ~10 lines |
| `SClaudeToolbar.h` | Add backend selector SLATE_EVENT + SLATE_ATTRIBUTE | ~5 lines |
| `SClaudeToolbar.cpp` | Add combo box widget in toolbar construction | ~30 lines |
| `UnrealClaudeConstants.h` | Add `OpenCode` namespace with port, timeouts | ~15 lines |

**Total existing code changes:** ~105 lines across 6 files. Low risk.

### New Components (do second)

| File | Purpose | Estimated Size |
|------|---------|---------------|
| `Public/OpenCodeRunner.h` | `FOpenCodeRunner : IClaudeRunner` header | ~80 lines |
| `Private/OpenCodeRunner.cpp` | HTTP client, session management, SSE connection | ~250 lines |
| `Private/OpenCodeSSEParser.h` | SSE stream parser header | ~40 lines |
| `Private/OpenCodeSSEParser.cpp` | SSE chunked parsing, event mapping | ~150 lines |
| `Private/OpenCodeServerLifecycle.h` | Server auto-detect/spawn header | ~40 lines |
| `Private/OpenCodeServerLifecycle.cpp` | Process spawning, health checking | ~120 lines |

**Total new code:** ~680 lines across 6 files (3 new classes).

### Files That Do NOT Need Changes

- `FClaudeCodeRunner` — untouched, continues working
- `IClaudeRunner.h` — interface already has what's needed (optionally add `GetBackendName()`)
- All MCP tools — completely independent of backend choice
- `FMCPToolRegistry`, `FMCPToolBase`, `FUnrealClaudeMCPServer` — unchanged
- `FProjectContextManager` — unchanged
- `FScriptExecutionManager` — unchanged
- Node.js MCP bridge — unchanged (OpenCode connects to it independently)
- All test files — unchanged (test MCP tools, not chat backend)

---

## Build Order (Dependency Chain)

Components must be built in this order due to compile-time and runtime dependencies:

```
Phase 1: Foundation (no dependencies on each other)
├── 1a. FOpenCodeSSEParser          — pure data transform, no UE HTTP deps
├── 1b. UnrealClaudeConstants::OpenCode  — just constants
└── 1c. EBackendType enum           — just an enum in a header

Phase 2: OpenCode Infrastructure (depends on Phase 1)
├── 2a. FOpenCodeServerLifecycle    — needs constants (port), uses FPlatformProcess
└── 2b. FOpenCodeRunner             — needs SSEParser, ServerLifecycle, IClaudeRunner

Phase 3: Subsystem Refactoring (depends on Phase 2)
└── 3a. FClaudeCodeSubsystem mods   — needs FOpenCodeRunner, EBackendType

Phase 4: UI Integration (depends on Phase 3)
├── 4a. SClaudeToolbar mods         — needs EBackendType, subsystem methods
├── 4b. SClaudeEditorWidget mods    — needs subsystem's new API
└── 4c. Status text / role labels   — cosmetic, depends on 4b

Phase 5: Runtime Wiring (depends on Phase 3+4)
├── 5a. MCP bridge registration     — POST /mcp from FOpenCodeRunner
├── 5b. Backend persistence         — save/load preferred backend in settings
└── 5c. Module startup integration  — connect lifecycle to FUnrealClaudeModule
```

**Why this order:**
- Phase 1 items can be unit-tested independently (SSE parser can be tested with canned strings)
- Phase 2 can be tested against a real/mock HTTP server without touching any existing code
- Phase 3 is the riskiest change (touching the orchestrator), so it happens after new components are proven
- Phase 4 is lowest risk (UI wiring) and depends on the subsystem API being stable
- Phase 5 is integration glue that ties everything together

---

## Threading Model

### Current Threading (Claude CLI)

```
Game Thread                    FRunnable Worker Thread
─────────────                  ──────────────────────
SendMessage() →
  SendPrompt() →
    ExecuteAsync() →
                               spawn subprocess
                               read NDJSON loop {
                                 parse line
                                 AsyncTask(GameThread, []{
                                   fire OnStreamEvent
  ← OnClaudeStreamEvent()       })
                               }
                               AsyncTask(GameThread, []{
                                 fire OnComplete
  ← OnClaudeResponse()          })
```

### New Threading (OpenCode HTTP)

```
Game Thread                    HTTP Module Thread(s)
─────────────                  ────────────────────
SendMessage() →
  SendPrompt() →
    ExecuteAsync() →
      POST /session →
      POST /session/{id}/message →
      GET /session/{id}/event (SSE) →
                               HTTP response chunks arrive {
                                 SSEParser.ProcessChunk()
                                 AsyncTask(GameThread, []{
                                   fire OnStreamEvent
  ← OnClaudeStreamEvent()       })
                               }
                               on SSE complete/error:
                               AsyncTask(GameThread, []{
                                 fire OnComplete
  ← OnClaudeResponse()          })
```

**Key difference:** Claude uses a dedicated `FRunnable` thread. OpenCode uses UE's built-in HTTP module threads (managed by `FHttpModule`). No need for a custom thread — UE's HTTP module handles async I/O.

**Critical constraint:** All delegate callbacks must marshal to the game thread via `AsyncTask(ENamedThreads::GameThread, ...)`. This is already the pattern in `FClaudeCodeRunner` and must be replicated in `FOpenCodeRunner`.

---

## UE HTTP Module Considerations

UE's `FHttpModule` and `IHttpRequest` support:
- Async request processing via `OnProcessRequestComplete` delegate
- **Streaming response** via `OnRequestProgress` delegate (called as data arrives)
- Header access via `OnHeaderReceived` delegate

For SSE, the approach is:
1. Create request: `FHttpModule::Get().CreateRequest()`
2. Set verb to GET, URL to SSE endpoint
3. Bind `OnRequestProgress` — receives `BytesSent`, `BytesReceived` and can read partial response
4. The SSE connection stays open; UE HTTP module keeps calling progress delegate as chunks arrive
5. On connection close: `OnProcessRequestComplete` fires

**Alternative approach:** If UE's HTTP module doesn't support true streaming response reading (some versions buffer), fall back to `FRunnable` + raw sockets or `libcurl` directly. This needs validation during implementation.

**Confidence: MEDIUM** — UE 5.7's HTTP module should support streaming via `OnRequestProgress` with `SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnHttpThread)`, but exact SSE streaming behavior needs empirical verification.

---

## Scalability Considerations

| Concern | Single Backend | Dual Backend | Future (3+ backends) |
|---------|----------------|--------------|---------------------|
| Runner management | One `TUniquePtr` | Two `TUniquePtr` + pointer | `TMap<EBackendType, TUniquePtr<IClaudeRunner>>` |
| Config routing | Direct | Ignore inapplicable fields | Per-backend config subclass |
| UI switching | N/A | Combo box / toggle | Combo box (scales naturally) |
| History | Single stream | Tagged with backend | Tagged with backend |
| Server lifecycle | N/A (subprocess) | One lifecycle manager | Per-backend lifecycle manager |

The recommended architecture (two concrete runners + pointer switch) handles the dual-backend case cleanly. If a third backend is ever needed, the refactoring to a `TMap` is straightforward, but building that generalization now is premature.

---

## Sources

- **Existing codebase analysis:** `.planning/codebase/ARCHITECTURE.md`, `STRUCTURE.md`, `CONVENTIONS.md` — HIGH confidence
- **IClaudeRunner.h source:** Direct code reading — HIGH confidence
- **ClaudeSubsystem.h/cpp source:** Direct code reading — HIGH confidence
- **ClaudeEditorWidget.h/cpp source:** Direct code reading — HIGH confidence
- **OpenCode serve API:** Based on PROJECT.md context (REST + SSE, port 4096, `/session`, `/mcp` endpoints) — MEDIUM confidence (needs verification against actual `opencode serve --help` output)
- **UE 5.7 FHttpModule streaming:** Training data + UE conventions — MEDIUM confidence (needs empirical validation for SSE long-poll behavior)

---

*Architecture analysis: 2026-03-30*
