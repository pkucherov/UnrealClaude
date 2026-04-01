# Phase 3: SSE Parser & Server Lifecycle - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-01
**Phase:** 03-sse-parser-server-lifecycle
**Areas discussed:** SSE event mapping, Server lifecycle behavior, Disconnect/reconnect UX, SSE transport mechanism, Testing strategy, Error classification, API contract source, File organization, Server spawn arguments, Connection state machine

---

## SSE Event Mapping

| Option | Description | Selected |
|--------|-------------|----------|
| Map to existing types only | Map every OpenCode event to closest EChatStreamEventType. No new enum values. Keeps Phase 3 isolated | |
| Extend enum with new types | Add new EChatStreamEventType values for OpenCode-specific events that don't fit existing types | ✓ |
| Existing types + metadata overflow | Use existing types + metadata FString field for extra data | |

**User's choice:** Extend enum with new types
**Notes:** Chose to add PermissionRequest, StatusUpdate, AgentThinking, plus researcher discretion for additional types

| Option | Description | Selected |
|--------|-------------|----------|
| Stateless parser | Static Parse() function, no state between calls | |
| Stateful parser with internal buffer | Parser holds buffer, accumulates partial chunks, emits via delegate | ✓ |

**User's choice:** Stateful parser with internal buffer
**Notes:** Mirrors FClaudeCodeRunner's NdjsonLineBuffer pattern

| Option | Description | Selected |
|--------|-------------|----------|
| Delegate callback | Emit events via FOnChatStreamEvent delegate | ✓ |
| Return array from ProcessChunk | Return TArray from ProcessChunk() | |

**User's choice:** Delegate callback

| Option | Description | Selected |
|--------|-------------|----------|
| Log + skip | Log warning and skip malformed event | |
| Emit as error event | Emit FChatStreamEvent with bIsError=true and raw data in RawJson | ✓ |
| Log + emit error event | Both log and emit error event | |

**User's choice:** Emit as error event

| Option | Description | Selected |
|--------|-------------|----------|
| Track and use Last-Event-ID | Track SSE Last-Event-ID for replay on reconnect | |
| No event ID tracking | Don't track, reconnection starts fresh | |
| You decide after research | Let researcher verify OpenCode support first | ✓ |

**User's choice:** You decide after research

---

## Server Lifecycle Behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Health endpoint check | Hit GET /global/health on configured port | |
| Health check + port scan | Health endpoint + scan port range 4096-4100 | |
| PID file first, then health check | Read PID file first, check process alive, then health | ✓ |

**User's choice:** PID file first, then health check

| Option | Description | Selected |
|--------|-------------|----------|
| Saved/UnrealClaude/ | Per-project PID file under Saved/UnrealClaude/ | ✓ |
| System temp directory | System temp dir, no per-project isolation | |
| OpenCode data directory | ~/.opencode/ directory | |

**User's choice:** Saved/UnrealClaude/

| Option | Description | Selected |
|--------|-------------|----------|
| Retry 3x with delay | Retry spawn 3 times with 2s delay | |
| Fail fast, no retry | Immediate failure with error message | |
| Fail fast + try alternate ports | Try ports 4097-4099 if default fails | ✓ |

**User's choice:** Fail fast + try alternate ports

| Option | Description | Selected |
|--------|-------------|----------|
| Only kill managed servers | Only terminate plugin-spawned servers | ✓ |
| Kill any connected server | Terminate any connected server on shutdown | |
| Prompt user on shutdown | Ask user via dialog | |

**User's choice:** Only kill managed servers

| Option | Description | Selected |
|--------|-------------|----------|
| Stale PID cleanup + reconnect | Clean stale PID, reconnect to alive processes | |
| Stale PID cleanup + kill zombies + reconnect | Clean stale PID, kill unresponsive processes, reconnect | ✓ |

**User's choice:** Stale PID cleanup + kill zombies + reconnect

| Option | Description | Selected |
|--------|-------------|----------|
| Standalone, owned by runner | Standalone class owned by FOpenCodeRunner as member | ✓ |
| Singleton with static Get() | Global singleton pattern | |
| Owned by FChatSubsystem | Subsystem owns it | |

**User's choice:** Standalone, owned by runner

| Option | Description | Selected |
|--------|-------------|----------|
| Periodic health poll | Poll health endpoint every 10s while connected | ✓ |
| SSE connection drop only | Rely solely on SSE drop detection | |
| Health poll + SSE heartbeat | Health poll + keep-alive ping | |

**User's choice:** Periodic health poll

| Option | Description | Selected |
|--------|-------------|----------|
| Lazy on first use | Detect/spawn when user switches to OpenCode backend | ✓ |
| Eager at editor startup | Detect/spawn during StartupModule() | |
| Lazy on first message | Don't connect until first message sent | |

**User's choice:** Lazy on first use

| Option | Description | Selected |
|--------|-------------|----------|
| Config JSON + env var fallback | Config JSON priority, env var fallback, constant default | ✓ |
| Config JSON field | Config JSON only | |
| Environment variable | Env var only | |

**User's choice:** Config JSON + env var fallback

| Option | Description | Selected |
|--------|-------------|----------|
| 5s graceful + force kill | 5-second graceful timeout | |
| 2s graceful + force kill | 2-second graceful timeout | |
| 10s graceful + force kill | 10-second graceful timeout | ✓ |

**User's choice:** 10s graceful + force kill

---

## Disconnect/Reconnect UX

| Option | Description | Selected |
|--------|-------------|----------|
| Queue messages, non-blocking | User can type, messages queued, sent on reconnect | |
| Block input during disconnect | Input blocked, 'Reconnecting...' shown | ✓ |
| Allow typing, warn on send, no queue | Typing allowed but send warned, no queue | |

**User's choice:** Block input during disconnect

| Option | Description | Selected |
|--------|-------------|----------|
| Reset on any success | Reset backoff on successful reconnect OR health check | ✓ |
| Reset on SSE reconnect only | Reset only on SSE reconnect | |
| Reset on first event received | Reset only after first successful event | |

**User's choice:** Reset on any success

| Option | Description | Selected |
|--------|-------------|----------|
| Infinite retries | Retry indefinitely, backoff capped at 30s | ✓ |
| Max 10 retries then stop | Give up after 10 failures | |
| 5-minute retry window | Time-based limit | |

**User's choice:** Infinite retries

| Option | Description | Selected |
|--------|-------------|----------|
| Show partial + marker | Show partial response with '[disconnected]' marker | ✓ |
| Discard partial response | Throw away partial response | |
| Show partial + auto-resend | Show partial and auto-resend on reconnect | |

**User's choice:** Show partial + marker

---

## SSE Transport Mechanism

| Option | Description | Selected |
|--------|-------------|----------|
| Try FHttpModule, fallback to FSocket | Start with UE HTTP module, fall back to raw socket if needed | ✓ |
| Raw FSocket + FRunnable directly | Skip FHttpModule, use raw sockets | |
| FHttpModule for REST, FSocket for SSE | Split transport by request type | |

**User's choice:** Try FHttpModule, fallback to FSocket

| Option | Description | Selected |
|--------|-------------|----------|
| FHttpModule for REST | UE HTTP module for non-streaming calls | ✓ |
| FSocket for everything | Raw sockets for all HTTP | |
| Direct libcurl access | Access curl backend directly | |

**User's choice:** FHttpModule for REST

| Option | Description | Selected |
|--------|-------------|----------|
| FRunnable + game thread dispatch | Dedicated background thread, dispatch to game thread | ✓ |
| FHttpModule async callbacks | UE async callbacks, no dedicated thread | |
| FRunnable + tick-based consumption | Background thread, queue consumed on tick | |

**User's choice:** FRunnable + game thread dispatch

| Option | Description | Selected |
|--------|-------------|----------|
| Full HTTP client with logging/retry | FOpenCodeHttpClient with logging, retry logic, timeouts | ✓ |
| Thin HTTP client wrapper | Minimal wrapper around FHttpModule | |
| Direct FHttpModule calls | No wrapper, direct calls | |

**User's choice:** Full HTTP client with logging/retry

| Option | Description | Selected |
|--------|-------------|----------|
| One SSE connection | Single connection per FOpenCodeRunner instance | ✓ |
| Multiple SSE connections | Support multiple simultaneous connections | |

**User's choice:** One SSE connection

---

## Testing Strategy

| Option | Description | Selected |
|--------|-------------|----------|
| Inline test strings | Hardcoded FString constants in test files | |
| Test fixture files | Load from .txt files in Tests/Fixtures/ | |
| Recorded SSE captures | Capture real SSE output from live OpenCode server | ✓ |

**User's choice:** Recorded SSE captures

| Option | Description | Selected |
|--------|-------------|----------|
| Fully mocked | Mock all external dependencies (HTTP, process, file I/O) | ✓ |
| Real server when available, mock fallback | Integration tests when possible | |
| Mock process, real HTTP with test server | Middle ground approach | |

**User's choice:** Fully mocked

| Option | Description | Selected |
|--------|-------------|----------|
| >=90% with documented gaps | Same target as Phase 1 for all Phase 3 components | ✓ |
| 100% parser, >=90% lifecycle | Full coverage for parser, standard for lifecycle | |
| Scenario-based, no line target | Focus on scenarios over line counts | |

**User's choice:** >=90% with documented gaps

| Option | Description | Selected |
|--------|-------------|----------|
| Interface injection | Constructor injection of IHttpClient, IProcessLauncher interfaces | ✓ |
| Virtual method overrides | Override in test subclasses | |
| Swappable function pointers | Global function pointers tests can swap | |

**User's choice:** Interface injection

---

## Error Classification

| Option | Description | Selected |
|--------|-------------|----------|
| Typed error enum | EOpenCodeErrorType enum stored alongside bIsError in FChatStreamEvent | ✓ |
| String-based errors only | bIsError + error message string, no enum | |
| Enum + error code + message | Full enum + integer code + human message | |

**User's choice:** Typed error enum

| Option | Description | Selected |
|--------|-------------|----------|
| Emit as error events | Surface network errors as FChatStreamEvent with bIsError and typed enum | ✓ |
| Separate lifecycle error delegate | Lifecycle manager fires separate delegate | |
| Split: lifecycle for transport, parser for app errors | Separate delegates by error origin | |

**User's choice:** Emit as error events

---

## API Contract Source

| Option | Description | Selected |
|--------|-------------|----------|
| Docs + live validation | Research GitHub repo + official docs, validate against live instance | ✓ |
| Live /doc endpoint as primary | Use running server's /doc as primary spec | |
| Source code analysis only | Read Go source code for precise behavior | |

**User's choice:** Docs + live validation

| Option | Description | Selected |
|--------|-------------|----------|
| Latest stable at research time | Target latest stable release, document version | ✓ |
| Pin to specific version | Lock to known-good version | |
| Latest main branch | Target main branch | |

**User's choice:** Latest stable at research time

---

## File Organization

| Option | Description | Selected |
|--------|-------------|----------|
| Private/OpenCode/ subdirectory | Group all OpenCode files in subdirectory | ✓ |
| Flat with OpenCode prefix | Keep flat, prefix files with OpenCode | |
| Private/Backend/OpenCode/ hierarchy | Deeper nesting with Backend/ parent | |

**User's choice:** Private/OpenCode/ subdirectory

| Option | Description | Selected |
|--------|-------------|----------|
| Minimal public surface | Only IChatBackend.h, ChatBackendTypes.h, ChatBackendFactory.h public | ✓ |
| Runner public, internals private | FOpenCodeRunner header public | |

**User's choice:** Minimal public surface

---

## Server Spawn Arguments

| Option | Description | Selected |
|--------|-------------|----------|
| Minimal: --port + --format json | Pass port and format only | |
| Minimal + --quiet | Add quiet mode flag | |
| You decide after research | Let researcher determine flags | ✓ |

**User's choice:** You decide after research

| Option | Description | Selected |
|--------|-------------|----------|
| UE project root | FPaths::ProjectDir() as working directory | ✓ |
| Workspace root | Workspace root containing .uproject | |
| Configurable, default project root | User-configurable with project root default | |

**User's choice:** UE project root

---

## Connection State Machine

| Option | Description | Selected |
|--------|-------------|----------|
| 4-state enum + delegate | Disconnected, Connecting, Connected, Reconnecting | |
| 6-state enum + delegate | Disconnected, Detecting, Spawning, Connecting, Connected, Reconnecting | ✓ |
| Boolean + status string | Simple connected/disconnected with string | |

**User's choice:** 6-state enum + delegate

| Option | Description | Selected |
|--------|-------------|----------|
| Lifecycle manager owns it | FOpenCodeServerLifecycle owns state, fires delegate | ✓ |
| Dedicated state class | Separate FOpenCodeConnectionState class | |
| Runner owns it (Phase 4) | Deferred to FOpenCodeRunner | |

**User's choice:** Lifecycle manager owns it

---

## Agent's Discretion

- Last-Event-ID tracking — depends on OpenCode SSE support
- Exact command-line flags for `opencode serve` — researcher determines
- Additional EChatStreamEventType values — researcher may identify more
- Internal FOpenCodeHttpClient organization
- Health polling interval adjustment
- SSE `retry:` field handling

## Deferred Ideas

None — discussion stayed within phase scope.
