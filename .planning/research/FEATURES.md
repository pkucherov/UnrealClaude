# Feature Landscape

**Domain:** Multi-backend AI chat integration in an Unreal Engine 5.7 editor plugin
**Researched:** 2026-03-30
**Overall confidence:** HIGH (architecture is well-understood; OpenCode API docs verified against official sources)

---

## Table Stakes

Features users expect. Missing = integration feels broken or incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **Send messages and receive streaming responses via OpenCode** | Core functionality — if messages don't flow, nothing works | High | Requires SSE parsing (`GET /event`), mapping OpenCode's `Part[]` event model to existing `FClaudeStreamEvent` / `EClaudeStreamEventType`. This is the most technically complex table-stakes feature because the two backends use entirely different streaming protocols (NDJSON subprocess vs SSE over HTTP) |
| **Backend-agnostic runner interface** | Claude and OpenCode must be interchangeable without changing callers. Without this, every UI/subsystem change doubles | Medium | Generalize `IClaudeRunner` into `IAIBackendRunner` (or similar). Both `FClaudeCodeRunner` and `FOpenCodeRunner` implement it. `FClaudeCodeSubsystem` holds a pointer to whichever is active. The existing `IClaudeRunner` interface is already close — main changes are renaming and adjusting `FClaudeRequestConfig` to be backend-neutral |
| **Toolbar backend switcher** | Users must be able to switch backends without restarting the editor. This is the primary UX promise of the integration | Low | Dropdown or toggle button in `SClaudeToolbar`. Calls `FClaudeCodeSubsystem::SetActiveBackend()`. Disables switching while a request is in flight |
| **Backend availability detection** | Must show clear status when a backend isn't installed or isn't running. Users shouldn't get cryptic errors | Low | `IClaudeRunner::IsAvailable()` already exists. OpenCode version: `GET /global/health` returning `{ healthy: true, version }`. Show in status bar with color-coded indicator |
| **OpenCode session lifecycle management** | OpenCode is session-based (`POST /session`, `POST /session/:id/message`). Without proper session creation/teardown, messages can't be sent | Medium | Create session on first message or on backend switch. Store active session ID. Map session abort (`POST /session/:id/abort`) to `Cancel()`. Handle session expiry gracefully |
| **MCP tool registration with OpenCode** | OpenCode needs access to the 30+ editor tools. Without this, OpenCode is just a text chatbot — it can't manipulate the editor | Low | `POST /mcp` with `{ name: "unreal-editor", config: { type: "local", ... } }` pointing at the existing MCP bridge on port 3000. One HTTP call at connection time. OpenCode's dynamic MCP registration makes this trivial |
| **OpenCode server auto-spawn** | Users shouldn't need to manually run `opencode serve` before opening the plugin. That's unacceptable UX friction | Medium | Try connecting to `localhost:4096` (default). If unreachable, spawn `opencode serve --port 4096` as a background process via `FPlatformProcess::CreateProc`. Monitor process health. Kill on plugin shutdown |
| **Cancel in-flight requests** | Users expect to stop a running generation. Works for Claude today, must work for OpenCode | Low | `POST /session/:id/abort` maps directly to existing `Cancel()` on the interface. Wire through |
| **Error display on backend failure** | If OpenCode server crashes, connection drops, or API returns errors, user needs clear feedback — not a frozen UI | Low | Map HTTP errors, connection refused, and SSE disconnect to `FClaudeStreamEvent` with `bIsError = true`. Display in chat bubble with retry option |
| **Persist default backend preference** | When user restarts editor, their chosen backend should be remembered | Low | Save to `Saved/UnrealClaude/settings.json` or a `UDeveloperSettings` subclass. Load at module startup |

---

## Differentiators

Features that set the plugin apart. Not strictly expected, but provide competitive advantage.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **Unified conversation history (both backends in one timeline)** | Users switching backends mid-session keep full visible context. No jarring "start over" feeling. Rare — most multi-backend UIs silo conversations | High | Each message in `FClaudeSessionManager` needs a backend tag. UI renders all messages in order regardless of source. Challenge: neither backend sees the other's history natively (Claude CLI has its own context, OpenCode has its own sessions). The unified view is display-only; each backend's server-side context is separate. Must clearly label which backend produced each response |
| **OpenCode agent/model selection from toolbar** | OpenCode supports multiple agents (build, plan) and model switching (`providerID/modelID`). Exposing this in the UE toolbar lets users leverage OpenCode's multi-provider support without leaving the editor | Medium | Query `GET /agent` and `GET /config/providers` at connection time. Add dropdown to toolbar showing available agents/models. Pass `model` and `agent` fields in `POST /session/:id/message` body. This is a unique differentiator because Claude Code CLI has no equivalent model-switching ability |
| **Connection health indicator with auto-reconnect** | Show real-time backend health (connected/disconnected/connecting). Auto-reconnect on SSE disconnect with exponential backoff. Professional polish | Medium | SSE connections can drop. Implement reconnect logic with backoff (1s, 2s, 4s, max 30s). Show pulsing icon in toolbar during reconnection. Resume event subscription after reconnect |
| **Backend-specific capability badges** | Show what each backend can do (e.g., "Claude: has conversation history context" vs "OpenCode: multi-model, agent switching"). Helps users make informed backend choices | Low | Static metadata on each backend implementation. Display as tooltip or info panel when hovering the backend selector |
| **OpenCode session fork/share from UI** | OpenCode supports `POST /session/:id/fork` and `POST /session/:id/share`. Expose these as toolbar actions when OpenCode is active | Low | Two buttons, conditional on active backend being OpenCode. Fork creates a branch point. Share generates a shareable URL. Nice for team workflows |
| **Permission request handling** | OpenCode asks for permissions before file edits/bash commands (configurable per agent). The UE plugin should display these permission prompts and let users approve/deny from the chat panel | Medium | Listen for permission events on SSE stream. Display inline permission dialog (reuse existing `FScriptExecutionManager` permission pattern). Respond via `POST /session/:id/permissions/:permissionID`. Without this, OpenCode will block waiting for permission approval from a TUI that isn't running |
| **Graceful mid-conversation backend switch** | When switching backends mid-conversation, inject a summary of prior context as a system prompt to the new backend. Provides continuity | High | On switch: summarize last N exchanges, inject as system message to new backend. OpenCode supports `noReply: true` for injecting context without triggering a response (`POST /session/:id/message` with `noReply: true`). Claude supports system prompt injection via existing `FClaudePromptOptions`. Complex because summarization quality varies |

---

## Anti-Features

Features to explicitly NOT build. These would add complexity without proportional value, or would violate project constraints.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| **Separate chat panels per backend** | Fragments the UX. Users would need to context-switch between panels. The project explicitly scopes this out. Single panel with backend toggle is the correct mental model | Single shared panel with backend indicator in toolbar |
| **OpenCode CLI subprocess mode (`opencode run`)** | `opencode serve` provides a richer API (sessions, SSE, dynamic MCP, agents, models). Subprocess mode would be a simpler but inferior integration that duplicates the Claude CLI approach | Use `opencode serve` HTTP API exclusively |
| **Cross-backend conversation context injection** | Automatically feeding Claude's conversation to OpenCode (or vice versa) as hidden system context. This sounds appealing but creates confusion: the LLM sees context it didn't produce, leading to inconsistent behavior, hallucinated references to "earlier" discussion, and debugging nightmares | Keep server-side contexts separate. Show unified timeline in UI only. If user wants continuity, they can manually copy context or use the differentiator "graceful mid-conversation switch" which does a one-time summary injection explicitly |
| **OpenCode TUI/web integration** | OpenCode has TUI and web modes. Embedding these adds massive complexity for no gain — the UE plugin IS the UI | Use headless server mode only (`opencode serve`) |
| **MCP bridge modifications** | Tempting to modify the bridge to natively support OpenCode's MCP protocol. But OpenCode already consumes standard MCP servers via `POST /mcp` registration. The bridge works as-is | Register existing bridge as-is via OpenCode's dynamic MCP registration |
| **Backend-specific UI rendering** | Building different chat bubble styles, different tool call displays, etc. per backend. This creates visual inconsistency and doubles UI maintenance | Normalize both backends' events to the same `FClaudeStreamEvent` model. Render identically with only a small backend badge/label |
| **Automatic backend selection based on prompt content** | Routing prompts to "the best backend" automatically. This is unpredictable, removes user control, and would confuse users who don't understand why their backend keeps switching | Always use the user's explicitly selected backend. Show clear indicator of which is active |
| **Claude Code SDK/API migration** | Replacing the existing Claude CLI subprocess approach with Claude's newer SDK or API. Out of scope — the existing integration works and isn't broken | Keep existing `FClaudeCodeRunner` as-is. Only add OpenCode as second backend |

---

## Feature Dependencies

```
Backend-agnostic runner interface → All other features (everything depends on this abstraction)
    ├── OpenCode server auto-spawn → Send messages via OpenCode
    │       └── MCP tool registration → OpenCode can use editor tools
    ├── Toolbar backend switcher → Persist default backend preference
    │       └── Backend availability detection (must know what's available before showing switcher)
    ├── Send messages via OpenCode
    │       ├── OpenCode session lifecycle management (sessions must exist before messages)
    │       ├── SSE event stream parsing (streaming responses require event subscription)
    │       ├── Cancel in-flight requests
    │       └── Error display on backend failure
    └── Unified conversation history → Backend-specific message tagging

Permission request handling → SSE event stream parsing (permissions arrive as events)
OpenCode agent/model selection → Backend availability detection (need to query available models)
Connection health indicator → SSE event stream parsing (detect disconnect from event stream)
Graceful mid-conversation switch → Unified conversation history + Toolbar backend switcher
OpenCode session fork/share → OpenCode session lifecycle management
```

---

## MVP Recommendation

**Prioritize in this order:**

1. **Backend-agnostic runner interface** — Foundation. Refactor `IClaudeRunner` / `FClaudeRequestConfig` / `FClaudeStreamEvent` into backend-neutral abstractions. Without this, nothing else can be built cleanly.

2. **OpenCode server auto-spawn + health check** — Infrastructure. Users need the server running automatically. Combine with `GET /global/health` detection.

3. **SSE event stream parsing + message send/receive** — Core functionality. Implement `FOpenCodeRunner` that creates sessions, sends messages via `POST /session/:id/message`, and parses SSE events from `GET /event` into the unified stream event model.

4. **MCP tool registration** — Single `POST /mcp` call. Without this, OpenCode is a text-only chatbot in an editor context where tool use is the entire value proposition.

5. **Toolbar backend switcher + availability detection** — UX. The visible feature users interact with. Simple dropdown in `SClaudeToolbar`.

6. **Cancel, error handling, persist preference** — Polish. Low complexity, high perceived quality.

**Defer:**
- **Unified conversation history**: Complex display-only feature. Start with separate session timelines per backend (clearing chat on switch) and add unified history as a polish item.
- **OpenCode agent/model selection**: Nice-to-have. Start with OpenCode's default build agent and configured model. Add selection UI later.
- **Permission request handling**: Important for OpenCode's permission system, but can start by running OpenCode with `--yes` or permissive agent config to skip permissions initially.
- **Graceful mid-conversation switch**: Complex and reliant on other features. Build last.

---

## Sources

- OpenCode Server API docs: https://opencode.ai/docs/server (verified 2026-03-30) — HIGH confidence
- OpenCode SDK docs: https://opencode.ai/docs/sdk (verified 2026-03-30) — HIGH confidence
- OpenCode MCP server docs: https://opencode.ai/docs/mcp-servers (verified 2026-03-30) — HIGH confidence
- OpenCode Agents docs: https://opencode.ai/docs/agents (verified 2026-03-30) — HIGH confidence
- OpenCode GitHub README: https://github.com/anomalyco/opencode (verified 2026-03-30, v1.3.7) — HIGH confidence
- Existing plugin codebase: `IClaudeRunner.h`, `ClaudeSubsystem.h`, `SClaudeToolbar.h`, `ClaudeEditorWidget.h`, `ClaudeSessionManager.cpp` — direct code review
- Project planning docs: `.planning/PROJECT.md`, `.planning/codebase/ARCHITECTURE.md` — direct review
