# Requirements: UnrealClaude — OpenCode Integration

**Defined:** 2026-03-30
**Core Value:** Users can get AI coding assistance inside the Unreal Editor from either Claude Code or OpenCode, choosing whichever backend suits their workflow, without leaving the editor or losing conversation context.

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### Abstraction Layer (ABST)

- [x] **ABST-01**: IClaudeRunner (or successor interface) generalized to IChatBackend with no Claude-specific assumptions — supports both subprocess and HTTP-based backends
- [x] **ABST-02**: EBackendType enum (Claude, OpenCode) introduced with serialization support for config persistence
- [x] **ABST-03**: ClaudeSubsystem routes chat requests to the active backend via the abstraction layer, swapping backends without restarting the editor

### Communication Layer (COMM)

- [ ] **COMM-01**: FOpenCodeRunner implements IChatBackend, sending prompts via REST POST to `opencode serve` and returning structured responses
- [ ] **COMM-02**: FOpenCodeSSEParser connects to the `/event` SSE endpoint, parses streaming events, and dispatches them to the chat UI in real time
- [ ] **COMM-03**: SSE events mapped to the existing chat UI message model (assistant text, tool calls, tool results, errors) with no UI-layer changes required
- [ ] **COMM-04**: Session lifecycle managed via REST API — create session (`POST /session`), send messages (`POST /session/:id/message`), retrieve history (`GET /session/:id`)
- [ ] **COMM-05**: OpenCode permission prompts (file writes, command execution) surfaced to the user through the existing permission dialog flow
- [ ] **COMM-06**: Agent type and model selection forwarded to OpenCode via the `POST /session/:id/prompt_async` payload

### Server Management (SRVR)

- [ ] **SRVR-01**: On backend switch to OpenCode, plugin auto-detects a running `opencode serve` instance by hitting `GET /global/health`
- [ ] **SRVR-02**: If no server detected, plugin spawns `opencode serve` as a managed child process on the configured port (default 4096)
- [ ] **SRVR-03**: On editor shutdown, managed `opencode serve` process is gracefully terminated (SIGTERM, then force-kill after timeout)
- [ ] **SRVR-04**: PID of managed server tracked in a temp file to prevent orphan processes on crash recovery
- [ ] **SRVR-05**: SSE connection auto-reconnects with exponential backoff (1s → 2s → 4s → max 30s) on disconnect without losing queued events

### MCP Integration (MCPI)

- [ ] **MCPI-01**: Existing MCP bridge URL dynamically registered with OpenCode via `POST /mcp` so all 30+ editor tools are available to the OpenCode backend

### Chat UI (CHUI)

- [ ] **CHUI-01**: Toolbar dropdown or toggle added to SClaudeToolbar allowing the user to switch between Claude and OpenCode backends
- [ ] **CHUI-02**: Backend availability indicator shown next to each option (green = connected, grey = unavailable, yellow = connecting)
- [ ] **CHUI-03**: User can cancel an in-flight OpenCode request, which sends `POST /session/:id/cancel` and updates UI accordingly
- [ ] **CHUI-04**: OpenCode-specific errors (server unreachable, auth failure, rate limit) displayed inline with actionable retry/settings buttons
- [ ] **CHUI-05**: Selected backend preference persisted in editor settings and restored on next editor launch
- [ ] **CHUI-06**: Conversation history displayed as a unified timeline — messages from both backends appear in sequence with a badge indicating which backend produced them
- [ ] **CHUI-07**: Health indicator visible in toolbar showing OpenCode server status when OpenCode backend is active

### Testing (TEST)

- [x] **TEST-00**: Existing unit test coverage baseline established; coverage increased to ≥90% for pre-existing components where feasible
- [ ] **TEST-01**: All new classes (FOpenCodeRunner, FOpenCodeSSEParser, FOpenCodeServerLifecycle) have ≥90% unit test coverage
- [ ] **TEST-02**: SSE parser tested against malformed streams, partial chunks, reconnect tokens, and multi-event batches
- [ ] **TEST-03**: Server lifecycle tested: spawn, health-check, reconnect, graceful shutdown, crash recovery, PID file cleanup
- [ ] **TEST-04**: Backend switching tested: mid-conversation switch, switch during streaming response, switch with pending tool calls

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Extended Backends

- **EXTB-01**: Support for additional backends beyond Claude and OpenCode (generic backend plugin system)
- **EXTB-02**: Per-project backend preference (different projects can default to different backends)

### Advanced Features

- **ADVF-01**: Cost tracking per backend with aggregated dashboard in editor
- **ADVF-02**: Backend performance comparison (response latency, token throughput) shown in settings
- **ADVF-03**: Conversation export to markdown/JSON with backend attribution

### Collaboration

- **COLB-01**: Shared sessions where multiple developers see the same AI conversation
- **COLB-02**: Backend recommendation based on task type (code gen → Claude, refactor → OpenCode, etc.)

## Out of Scope

| Feature | Reason |
|---------|--------|
| Replacing Claude backend with OpenCode | Explicitly rejected — both coexist as parallel options |
| CLI subprocess mode for OpenCode | HTTP server API chosen for richer streaming + MCP registration |
| Separate conversation histories per backend | User chose unified timeline with backend badges |
| OpenCode WebSocket transport | SSE is the documented transport; WebSocket not supported by OpenCode API |
| Custom OpenCode server builds | Plugin targets stock `opencode serve` from official CLI |
| Mobile/remote editor support | Desktop editor only; network transport not designed for WAN latency |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| TEST-00 | Phase 1: Test Baseline | Complete |
| ABST-01 | Phase 2: Backend Abstraction | Complete |
| ABST-02 | Phase 2: Backend Abstraction | Complete |
| ABST-03 | Phase 2: Backend Abstraction | Complete |
| COMM-02 | Phase 3: SSE Parser & Server Lifecycle | Pending |
| COMM-03 | Phase 3: SSE Parser & Server Lifecycle | Pending |
| SRVR-01 | Phase 3: SSE Parser & Server Lifecycle | Pending |
| SRVR-02 | Phase 3: SSE Parser & Server Lifecycle | Pending |
| SRVR-03 | Phase 3: SSE Parser & Server Lifecycle | Pending |
| SRVR-04 | Phase 3: SSE Parser & Server Lifecycle | Pending |
| SRVR-05 | Phase 3: SSE Parser & Server Lifecycle | Pending |
| TEST-02 | Phase 3: SSE Parser & Server Lifecycle | Pending |
| TEST-03 | Phase 3: SSE Parser & Server Lifecycle | Pending |
| COMM-01 | Phase 4: OpenCode Runner & Sessions | Pending |
| COMM-04 | Phase 4: OpenCode Runner & Sessions | Pending |
| COMM-05 | Phase 4: OpenCode Runner & Sessions | Pending |
| COMM-06 | Phase 4: OpenCode Runner & Sessions | Pending |
| MCPI-01 | Phase 4: OpenCode Runner & Sessions | Pending |
| TEST-01 | Phase 4: OpenCode Runner & Sessions | Pending |
| CHUI-01 | Phase 5: Chat UI & Backend Switching | Pending |
| CHUI-02 | Phase 5: Chat UI & Backend Switching | Pending |
| CHUI-03 | Phase 5: Chat UI & Backend Switching | Pending |
| CHUI-04 | Phase 5: Chat UI & Backend Switching | Pending |
| CHUI-05 | Phase 5: Chat UI & Backend Switching | Pending |
| CHUI-06 | Phase 5: Chat UI & Backend Switching | Pending |
| CHUI-07 | Phase 5: Chat UI & Backend Switching | Pending |
| TEST-04 | Phase 5: Chat UI & Backend Switching | Pending |

**Coverage:**
- v1 requirements: 27 total
- Mapped to phases: 27 ✓
- Unmapped: 0

---
*Requirements defined: 2026-03-30*
*Last updated: 2026-03-30 after initial definition*
