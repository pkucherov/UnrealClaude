# Phase 2: Backend Abstraction - Research

**Researched:** 2026-03-31
**Domain:** C++ interface refactoring, type renaming, runtime backend abstraction in Unreal Engine 5.7
**Confidence:** HIGH

## Summary

Phase 2 is a large-scale rename + abstraction refactor of the UnrealClaude plugin's chat interface layer. The phase has two interleaved concerns: (1) renaming 13+ Claude-prefixed types and their physical files to use `Chat`-prefix names, and (2) introducing new abstraction constructs (EChatBackendType enum, FChatBackendFactory, FormatPrompt/CreateConfig interface methods, GetDisplayName/GetBackendType methods). The rename touches ~35+ source files across the codebase including all 17 test files, 3 widget files, the subsystem, runner, session manager, and module entry point.

The key risk is compilation breakage from incomplete renames — a single missed reference in any of the 70+ source files will cause a build failure. The phase must be executed in a specific order: new interface files first, then consumers, then tests, with each plan producing a compilable state. Since this is a pure refactor (no behavioral changes except adding new interface methods), the existing 281 tests serve as the regression safety net.

**Primary recommendation:** Execute as a staged rename: create new abstraction files first (IChatBackend.h, ChatBackendTypes.h, EChatBackendType), then rename/refactor consumers file-by-file in dependency order, updating tests alongside each component they test. Never leave the codebase in an uncompilable state between plans.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Full rename of all 13+ Claude-prefixed types AND their physical .h/.cpp files to use `Chat` prefix
- **D-02:** Rename map: IClaudeRunner -> IChatBackend, FClaudeStreamEvent -> FChatStreamEvent, EClaudeStreamEventType -> EChatStreamEventType, FClaudeRequestConfig -> FChatRequestConfig, FOnClaudeResponse -> FOnChatResponse, FOnClaudeProgress -> FOnChatProgress, FOnClaudeStreamEvent -> FOnChatStreamEvent, FClaudePromptOptions -> FChatPromptOptions, FClaudeCodeSubsystem -> FChatSubsystem, FClaudeSessionManager -> FChatSessionManager
- **D-03:** Slate widgets renamed: SClaudeEditorWidget -> SChatEditorWidget, SClaudeInputArea -> SChatInputArea, SClaudeToolbar -> SChatToolbar (files renamed too)
- **D-04:** Plugin module stays `UnrealClaude` — .uplugin, Build.cs, IMPLEMENT_MODULE, LogUnrealClaude log category all unchanged
- **D-05:** Tools menu entry and docked tab label stay "Claude Assistant" — this is the product name, not a backend indicator
- **D-06:** FClaudeCodeRunner keeps its name — it IS the Claude backend implementation. Concrete backends keep Claude/OpenCode-specific names
- **D-07:** All 281 Phase 1 tests updated as part of the rename. FMockClaudeRunner -> FMockChatBackend. Every test file touched. No stale references left
- **D-08:** IChatBackend provides GetDisplayName() returning backend-specific name ("Claude", "OpenCode"). UI uses this for role labels and status text instead of hardcoded "Claude"
- **D-09:** FChatStreamEvent (renamed from FClaudeStreamEvent) keeps all fields flat — NumTurns, TotalCostUsd, DurationMs remain as fields. Backends that don't populate them leave values at 0. UI checks for valid values before displaying
- **D-10:** No inheritance hierarchy for events, no metadata map — one flat struct for all backends
- **D-11:** Inheritance hierarchy: base FChatRequestConfig with common fields (Prompt, SystemPrompt, WorkingDirectory, TimeoutSeconds, AttachedImagePaths, OnStreamEvent). FClaudeRequestConfig extends with Claude-specific fields (bSkipPermissions, bUseJsonOutput, AllowedTools)
- **D-12:** IChatBackend interface method takes FChatRequestConfig by const ref. FClaudeCodeRunner internally static_casts to FClaudeRequestConfig
- **D-13:** IChatBackend has a CreateConfig() factory method returning TUniquePtr<FChatRequestConfig>. Subsystem calls backend->CreateConfig(), populates common fields. Backend owns its derived fields
- **D-14:** Switching backends while a request is active is BLOCKED — user must wait for completion or manually cancel first
- **D-15:** One backend instance at a time — switching destroys the old backend and creates the new one
- **D-16:** Separate FChatBackendFactory class creates backends by EChatBackendType. Decoupled from subsystem
- **D-17:** Auto-detect at startup: prefer OpenCode if binary found on PATH, fall back to Claude. Binary check only (no server reachability check at startup)
- **D-18:** PATH search for OpenCode binary uses same platform-specific search pattern as existing GetClaudePath() — look for opencode/opencode.cmd/opencode.exe
- **D-19:** Shared conversation history across backend swaps — both backends read from and write to the same FChatSessionManager. Each backend formats the shared history into its own prompt format internally
- **D-20:** IChatBackend interface methods: ExecuteAsync(), ExecuteSync(), Cancel(), IsExecuting(), IsAvailable() (carried from IClaudeRunner) PLUS new methods: GetDisplayName(), GetBackendType(), FormatPrompt(), CreateConfig()
- **D-21:** FormatPrompt(History, SystemPrompt, ProjectContext) is part of the IChatBackend interface — each backend knows how to format prompts for its own API
- **D-22:** Minimal interface — no capabilities struct, no SupportsStreaming(). Add more methods in later phases when concrete needs arise
- **D-23:** Each backend owns its prompt formatting via FormatPrompt(). Claude formats as Human:/Assistant:, OpenCode will format differently in Phase 4
- **D-24:** Subsystem gathers context (FProjectContextManager output, UE5.7 system prompt) and passes raw strings to FormatPrompt(). Backend decides where to inject them
- **D-25:** Widget checks backend availability through FChatSubsystem::Get().IsBackendAvailable() — never calls concrete backend static methods directly
- **D-26:** Remove direct FClaudeCodeRunner::IsClaudeAvailable() calls from widget code
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

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| ABST-01 | IClaudeRunner generalized to IChatBackend with no Claude-specific assumptions — supports both subprocess and HTTP-based backends | Rename map (D-02), new interface methods (D-20), CreateConfig factory pattern (D-13), FChatRequestConfig inheritance (D-11) |
| ABST-02 | EChatBackendType enum (Claude, OpenCode) introduced with serialization support for config persistence | Enum design (D-27/D-28/D-29), constants pattern in UnrealClaudeConstants.h |
| ABST-03 | ClaudeSubsystem routes chat requests to the active backend via the abstraction layer, swapping backends without restarting the editor | Subsystem refactor, FChatBackendFactory (D-16), auto-detect (D-17), swap blocking (D-14), shared history (D-19) |
</phase_requirements>

## Standard Stack

This phase adds no external dependencies. Everything uses existing UE 5.7 C++ constructs.

### Core
| Library/Module | Version | Purpose | Why Standard |
|----------------|---------|---------|--------------|
| UE 5.7 Core C++ | 5.7.x | TUniquePtr, TSharedPtr, TArray, FString, delegates | Already in use throughout codebase |
| UE JSON module | 5.7.x | Config persistence for EChatBackendType | Already in Build.cs PrivateDependency |
| UE Automation Test | 5.7.x | IMPLEMENT_SIMPLE_AUTOMATION_TEST | Already in use for 281 tests |

### Supporting
| Library/Module | Version | Purpose | When to Use |
|----------------|---------|---------|-------------|
| FPlatformMisc | 5.7.x | GetEnvironmentVariable for PATH search | OpenCode binary detection (D-18) |
| FPlatformProcess | 5.7.x | CreateProc for binary existence check | Same as existing GetClaudePath() |
| IFileManager | 5.7.x | FileExists for binary path verification | Same pattern |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Manual enum FString helpers | UENUM() macro reflection | UENUM requires UObject — team chose manual (D-28) |
| TUniquePtr<FChatRequestConfig> from CreateConfig | Template CreateConfig<T>() | Template approach is simpler but locks the derived type at the call site; TUniquePtr is more flexible |
| Capabilities struct | Simple bool methods | Decision D-22 chose minimal interface; capabilities can be added in later phases |

**No new Build.cs dependencies needed.** All required UE modules are already included.

## Architecture Patterns

### Recommended Project Structure
```
Source/UnrealClaude/
├── Public/
│   ├── IChatBackend.h          # NEW: IChatBackend interface (replaces IClaudeRunner.h)
│   ├── ChatBackendTypes.h      # NEW: EChatBackendType, FChatStreamEvent, delegates, FChatRequestConfig base
│   ├── ChatSubsystem.h         # RENAMED from ClaudeSubsystem.h
│   ├── ChatSessionManager.h    # RENAMED from ClaudeSessionManager.h
│   ├── ChatEditorWidget.h      # RENAMED from ClaudeEditorWidget.h
│   ├── ChatPromptOptions.h     # NEW or inline in ChatSubsystem.h: FChatPromptOptions
│   ├── ClaudeCodeRunner.h      # KEPT: concrete Claude backend (implements IChatBackend)
│   ├── ClaudeRequestConfig.h   # NEW: FClaudeRequestConfig extends FChatRequestConfig
│   └── [unchanged files]
├── Private/
│   ├── ChatSubsystem.cpp       # RENAMED
│   ├── ChatSessionManager.cpp  # RENAMED
│   ├── ChatEditorWidget.cpp    # RENAMED
│   ├── ChatBackendFactory.h/.cpp # NEW: FChatBackendFactory
│   ├── ClaudeCodeRunner.cpp    # KEPT (updated includes)
│   ├── Widgets/
│   │   ├── SChatInputArea.h/.cpp   # RENAMED from SClaudeInputArea
│   │   └── SChatToolbar.h/.cpp     # RENAMED from SClaudeToolbar
│   └── Tests/
│       ├── TestUtils.h          # UPDATED: FMockChatBackend
│       ├── ChatBackendTests.cpp # RENAMED from ClaudeRunnerTests.cpp
│       ├── ChatSubsystemTests.cpp # RENAMED from ClaudeSubsystemTests.cpp
│       └── [all other test files updated]
```

### Pattern 1: Interface Inheritance for Config
**What:** Base FChatRequestConfig with common fields, derived FClaudeRequestConfig with Claude-specific fields.
**When to use:** When different backends need different config fields but share a common submission path.
**Example:**
```cpp
// In ChatBackendTypes.h (or IChatBackend.h)
struct UNREALCLAUDE_API FChatRequestConfig
{
    FString Prompt;
    FString SystemPrompt;
    FString WorkingDirectory;
    float TimeoutSeconds = 300.0f;
    TArray<FString> AttachedImagePaths;
    FOnChatStreamEvent OnStreamEvent;

    virtual ~FChatRequestConfig() = default;
};

// In ClaudeRequestConfig.h
struct UNREALCLAUDE_API FClaudeRequestConfig : public FChatRequestConfig
{
    bool bUseJsonOutput = false;
    bool bSkipPermissions = true;
    TArray<FString> AllowedTools;
};
```

### Pattern 2: Backend Factory with Enum Dispatch
**What:** Separate factory class creates backends by enum value.
**When to use:** Decouples subsystem from concrete backend constructors.
**Example:**
```cpp
// ChatBackendFactory.h
class FChatBackendFactory
{
public:
    static TUniquePtr<IChatBackend> Create(EChatBackendType Type);
    static EChatBackendType DetectPreferredBackend();
    static bool IsBinaryAvailable(EChatBackendType Type);
};
```

### Pattern 3: CreateConfig() Factory Method on Interface
**What:** Each backend creates its own config type via a virtual factory method.
**When to use:** Subsystem populates common fields without knowing the derived type.
**Example:**
```cpp
// IChatBackend
virtual TUniquePtr<FChatRequestConfig> CreateConfig() const = 0;

// FClaudeCodeRunner implementation
TUniquePtr<FChatRequestConfig> FClaudeCodeRunner::CreateConfig() const
{
    return MakeUnique<FClaudeRequestConfig>();
}
```

### Pattern 4: FormatPrompt() Delegation
**What:** Subsystem gathers raw context strings; backend formats them for its protocol.
**When to use:** Each backend has different prompt format requirements.
**Example:**
```cpp
// IChatBackend
virtual FString FormatPrompt(
    const TArray<TPair<FString, FString>>& History,
    const FString& SystemPrompt,
    const FString& ProjectContext) const = 0;

// FClaudeCodeRunner (existing Human:/Assistant: format)
FString FClaudeCodeRunner::FormatPrompt(...) const
{
    FString Formatted;
    for (const auto& Exchange : History)
    {
        Formatted += FString::Printf(TEXT("Human: %s\n\nAssistant: %s\n\n"),
            *Exchange.Key, *Exchange.Value);
    }
    return Formatted;
}
```

### Anti-Patterns to Avoid
- **Partial rename:** Never leave both old and new names coexisting in a compilable state — this invites dangling references. Each plan must complete the rename for its scope.
- **Cast without CreateConfig:** Don't `static_cast<FClaudeRequestConfig*>` a base config the subsystem created. Use `backend->CreateConfig()` so the backend owns the derived type.
- **Widget directly accessing concrete backend:** Widget must go through `FChatSubsystem::IsBackendAvailable()`, never through `FClaudeCodeRunner::IsClaudeAvailable()`.
- **Putting EChatBackendType in a UENUM:** Decision D-28 explicitly chose manual string helpers over UENUM reflection.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Cross-platform binary detection | Custom PATH parser | Copy GetClaudePath() pattern from ClaudeCodeRunner.cpp | Existing pattern handles Windows/Linux/macOS, npm paths, `where`/`which` fallback — proven in production |
| File rename in git | Manual git mv + content edit | git mv for rename tracking, then content-level refactor | Preserves blame history and makes merge conflicts clearer |
| Config persistence (EChatBackendType) | Custom file format | UE JSON serialization (FJsonObject + FFileHelper) | Already used by FClaudeSessionManager for session.json |
| Singleton pattern | Custom singleton | Existing static Get() pattern from FClaudeCodeSubsystem | Already established and tested in Phase 1 |

**Key insight:** This phase is almost entirely a rename + new interface layering. All building blocks (TUniquePtr ownership, singleton pattern, delegate types, PATH searching, JSON serialization) already exist in the codebase. No new patterns need invention.

## Runtime State Inventory

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | FChatSessionManager saves to `Saved/UnrealClaude/session.json` — format is backend-agnostic (user/assistant pairs). MCP config written to `Saved/UnrealClaude/mcp-config.json` | None — session format has no Claude-specific keys. File paths use "UnrealClaude" (the plugin name, unchanged per D-04) |
| Live service config | MCP HTTP server on port 3000, registered in Claude CLI via `--mcp-config` flag at runtime | None — MCP server is plugin infrastructure, not backend-specific. Config file path unchanged |
| OS-registered state | None — no Windows Task Scheduler tasks, no services, no system-level registrations | None |
| Secrets/env vars | `UNREAL_MCP_URL` env var used by MCP bridge — refers to plugin server, not backend | None — env var name references plugin, not "Claude" |
| Build artifacts | Compiled UE binaries in `PluginBuild/` and engine Marketplace folder carry `UnrealClaude` module name | None — module name unchanged (D-04). Rebuild required after rename but that's standard |

**The canonical question:** After every file in the repo is updated, what runtime systems still have the old string cached, stored, or registered? **Answer: None.** The "Claude" strings being renamed are purely compile-time C++ type names and file names. The plugin module name, log category, menu entries, and session files all retain their existing names per decisions D-04/D-05. The only runtime string being generalized is the UI's use of "Claude" for role labels (D-08: GetDisplayName() instead of hardcoded).

## Common Pitfalls

### Pitfall 1: Incomplete Rename Leaving Dangling References
**What goes wrong:** A `#include "IClaudeRunner.h"` or `FClaudeStreamEvent` reference survives in one file, causing compilation failure that cascades.
**Why it happens:** With 70+ source files and 182+ references to rename, it's easy to miss one.
**How to avoid:** After each rename step, do a project-wide grep for the old name. Build after each plan. Use `replaceAll` for bulk in-file renames.
**Warning signs:** Compiler errors mentioning "undeclared identifier" or "cannot open include file."

### Pitfall 2: File Rename Without Include Update
**What goes wrong:** Renaming `ClaudeSubsystem.h` to `ChatSubsystem.h` but not updating `#include "ClaudeSubsystem.h"` in all 8+ consumer files.
**Why it happens:** File rename and content update are separate operations.
**How to avoid:** For each file rename, immediately grep the old filename across the entire source tree and update all includes.
**Warning signs:** "Cannot open include file" errors.

### Pitfall 3: Circular Dependency from Split Headers
**What goes wrong:** Moving delegates/events to `ChatBackendTypes.h` and the interface to `IChatBackend.h` creates a circular include if both need each other.
**Why it happens:** Currently, delegates, events, config, and interface are all in one file (`IClaudeRunner.h`).
**How to avoid:** Types/delegates/events go in `ChatBackendTypes.h`. Interface `IChatBackend.h` includes `ChatBackendTypes.h`. Never have types include the interface. This is a clean one-way dependency.
**Warning signs:** "Incomplete type" or "use of undefined type" errors.

### Pitfall 4: Breaking the Singleton During Refactor
**What goes wrong:** FClaudeCodeSubsystem::Get() is a static singleton. If renamed mid-refactor while callers still reference the old name, the link fails.
**Why it happens:** The singleton is referenced from 10+ call sites (module, widgets, tests).
**How to avoid:** Rename the class AND all call sites in the same plan. Or use a typedef bridge temporarily.
**Warning signs:** Linker errors "unresolved external symbol."

### Pitfall 5: FClaudeCodeRunner Accidentally Renamed
**What goes wrong:** Per D-06, FClaudeCodeRunner keeps its name. An overzealous rename changes it.
**Why it happens:** Pattern-matching "Claude" prefix without checking the exclusion list.
**How to avoid:** The rename map in D-02 is explicit — only listed types get renamed. FClaudeCodeRunner is explicitly excluded.
**Warning signs:** Test failures for Claude-specific functionality.

### Pitfall 6: Static Method References After Interface Change
**What goes wrong:** `FClaudeCodeRunner::IsClaudeAvailable()` is a static method called from `SClaudeEditorWidget::IsClaudeAvailable()` and `FUnrealClaudeModule::StartupModule()`. After the abstraction, widgets should go through the subsystem (D-25/D-26), but the module startup still needs to check availability.
**Why it happens:** Static methods on concrete classes don't fit neatly into an interface pattern.
**How to avoid:** Add `IsBackendAvailable()` to FChatSubsystem. Widget uses subsystem. Module startup uses FChatBackendFactory::IsBinaryAvailable() or similar static check.
**Warning signs:** Widget still importing ClaudeCodeRunner.h after refactor.

### Pitfall 7: Test String Identifiers Broken
**What goes wrong:** Tests have string IDs like `"UnrealClaude.ClaudeRunner.Interface.MockRunnerAsIClaudeRunner"`. After renaming, these should be updated to avoid confusion, but changing them breaks test selection commands.
**Why it happens:** Test names are strings, not validated by compiler.
**How to avoid:** Update test string IDs to reflect new class names (e.g., `"UnrealClaude.ChatBackend.Interface.MockAsIChatBackend"`). Document the new test naming in PLAN.md.
**Warning signs:** Tests passing but with misleading names; `Automation RunTests` filter patterns breaking.

## Code Examples

Verified patterns from existing codebase:

### EChatBackendType Enum with Manual String Conversion
```cpp
// Source: Decision D-27/D-28, pattern from MCPErrors.h enum style
enum class EChatBackendType : uint8
{
    Claude,
    OpenCode
};

namespace ChatBackendUtils
{
    inline FString EnumToString(EChatBackendType Type)
    {
        switch (Type)
        {
        case EChatBackendType::Claude:   return TEXT("Claude");
        case EChatBackendType::OpenCode: return TEXT("OpenCode");
        default:                         return TEXT("Unknown");
        }
    }

    inline EChatBackendType StringToEnum(const FString& Str)
    {
        if (Str == TEXT("Claude"))   return EChatBackendType::Claude;
        if (Str == TEXT("OpenCode")) return EChatBackendType::OpenCode;
        return EChatBackendType::Claude; // Default fallback
    }
}
```

### IChatBackend Interface (Full)
```cpp
// Source: Decision D-20, extending existing IClaudeRunner pattern
class UNREALCLAUDE_API IChatBackend
{
public:
    virtual ~IChatBackend() = default;

    // Carried from IClaudeRunner
    virtual bool ExecuteAsync(
        const FChatRequestConfig& Config,
        FOnChatResponse OnComplete,
        FOnChatProgress OnProgress = FOnChatProgress()) = 0;
    virtual bool ExecuteSync(const FChatRequestConfig& Config, FString& OutResponse) = 0;
    virtual void Cancel() = 0;
    virtual bool IsExecuting() const = 0;
    virtual bool IsAvailable() const = 0;

    // New methods
    virtual FString GetDisplayName() const = 0;
    virtual EChatBackendType GetBackendType() const = 0;
    virtual FString FormatPrompt(
        const TArray<TPair<FString, FString>>& History,
        const FString& SystemPrompt,
        const FString& ProjectContext) const = 0;
    virtual TUniquePtr<FChatRequestConfig> CreateConfig() const = 0;
};
```

### FChatSubsystem Backend Swap
```cpp
// Source: Decision D-14/D-15/D-16
void FChatSubsystem::SetActiveBackend(EChatBackendType Type)
{
    if (Backend.IsValid() && Backend->IsExecuting())
    {
        UE_LOG(LogUnrealClaude, Warning,
            TEXT("Cannot switch backends while request is active"));
        return;
    }
    Backend = FChatBackendFactory::Create(Type);
    ActiveBackendType = Type;
}
```

### OpenCode Binary Detection (Pattern from GetClaudePath)
```cpp
// Source: ClaudeCodeRunner.cpp GetClaudePath(), Decision D-18
static FString GetOpenCodePath()
{
    // Same pattern as GetClaudePath but searching for opencode binary
    TArray<FString> PossiblePaths;
#if PLATFORM_WINDOWS
    FString PathEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
    TArray<FString> PathDirs;
    PathEnv.ParseIntoArray(PathDirs, TEXT(";"), true);
    for (const FString& Dir : PathDirs)
    {
        PossiblePaths.Add(FPaths::Combine(Dir, TEXT("opencode.cmd")));
        PossiblePaths.Add(FPaths::Combine(Dir, TEXT("opencode.exe")));
    }
#else
    FString PathEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
    TArray<FString> PathDirs;
    PathEnv.ParseIntoArray(PathDirs, TEXT(":"), true);
    for (const FString& Dir : PathDirs)
    {
        PossiblePaths.Add(FPaths::Combine(Dir, TEXT("opencode")));
    }
#endif
    for (const FString& Path : PossiblePaths)
    {
        if (IFileManager::Get().FileExists(*Path))
        {
            return Path;
        }
    }
    return FString();
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| IClaudeRunner with 5 methods | IChatBackend with 9 methods (5 + 4 new) | This phase | Enables backend-agnostic subsystem |
| FClaudeRequestConfig (monolithic) | FChatRequestConfig base + FClaudeRequestConfig derived | This phase | Each backend defines its own config extension |
| Direct FClaudeCodeRunner::IsClaudeAvailable() from widgets | FChatSubsystem::IsBackendAvailable() delegation | This phase | Widgets decoupled from concrete backends |
| Hardcoded "Claude" in UI role labels | IChatBackend::GetDisplayName() | This phase | UI adapts to active backend |

**Deprecated/outdated after this phase:**
- `IClaudeRunner.h` — replaced by `IChatBackend.h` + `ChatBackendTypes.h`
- `ClaudeSubsystem.h/.cpp` — replaced by `ChatSubsystem.h/.cpp`
- `ClaudeSessionManager.h/.cpp` — replaced by `ChatSessionManager.h/.cpp`
- `ClaudeEditorWidget.h/.cpp` — replaced by `ChatEditorWidget.h/.cpp`
- `SClaudeInputArea.h/.cpp` — replaced by `SChatInputArea.h/.cpp`
- `SClaudeToolbar.h/.cpp` — replaced by `SChatToolbar.h/.cpp`
- `ClaudeRunnerTests.cpp` — replaced by `ChatBackendTests.cpp`
- `ClaudeSubsystemTests.cpp` — replaced by `ChatSubsystemTests.cpp`

## Open Questions

1. **FChatRequestConfig base field set — what does OpenCode need?**
   - What we know: Claude needs Prompt, SystemPrompt, WorkingDirectory, TimeoutSeconds, AttachedImagePaths, OnStreamEvent (common), plus bSkipPermissions, bUseJsonOutput, AllowedTools (Claude-specific)
   - What's unclear: OpenCode's REST API payload fields won't be known until Phase 3/4 research
   - Recommendation: Use the common fields listed above as the base. OpenCode-specific fields will be added in Phase 4 when FOpenCodeRequestConfig is created. The virtual destructor on FChatRequestConfig ensures proper cleanup.

2. **LogUnrealClaude log category — rename?**
   - What we know: D-04 says the module name stays `UnrealClaude`. The log category is `LogUnrealClaude`.
   - What's unclear: Whether renaming the log category to `LogChat` or similar would improve clarity
   - Recommendation: **Keep as LogUnrealClaude.** It's the plugin name, not a backend indicator. Renaming it would require touching every UE_LOG call in 60+ files — massive change for zero functional benefit. This aligns with D-04's spirit.

3. **UE5.7 system prompt cache during backend swaps**
   - What we know: The cached system prompt is a `static const FString` in ClaudeSubsystem.cpp. It's backend-agnostic content (UE5.7 API guidance).
   - What's unclear: Whether each backend should get its own system prompt variant
   - Recommendation: **Keep the cached prompt as-is in FChatSubsystem.** Pass it to FormatPrompt() — each backend decides how to incorporate it. The cache doesn't need invalidation on swap since the content is the same regardless of backend.

4. **Test file split strategy for 500-line limit**
   - What we know: Some test files may exceed 500 lines after adding new abstraction tests
   - What's unclear: Which specific files will exceed the limit
   - Recommendation: Monitor during implementation. If `ChatBackendTests.cpp` (renamed from `ClaudeRunnerTests.cpp`) exceeds 500 lines with added factory/enum tests, split into `ChatBackendTests.cpp` (interface tests) and `ClaudeRunnerTests.cpp` (Claude-specific runner tests). Session manager tests and subsystem tests are both under 400 lines currently so should be fine.

5. **FormatPrompt parameter types**
   - What we know: D-21 specifies History + SystemPrompt + ProjectContext
   - What's unclear: Whether to use raw FString params or a structured FPromptContext
   - Recommendation: **Use raw FString params.** Three parameters is manageable. A struct adds unnecessary abstraction for what's currently a simple concatenation. Can refactor to struct in later phases if the parameter list grows.

## Affected File Inventory

### Files to RENAME (git mv)
| Old Path | New Path | Reason |
|----------|----------|--------|
| `Public/IClaudeRunner.h` | `Public/IChatBackend.h` | D-02 |
| `Public/ClaudeSubsystem.h` | `Public/ChatSubsystem.h` | D-02 |
| `Public/ClaudeSessionManager.h` | `Public/ChatSessionManager.h` | D-02 |
| `Public/ClaudeEditorWidget.h` | `Public/ChatEditorWidget.h` | D-03 |
| `Private/ClaudeSubsystem.cpp` | `Private/ChatSubsystem.cpp` | D-02 |
| `Private/ClaudeSessionManager.cpp` | `Private/ChatSessionManager.cpp` | D-02 |
| `Private/ClaudeEditorWidget.cpp` | `Private/ChatEditorWidget.cpp` | D-03 |
| `Private/Widgets/SClaudeInputArea.h` | `Private/Widgets/SChatInputArea.h` | D-03 |
| `Private/Widgets/SClaudeInputArea.cpp` | `Private/Widgets/SChatInputArea.cpp` | D-03 |
| `Private/Widgets/SClaudeToolbar.h` | `Private/Widgets/SChatToolbar.h` | D-03 |
| `Private/Widgets/SClaudeToolbar.cpp` | `Private/Widgets/SChatToolbar.cpp` | D-03 |
| `Private/Tests/ClaudeRunnerTests.cpp` | `Private/Tests/ChatBackendTests.cpp` | D-07 |
| `Private/Tests/ClaudeSubsystemTests.cpp` | `Private/Tests/ChatSubsystemTests.cpp` | D-07 |

### Files to CREATE
| Path | Purpose |
|------|---------|
| `Public/ChatBackendTypes.h` | EChatBackendType enum, FChatStreamEvent, all renamed delegates, base FChatRequestConfig |
| `Public/ClaudeRequestConfig.h` | FClaudeRequestConfig : FChatRequestConfig (Claude-specific fields split out) |
| `Private/ChatBackendFactory.h` | FChatBackendFactory declaration |
| `Private/ChatBackendFactory.cpp` | Factory implementation + OpenCode binary detection |

### Files to MODIFY (content only, no rename)
| Path | Changes |
|------|---------|
| `Public/ClaudeCodeRunner.h` | Update: implements IChatBackend, add new methods, include path update |
| `Private/ClaudeCodeRunner.cpp` | Update: implement GetDisplayName, GetBackendType, FormatPrompt, CreateConfig; update delegate types |
| `Private/UnrealClaudeModule.cpp` | Update: includes, use FChatSubsystem, add auto-detect |
| `Private/Tests/TestUtils.h` | Update: FMockChatBackend, implement new interface methods |
| `Private/Tests/SlateWidgetTests.cpp` | Update: widget class names, includes |
| `Private/Tests/ModuleTests.cpp` | Update: subsystem name, includes |
| `Private/Tests/SessionManagerTests.cpp` | Update: class name |
| `Private/Tests/ClipboardImageTests.cpp` | Update: type names, includes |
| `Private/Tests/MCPServerTests.cpp` | Check for any runner/subsystem references |
| `Public/UnrealClaudeConstants.h` | Add EChatBackendType constants namespace |

### Files UNCHANGED
All MCP tool files, MCP infrastructure, animation helpers, blueprint helpers, scripting infrastructure — none of these reference the types being renamed.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | UE Automation Test Framework (built into UE 5.7) |
| Config file | None — tests registered via IMPLEMENT_SIMPLE_AUTOMATION_TEST macro |
| Quick run command | `Automation RunTests UnrealClaude.ChatBackend` (after rename) |
| Full suite command | `Automation RunTests UnrealClaude` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| ABST-01 | IChatBackend has all 9 methods, FMockChatBackend implements them | unit | `Automation RunTests UnrealClaude.ChatBackend.Interface` | ❌ Wave 0 — new tests in ChatBackendTests.cpp |
| ABST-01 | FChatRequestConfig base/derived inheritance works | unit | `Automation RunTests UnrealClaude.ChatBackend.Config` | ❌ Wave 0 — new tests |
| ABST-01 | FClaudeCodeRunner implements IChatBackend correctly | unit | `Automation RunTests UnrealClaude.ChatBackend.ClaudeRunner` | ✅ Exists (rename from ClaudeRunnerTests.cpp) |
| ABST-02 | EChatBackendType serializes to/from string | unit | `Automation RunTests UnrealClaude.ChatBackend.Enum` | ❌ Wave 0 — new tests |
| ABST-03 | FChatSubsystem routes through abstraction, swaps backend | unit | `Automation RunTests UnrealClaude.ChatSubsystem` | ✅ Exists (rename from ClaudeSubsystemTests.cpp) |
| ABST-03 | Backend swap blocked during active request | unit | `Automation RunTests UnrealClaude.ChatSubsystem.BackendSwap` | ❌ Wave 0 — new test |
| ABST-03 | FChatBackendFactory creates backends by type | unit | `Automation RunTests UnrealClaude.ChatBackend.Factory` | ❌ Wave 0 — new tests |
| ABST-03 | Auto-detect prefers OpenCode if available | unit | `Automation RunTests UnrealClaude.ChatBackend.AutoDetect` | ❌ Wave 0 — new test |
| ALL | All 281 existing tests still pass after rename | regression | `Automation RunTests UnrealClaude` | ✅ All exist (will be renamed) |

### Sampling Rate
- **Per task commit:** Build the plugin. If editor available: `Automation RunTests UnrealClaude`
- **Per wave merge:** Full suite: `Automation RunTests UnrealClaude`
- **Phase gate:** Full suite green before verification

### Wave 0 Gaps
- [ ] New tests for IChatBackend interface contract (GetDisplayName, GetBackendType, FormatPrompt, CreateConfig)
- [ ] New tests for EChatBackendType enum string conversion
- [ ] New tests for FChatRequestConfig base/derived pattern
- [ ] New tests for FChatBackendFactory::Create() and DetectPreferredBackend()
- [ ] New tests for backend swap blocking during active request
- [ ] Updated FMockChatBackend in TestUtils.h implementing all 9 interface methods

## Discretion Recommendations

### FChatRequestConfig Base Field Set
**Recommendation:** Use these common fields:
- `FString Prompt` — universal
- `FString SystemPrompt` — universal
- `FString WorkingDirectory` — universal
- `float TimeoutSeconds = 300.0f` — universal
- `TArray<FString> AttachedImagePaths` — used by Claude, may be used by OpenCode
- `FOnChatStreamEvent OnStreamEvent` — universal streaming callback

Split out from base (Claude-specific, goes in FClaudeRequestConfig):
- `bool bUseJsonOutput` — Claude CLI flag
- `bool bSkipPermissions` — Claude CLI flag
- `TArray<FString> AllowedTools` — Claude CLI flag

**Confidence:** HIGH — based on analysis of existing IClaudeRunner.h fields and OpenCode REST API shape (POST body only needs prompt + system prompt).

### FormatPrompt Parameters
**Recommendation:** Three raw FString parameters:
```cpp
virtual FString FormatPrompt(
    const TArray<TPair<FString, FString>>& History,
    const FString& SystemPrompt,
    const FString& ProjectContext) const = 0;
```
This matches the existing `BuildPromptWithHistory` data flow in ClaudeSubsystem.cpp. The subsystem already gathers these three pieces independently.

### System Prompt Cache During Backend Swaps
**Recommendation:** Keep the static `CachedUE57SystemPrompt` in ChatSubsystem.cpp unchanged. It's UE5.7 engine guidance — backend-agnostic. Pass it to `FormatPrompt()` on every call. No invalidation needed on backend swap.

### Log Category
**Recommendation:** Keep `LogUnrealClaude` unchanged. The log category is the plugin's identity, not the active backend's identity. Consistent with D-04.

## Project Constraints (from CLAUDE.md)

- **Max 500 lines per file** — monitor new ChatBackendTypes.h and ChatBackendTests.cpp
- **Max 50 lines per function** — applies to all new method implementations
- **Copyright header:** Every new file must open with `// Copyright Natali Caggiano. All Rights Reserved.`
- **Doxygen comments:** `/** ... */` above all public classes and methods
- **File naming:** `PascalCase.h/.cpp` for all files
- **Commit convention:** `refactor:` for rename changes, `feat:` for new abstraction types
- **Pre-commit:** Run `Automation RunTests UnrealClaude` before committing (if editor available)
- **Test pattern:** `IMPLEMENT_SIMPLE_AUTOMATION_TEST` with `EditorContext | ProductFilter` flags
- **Boolean prefix:** `b` prefix for boolean members
- **Out-parameters:** `Out` prefix
- **TEXT() macro:** All string literals wrapped in TEXT()

## Sources

### Primary (HIGH confidence)
- Codebase analysis: Direct reading of all canonical reference files listed in CONTEXT.md
  - `IClaudeRunner.h` (153 lines) — current interface, all delegates and types
  - `ClaudeCodeRunner.h/.cpp` (101 + 1251 lines) — concrete implementation, subprocess pattern, GetClaudePath()
  - `ClaudeSubsystem.h/.cpp` (113 + 268 lines) — singleton, prompt building, history management
  - `ClaudeSessionManager.h/.cpp` (46 lines) — session persistence (backend-agnostic already)
  - `ClaudeEditorWidget.h/.cpp` (217 lines) — widget with IsClaudeAvailable() static call
  - `SClaudeInputArea.h/.cpp` (107 lines) — input widget
  - `SClaudeToolbar.h/.cpp` (50 lines) — toolbar widget
  - `TestUtils.h` (149 lines) — FMockClaudeRunner
  - `UnrealClaudeModule.cpp` (269 lines) — module startup with direct FClaudeCodeRunner references
  - `UnrealClaudeConstants.h` (236 lines) — constants pattern to follow
  - `UnrealClaude.Build.cs` (79 lines) — dependency list confirmed
- `.planning/codebase/STRUCTURE.md` — file organization and naming conventions
- `.planning/codebase/CONVENTIONS.md` — coding patterns and documentation requirements
- `.planning/codebase/TESTING.md` — test framework and known limitations
- Grep analysis: 182+ references to Claude-prefixed types across .cpp files, 28 references in .h files, 67+ references to widget/mock types

### Secondary (MEDIUM confidence)
- None needed — this is a codebase-internal refactor with no external dependencies

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new dependencies, all existing UE modules
- Architecture: HIGH — patterns directly from existing codebase, all decisions locked
- Pitfalls: HIGH — based on extensive codebase analysis and rename experience
- Code examples: HIGH — derived directly from existing code patterns

**Research date:** 2026-03-31
**Valid until:** 2026-04-30 (stable — internal refactor, no external API dependencies)
