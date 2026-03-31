# Phase 2: Backend Abstraction - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md -- this log preserves the alternatives considered.

**Date:** 2026-03-31
**Phase:** 02-backend-abstraction
**Areas discussed:** Renaming scope, Request config model, Backend swap behavior, Prompt formatting ownership, Interface method set, Availability detection, UI availability check path, Enum serialization approach

---

## Renaming Scope

### How aggressive should the renaming be?

| Option | Description | Selected |
|--------|-------------|----------|
| Full rename types + files | Rename all 13+ types AND physical files. High churn but clean result | ✓ |
| Rename types only, keep filenames | Types renamed but files keep old names. Less churn, misleading filenames | |
| Wrapper/adapter, no rename | New IChatBackend wraps IClaudeRunner. Minimal churn, doubles abstractions | |

**User's choice:** Full rename types + files
**Notes:** None

### Plugin module name?

| Option | Description | Selected |
|--------|-------------|----------|
| Keep plugin name UnrealClaude | Module stays UnrealClaude, only internal abstractions generalized | ✓ |
| Rename plugin module too | Rename everything including .uplugin, Build.cs, module macros | |

**User's choice:** Keep plugin name UnrealClaude
**Notes:** None

### Naming prefix for generalized types?

| Option | Description | Selected |
|--------|-------------|----------|
| Chat prefix | IChatBackend, FChatStreamEvent, FChatRequestConfig, etc. | ✓ |
| AI prefix | IAIBackend, FAIStreamEvent. Could conflict with UE AI module | |
| Assistant prefix | IAssistantBackend, FAssistantStreamEvent. Longer but precise | |

**User's choice:** Chat prefix
**Notes:** None

### Test update strategy?

| Option | Description | Selected |
|--------|-------------|----------|
| Update all tests in Phase 2 | All 281 tests updated, FMockClaudeRunner -> FMockChatBackend. Clean baseline | ✓ |
| Aliases for tests, update later | Core types renamed, tests use typedefs until cleanup pass | |
| Update only broken tests | Only fix tests that fail compilation after rename | |

**User's choice:** Update all tests in Phase 2
**Notes:** None

### Stream event fields?

| Option | Description | Selected |
|--------|-------------|----------|
| Keep all fields, zero unused | NumTurns, TotalCostUsd stay. Non-Claude backends leave at 0. Simple flat struct | ✓ |
| Common fields + metadata map | Core fields + TSharedPtr<FJsonObject> for extras. Flexible but complex | |
| Inheritance hierarchy | Base + derived event types per backend. Requires downcasting | |

**User's choice:** Keep all fields, zero unused
**Notes:** None

### UI label source?

| Option | Description | Selected |
|--------|-------------|----------|
| Backend provides display name | IChatBackend::GetDisplayName() returns "Claude" or "OpenCode". UI adapts dynamically | ✓ |
| Generic 'Assistant' label | UI always shows "Assistant" regardless of backend | |
| You decide | Agent's discretion | |

**User's choice:** Backend provides display name
**Notes:** None

### Widget file rename?

| Option | Description | Selected |
|--------|-------------|----------|
| Rename widget files too | SClaudeEditorWidget -> SChatEditorWidget, etc. Consistent with full rename | ✓ |
| Keep widget names | Widgets are plugin-internal, keep Claude branding | |

**User's choice:** Rename widget files too
**Notes:** None

### Tab/menu naming?

| Option | Description | Selected |
|--------|-------------|----------|
| Keep 'Claude Assistant' | Product name, not backend indicator. Plugin is UnrealClaude | ✓ |
| Rename to 'AI Assistant' | More generic but naming disconnect with plugin | |
| Defer to Phase 5 | Decide during UI phase | |

**User's choice:** Keep 'Claude Assistant'
**Notes:** None

### Subsystem rename?

| Option | Description | Selected |
|--------|-------------|----------|
| Rename to FChatSubsystem | FClaudeCodeSubsystem -> FChatSubsystem, files renamed. Consistent | ✓ |
| Keep FClaudeCodeSubsystem | Plugin's orchestrator keeps Claude name | |

**User's choice:** Rename to FChatSubsystem
**Notes:** None

### Prompt options rename?

| Option | Description | Selected |
|--------|-------------|----------|
| Rename to FChatPromptOptions | FClaudePromptOptions -> FChatPromptOptions. All fields generic | ✓ |
| Keep current name | Plugin-level struct keeps old name | |

**User's choice:** Rename to FChatPromptOptions
**Notes:** None

---

## Request Config Model

### Config struct approach?

| Option | Description | Selected |
|--------|-------------|----------|
| Flat struct, union of all fields | One struct with all backend fields. Unused fields ignored. Simple | |
| Generic fields + extras map | Common fields + TSharedPtr<FJsonObject> for extras. Flexible | |
| Inheritance hierarchy | Base FChatRequestConfig + derived per backend. Type-safe | ✓ |

**User's choice:** Inheritance hierarchy
**Notes:** None

### Interface accepts derived config how?

| Option | Description | Selected |
|--------|-------------|----------|
| Static cast in backend | IChatBackend takes base by const ref. Backend static_casts internally | ✓ |
| Template on config type | IChatBackend<ConfigType> template. Compile-time safe but complex pointer storage | |
| Runtime type check | dynamic_cast or Type enum. Safe but adds RTTI overhead | |

**User's choice:** Static cast in backend
**Notes:** None

### Config creation ownership?

| Option | Description | Selected |
|--------|-------------|----------|
| Backend factory method | IChatBackend::CreateConfig() returns TUniquePtr<FChatRequestConfig>. Clean separation | ✓ |
| Subsystem switches on backend type | Subsystem constructs right type directly. Couples to all backends | |
| You decide | Agent's discretion | |

**User's choice:** Backend factory method
**Notes:** None

---

## Backend Swap Behavior

### In-flight request handling?

| Option | Description | Selected |
|--------|-------------|----------|
| Cancel in-flight, then swap | Cancel active request, destroy old, create new. Clean | |
| Block switch during active request | User must wait or cancel manually before switching | ✓ |
| Queue swap after completion | Current request finishes, then swap. Delayed | |

**User's choice:** Block switch during active request
**Notes:** None

### Backend instance lifecycle?

| Option | Description | Selected |
|--------|-------------|----------|
| Lazy create, keep alive | Map of backends. Persist once created. Lower switch time | |
| One at a time, create/destroy | Only active backend exists. Lower memory, slower switch | ✓ |
| Both created at startup | Always ready. Higher memory and startup cost | |

**User's choice:** One at a time, create/destroy
**Notes:** None

### Backend creation mechanism?

| Option | Description | Selected |
|--------|-------------|----------|
| Subsystem owns factory | FChatSubsystem has switch/factory for backend creation | |
| Separate factory class | FChatBackendFactory creates by type. Decoupled | ✓ |
| Static factory on interface | IChatBackend::Create(type). Requires static on interface | |

**User's choice:** Separate factory class
**Notes:** None

### Default backend at startup?

| Option | Description | Selected |
|--------|-------------|----------|
| Claude default, persist choice | Claude first launch, then last-used from settings. Backwards compatible | |
| Always Claude, no persistence | Always start Claude. Must switch each session | |
| Auto-detect, prefer OpenCode | If OpenCode binary found, use it; otherwise Claude | ✓ |

**User's choice:** Auto-detect, prefer OpenCode
**Notes:** OpenCode gets priority when available. Startup sequence needs availability check.

### History on backend swap?

| Option | Description | Selected |
|--------|-------------|----------|
| Shared history, backend formats internally | Same FChatSessionManager. Each backend formats history for its own prompt | ✓ |
| Separate histories per backend | Each backend has own session. Switching starts fresh | |
| You decide | Agent's discretion | |

**User's choice:** Shared history, backend formats internally
**Notes:** Aligned with PROJECT.md unified conversation history decision

---

## Prompt Formatting Ownership

### Who owns prompt formatting?

| Option | Description | Selected |
|--------|-------------|----------|
| Backend owns formatting | Backend implements FormatPrompt(). Claude does Human:/Assistant: | ✓ |
| Subsystem formats generically | Subsystem concatenates, backends receive pre-formatted string | |
| You decide | Agent's discretion | |

**User's choice:** Backend owns formatting
**Notes:** None

### Context gathering vs injection?

| Option | Description | Selected |
|--------|-------------|----------|
| Subsystem gathers, backend injects | FProjectContextManager stays in subsystem. Passes raw to FormatPrompt() | ✓ |
| Backend gathers and injects | Backend calls FProjectContextManager directly | |
| You decide | Agent's discretion | |

**User's choice:** Subsystem gathers, backend injects
**Notes:** None

---

## Interface Method Set

### Additional methods for IChatBackend?

| Option | Description | Selected |
|--------|-------------|----------|
| Minimal additions | GetDisplayName(), GetBackendType() added. No capabilities struct | ✓ |
| Include capabilities struct | Add GetCapabilities() with feature flags | |
| No new methods until Phase 4 | Keep exactly current 5 methods | |

**User's choice:** Minimal additions
**Notes:** None

### FormatPrompt() on interface?

| Option | Description | Selected |
|--------|-------------|----------|
| Add FormatPrompt | Part of IChatBackend contract. Backend receives raw data, returns config | ✓ |
| Keep internal to backend | Internal implementation detail. Not in interface | |

**User's choice:** Add FormatPrompt
**Notes:** None

---

## Availability Detection

### When does auto-detect run?

| Option | Description | Selected |
|--------|-------------|----------|
| Binary check at startup | Check at StartupModule. For Claude/OpenCode: binary on PATH. No network | ✓ |
| Lazy on first chat | No startup cost. Delayed default selection | |
| Background poll | Always up-to-date. Adds complexity | |

**User's choice:** Binary check at startup
**Notes:** None

### What counts as 'OpenCode available'?

| Option | Description | Selected |
|--------|-------------|----------|
| PATH search like Claude | Same platform-specific search pattern as GetClaudePath() | ✓ |
| Configurable path + PATH fallback | User sets path in settings, falls back to PATH | |
| You decide | Agent's discretion | |

**User's choice:** PATH search like Claude
**Notes:** None

---

## UI Availability Check Path

### How does UI check availability?

| Option | Description | Selected |
|--------|-------------|----------|
| Through subsystem | FChatSubsystem::Get().IsBackendAvailable(). Clean separation | ✓ |
| Static on interface | IChatBackend::IsAvailable(type). More direct | |
| You decide | Agent's discretion | |

**User's choice:** Through subsystem
**Notes:** None

---

## Enum Serialization Approach

### How does EChatBackendType serialize?

| Option | Description | Selected |
|--------|-------------|----------|
| Manual FString conversion | EnumToString/StringToEnum helpers. Consistent with codebase | ✓ |
| UENUM() with UE reflection | Automatic serialization. Would be first UENUM in codebase | |
| You decide | Agent's discretion | |

**User's choice:** Manual FString conversion
**Notes:** None

---

## Agent's Discretion

- Exact FChatRequestConfig base field set
- LogUnrealClaude log category rename decision
- Internal factory class organization
- Exact FormatPrompt() parameter types
- UE5.7 system prompt cache handling during swaps
- Test file split if exceeding 500-line limit

## Deferred Ideas

None -- discussion stayed within phase scope.
