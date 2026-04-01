# UnrealClaude — OpenCode Integration

## What This Is

An Unreal Engine 5.7 Editor plugin that integrates AI coding assistants directly into the editor. Currently supports Claude Code CLI as a chat backend with 30+ MCP tools for editor manipulation. This project adds OpenCode CLI as a second chat backend, so users can switch between Claude and OpenCode from the same chat panel.

## Core Value

Users can get AI coding assistance inside the Unreal Editor from either Claude Code or OpenCode, choosing whichever backend suits their workflow, without leaving the editor or losing conversation context.

## Requirements

### Validated

- ✓ Claude Code CLI subprocess chat integration (NDJSON streaming) — existing
- ✓ MCP REST server on port 3000 with 30+ editor tools (actors, blueprints, assets, materials, animation, input, levels, scripts) — existing
- ✓ Chat panel UI (SClaudeEditorWidget) with streaming responses, tool call grouping, code block rendering — existing
- ✓ Session persistence across editor sessions (FClaudeSessionManager) — existing
- ✓ Project context auto-gathering and system prompt injection (FProjectContextManager) — existing
- ✓ Script execution pipeline with permission gating (C++, Python, Console, EditorUtility) — existing
- ✓ Node.js MCP bridge translating MCP protocol (JSON-RPC 2.0) to HTTP REST — existing
- ✓ Async task queue for long-running tool operations (FMCPTaskQueue) — existing
- ✓ Multi-platform support: Win64, Linux, macOS (Apple Silicon) — existing
- ✓ Keyboard shortcuts and Tools menu integration — existing
- ✓ IClaudeRunner abstraction for Claude CLI execution — existing
- ✓ IChatBackend abstraction with 9 pure virtual methods, EChatBackendType enum, polymorphic config, FChatBackendFactory — Validated in Phase 2: Backend Abstraction
- ✓ FOpenCodeSSEParser: stateful SSE chunk-to-event parser (10 event type mappings, 21 tests) — Validated in Phase 3: SSE Parser & Server Lifecycle
- ✓ FOpenCodeHttpClient: production HTTP/SSE client (synchronous Get/Post, BeginSSEStream, CheckHealth, 9 tests) — Validated in Phase 3
- ✓ FOpenCodeServerLifecycle: six-state server lifecycle manager (PID file, port retry, backoff, 19 tests) — Validated in Phase 3

### Active

- [ ] OpenCode HTTP server backend (communicate via REST API + SSE events with `opencode serve`)
- [ ] Auto-detect/spawn OpenCode server (try connecting to existing `opencode serve` instance, spawn one if none found)
- [ ] Toolbar backend switcher (dropdown/button in chat toolbar to switch between Claude and OpenCode at runtime)
- [ ] Plugin settings for default backend (configurable in UE Editor preferences, persisted across sessions)
- [ ] Dynamic MCP registration with OpenCode (register the existing MCP bridge with OpenCode via `POST /mcp` on its server API)
- [ ] Unified conversation history (both backends write to same session manager, single timeline in chat panel)
- [ ] SSE event stream parsing (parse Server-Sent Events from OpenCode's `/event` endpoint, map to existing UI event model)
- [ ] OpenCode session management (create/manage sessions via OpenCode's REST API: `/session`, `/session/:id/message`, etc.)

### Out of Scope

- Replacing Claude Code CLI — both backends coexist, not replacing one with the other
- OpenCode CLI subprocess mode — using HTTP server API, not spawning `opencode run` as subprocess
- Separate chat panels per backend — single shared panel with backend toggle
- OpenCode TUI integration — using headless server mode only
- OpenCode web/IDE modes — only the `opencode serve` HTTP API is used
- MCP bridge modifications — reusing the existing bridge as-is; OpenCode connects to it via its own MCP config

## Context

- **Existing architecture:** The plugin uses an `IChatBackend` interface (9 pure virtual methods) that abstracts chat execution. `FClaudeCodeRunner` implements it by spawning `claude -p --stream-json` as a subprocess. `FChatSubsystem` owns the active backend via `TUniquePtr<IChatBackend>` and can swap backends at runtime via `FChatBackendFactory`. A new `FOpenCodeRunner` would implement `IChatBackend` to talk to OpenCode's HTTP server.
- **OpenCode CLI capabilities:** OpenCode provides `opencode serve` which starts a headless HTTP server (default port 4096) with a full REST API including session management, message sending, SSE event streaming, and dynamic MCP server registration. It supports `--format json` for raw JSON events.
- **OpenCode MCP support:** OpenCode can consume MCP servers configured in `opencode.json` or registered dynamically via `POST /mcp`. The existing MCP bridge can be registered as a local MCP server so OpenCode's LLM gets access to all 30+ editor tools.
- **Stream format difference:** Claude uses NDJSON (newline-delimited JSON) over subprocess stdout. OpenCode uses Server-Sent Events over HTTP. Both map to the existing `FChatStreamEvent` / `EChatStreamEventType` model in `ChatBackendTypes.h`.
- **Existing codebase map:** Full architectural analysis available in `.planning/codebase/` covering architecture, stack, structure, conventions, integrations, testing, and concerns.

## Constraints

- **Tech stack**: Must stay within UE 5.7 C++ (Slate, HTTP module, FRunnable) — no new external runtime dependencies beyond OpenCode CLI itself
- **Backwards compatible**: Existing Claude Code CLI integration must continue working exactly as before
- **No UE module split**: Plugin remains a single `UnrealClaude` module
- **OpenCode install**: Users must have `opencode` CLI installed separately (similar to existing Claude CLI requirement)
- **Platform parity**: OpenCode backend must work on all three platforms (Win64, Linux, Mac) where the plugin currently works

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| HTTP server API over CLI subprocess | OpenCode's `opencode serve` provides richer API (sessions, SSE, dynamic MCP registration) vs `opencode run` which is simpler but less capable | — Pending |
| Auto-detect then spawn | Best UX: seamless if server already running, automatic if not. Avoids requiring user to manage server lifecycle | — Pending |
| Shared chat panel with toolbar toggle | Simpler UX than separate panels; consistent with the "one chat assistant" mental model | — Pending |
| Unified conversation history | Users switching backends mid-session keep full context visible; simpler than separate timelines | — Pending |
| Dynamic MCP registration via POST /mcp | No config file changes needed; plugin registers bridge at runtime when connecting to OpenCode | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-04-01 after Phase 3 completion*
