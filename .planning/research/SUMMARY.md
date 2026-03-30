# Project Research Summary

**Project:** UnrealClaude — OpenCode HTTP Backend Integration
**Domain:** Multi-backend AI coding assistant integration in UE5.7 editor plugin
**Researched:** 2026-03-30
**Confidence:** HIGH

## Executive Summary

This project adds OpenCode as a second AI backend to an existing UE5.7 editor plugin that currently only supports Claude CLI. The integration surface is OpenCode's `opencode serve` HTTP API: standard REST endpoints for session/message CRUD and an SSE (Server-Sent Events) stream for real-time response streaming. The critical architectural insight is that the existing plugin is *almost* multi-backend-ready — `IClaudeRunner` already abstracts execution — but three coupling points in the subsystem and widget need refactoring, and three entirely new components are needed (HTTP runner, SSE parser, server lifecycle manager). No new UE module dependencies are required; everything is achievable with `HTTP`, `Json`, `Sockets`, and `Core` modules already in the build file.

The recommended approach is a **bottom-up build**: start with the interface refactoring (generalize `IClaudeRunner` to remove subprocess assumptions), then build the new OpenCode components in isolation (SSE parser → server lifecycle → HTTP runner), then wire into the subsystem, and finally add UI switching. This order is driven by dependency analysis and risk mitigation — the SSE parser and server lifecycle can be tested independently before touching any existing code, and the riskiest change (subsystem modification) happens only after the new components are proven.

The top risks are: (1) UE's HTTP module silently killing long-lived SSE connections via default timeouts — must explicitly clear timeouts or fall back to raw sockets; (2) zombie `opencode serve` processes surviving editor crashes — mitigate with PID files, connect-before-spawn strategy, and shutdown hooks; (3) threading mistakes where SSE callbacks on background threads directly modify Slate widgets — follow the existing `AsyncTask(GameThread)` pattern already proven in `FClaudeCodeRunner`. All three have clear mitigations documented in the existing codebase's patterns.

## Key Findings

### Recommended Stack

The integration requires zero new external dependencies. OpenCode's `opencode serve` (v1.3.7+) provides a full REST API on port 4096 plus SSE streaming at `GET /event`. UE's built-in `HTTP` module handles REST calls, a custom `FRunnable` + `FSocket` implementation handles the persistent SSE connection, and the existing `Json` module handles all serialization. The only new external requirement is the `opencode` CLI binary installed on the user's system.

**Core technologies:**
- **`opencode serve` HTTP API**: REST + SSE backend providing sessions, streaming, dynamic MCP registration — replaces the need for subprocess I/O
- **UE `HTTP` module (`FHttpModule`)**: Built-in HTTP client for all REST calls — already a dependency, no build file changes
- **UE `FSocket` + `FRunnable`**: Raw TCP socket on background thread for persistent SSE connection — UE's HTTP module doesn't support indefinite streaming natively
- **UE `Json` module**: JSON serialization/deserialization for all API payloads — already used throughout codebase
- **`FPlatformProcess::CreateProc()`**: Process spawning for auto-starting `opencode serve` — same pattern as existing Claude CLI spawning

### Expected Features

**Must have (table stakes):**
- **Backend-agnostic runner interface** — Foundation; everything depends on this abstraction
- **Send/receive streaming messages via OpenCode** — Core functionality requiring SSE parsing and event mapping
- **OpenCode server auto-spawn + health detection** — Users shouldn't manually run `opencode serve`
- **MCP tool registration** — Single `POST /mcp` call; without it, OpenCode is text-only in a tool-centric plugin
- **Toolbar backend switcher + availability detection** — Primary UX promise of the integration
- **OpenCode session lifecycle management** — Sessions must exist before messages can be sent
- **Cancel in-flight requests** — Maps to `POST /session/:id/abort`
- **Error display on backend failure** — Clear feedback, not frozen UI
- **Persist default backend preference** — Remember choice across editor restarts

**Should have (differentiators):**
- **Permission request handling** — OpenCode blocks on permission prompts; plugin must handle these
- **Connection health indicator with auto-reconnect** — Professional polish; SSE connections drop
- **OpenCode agent/model selection** — Leverage OpenCode's multi-provider capability from the toolbar
- **Unified conversation history** — Both backends in one timeline with backend badges

**Defer (v2+):**
- **Graceful mid-conversation backend switch** — Complex context injection; reliant on other features
- **OpenCode session fork/share** — Nice for teams but not core
- **Backend-specific capability badges** — Low impact polish

### Architecture Approach

The architecture adds a second `IClaudeRunner` implementation (`FOpenCodeRunner`) behind the existing interface, with the subsystem routing to the active runner via a pointer swap. The UI layer remains completely backend-agnostic — both runners emit the same `FClaudeStreamEvent` types through the same delegate chain. Three new classes are needed (~680 lines total), and ~105 lines across 6 existing files need modification. The existing `FClaudeCodeRunner`, all MCP tools, the MCP bridge, and all test files remain unchanged.

**Major components:**
1. **`FOpenCodeRunner` (NEW)** — Implements `IClaudeRunner`; HTTP client for REST calls + SSE stream management; session lifecycle; ~330 lines
2. **`FOpenCodeSSEParser` (NEW)** — Parses raw SSE byte stream into `FClaudeStreamEvent` types with line buffering across chunk boundaries; ~190 lines
3. **`FOpenCodeServerLifecycle` (NEW)** — Auto-detect/spawn/monitor/shutdown `opencode serve`; PID tracking; health polling; ~160 lines
4. **`FClaudeCodeSubsystem` (MODIFIED)** — Replace single runner with runner registry + active pointer; add `SetActiveBackend()`; ~45 lines changed
5. **`SClaudeToolbar` (MODIFIED)** — Add backend selector combo box/toggle; ~35 lines added
6. **`SClaudeEditorWidget` (MODIFIED)** — Fix 3 coupling points (static `IsClaudeAvailable()` calls, status text, role label); ~10 lines changed

### Critical Pitfalls

1. **UE HTTP module kills SSE connections via default timeout** — SSE is indefinite; UE defaults to 30-60s timeout. Must call `Request->ClearTimeout()` or use raw sockets for the SSE connection. Validate with a 5-minute idle test.

2. **SSE callbacks fire on non-game thread** — UE's streaming body delegate runs on HTTP background threads. Directly modifying Slate widgets causes intermittent crashes. Must marshal every event via `AsyncTask(ENamedThreads::GameThread)` — pattern already proven in `FClaudeCodeRunner`.

3. **Zombie `opencode serve` processes** — If editor crashes, the spawned server outlives it, holding the port. Mitigate with connect-before-spawn strategy, PID file tracking, and `FCoreDelegates::OnPreExit` shutdown hooks.

4. **`IClaudeRunner` bakes in subprocess assumptions** — `ExecuteSync()` can deadlock with HTTP, `WorkingDirectory`/`AllowedTools` are CLI-specific. Interface must be generalized before building the HTTP runner.

5. **OpenCode SSE event bus is global** — `/event` publishes ALL events from all sessions. Must filter by session ID on every event or the UI shows content from other sessions.

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 1: Interface Generalization & Foundation
**Rationale:** The runner interface must be backend-agnostic before any new backend can be built. Pitfall 4 (subprocess assumptions) and Pitfall 8 (concrete type ownership) make this non-negotiable as the first step. This is also the lowest-risk change — it's refactoring without new functionality.
**Delivers:** Generalized `IClaudeRunner` (or `IChatBackend`), `EBackendType` enum, subsystem modified to hold multiple runners via interface pointer, all existing `IsClaudeAvailable()` coupling points fixed.
**Addresses features:** Backend-agnostic runner interface (table stakes #1)
**Avoids pitfalls:** #4 (subprocess assumptions), #8 (concrete type ownership), #15 (deadlock risk from `ExecuteSync` — audit call sites here)

### Phase 2: SSE Parser & Server Lifecycle (Independent Components)
**Rationale:** These components have zero dependencies on existing code and can be built and tested in isolation. The SSE parser is the most technically complex piece (chunked parsing, event mapping) and must be rock-solid before integration. Server lifecycle is straightforward process management. Building these before wiring into the subsystem means failures are contained.
**Delivers:** `FOpenCodeSSEParser` (tested with canned SSE data), `FOpenCodeServerLifecycle` (auto-detect, spawn, health check, shutdown), constants for ports/timeouts.
**Addresses features:** OpenCode server auto-spawn (table stakes), groundwork for streaming messages
**Avoids pitfalls:** #1 (SSE timeout — validated here), #5 (chunked frame parsing), #3 (zombie processes — PID file + shutdown hooks), #7 (MCP registration race — health polling), #9 (platform-specific spawning), #14 (heartbeat false alarms — 30s timeout)

### Phase 3: OpenCode HTTP Runner (Core Integration)
**Rationale:** With the interface generalized (Phase 1) and independent components proven (Phase 2), the runner can be assembled by composing them. This phase connects to a real `opencode serve` instance for the first time. Session creation, message sending, SSE event subscription, MCP registration, and cancellation all come together here.
**Delivers:** `FOpenCodeRunner` implementing `IClaudeRunner`, session CRUD, `POST /session/:id/prompt_async` + SSE event handling, MCP bridge registration via `POST /mcp`, `Cancel()` via `POST /session/:id/abort`.
**Addresses features:** Send/receive streaming messages (table stakes), OpenCode session lifecycle (table stakes), MCP tool registration (table stakes), Cancel requests (table stakes), Error display (table stakes)
**Avoids pitfalls:** #2 (non-game-thread callbacks — `AsyncTask` marshaling), #6 (global event bus — session ID filtering), #10 (HTTP concurrency — serialize setup calls), #13 (async message flow — SSE before prompt_async)

### Phase 4: UI Integration & Backend Switching
**Rationale:** UI changes are lowest risk and depend on the subsystem API being stable (from Phases 1+3). This is the user-visible payoff — the toolbar gets a backend selector, status text shows the active backend, and switching is seamless.
**Delivers:** Toolbar backend selector (combo box), backend availability indicators, active backend name in status text/role label, persisted backend preference.
**Addresses features:** Toolbar backend switcher (table stakes), Backend availability detection (table stakes), Persist default backend (table stakes)
**Avoids pitfalls:** #11 (unified session history — start with separate sessions per backend, clear chat on switch)

### Phase 5: Polish & Differentiators
**Rationale:** With core functionality working, add robustness and differentiating features. Auto-reconnect, permission handling, and agent/model selection all build on the working SSE connection and runner infrastructure.
**Delivers:** SSE auto-reconnect with exponential backoff, permission request handling UI, OpenCode agent/model selector in toolbar, optional basic auth support, unified conversation history with backend badges.
**Addresses features:** Connection health indicator (differentiator), Permission request handling (differentiator), Agent/model selection (differentiator), Unified conversation history (differentiator)
**Avoids pitfalls:** #12 (basic auth — optional credential fields)

### Phase Ordering Rationale

- **Dependency-driven:** Phase 1 (interface) → Phase 2 (components) → Phase 3 (composition) → Phase 4 (UI) → Phase 5 (polish) follows the compile-time dependency chain identified in the architecture research
- **Risk-first:** The riskiest technical challenge (SSE parsing) is isolated in Phase 2 where it can be unit-tested with canned data. The riskiest architectural change (subsystem modification) is Phase 1 but has the smallest scope (~45 lines)
- **Pitfall-aware:** Every phase's pitfall mitigations are addressed within that phase, not deferred
- **Value delivery:** Phase 3 produces a functional (if ugly) OpenCode integration. Phase 4 makes it user-facing. Phase 5 makes it polished. Partial completion still delivers value

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 2 (SSE Parser):** UE5.7's `FHttpModule` streaming behavior needs empirical verification — does `OnRequestProgress` expose partial response content? If yes, raw sockets are unnecessary. If no, the `FSocket` approach is required. This can only be determined by testing against a live `opencode serve` instance.
- **Phase 3 (HTTP Runner):** The exact SSE event payload structure (JSON field names within `data:` lines) needs validation against `opencode serve`'s `/doc` OpenAPI spec. Event type names are documented but property structures are not fully specified.

Phases with standard patterns (skip research-phase):
- **Phase 1 (Interface Generalization):** Pure refactoring of existing code with well-understood patterns
- **Phase 4 (UI Integration):** Standard Slate widget additions following existing toolbar patterns
- **Phase 5 (Polish):** All features use documented APIs and follow established codebase patterns

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All UE modules verified against existing build file. OpenCode API verified against official docs. Zero new dependencies needed. |
| Features | HIGH | Feature set derived from official OpenCode API docs + existing codebase analysis. Clear table-stakes vs differentiator separation. |
| Architecture | HIGH (structure), MEDIUM (SSE details) | Component boundaries and data flow are clear. SSE streaming approach in UE5.7 needs empirical validation — may need raw sockets vs HTTP module. |
| Pitfalls | HIGH | Source-level analysis of both UE HTTP internals and OpenCode server implementation. Existing codebase patterns already solve analogous problems (NDJSON buffering, game-thread marshaling). |

**Overall confidence:** HIGH

### Gaps to Address

- **SSE wire format verification:** The exact JSON structure within OpenCode SSE `data:` fields needs validation against a running `opencode serve` instance. Event type names are reliable; property names within `properties` may differ from documentation. Fetch the OpenAPI spec from `/doc` endpoint during Phase 2/3 implementation.
- **UE5.7 HTTP streaming behavior:** Whether `FHttpModule`'s `OnRequestProgress` delegate supports reading partial response content for indefinite SSE streams is unconfirmed. Phase 2 must test this empirically and fall back to `FSocket` if needed.
- **`prompt_async` vs `message` endpoint behavior:** Documentation confirms the difference (204 async vs blocking sync), but edge cases around SSE reconnection mid-generation need live testing.
- **Platform-specific process spawning:** `opencode` CLI path resolution and process spawning via `FPlatformProcess::CreateProc` needs testing on Windows, Linux, and macOS. Known differences in PATH inheritance and script wrapper handling.
- **OpenCode API versioning:** OpenCode is at v1.3.7 with fast release cadence (745 releases). API should be stable (OpenAPI spec + SDK generation suggest design-for-stability), but pin to a minimum version requirement.

## Sources

### Primary (HIGH confidence)
- OpenCode Server API docs — https://opencode.ai/docs/server (verified 2026-03-30)
- OpenCode SDK docs — https://opencode.ai/docs/sdk (verified 2026-03-30)
- OpenCode CLI docs — https://opencode.ai/docs/cli (verified 2026-03-30)
- OpenCode MCP servers docs — https://opencode.ai/docs/mcp-servers (verified 2026-03-30)
- OpenCode GitHub repo — https://github.com/anomalyco/opencode (v1.3.7, March 2026)
- Existing plugin source — `IClaudeRunner.h`, `ClaudeSubsystem.h/cpp`, `ClaudeCodeRunner.cpp`, `SClaudeToolbar.h`, `ClaudeEditorWidget.h/cpp`, `UnrealClaude.Build.cs`
- Existing architecture docs — `.planning/codebase/ARCHITECTURE.md`, `STRUCTURE.md`, `CONVENTIONS.md`, `CONCERNS.md`
- W3C SSE specification — https://html.spec.whatwg.org/multipage/server-sent-events.html
- UE5.7 HTTP module API — stable since UE4, `FHttpModule::Get().CreateRequest()`

### Secondary (MEDIUM confidence)
- OpenCode Plugins/event system docs — https://opencode.ai/docs/plugins — event type names reliable, exact SSE payload structure needs live verification
- UE5.7 `FHttpModule` streaming support — `OnRequestProgress` and `SetResponseBodyReceiveStreamDelegate` behavior for indefinite SSE connections unconfirmed in 5.7
- OpenCode server source (dev branch) — `server.ts`, `routes/event.ts` — SSE uses `Bus.subscribeAll()` with 10s heartbeat

### Tertiary (LOW confidence)
- None — all research backed by at least two corroborating sources

---
*Research completed: 2026-03-30*
*Ready for roadmap: yes*
