# Phase 4: OpenCode Runner & Sessions - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-01
**Phase:** 04-opencode-runner-sessions
**Areas discussed:** Session lifecycle mapping, Permission bridge flow, MCP registration timing, Agent & model configuration

---

## Session Lifecycle Mapping

### Session Source Model

| Option | Description | Selected |
|--------|-------------|----------|
| Local-primary | FChatSessionManager remains single display/persistence layer. OpenCode session is an implementation detail. | ✓ |
| OpenCode-primary | OpenCode server session is source of truth. Plugin syncs local history TO OpenCode. | |
| Dual-write with merge | Both maintain independent histories, displayed merged in UI. | |

**User's choice:** Local-primary
**Notes:** Keeps the existing architecture simple. OpenCode session provides server-side context but the plugin's local history is what the UI displays and persists.

### Session Creation Timing

| Option | Description | Selected |
|--------|-------------|----------|
| One per editor session | Create one OpenCode session on init, reuse for all messages. | ✓ |
| One per prompt | New session per prompt. No server-side context carryover. | |
| Persistent across restarts | Persist session ID to disk. Resume on editor restart. | |

**User's choice:** One per editor session
**Notes:** Balances simplicity with context continuity. Server-side session accumulates context naturally within one editor session.

---

## Permission Bridge Flow

### Permission UI

| Option | Description | Selected |
|--------|-------------|----------|
| Modal dialog | Pause on permission.updated, show modal dialog, send response via REST. | ✓ |
| Inline chat widget | Permission prompts inline in chat with Approve/Deny buttons. | |
| Auto-approve (dev only) | Skip dialog entirely. Security concern. | |

**User's choice:** Modal dialog
**Notes:** Consistent with FScriptExecutionManager's existing permission UX pattern.

### SSE Stream During Permission

| Option | Description | Selected |
|--------|-------------|----------|
| Buffer events | SSE stream stays open, buffer events while modal shown, process after response. | ✓ |
| Hard pause | Pause SSE processing entirely. Risk of timeout. | |
| Continue processing | Process events normally behind dialog. | |

**User's choice:** Buffer events
**Notes:** No data loss, stream stays alive. Buffered events processed in order after user responds.

---

## MCP Registration Timing

### Registration Timing

| Option | Description | Selected |
|--------|-------------|----------|
| On connect | Register immediately after health check passes, before session creation. | ✓ |
| On first prompt | Register when first prompt is sent. Delays tool availability. | |
| On backend switch | Register during backend swap event. | |

**User's choice:** On connect
**Notes:** Ensures tools available from first prompt. Re-register on every reconnect.

### MCP Registration Failure

| Option | Description | Selected |
|--------|-------------|----------|
| Degrade gracefully | Log warning, continue without editor tools. Retry on reconnect. | ✓ |
| Block backend | Fail entire backend init. Strict but frustrating. | |
| Retry then degrade | 3 retries with 2s delays, then degrade. | |

**User's choice:** Degrade gracefully
**Notes:** OpenCode still useful for chat even without MCP tools. Automatic retry on reconnect covers transient failures.

---

## Agent & Model Configuration

### Configuration Level

| Option | Description | Selected |
|--------|-------------|----------|
| Hardcoded defaults | Sensible defaults, no user settings in Phase 4. | ✓ |
| Config file only | Settings in config file, no UI. | |
| Toolbar dropdowns | UI dropdowns in toolbar. Phase 5 scope overlap. | |

**User's choice:** Hardcoded defaults
**Notes:** Phase 5 can add UI for these if needed. Keep Phase 4 focused on core integration.

### Request Config Structure

| Option | Description | Selected |
|--------|-------------|----------|
| Derived config class | FOpenCodeRequestConfig : FChatRequestConfig with AgentType, ModelId, SessionId. | ✓ |
| Extend base class | Add fields to base FChatRequestConfig. Pollutes base. | |

**User's choice:** Derived config class
**Notes:** Matches FClaudeRequestConfig pattern from Phase 2.

---

## Agent's Discretion

- Exact OpenCode REST API endpoint names (researcher validates)
- Internal state management within FOpenCodeRunner
- Event buffering implementation details
- Permission dialog widget reuse vs. new dialog
- Thread synchronization details
- Session expiration edge cases

## Deferred Ideas

None — discussion stayed within phase scope.
