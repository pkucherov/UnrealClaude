# Technology Stack — OpenCode HTTP Server Integration

**Project:** UnrealClaude OpenCode Backend
**Researched:** 2026-03-30
**Research Mode:** Stack (focused dimension)

## Executive Summary

Integrating OpenCode's `opencode serve` HTTP API into the existing UE5.7 editor plugin requires: (1) UE's built-in `HTTP` module for REST calls, (2) a custom SSE streaming parser built on `FRunnable` + raw socket/HTTP chunked reads, and (3) the existing `Json` module for all request/response serialization. No new external dependencies are needed — everything is achievable within UE5.7's module ecosystem plus the OpenCode CLI binary itself.

The critical integration surface is:
- **REST API** (standard request/response): Session CRUD, message sending, health checks, MCP registration
- **SSE stream** (`GET /event`): Real-time events for streaming assistant responses, tool calls, session status changes
- **Two integration patterns**: Synchronous REST (for prompt/response via `POST /session/:id/message`) and async SSE (for real-time streaming via `GET /event` + `POST /session/:id/prompt_async`)

---

## Recommended Stack

### OpenCode Server API (External Dependency)

| Technology | Version | Purpose | Why |
|---|---|---|---|
| `opencode-ai` CLI | v1.3.7+ (latest) | AI coding agent backend | Provides `opencode serve` which runs headless HTTP server on port 4096; full REST API + SSE event stream; dynamic MCP registration; session management |

**Confidence:** HIGH — verified from official docs at opencode.ai/docs/server (March 29, 2026)

#### Key API Endpoints Used

| Endpoint | Method | Purpose in Plugin | Notes |
|---|---|---|---|
| `/global/health` | GET | Health check, availability detection | Returns `{ healthy: true, version: string }` |
| `/session` | POST | Create new chat session | Body: `{ title? }`, returns `Session` object with `id` |
| `/session` | GET | List existing sessions | Returns `Session[]` |
| `/session/:id` | GET | Get session details | Returns `Session` with metadata |
| `/session/:id` | DELETE | Delete a session | Returns `boolean` |
| `/session/:id/message` | POST | Send prompt and wait for response (sync) | Body: `{ model?, agent?, parts, noReply? }`. **Blocking** — returns only when AI completes. Returns `{ info: Message, parts: Part[] }` |
| `/session/:id/prompt_async` | POST | Send prompt without waiting (async) | Same body as `/message`, returns `204 No Content` immediately. Events arrive on SSE stream |
| `/session/:id/message` | GET | List messages in session | Query: `limit?`, returns array of `{ info: Message, parts: Part[] }` |
| `/session/:id/abort` | POST | Cancel running generation | Returns `boolean` |
| `/mcp` | POST | Register MCP server dynamically | Body: `{ name, config }` — registers the existing MCP bridge as a tool source |
| `/mcp` | GET | Check MCP server status | Returns `{ [name]: MCPStatus }` |
| `/event` | GET | SSE event stream | Long-lived connection; first event is `server.connected`, then bus events for all session activity |
| `/global/event` | GET | Global SSE event stream | Alternative event endpoint |

**Confidence:** HIGH — all endpoints documented in official server docs

#### SSE Event Types (from `/event`)

The SSE stream at `GET /event` delivers events from OpenCode's internal event bus. Based on the plugin system documentation, the key event types are:

| Event Type | When Fired | Maps to Existing Plugin Event |
|---|---|---|
| `server.connected` | First event on SSE connect | Connection established confirmation |
| `message.part.updated` | Text chunk streamed, tool call started/updated | `FClaudeStreamEvent` with `TextContent`, `ToolUse` |
| `message.updated` | Full message updated (completion) | `FClaudeStreamEvent` with `Result` |
| `session.status` | Session status change (busy/idle) | Execution state tracking |
| `session.idle` | Session finished processing | Completion signal |
| `session.error` | Error occurred | `FClaudeStreamEvent` with `bIsError=true` |
| `permission.asked` | Tool needs permission | Permission dialog (new feature) |
| `tool.execute.before` / `tool.execute.after` | Tool execution lifecycle | `ToolUse` / `ToolResult` events |

**Confidence:** MEDIUM — event types come from plugin docs; exact SSE wire format (field names, JSON structure) needs validation against actual `opencode serve` output at implementation time. The OpenAPI spec at `/doc` will be the authoritative source.

#### MCP Registration Format

To register the existing MCP bridge with OpenCode dynamically:

```json
POST /mcp
{
  "name": "unrealclaude",
  "config": {
    "type": "local",
    "command": ["node", "<path>/mcp-bridge/index.js"],
    "environment": {
      "UNREAL_MCP_URL": "http://localhost:3000"
    }
  }
}
```

**Confidence:** HIGH — `POST /mcp` with `{ name, config }` is documented in server docs; MCP config format matches the `opencode.json` MCP server schema from MCP servers docs.

---

### UE5.7 Modules for HTTP Client

| Module | Already Dep? | Purpose | Why This Module |
|---|---|---|---|
| `HTTP` | YES (private dep) | HTTP client for REST calls to OpenCode server | UE's built-in HTTP client module provides `FHttpModule::Get().CreateRequest()` for making outgoing HTTP requests. The plugin already depends on this module (used for `HTTPServer`), but the `HTTP` module also includes the client-side `IHttpRequest`/`IHttpResponse` interfaces |
| `Json` | YES (private dep) | JSON serialization/deserialization | Already used for MCP protocol; reuse for OpenCode REST API request/response bodies |
| `JsonUtilities` | YES (private dep) | Higher-level JSON helpers | Struct-to-JSON and JSON-to-struct conversion utilities |
| `Sockets` + `Networking` | YES (private dep) | Raw TCP for SSE fallback | If `FHttpModule` doesn't support chunked/streaming HTTP responses adequately, fall back to `FSocket` for the SSE connection |
| `Core` (FRunnable) | YES (public dep) | Background thread for SSE listener | SSE requires a persistent connection that reads indefinitely; must run on a background thread like the existing `FClaudeCodeRunner` |

**Confidence:** HIGH — the `HTTP` module in UE5.7 provides both server (`FHttpServerModule`) and client (`FHttpModule`) APIs. The plugin already depends on `HTTP`. No new module dependencies needed.

#### Why NOT These Alternatives

| Don't Use | Why Not |
|---|---|
| libcurl directly | UE wraps libcurl internally via `FHttpModule`. Using libcurl directly bypasses UE's threading model, certificate management, and proxy support. The `HTTP` module is the correct abstraction |
| Third-party HTTP libraries (cpp-httplib, cpr) | Adds external build dependencies, conflicts with UE's build system, and duplicates functionality already available in the engine |
| WebSocket module | OpenCode uses SSE (Server-Sent Events), not WebSockets. SSE is unidirectional HTTP; WebSocket is bidirectional. Wrong protocol |
| `FPlatformHttp` directly | Too low-level; `FHttpModule` wraps this properly with request lifecycle management |
| OpenCode's JS/TS SDK (`@opencode-ai/sdk`) | This is a JavaScript SDK — cannot be used from C++. We implement the equivalent HTTP calls directly using UE's HTTP module |

---

### HTTP Client Usage Pattern in UE5.7

```cpp
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

// Create and send a REST request
TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
Request->SetURL(TEXT("http://127.0.0.1:4096/session"));
Request->SetVerb(TEXT("POST"));
Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
Request->SetContentAsString(JsonBody);
Request->OnProcessRequestComplete().BindLambda(
    [](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bConnected)
    {
        if (bConnected && Resp.IsValid() && Resp->GetResponseCode() == 200)
        {
            FString Body = Resp->GetContentAsString();
            // Parse JSON...
        }
    });
Request->ProcessRequest();
```

**Confidence:** HIGH — this is standard UE HTTP client API, unchanged since UE4 and well-documented in engine source.

---

### SSE (Server-Sent Events) Parsing in UE5.7 C++

This is the most technically nuanced piece. UE's `FHttpModule` is designed for request/response patterns, not long-lived streaming connections. There is **no built-in SSE client** in Unreal Engine.

#### Recommended Approach: FRunnable + FSocket TCP Stream

Build a custom `FOpenCodeSSEClient` class that:

1. **Runs on a background `FRunnable` thread** (same pattern as `FClaudeCodeRunner`)
2. **Opens a raw TCP socket** to `127.0.0.1:4096` using UE's `FSocket` API
3. **Sends an HTTP GET request** manually for `GET /event HTTP/1.1` with `Accept: text/event-stream`
4. **Reads the chunked response** line-by-line in a loop
5. **Parses SSE format** (lines starting with `data:`, `event:`, `id:`, empty line = event boundary)
6. **Fires delegates on the game thread** via `AsyncTask(ENamedThreads::GameThread, ...)` — same pattern as existing stream event dispatch

SSE wire format (per [W3C SSE spec](https://html.spec.whatwg.org/multipage/server-sent-events.html)):
```
event: message.part.updated
data: {"type":"message.part.updated","properties":{...}}

event: session.idle  
data: {"type":"session.idle","properties":{...}}

```

Each event is: optional `event:` line, `data:` line(s), terminated by blank line.

```cpp
// SSE line parser pseudocode
void FOpenCodeSSEClient::ProcessSSELine(const FString& Line)
{
    if (Line.StartsWith(TEXT("event:")))
    {
        CurrentEventType = Line.Mid(6).TrimStartAndEnd();
    }
    else if (Line.StartsWith(TEXT("data:")))
    {
        CurrentData += Line.Mid(5).TrimStartAndEnd();
    }
    else if (Line.IsEmpty() && !CurrentData.IsEmpty())
    {
        // Empty line = event boundary, dispatch event
        DispatchEvent(CurrentEventType, CurrentData);
        CurrentEventType.Empty();
        CurrentData.Empty();
    }
}
```

**Confidence:** HIGH for the approach. UE's `FSocket` + `FRunnable` pattern is battle-tested (used in multiplayer networking). SSE is a simple text protocol over HTTP — no special library needed.

#### Why Not UE's HTTP Module for SSE

UE's `FHttpModule` requests buffer the entire response before calling the completion delegate. There is no streaming/chunked response callback. The `OnRequestProgress` delegate provides download progress but not partial content access in a reliable way for indefinite SSE streams.

**Confidence:** MEDIUM — UE5.7 may have improved streaming support vs earlier versions. Verify at implementation time whether `FHttpModule`'s `OnRequestProgress` delegate exposes partial response content. If it does, use it instead of raw sockets (simpler). If not, the FSocket approach is the reliable fallback.

#### Alternative: Hybrid Approach

Use `FHttpModule` for all REST calls (session CRUD, message sending, health checks, MCP registration) and only use `FSocket` for the single long-lived SSE connection. This minimizes raw socket usage to exactly one connection.

---

### Process Management for `opencode serve`

| Technology | Purpose | Why |
|---|---|---|
| `FPlatformProcess::CreateProc()` | Spawn `opencode serve` subprocess | Same UE API used by `FClaudeCodeRunner` to spawn `claude` CLI. Consistent with existing patterns |
| `FPlatformProcess::IsProcRunning()` | Check if spawned server is still alive | Health monitoring |
| `FPlatformProcess::TerminateProc()` | Clean shutdown when plugin unloads | Prevents orphaned processes |

Auto-detect strategy:
1. Try `GET http://127.0.0.1:4096/global/health` — if responds, server already running
2. If no response, spawn `opencode serve --port 4096 --hostname 127.0.0.1`
3. Poll health endpoint until ready (with timeout)
4. On plugin shutdown, terminate spawned process (only if we spawned it)

**Confidence:** HIGH — `FPlatformProcess` is the same API used for Claude CLI subprocess spawning.

---

### Event-to-Event Mapping

How OpenCode SSE events map to the existing `FClaudeStreamEvent` / `EClaudeStreamEventType`:

| OpenCode SSE Event | Existing UE Event Type | Mapping Notes |
|---|---|---|
| `server.connected` | `SessionInit` | Connection established; extract session info |
| `message.part.updated` (text part) | `TextContent` | Extract text content from part data |
| `message.part.updated` (tool_use part) | `ToolUse` | Extract tool name, input, call ID |
| `message.part.updated` (tool_result part) | `ToolResult` | Extract tool result content |
| `message.updated` | `Result` | Final message with stats |
| `session.idle` | `Result` | Session finished — combine with last message for stats |
| `session.error` | `Unknown` with `bIsError=true` | Error event |

**Confidence:** MEDIUM — the exact JSON structure within each SSE event's `data:` field needs validation against the OpenAPI spec (`/doc` endpoint) at implementation time. The event type names come from OpenCode's plugin docs and are reliable, but the property names within `properties` need live verification.

---

## Alternatives Considered

| Category | Recommended | Alternative | Why Not Alternative |
|---|---|---|---|
| HTTP Client | UE `HTTP` module (`FHttpModule`) | Raw libcurl, cpp-httplib | UE already wraps libcurl; external libs conflict with UBT |
| SSE Parsing | Custom `FRunnable` + `FSocket` | UE HTTP streaming callback | UE HTTP module not designed for indefinite streaming; may buffer |
| JSON Parsing | UE `Json` module (`FJsonObject`) | nlohmann/json, RapidJSON | UE's JSON is already in use throughout the codebase; zero new deps |
| Process Spawning | `FPlatformProcess::CreateProc` | `system()`, `popen()` | UE's process API handles cross-platform, doesn't block game thread |
| OpenCode Integration | Direct HTTP to `opencode serve` | `opencode run` subprocess (like Claude CLI) | `opencode serve` provides sessions, MCP registration, SSE streaming — `opencode run` is one-shot only |
| SSE Protocol | Raw SSE over HTTP | WebSocket | OpenCode server exposes SSE at `/event`, not WebSocket. SSE is simpler (unidirectional, auto-reconnect semantics) |

---

## No New Build Dependencies Required

The integration requires **zero new module dependencies** in `UnrealClaude.Build.cs`. All required modules are already declared:

```csharp
// Already in PrivateDependencyModuleNames:
"HTTP",           // HTTP client (FHttpModule::Get().CreateRequest())
"Json",           // JSON parsing (FJsonSerializer, FJsonObject)
"JsonUtilities",  // JSON struct helpers
"Sockets",        // FSocket for SSE connection
"Networking",     // Network address resolution
```

The only new external requirement is the `opencode` CLI binary installed on the user's system (parallel to the existing `claude` CLI requirement).

**Confidence:** HIGH — verified against existing `UnrealClaude.Build.cs` contents.

---

## Key Implementation Classes (Recommended)

| Class | Responsibility | Pattern Source |
|---|---|---|
| `FOpenCodeRunner` | Implements `IClaudeRunner` (or generalized interface); orchestrates REST calls to OpenCode server | Mirrors `FClaudeCodeRunner` structure |
| `FOpenCodeSSEClient` | Background `FRunnable` thread managing SSE connection to `GET /event`; parses SSE lines; dispatches typed events | Mirrors `FClaudeCodeRunner`'s subprocess read loop |
| `FOpenCodeServerManager` | Lifecycle management: detect/spawn/monitor/shutdown `opencode serve` process | New; uses `FPlatformProcess` |
| `FOpenCodeHTTPClient` | Thin wrapper around `FHttpModule` for typed REST calls (CreateSession, SendMessage, RegisterMCP, etc.) | New; simple request/response helpers |

---

## Sources

| Source | Confidence | URL |
|---|---|---|
| OpenCode Server API docs | HIGH | https://opencode.ai/docs/server |
| OpenCode SDK docs | HIGH | https://opencode.ai/docs/sdk |
| OpenCode CLI docs | HIGH | https://opencode.ai/docs/cli |
| OpenCode MCP servers docs | HIGH | https://opencode.ai/docs/mcp-servers |
| OpenCode Plugins docs (event types) | MEDIUM | https://opencode.ai/docs/plugins |
| OpenCode GitHub repo | HIGH | https://github.com/anomalyco/opencode (v1.3.7, March 2026) |
| Existing `UnrealClaude.Build.cs` | HIGH | Local file — verified module dependencies |
| Existing `IClaudeRunner.h` | HIGH | Local file — verified interface and event types |
| Existing `FClaudeCodeRunner` | HIGH | Local file — verified subprocess/threading pattern |
| UE5.7 HTTP module API | HIGH | Training data (API stable since UE4; `FHttpModule::Get().CreateRequest()`) |
| UE5.7 `FSocket` API | HIGH | Training data (stable networking API) |
| W3C SSE specification | HIGH | https://html.spec.whatwg.org/multipage/server-sent-events.html |

---

## Risk Flags

1. **SSE event payload structure** (MEDIUM risk): The exact JSON within each SSE `data:` field is not fully documented in the server docs — only event type names are listed. The OpenAPI spec at `/doc` is the authoritative source and should be fetched from a running `opencode serve` instance during implementation.

2. **`FHttpModule` streaming capability** (LOW risk): If UE5.7's HTTP module gained streaming response support, the `FSocket` approach is unnecessarily complex. Check `OnRequestProgress` delegate behavior with chunked/SSE responses before committing to raw sockets.

3. **OpenCode API stability** (LOW risk): OpenCode is at v1.3.7 with 745 releases. The server API generates an OpenAPI spec and SDK, suggesting it's designed for stability. However, it's a fast-moving project — pin to a minimum version requirement.

4. **`prompt_async` vs `message` endpoint choice** (LOW risk): Two patterns exist — sync (`POST /message` blocks until complete) and async (`POST /prompt_async` + SSE events). The async pattern is preferred for streaming UI but adds complexity. The sync pattern works for simpler integration but blocks the HTTP request for the entire generation duration (could be minutes).

---

*Stack research: 2026-03-30*
