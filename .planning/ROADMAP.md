# Roadmap: UnrealClaude — OpenCode Integration

## Overview

This roadmap adds OpenCode as a second AI backend to the UnrealClaude editor plugin. The journey starts by establishing a test baseline before any changes, then generalizing the existing Claude-specific interface into a backend-agnostic abstraction, building the independent OpenCode components (SSE parser, server lifecycle), composing them into a working HTTP runner with session and MCP support, and finally wiring the UI with a backend switcher and unified conversation history. Each phase delivers a verifiable capability that builds on the previous one.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Test Baseline** - Establish existing test coverage baseline before any code changes
- [x] **Phase 2: Backend Abstraction** - Generalize IClaudeRunner into backend-agnostic IChatBackend interface
- [x] **Phase 3: SSE Parser & Server Lifecycle** - Build and test the independent OpenCode components (SSE stream parsing, server auto-detect/spawn/shutdown)
- [ ] **Phase 4: OpenCode Runner & Sessions** - Compose components into FOpenCodeRunner with session management, MCP registration, and full API integration
- [ ] **Phase 5: Chat UI & Backend Switching** - Add toolbar backend switcher, status indicators, unified history, and backend switching tests

## Phase Details

### Phase 1: Test Baseline
**Goal**: Existing test coverage is measured and gaps are filled, providing a safety net before any refactoring begins
**Depends on**: Nothing (first phase)
**Requirements**: TEST-00
**Success Criteria** (what must be TRUE):
  1. All existing automation tests pass when run via `Automation RunTests UnrealClaude`
  2. Test coverage for pre-existing components (subsystem, runner, session manager, MCP tools) is documented with current counts
  3. Coverage gaps in core components (IClaudeRunner consumers, ClaudeSubsystem, session manager) are filled with new tests targeting ≥90% where feasible
**Plans:** 5 plans

Plans:
- [x] 01-01-PLAN.md — Test infrastructure (TestUtils.h) + ClaudeSubsystem + ClaudeRunner tests
- [x] 01-02-PLAN.md — SessionManager + MCPServer + Constants tests
- [x] 01-03-PLAN.md — ProjectContext + ScriptExecution + Module startup tests
- [x] 01-04-PLAN.md — Slate widget tests + MCPTaskQueue latent test fix attempt
- [x] 01-05-PLAN.md — Comprehensive coverage report + MCP bridge JS test documentation

### Phase 2: Backend Abstraction
**Goal**: The plugin's chat interface is fully backend-agnostic — any backend can be plugged in without touching the subsystem or UI
**Depends on**: Phase 1
**Requirements**: ABST-01, ABST-02, ABST-03
**Success Criteria** (what must be TRUE):
  1. IClaudeRunner (or successor IChatBackend) has no Claude-specific or subprocess-specific method signatures — both HTTP and subprocess backends can implement it without workarounds
  2. EBackendType enum exists with Claude and OpenCode values, serializes to/from config, and is usable in settings
  3. ClaudeSubsystem routes chat requests through the abstraction layer and can swap the active backend pointer at runtime without restarting the editor
  4. All existing Claude Code CLI functionality continues working identically after the refactor (no regressions in existing tests from Phase 1)
**Plans:** 1 plan

Plans:
- [x] 02-01-PLAN.md — Full rename + abstraction: new IChatBackend/ChatBackendTypes, rename 13+ files, refactor subsystem/widgets/runner, update all tests, add ~19 new abstraction tests

### Phase 3: SSE Parser & Server Lifecycle
**Goal**: The two most technically complex OpenCode components are built and thoroughly tested in isolation, before any integration with existing code
**Depends on**: Phase 2
**Requirements**: COMM-02, COMM-03, SRVR-01, SRVR-02, SRVR-03, SRVR-04, SRVR-05, TEST-02, TEST-03
**Success Criteria** (what must be TRUE):
  1. FOpenCodeSSEParser correctly parses well-formed SSE streams into the existing chat UI message model (assistant text, tool calls, tool results, errors) with no UI-layer changes required
  2. SSE parser handles malformed streams, partial chunks, reconnect tokens, and multi-event batches without crashing or losing data
  3. Plugin auto-detects a running `opencode serve` instance via health endpoint, and spawns one if none found
  4. Managed `opencode serve` process is gracefully terminated on editor shutdown, with PID file tracking preventing orphan processes on crash recovery
  5. SSE connection auto-reconnects with exponential backoff (1s → 2s → 4s → max 30s) on disconnect
**Plans:** 4/4 plans complete

Plans:
- [x] 03-01-PLAN.md — Foundation types, interfaces, constants, and mock implementations
- [x] 03-02-PLAN.md — FOpenCodeSSEParser (TDD) — stateful SSE chunk-to-event parser
- [x] 03-03-PLAN.md — FOpenCodeHttpClient — production HTTP/SSE client + interface tests
- [x] 03-04-PLAN.md — FOpenCodeServerLifecycle (TDD) — six-state server lifecycle manager

### Phase 4: OpenCode Runner & Sessions
**Goal**: A fully functional OpenCode backend can send and receive streaming messages, manage sessions, register MCP tools, and handle permissions — ready for UI wiring
**Depends on**: Phase 3
**Requirements**: COMM-01, COMM-04, COMM-05, COMM-06, MCPI-01, TEST-01
**Success Criteria** (what must be TRUE):
  1. FOpenCodeRunner implements IChatBackend, sends prompts via REST to `opencode serve`, and returns structured streaming responses through the same delegate chain as Claude
  2. Session lifecycle works end-to-end: create session, send messages, retrieve history via OpenCode's REST API
  3. Existing MCP bridge URL is dynamically registered with OpenCode via `POST /mcp`, making all 30+ editor tools available to the OpenCode backend
  4. OpenCode permission prompts (file writes, command execution) are surfaced to the user through the existing permission dialog flow
  5. All new classes (FOpenCodeRunner, FOpenCodeSSEParser, FOpenCodeServerLifecycle) have ≥90% unit test coverage
**Plans**: TBD

### Phase 5: Chat UI & Backend Switching
**Goal**: Users can switch between Claude and OpenCode from the chat toolbar, see connection status, and have a unified conversation history across both backends
**Depends on**: Phase 4
**Requirements**: CHUI-01, CHUI-02, CHUI-03, CHUI-04, CHUI-05, CHUI-06, CHUI-07, TEST-04
**Success Criteria** (what must be TRUE):
  1. User can switch between Claude and OpenCode backends via a toolbar dropdown without leaving the chat panel
  2. Backend availability is shown next to each option (green = connected, grey = unavailable, yellow = connecting) and a health indicator is visible when OpenCode is active
  3. User can cancel an in-flight OpenCode request, which sends the abort API call and updates UI accordingly
  4. OpenCode-specific errors (server unreachable, auth failure, rate limit) are displayed inline with actionable retry/settings buttons
  5. Selected backend preference persists in editor settings and is restored on next editor launch
  6. Conversation history displays as a unified timeline with backend badges, and backend switching mid-conversation, during streaming, and with pending tool calls all work correctly
**Plans**: TBD
**UI hint**: yes

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Test Baseline | 5/5 | Complete | 2026-03-31 |
| 2. Backend Abstraction | 1/1 | Complete | 2026-03-31 |
| 3. SSE Parser & Server Lifecycle | 4/4 | Complete    | 2026-04-01 |
| 4. OpenCode Runner & Sessions | 0/TBD | Not started | - |
| 5. Chat UI & Backend Switching | 0/TBD | Not started | - |
