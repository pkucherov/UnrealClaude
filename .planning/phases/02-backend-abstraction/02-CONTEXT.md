# Phase 2: Backend Abstraction - Context

**Gathered:** 2026-03-31
**Status:** Ready for planning

<domain>
## Phase Boundary

Generalize the existing Claude-specific IClaudeRunner interface into a backend-agnostic IChatBackend abstraction. Introduce EChatBackendType enum, refactor FChatSubsystem to route through the abstraction layer, and enable runtime backend swapping. All existing Claude Code CLI functionality must continue working identically. No new backends are implemented in this phase — only the abstraction that enables them.

</domain>

<decisions>
## Implementation Decisions

### Renaming Scope
- **D-01:** Full rename of all 13+ Claude-prefixed types AND their physical .h/.cpp files to use `Chat` prefix
- **D-02:** Rename map: IClaudeRunner -> IChatBackend, FClaudeStreamEvent -> FChatStreamEvent, EClaudeStreamEventType -> EChatStreamEventType, FClaudeRequestConfig -> FChatRequestConfig, FOnClaudeResponse -> FOnChatResponse, FOnClaudeProgress -> FOnChatProgress, FOnClaudeStreamEvent -> FOnChatStreamEvent, FClaudePromptOptions -> FChatPromptOptions, FClaudeCodeSubsystem -> FChatSubsystem, FClaudeSessionManager -> FChatSessionManager
- **D-03:** Slate widgets renamed: SClaudeEditorWidget -> SChatEditorWidget, SClaudeInputArea -> SChatInputArea, SClaudeToolbar -> SChatToolbar (files renamed too)
- **D-04:** Plugin module stays `UnrealClaude` — .uplugin, Build.cs, IMPLEMENT_MODULE, LogUnrealClaude log category all unchanged
- **D-05:** Tools menu entry and docked tab label stay "Claude Assistant" — this is the product name, not a backend indicator
- **D-06:** FClaudeCodeRunner keeps its name — it IS the Claude backend implementation. Concrete backends keep Claude/OpenCode-specific names
- **D-07:** All 281 Phase 1 tests updated as part of the rename. FMockClaudeRunner -> FMockChatBackend. Every test file touched. No stale references left
- **D-08:** IChatBackend provides GetDisplayName() returning backend-specific name ("Claude", "OpenCode"). UI uses this for role labels and status text instead of hardcoded "Claude"

### Stream Event Model
- **D-09:** FChatStreamEvent (renamed from FClaudeStreamEvent) keeps all fields flat — NumTurns, TotalCostUsd, DurationMs remain as fields. Backends that don't populate them leave values at 0. UI checks for valid values before displaying
- **D-10:** No inheritance hierarchy for events, no metadata map — one flat struct for all backends

### Request Config Model
- **D-11:** Inheritance hierarchy: base FChatRequestConfig with common fields (Prompt, SystemPrompt, WorkingDirectory, TimeoutSeconds, AttachedImagePaths, OnStreamEvent). FClaudeRequestConfig extends with Claude-specific fields (bSkipPermissions, bUseJsonOutput, AllowedTools)
- **D-12:** IChatBackend interface method takes FChatRequestConfig by const ref. FClaudeCodeRunner internally static_casts to FClaudeRequestConfig
- **D-13:** IChatBackend has a CreateConfig() factory method returning TUniquePtr<FChatRequestConfig>. Subsystem calls backend->CreateConfig(), populates common fields. Backend owns its derived fields

### Backend Swap Mechanism
- **D-14:** Switching backends while a request is active is BLOCKED — user must wait for completion or manually cancel first
- **D-15:** One backend instance at a time — switching destroys the old backend and creates the new one
- **D-16:** Separate FChatBackendFactory class creates backends by EChatBackendType. Decoupled from subsystem
- **D-17:** Auto-detect at startup: prefer OpenCode if binary found on PATH, fall back to Claude. Binary check only (no server reachability check at startup)
- **D-18:** PATH search for OpenCode binary uses same platform-specific search pattern as existing GetClaudePath() — look for opencode/opencode.cmd/opencode.exe
- **D-19:** Shared conversation history across backend swaps — both backends read from and write to the same FChatSessionManager. Each backend formats the shared history into its own prompt format internally

### Interface Method Set
- **D-20:** IChatBackend interface methods: ExecuteAsync(), ExecuteSync(), Cancel(), IsExecuting(), IsAvailable() (carried from IClaudeRunner) PLUS new methods: GetDisplayName(), GetBackendType(), FormatPrompt(), CreateConfig()
- **D-21:** FormatPrompt(History, SystemPrompt, ProjectContext) is part of the IChatBackend interface — each backend knows how to format prompts for its own API
- **D-22:** Minimal interface — no capabilities struct, no SupportsStreaming(). Add more methods in later phases when concrete needs arise

### Prompt Formatting Ownership
- **D-23:** Each backend owns its prompt formatting via FormatPrompt(). Claude formats as Human:/Assistant:, OpenCode will format differently in Phase 4
- **D-24:** Subsystem gathers context (FProjectContextManager output, UE5.7 system prompt) and passes raw strings to FormatPrompt(). Backend decides where to inject them

### UI Availability Check
- **D-25:** Widget checks backend availability through FChatSubsystem::Get().IsBackendAvailable() — never calls concrete backend static methods directly
- **D-26:** Remove direct FClaudeCodeRunner::IsClaudeAvailable() calls from widget code

### Enum Design
- **D-27:** EChatBackendType enum class with values: Claude, OpenCode
- **D-28:** Manual FString conversion helpers (EnumToString/StringToEnum) — no UENUM() reflection. Consistent with existing codebase patterns
- **D-29:** Stored as string in config JSON for persistence

### Agent's Discretion
- Exact FChatRequestConfig base field set (researcher should verify against OpenCode API needs)
- LogUnrealClaude log category — rename or keep (low-impact decision)
- Internal organization of the factory class
- Exact FormatPrompt() parameter types (raw strings vs. structured data)
- How to handle the UE5.7 system prompt cache during backend swaps
- Test file split strategy if individual test files exceed 500-line limit after rename

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Core interface being refactored
- `UnrealClaude/Source/UnrealClaude/Public/IClaudeRunner.h` -- Current IClaudeRunner interface, all delegates, FClaudeStreamEvent, FClaudeRequestConfig. This is the primary file being generalized
- `UnrealClaude/Source/UnrealClaude/Public/ClaudeCodeRunner.h` -- Concrete Claude runner. Stays as Claude backend impl but implements renamed interface
- `UnrealClaude/Source/UnrealClaude/Private/ClaudeCodeRunner.cpp` -- Subprocess spawning, NDJSON parsing, GetClaudePath(). Reference for OpenCode binary search pattern

### Subsystem being refactored
- `UnrealClaude/Source/UnrealClaude/Public/ClaudeSubsystem.h` -- FClaudeCodeSubsystem with FClaudePromptOptions, Runner storage, SendPrompt API
- `UnrealClaude/Source/UnrealClaude/Private/ClaudeSubsystem.cpp` -- BuildPromptWithHistory(), config construction, singleton pattern

### Session manager
- `UnrealClaude/Source/UnrealClaude/Public/ClaudeSessionManager.h` -- Session persistence API, rename target
- `UnrealClaude/Source/UnrealClaude/Private/ClaudeSessionManager.cpp` -- JSON serialization, max history enforcement

### UI widgets being renamed
- `UnrealClaude/Source/UnrealClaude/Public/ClaudeEditorWidget.h` -- Main chat widget, direct FClaudeCodeRunner dependency to remove
- `UnrealClaude/Source/UnrealClaude/Private/ClaudeEditorWidget.cpp` -- Hardcoded "Claude" labels, IsClaudeAvailable() static call, stream event handling
- `UnrealClaude/Source/UnrealClaude/Private/Widgets/SClaudeInputArea.h` -- Input widget rename
- `UnrealClaude/Source/UnrealClaude/Private/Widgets/SClaudeToolbar.h` -- Toolbar widget rename

### Test infrastructure
- `UnrealClaude/Source/UnrealClaude/Private/Tests/TestUtils.h` -- FMockClaudeRunner -> FMockChatBackend, test helpers

### Codebase analysis
- `.planning/codebase/ARCHITECTURE.md` -- Full component map, data flow diagrams, entry points
- `.planning/codebase/STRUCTURE.md` -- File locations, naming conventions, where to add new code
- `.planning/codebase/CONVENTIONS.md` -- Code patterns, file size limits, documentation requirements
- `.planning/codebase/TESTING.md` -- Test framework, runner patterns, known limitations

### Phase 1 artifacts
- `.planning/phases/01-test-baseline/01-CONTEXT.md` -- Test decisions, coverage targets, naming conventions established

### Requirements
- `.planning/REQUIREMENTS.md` -- ABST-01, ABST-02, ABST-03 define the phase requirements

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **IClaudeRunner interface** (`Public/IClaudeRunner.h`): 5-method abstract class that already serves as the abstraction boundary. Rename and extend rather than rewrite
- **FMockClaudeRunner** (`Private/Tests/TestUtils.h`): Test double implementing IClaudeRunner with synchronous callbacks. Rename to FMockChatBackend and update for new interface methods
- **FClaudeCodeRunner::GetClaudePath()**: Platform-specific binary search logic. Pattern to replicate for OpenCode binary detection
- **UnrealClaudeConstants.h**: Namespaced constants pattern. Add new EChatBackendType-related constants here

### Established Patterns
- **Singleton with static Get()**: FClaudeCodeSubsystem::Get() pattern. FChatSubsystem keeps this pattern
- **TUniquePtr ownership**: Subsystem owns runner via TUniquePtr. Same pattern for backend lifecycle
- **FRunnable for background threads**: FClaudeCodeRunner : FRunnable for async I/O. Pattern available for future OpenCode HTTP polling
- **FMCPToolResult-style result types**: Success/Error factory methods. Consider for backend operation results
- **IMPLEMENT_SIMPLE_AUTOMATION_TEST**: All tests use this macro with EditorContext|ProductFilter flags

### Integration Points
- **FUnrealClaudeModule::StartupModule()**: Where backend auto-detect and initial creation should happen
- **SClaudeEditorWidget::SendMessage()**: Entry point for chat flow — calls subsystem. Only change: remove direct FClaudeCodeRunner reference
- **SClaudeEditorWidget::IsClaudeAvailable()**: Direct static call that needs routing through subsystem
- **FClaudeSessionManager**: Backend-agnostic already — user/assistant pairs with no Claude-specific fields. Just needs rename
- **Build.cs**: May need no new dependencies — all required UE modules already included

</code_context>

<specifics>
## Specific Ideas

No specific requirements -- open to standard approaches within the decisions captured above.

</specifics>

<deferred>
## Deferred Ideas

None -- discussion stayed within phase scope.

</deferred>

---

*Phase: 02-backend-abstraction*
*Context gathered: 2026-03-31*
