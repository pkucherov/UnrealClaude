---
phase: 02-backend-abstraction
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  # NEW files (abstraction layer foundation)
  - UnrealClaude/Source/UnrealClaude/Public/ChatBackendTypes.h
  - UnrealClaude/Source/UnrealClaude/Public/IChatBackend.h
  - UnrealClaude/Source/UnrealClaude/Public/ClaudeRequestConfig.h
  - UnrealClaude/Source/UnrealClaude/Private/ChatBackendFactory.h
  - UnrealClaude/Source/UnrealClaude/Private/ChatBackendFactory.cpp
  # RENAMED files (core types + subsystem + session)
  - UnrealClaude/Source/UnrealClaude/Public/IClaudeRunner.h        # DELETED after rename
  - UnrealClaude/Source/UnrealClaude/Public/ClaudeSubsystem.h      # DELETED after rename
  - UnrealClaude/Source/UnrealClaude/Public/ChatSubsystem.h        # NEW (renamed)
  - UnrealClaude/Source/UnrealClaude/Private/ClaudeSubsystem.cpp   # DELETED after rename
  - UnrealClaude/Source/UnrealClaude/Private/ChatSubsystem.cpp     # NEW (renamed)
  - UnrealClaude/Source/UnrealClaude/Public/ClaudeSessionManager.h # DELETED after rename
  - UnrealClaude/Source/UnrealClaude/Public/ChatSessionManager.h   # NEW (renamed)
  - UnrealClaude/Source/UnrealClaude/Private/ClaudeSessionManager.cpp # DELETED after rename
  - UnrealClaude/Source/UnrealClaude/Private/ChatSessionManager.cpp   # NEW (renamed)
  # MODIFIED files (update to new interfaces)
  - UnrealClaude/Source/UnrealClaude/Public/ClaudeCodeRunner.h
  - UnrealClaude/Source/UnrealClaude/Private/ClaudeCodeRunner.cpp
  - UnrealClaude/Source/UnrealClaude/Public/UnrealClaudeConstants.h
  # RENAMED widget files
  - UnrealClaude/Source/UnrealClaude/Public/ClaudeEditorWidget.h    # DELETED after rename
  - UnrealClaude/Source/UnrealClaude/Public/ChatEditorWidget.h      # NEW (renamed)
  - UnrealClaude/Source/UnrealClaude/Private/ClaudeEditorWidget.cpp # DELETED after rename
  - UnrealClaude/Source/UnrealClaude/Private/ChatEditorWidget.cpp   # NEW (renamed)
  - UnrealClaude/Source/UnrealClaude/Private/Widgets/SClaudeInputArea.h   # DELETED after rename
  - UnrealClaude/Source/UnrealClaude/Private/Widgets/SChatInputArea.h     # NEW (renamed)
  - UnrealClaude/Source/UnrealClaude/Private/Widgets/SClaudeInputArea.cpp # DELETED after rename
  - UnrealClaude/Source/UnrealClaude/Private/Widgets/SChatInputArea.cpp   # NEW (renamed)
  - UnrealClaude/Source/UnrealClaude/Private/Widgets/SClaudeToolbar.h     # DELETED after rename
  - UnrealClaude/Source/UnrealClaude/Private/Widgets/SChatToolbar.h       # NEW (renamed)
  - UnrealClaude/Source/UnrealClaude/Private/Widgets/SClaudeToolbar.cpp   # DELETED after rename
  - UnrealClaude/Source/UnrealClaude/Private/Widgets/SChatToolbar.cpp     # NEW (renamed)
  # Module entry point
  - UnrealClaude/Source/UnrealClaude/Private/UnrealClaudeModule.cpp
  - UnrealClaude/Source/UnrealClaude/Public/UnrealClaudeModule.h
  # RENAMED + MODIFIED test files
  - UnrealClaude/Source/UnrealClaude/Private/Tests/TestUtils.h
  - UnrealClaude/Source/UnrealClaude/Private/Tests/ClaudeRunnerTests.cpp     # DELETED after rename
  - UnrealClaude/Source/UnrealClaude/Private/Tests/ChatBackendTests.cpp      # NEW (renamed + expanded)
  - UnrealClaude/Source/UnrealClaude/Private/Tests/ClaudeSubsystemTests.cpp  # DELETED after rename
  - UnrealClaude/Source/UnrealClaude/Private/Tests/ChatSubsystemTests.cpp    # NEW (renamed + expanded)
  - UnrealClaude/Source/UnrealClaude/Private/Tests/SessionManagerTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/SlateWidgetTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/ModuleTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/ClipboardImageTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/MCPServerTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/MCPToolTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/MCPTaskQueueTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/MCPParamValidatorTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/MCPAssetToolTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/MCPAnimBlueprintTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/MaterialAssetToolTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/CharacterToolTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/ConstantsTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/ProjectContextTests.cpp
  - UnrealClaude/Source/UnrealClaude/Private/Tests/ScriptExecutionTests.cpp
autonomous: true
requirements: [ABST-01, ABST-02, ABST-03]

must_haves:
  truths:
    - "IChatBackend interface exists with 9 pure virtual methods (5 carried + 4 new) and no Claude-specific types in its signatures"
    - "EChatBackendType enum has Claude and OpenCode values with string serialization helpers"
    - "FChatRequestConfig is the base config struct; FClaudeRequestConfig derives from it with Claude-specific fields"
    - "FClaudeCodeRunner implements IChatBackend (not IClaudeRunner) with GetDisplayName, GetBackendType, FormatPrompt, CreateConfig"
    - "FChatSubsystem routes requests through IChatBackend pointer and can swap backends at runtime via SetActiveBackend"
    - "FChatBackendFactory creates backends by EChatBackendType and detects preferred backend at startup"
    - "All widgets reference FChatSubsystem (not FClaudeCodeSubsystem) and check availability through subsystem (not static runner methods)"
    - "All 281 existing tests continue passing under their new class/file names"
    - "No file in the codebase contains a reference to the old type names (IClaudeRunner, FClaudeStreamEvent, etc.) after completion"
  artifacts:
    - path: "UnrealClaude/Source/UnrealClaude/Public/ChatBackendTypes.h"
      provides: "EChatBackendType, FChatStreamEvent, EChatStreamEventType, all renamed delegates, base FChatRequestConfig"
      contains: "enum class EChatBackendType"
    - path: "UnrealClaude/Source/UnrealClaude/Public/IChatBackend.h"
      provides: "IChatBackend pure virtual interface with 9 methods"
      contains: "class UNREALCLAUDE_API IChatBackend"
    - path: "UnrealClaude/Source/UnrealClaude/Public/ClaudeRequestConfig.h"
      provides: "FClaudeRequestConfig extending FChatRequestConfig"
      contains: "struct UNREALCLAUDE_API FClaudeRequestConfig : public FChatRequestConfig"
    - path: "UnrealClaude/Source/UnrealClaude/Private/ChatBackendFactory.h"
      provides: "FChatBackendFactory with Create, DetectPreferredBackend, IsBinaryAvailable"
      contains: "class FChatBackendFactory"
    - path: "UnrealClaude/Source/UnrealClaude/Private/ChatBackendFactory.cpp"
      provides: "Factory implementation, OpenCode binary detection"
      contains: "FChatBackendFactory::Create"
    - path: "UnrealClaude/Source/UnrealClaude/Public/ChatSubsystem.h"
      provides: "FChatSubsystem with SetActiveBackend, IsBackendAvailable, GetActiveBackendType"
      contains: "class UNREALCLAUDE_API FChatSubsystem"
  key_links:
    - from: "UnrealClaude/Source/UnrealClaude/Public/IChatBackend.h"
      to: "ChatBackendTypes.h"
      via: "includes FChatStreamEvent, delegates, FChatRequestConfig"
      pattern: '#include "ChatBackendTypes.h"'
    - from: "UnrealClaude/Source/UnrealClaude/Public/ClaudeCodeRunner.h"
      to: "IChatBackend.h"
      via: "implements IChatBackend"
      pattern: "class.*FClaudeCodeRunner.*IChatBackend"
    - from: "UnrealClaude/Source/UnrealClaude/Public/ChatSubsystem.h"
      to: "IChatBackend.h"
      via: "owns TUniquePtr<IChatBackend>"
      pattern: "TUniquePtr<IChatBackend>"
    - from: "UnrealClaude/Source/UnrealClaude/Private/ChatBackendFactory.cpp"
      to: "ClaudeCodeRunner.h"
      via: "creates FClaudeCodeRunner instances"
      pattern: "MakeUnique<FClaudeCodeRunner>"
    - from: "UnrealClaude/Source/UnrealClaude/Private/ChatEditorWidget.cpp"
      to: "ChatSubsystem.h"
      via: "FChatSubsystem::Get() for all backend interactions"
      pattern: "FChatSubsystem::Get()"
---

<objective>
Phase 2: Backend Abstraction — Generalize the existing Claude-specific IClaudeRunner interface into a backend-agnostic IChatBackend abstraction

Purpose: Transform the plugin's chat interface layer from Claude-specific to backend-agnostic, so that any backend (Claude, OpenCode, or future backends) can be plugged in without touching the subsystem or UI. This is the foundation that enables Phase 3–5 to add OpenCode without modifying core code.

Output: New abstraction files (IChatBackend.h, ChatBackendTypes.h, ClaudeRequestConfig.h, ChatBackendFactory.h/.cpp), renamed files (13 renames per D-01/D-02/D-03), updated FClaudeCodeRunner implementing IChatBackend, refactored FChatSubsystem with backend swapping, and all 281+ tests passing under new names.
</objective>

<execution_context>
@$HOME/.config/opencode/get-shit-done/workflows/execute-plan.md
@$HOME/.config/opencode/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/STATE.md
@.planning/REQUIREMENTS.md
@.planning/phases/02-backend-abstraction/02-CONTEXT.md
@.planning/phases/02-backend-abstraction/02-RESEARCH.md
@.planning/codebase/ARCHITECTURE.md
@.planning/codebase/STRUCTURE.md

<interfaces>
<!-- Key types and contracts the executor needs. Extracted from codebase. -->

From Public/IClaudeRunner.h (BEING REPLACED):
```cpp
DECLARE_DELEGATE_TwoParams(FOnClaudeResponse, const FString&, bool);
DECLARE_DELEGATE_OneParam(FOnClaudeProgress, const FString&);
DECLARE_DELEGATE_OneParam(FOnClaudeStreamEvent, const FClaudeStreamEvent&);

enum class EClaudeStreamEventType : uint8 {
    SessionInit, TextContent, ToolUse, ToolResult, Result, AssistantMessage, Unknown
};

struct UNREALCLAUDE_API FClaudeStreamEvent { /* 13 fields — see file */ };

struct UNREALCLAUDE_API FClaudeRequestConfig {
    FString Prompt;
    FString SystemPrompt;
    FString WorkingDirectory;
    bool bUseJsonOutput = false;
    bool bSkipPermissions = true;
    float TimeoutSeconds = 300.0f;
    TArray<FString> AllowedTools;
    TArray<FString> AttachedImagePaths;
    FOnClaudeStreamEvent OnStreamEvent;
};

class UNREALCLAUDE_API IClaudeRunner {
    virtual bool ExecuteAsync(const FClaudeRequestConfig&, FOnClaudeResponse, FOnClaudeProgress) = 0;
    virtual bool ExecuteSync(const FClaudeRequestConfig&, FString&) = 0;
    virtual void Cancel() = 0;
    virtual bool IsExecuting() const = 0;
    virtual bool IsAvailable() const = 0;
};
```

From Public/ClaudeSubsystem.h (BEING RENAMED):
```cpp
struct UNREALCLAUDE_API FClaudePromptOptions {
    bool bIncludeEngineContext = true;
    bool bIncludeProjectContext = true;
    FOnClaudeProgress OnProgress;
    FOnClaudeStreamEvent OnStreamEvent;
    TArray<FString> AttachedImagePaths;
};

class UNREALCLAUDE_API FClaudeCodeSubsystem {
    static FClaudeCodeSubsystem& Get();
    void SendPrompt(const FString&, FOnClaudeResponse, const FClaudePromptOptions&);
    IClaudeRunner* GetRunner() const;
    // ... history, session, cancel methods
};
```

From Public/ClaudeCodeRunner.h (BEING UPDATED — keeps name per D-06):
```cpp
class UNREALCLAUDE_API FClaudeCodeRunner : public IClaudeRunner, public FRunnable {
    static bool IsClaudeAvailable();
    static FString GetClaudePath();
    // ... all IClaudeRunner methods + FRunnable methods
};
```

From Private/Tests/TestUtils.h (BEING UPDATED):
```cpp
class FMockClaudeRunner : public IClaudeRunner { /* synchronous mock */ };
```

From Public/ClaudeSessionManager.h (BEING RENAMED):
```cpp
class UNREALCLAUDE_API FClaudeSessionManager {
    const TArray<TPair<FString, FString>>& GetHistory() const;
    void AddExchange(const FString&, const FString&);
    // ...
};
```
</interfaces>
</context>

<tasks>

<!-- ============================================================ -->
<!-- WAVE 1: Create new abstraction files (foundation layer)      -->
<!-- These files introduce the NEW types without touching any     -->
<!-- existing files. The codebase remains compilable because      -->
<!-- nothing includes these yet.                                  -->
<!-- ============================================================ -->

<task type="auto">
  <name>Task 1: Create ChatBackendTypes.h — enum, events, delegates, base config (Wave 1)</name>
  <files>UnrealClaude/Source/UnrealClaude/Public/ChatBackendTypes.h</files>
  <action>
Create a NEW file `Public/ChatBackendTypes.h` containing ALL renamed types from IClaudeRunner.h PLUS the new EChatBackendType enum. This file is the foundation — everything else depends on it.

**File header:** `// Copyright Natali Caggiano. All Rights Reserved.`
**Doxygen:** `/** ... */` block comment on every public type.

**Contents (in order):**

1. **EChatBackendType enum** (per D-27, D-28):
   ```cpp
   enum class EChatBackendType : uint8 { Claude, OpenCode };
   ```
   Plus `ChatBackendUtils` namespace with inline `EnumToString()` and `StringToEnum()` functions (per D-28, D-29). Default fallback to `Claude`. Use `TEXT()` macro for all string literals.

2. **Renamed delegates** (per D-02):
   - `FOnChatResponse` (was `FOnClaudeResponse`) — `DECLARE_DELEGATE_TwoParams`
   - `FOnChatProgress` (was `FOnClaudeProgress`) — `DECLARE_DELEGATE_OneParam`
   
3. **EChatStreamEventType enum** (per D-02, renamed from `EClaudeStreamEventType`):
   - Same values: `SessionInit, TextContent, ToolUse, ToolResult, Result, AssistantMessage, Unknown`

4. **FChatStreamEvent struct** (per D-02, D-09, D-10):
   - Exact same fields as `FClaudeStreamEvent` — flat struct, no inheritance, no metadata map
   - `UNREALCLAUDE_API` export macro
   - All fields keep their names and defaults

5. **FOnChatStreamEvent delegate** (per D-02):
   - `DECLARE_DELEGATE_OneParam(FOnChatStreamEvent, const FChatStreamEvent&)`

6. **FChatRequestConfig base struct** (per D-11, D-13):
   - Common fields: `Prompt`, `SystemPrompt`, `WorkingDirectory`, `TimeoutSeconds` (default 300.0f), `AttachedImagePaths`, `OnStreamEvent`
   - `virtual ~FChatRequestConfig() = default;` (needed for polymorphic deletion per D-11)
   - `UNREALCLAUDE_API` export macro
   - Does NOT contain `bUseJsonOutput`, `bSkipPermissions`, `AllowedTools` — those are Claude-specific

7. **FChatPromptOptions struct** (per D-02, renamed from `FClaudePromptOptions`):
   - Same fields: `bIncludeEngineContext`, `bIncludeProjectContext`, `OnProgress` (now `FOnChatProgress`), `OnStreamEvent` (now `FOnChatStreamEvent`), `AttachedImagePaths`
   - Same constructors

**Conventions:** Max 500 lines. Boolean members use `b` prefix. All `TEXT()` macro. Keep file well under limit — target ~180-220 lines.
  </action>
  <verify>
    <automated>File exists and contains: enum class EChatBackendType, struct FChatStreamEvent, struct FChatRequestConfig, DECLARE_DELEGATE for all 3 delegates, struct FChatPromptOptions. Grep the file for all expected type names.</automated>
  </verify>
  <done>ChatBackendTypes.h exists with EChatBackendType (Claude/OpenCode), string helpers, all renamed delegates, FChatStreamEvent, FChatRequestConfig base, FChatPromptOptions. No Claude-specific fields in base config.</done>
</task>

<task type="auto">
  <name>Task 2: Create IChatBackend.h — the backend-agnostic interface (Wave 1)</name>
  <files>UnrealClaude/Source/UnrealClaude/Public/IChatBackend.h</files>
  <action>
Create a NEW file `Public/IChatBackend.h` containing the `IChatBackend` pure virtual interface.

**File header:** `// Copyright Natali Caggiano. All Rights Reserved.`
**Include:** `#include "ChatBackendTypes.h"` (one-way dependency — types never include interface)

**IChatBackend interface** (per D-20, D-22):
```cpp
class UNREALCLAUDE_API IChatBackend
{
public:
    virtual ~IChatBackend() = default;

    // === Carried from IClaudeRunner (5 methods) ===
    virtual bool ExecuteAsync(
        const FChatRequestConfig& Config,
        FOnChatResponse OnComplete,
        FOnChatProgress OnProgress = FOnChatProgress()) = 0;
    virtual bool ExecuteSync(const FChatRequestConfig& Config, FString& OutResponse) = 0;
    virtual void Cancel() = 0;
    virtual bool IsExecuting() const = 0;
    virtual bool IsAvailable() const = 0;

    // === New methods (4 methods) ===
    virtual FString GetDisplayName() const = 0;          // D-08
    virtual EChatBackendType GetBackendType() const = 0;  // D-20
    virtual FString FormatPrompt(                         // D-21, D-23
        const TArray<TPair<FString, FString>>& History,
        const FString& SystemPrompt,
        const FString& ProjectContext) const = 0;
    virtual TUniquePtr<FChatRequestConfig> CreateConfig() const = 0; // D-13
};
```

**Doxygen:** Full `/** ... */` doc on the class and every pure virtual method with `@param` and `@return` annotations.

**IMPORTANT:** The interface takes `const FChatRequestConfig&` (the BASE type) in ExecuteAsync/ExecuteSync per D-12. FClaudeCodeRunner will internally static_cast to FClaudeRequestConfig.

Target: ~80-100 lines including docs.
  </action>
  <verify>
    <automated>File exists with `class UNREALCLAUDE_API IChatBackend`, includes ChatBackendTypes.h, has exactly 9 pure virtual methods (5 carried + 4 new). Grep for `= 0;` should show 9 matches.</automated>
  </verify>
  <done>IChatBackend.h exists with 9 pure virtual methods, proper includes, full Doxygen docs, UNREALCLAUDE_API export. No Claude-specific types in method signatures.</done>
</task>

<task type="auto">
  <name>Task 3: Create ClaudeRequestConfig.h — Claude-specific config extension (Wave 1)</name>
  <files>UnrealClaude/Source/UnrealClaude/Public/ClaudeRequestConfig.h</files>
  <action>
Create a NEW file `Public/ClaudeRequestConfig.h` containing the Claude-specific request config.

**File header:** `// Copyright Natali Caggiano. All Rights Reserved.`
**Include:** `#include "ChatBackendTypes.h"`

**FClaudeRequestConfig struct** (per D-11, D-12):
```cpp
struct UNREALCLAUDE_API FClaudeRequestConfig : public FChatRequestConfig
{
    /** Use JSON output format for structured responses (Claude CLI flag) */
    bool bUseJsonOutput = false;

    /** Skip permission prompts (--dangerously-skip-permissions) (Claude CLI flag) */
    bool bSkipPermissions = true;

    /** Allow Claude to use tools (Read, Write, Bash, etc.) (Claude CLI flag) */
    TArray<FString> AllowedTools;
};
```

This is the derived config that FClaudeCodeRunner::CreateConfig() returns. The subsystem populates common fields on the base, and FClaudeCodeRunner populates Claude-specific fields internally.

Target: ~40-50 lines.
  </action>
  <verify>
    <automated>File exists with `struct UNREALCLAUDE_API FClaudeRequestConfig : public FChatRequestConfig`, has bUseJsonOutput, bSkipPermissions, AllowedTools fields.</automated>
  </verify>
  <done>ClaudeRequestConfig.h exists with FClaudeRequestConfig deriving from FChatRequestConfig, containing only Claude-specific fields (bUseJsonOutput, bSkipPermissions, AllowedTools).</done>
</task>

<!-- ============================================================ -->
<!-- WAVE 2: Rename + refactor core files to use new types        -->
<!-- This is the big rename wave. Each file is renamed via git mv -->
<!-- and then its contents updated to use the new type names.     -->
<!-- The entire wave must be done atomically — partial rename     -->
<!-- leaves the codebase uncompilable.                            -->
<!-- ============================================================ -->

<task type="auto">
  <name>Task 4: Rename + refactor IClaudeRunner.h consumers and FClaudeCodeRunner (Wave 2)</name>
  <files>
    UnrealClaude/Source/UnrealClaude/Public/IClaudeRunner.h
    UnrealClaude/Source/UnrealClaude/Public/ClaudeCodeRunner.h
    UnrealClaude/Source/UnrealClaude/Private/ClaudeCodeRunner.cpp
  </files>
  <action>
**Step 1: Delete IClaudeRunner.h** — This file is fully replaced by IChatBackend.h + ChatBackendTypes.h. Use `git rm` to remove it. All types it contained now live in the new files.

**Step 2: Update ClaudeCodeRunner.h** (per D-06 — keeps its name):
- Change `#include "IClaudeRunner.h"` → `#include "IChatBackend.h"` and `#include "ClaudeRequestConfig.h"`
- Change class declaration: `class UNREALCLAUDE_API FClaudeCodeRunner : public IChatBackend, public FRunnable`
  (was `IClaudeRunner`, now `IChatBackend`)
- Update all method signatures to use renamed types:
  - `const FClaudeRequestConfig&` → `const FChatRequestConfig&` in ExecuteAsync/ExecuteSync signatures
  - `FOnClaudeResponse` → `FOnChatResponse`
  - `FOnClaudeProgress` → `FOnChatProgress`
- Keep `static bool IsClaudeAvailable()` and `static FString GetClaudePath()` — these are Claude-specific statics (D-06)
- Add 4 new method declarations:
  ```cpp
  virtual FString GetDisplayName() const override;
  virtual EChatBackendType GetBackendType() const override;
  virtual FString FormatPrompt(
      const TArray<TPair<FString, FString>>& History,
      const FString& SystemPrompt,
      const FString& ProjectContext) const override;
  virtual TUniquePtr<FChatRequestConfig> CreateConfig() const override;
  ```
- Update private member types: `FClaudeRequestConfig CurrentConfig` stays as `FClaudeRequestConfig` (the derived type, since this IS the Claude runner), but update delegate member types to `FOnChatResponse`, `FOnChatProgress`

**Step 3: Update ClaudeCodeRunner.cpp**:
- Replace `#include "IClaudeRunner.h"` → `#include "IChatBackend.h"`
- Add `#include "ClaudeRequestConfig.h"` if not already there
- Update ALL references to old type names throughout the file:
  - `FClaudeStreamEvent` → `FChatStreamEvent` (used in ParseAndEmitNdjsonLine)
  - `EClaudeStreamEventType` → `EChatStreamEventType`
  - `FOnClaudeResponse` → `FOnChatResponse`
  - `FOnClaudeProgress` → `FOnChatProgress`
  - `FOnClaudeStreamEvent` → `FOnChatStreamEvent`
  - `FClaudeRequestConfig` → `FChatRequestConfig` in method SIGNATURES only (matching the interface)
- In method BODIES where Claude-specific config fields are needed (bUseJsonOutput, bSkipPermissions, AllowedTools), use `static_cast<const FClaudeRequestConfig&>(Config)` per D-12. Store the cast result in `CurrentConfig` (which remains `FClaudeRequestConfig` type).
- Implement 4 new methods (each under 50 lines):
  - `GetDisplayName()`: return `TEXT("Claude")`
  - `GetBackendType()`: return `EChatBackendType::Claude`
  - `FormatPrompt()`: Move the existing `Human:/Assistant:` formatting logic from ClaudeSubsystem::BuildPromptWithHistory into here. This is per D-21/D-23 — each backend owns its prompt format
  - `CreateConfig()`: return `MakeUnique<FClaudeRequestConfig>()`

**CRITICAL per D-06:** FClaudeCodeRunner keeps its name. Do NOT rename it.
**CRITICAL:** After this task, IClaudeRunner.h no longer exists. Every file that included it must be updated in this wave.
  </action>
  <verify>
    <automated>IClaudeRunner.h is deleted (git status shows deletion). ClaudeCodeRunner.h declares `: public IChatBackend`. Grep entire Source/ for "IClaudeRunner" should return 0 matches in .h files (excluding test files handled in Task 8). Grep ClaudeCodeRunner.h for "GetDisplayName\|GetBackendType\|FormatPrompt\|CreateConfig" shows 4 matches.</automated>
  </verify>
  <done>IClaudeRunner.h removed. FClaudeCodeRunner now implements IChatBackend with all 9 methods. All old delegate/event type names replaced. FormatPrompt moved from subsystem to runner. CreateConfig returns FClaudeRequestConfig.</done>
</task>

<task type="auto">
  <name>Task 5: Rename + refactor session manager and subsystem (Wave 2)</name>
  <files>
    UnrealClaude/Source/UnrealClaude/Public/ClaudeSessionManager.h
    UnrealClaude/Source/UnrealClaude/Private/ClaudeSessionManager.cpp
    UnrealClaude/Source/UnrealClaude/Public/ChatSessionManager.h
    UnrealClaude/Source/UnrealClaude/Private/ChatSessionManager.cpp
    UnrealClaude/Source/UnrealClaude/Public/ClaudeSubsystem.h
    UnrealClaude/Source/UnrealClaude/Private/ClaudeSubsystem.cpp
    UnrealClaude/Source/UnrealClaude/Public/ChatSubsystem.h
    UnrealClaude/Source/UnrealClaude/Private/ChatSubsystem.cpp
  </files>
  <action>
**Step 1: Rename session manager files** via `git mv`:
- `Public/ClaudeSessionManager.h` → `Public/ChatSessionManager.h`
- `Private/ClaudeSessionManager.cpp` → `Private/ChatSessionManager.cpp`

**Step 2: Update ChatSessionManager.h contents**:
- Class name: `FClaudeSessionManager` → `FChatSessionManager` (per D-02)
- Update Doxygen: "Manages chat conversation session persistence and history" (remove "Claude")
- All else stays the same — this class is already backend-agnostic (per research)

**Step 3: Update ChatSessionManager.cpp contents**:
- Update `#include` to `"ChatSessionManager.h"`
- All `FClaudeSessionManager::` → `FChatSessionManager::` method prefixes
- Session file path stays `Saved/UnrealClaude/session.json` (per D-04 — plugin name unchanged)

**Step 4: Rename subsystem files** via `git mv`:
- `Public/ClaudeSubsystem.h` → `Public/ChatSubsystem.h`
- `Private/ClaudeSubsystem.cpp` → `Private/ChatSubsystem.cpp`

**Step 5: Refactor ChatSubsystem.h** — this is the biggest change in the file:
- Class rename: `FClaudeCodeSubsystem` → `FChatSubsystem` (per D-02)
- Update includes: `#include "IChatBackend.h"` instead of `IClaudeRunner.h`
- Add `#include "ChatBackendTypes.h"` for `EChatBackendType`, `FChatPromptOptions`
- Forward declare: `class FChatSessionManager;` (was `FClaudeSessionManager`), `class FChatBackendFactory;` (new), remove `class FClaudeCodeRunner;`
- Rename `FClaudePromptOptions` → `FChatPromptOptions` (already moved to ChatBackendTypes.h, so REMOVE the struct definition from this file — it now lives in ChatBackendTypes.h)
- Update all delegate types: `FOnClaudeResponse` → `FOnChatResponse`, `FOnClaudeProgress` → `FOnChatProgress`, `FOnClaudeStreamEvent` → `FOnChatStreamEvent`
- **Add new public methods** (per D-14, D-15, D-16, D-25):
  ```cpp
  /** Switch the active backend (per D-14: blocked if request active) */
  void SetActiveBackend(EChatBackendType Type);
  
  /** Get the active backend type */
  EChatBackendType GetActiveBackendType() const;
  
  /** Check if the active backend is available (per D-25) */
  bool IsBackendAvailable() const;
  
  /** Get the active backend's display name (per D-08) */
  FString GetBackendDisplayName() const;
  ```
- Change private members:
  - `TUniquePtr<FClaudeCodeRunner> Runner` → `TUniquePtr<IChatBackend> Backend`
  - Add: `EChatBackendType ActiveBackendType`
  - `TUniquePtr<FClaudeSessionManager> SessionManager` → `TUniquePtr<FChatSessionManager> SessionManager`
- Update `GetRunner()` to `GetBackend()` returning `IChatBackend*`
- **Remove** `BuildPromptWithHistory()` — this logic moves to `IChatBackend::FormatPrompt()` (per D-21/D-23)

**Step 6: Refactor ChatSubsystem.cpp**:
- Update includes: `"ChatSubsystem.h"`, `"IChatBackend.h"`, `"ChatSessionManager.h"`, `"ChatBackendFactory.h"`, remove old includes
- Rename all `FClaudeCodeSubsystem::` → `FChatSubsystem::`
- **Constructor**: Use `FChatBackendFactory::DetectPreferredBackend()` to determine initial backend type (per D-17), then `FChatBackendFactory::Create(Type)` to create it
- **SendPrompt refactor**: 
  - Use `Backend->CreateConfig()` to get the right config type (per D-13)
  - Populate common fields (Prompt, SystemPrompt, WorkingDirectory, TimeoutSeconds, etc.) on the base config
  - Call `Backend->FormatPrompt(History, SystemPrompt, ProjectContext)` instead of `BuildPromptWithHistory()` (per D-21/D-24)
  - Call `Backend->ExecuteAsync(Config, OnComplete, OnProgress)`
- **Implement SetActiveBackend** (per D-14/D-15):
  - Check `Backend->IsExecuting()` — if true, log warning and return
  - `Backend = FChatBackendFactory::Create(Type)`
  - `ActiveBackendType = Type`
- **Implement IsBackendAvailable**: delegate to `Backend->IsAvailable()` (per D-25)
- **Implement GetBackendDisplayName**: delegate to `Backend->GetDisplayName()` (per D-08)
- **Shared history**: Session manager is NOT destroyed on backend swap (per D-19)

**Conventions:** Max 50 lines per function. Extract helpers if needed.
  </action>
  <verify>
    <automated>git status shows ClaudeSessionManager.h/.cpp deleted, ChatSessionManager.h/.cpp exist. git status shows ClaudeSubsystem.h/.cpp deleted, ChatSubsystem.h/.cpp exist. Grep for "FClaudeCodeSubsystem" in Source/**/*.h returns 0 matches. Grep ChatSubsystem.h for "SetActiveBackend\|IsBackendAvailable\|GetBackendDisplayName\|GetActiveBackendType" shows 4 matches. Grep ChatSubsystem.h for "FClaudeCodeRunner" returns 0 matches.</automated>
  </verify>
  <done>Session manager renamed to FChatSessionManager. Subsystem renamed to FChatSubsystem with backend abstraction — owns TUniquePtr&lt;IChatBackend&gt;, has SetActiveBackend/IsBackendAvailable/GetBackendDisplayName, uses factory for creation, delegates prompt formatting to backend.</done>
</task>

<task type="auto">
  <name>Task 6: Create ChatBackendFactory.h/.cpp — factory + auto-detect (Wave 2)</name>
  <files>
    UnrealClaude/Source/UnrealClaude/Private/ChatBackendFactory.h
    UnrealClaude/Source/UnrealClaude/Private/ChatBackendFactory.cpp
  </files>
  <action>
Create NEW files `Private/ChatBackendFactory.h` and `Private/ChatBackendFactory.cpp`.

**File header:** `// Copyright Natali Caggiano. All Rights Reserved.`

**ChatBackendFactory.h** (per D-16):
```cpp
#pragma once
#include "CoreMinimal.h"
#include "ChatBackendTypes.h"

class IChatBackend;

/** Factory for creating chat backend instances by type */
class FChatBackendFactory
{
public:
    /** Create a backend instance for the given type */
    static TUniquePtr<IChatBackend> Create(EChatBackendType Type);

    /** Detect preferred backend at startup (per D-17) */
    static EChatBackendType DetectPreferredBackend();

    /** Check if a backend's binary/server is available */
    static bool IsBinaryAvailable(EChatBackendType Type);

private:
    /** Check if OpenCode binary exists on PATH (per D-18) */
    static FString GetOpenCodePath();
};
```

**ChatBackendFactory.cpp**:
- `#include "ChatBackendFactory.h"`
- `#include "IChatBackend.h"`
- `#include "ClaudeCodeRunner.h"`
- `#include "HAL/PlatformProcess.h"`
- `#include "HAL/FileManager.h"`
- `#include "Misc/Paths.h"`

**Create()** implementation:
```cpp
TUniquePtr<IChatBackend> FChatBackendFactory::Create(EChatBackendType Type)
{
    switch (Type)
    {
    case EChatBackendType::Claude:
        return MakeUnique<FClaudeCodeRunner>();
    case EChatBackendType::OpenCode:
        // Phase 4 will add FOpenCodeRunner here
        UE_LOG(LogUnrealClaude, Warning, TEXT("OpenCode backend not yet implemented, falling back to Claude"));
        return MakeUnique<FClaudeCodeRunner>();
    default:
        return MakeUnique<FClaudeCodeRunner>();
    }
}
```

**DetectPreferredBackend()** (per D-17):
- Call `GetOpenCodePath()` — if non-empty, return `EChatBackendType::OpenCode`
- Else return `EChatBackendType::Claude`
- NOTE: For Phase 2, since OpenCode runner doesn't exist yet, the factory falls back to Claude anyway. But the detection logic is in place.

**IsBinaryAvailable()**: Switch on type — `Claude` delegates to `FClaudeCodeRunner::IsClaudeAvailable()`, `OpenCode` checks `!GetOpenCodePath().IsEmpty()`

**GetOpenCodePath()** (per D-18): Copy the same pattern as `FClaudeCodeRunner::GetClaudePath()`:
- On Windows: search PATH dirs for `opencode.cmd`, `opencode.exe`
- On Linux/Mac: search PATH dirs for `opencode`
- Return first existing path or empty string

**Conventions:** Max 50 lines per function. Use `TEXT()` macro. Log to `LogUnrealClaude` category.
  </action>
  <verify>
    <automated>ChatBackendFactory.h exists with Create, DetectPreferredBackend, IsBinaryAvailable methods. ChatBackendFactory.cpp includes ClaudeCodeRunner.h and constructs FClaudeCodeRunner. Grep for "GetOpenCodePath" shows implementation in .cpp.</automated>
  </verify>
  <done>FChatBackendFactory created with Create() dispatching by EChatBackendType, DetectPreferredBackend() with OpenCode binary search, IsBinaryAvailable(). OpenCode case falls back to Claude until Phase 4.</done>
</task>

<task type="auto">
  <name>Task 7: Rename + refactor widgets and module entry point (Wave 2)</name>
  <files>
    UnrealClaude/Source/UnrealClaude/Public/ClaudeEditorWidget.h
    UnrealClaude/Source/UnrealClaude/Private/ClaudeEditorWidget.cpp
    UnrealClaude/Source/UnrealClaude/Public/ChatEditorWidget.h
    UnrealClaude/Source/UnrealClaude/Private/ChatEditorWidget.cpp
    UnrealClaude/Source/UnrealClaude/Private/Widgets/SClaudeInputArea.h
    UnrealClaude/Source/UnrealClaude/Private/Widgets/SClaudeInputArea.cpp
    UnrealClaude/Source/UnrealClaude/Private/Widgets/SChatInputArea.h
    UnrealClaude/Source/UnrealClaude/Private/Widgets/SChatInputArea.cpp
    UnrealClaude/Source/UnrealClaude/Private/Widgets/SClaudeToolbar.h
    UnrealClaude/Source/UnrealClaude/Private/Widgets/SClaudeToolbar.cpp
    UnrealClaude/Source/UnrealClaude/Private/Widgets/SChatToolbar.h
    UnrealClaude/Source/UnrealClaude/Private/Widgets/SChatToolbar.cpp
    UnrealClaude/Source/UnrealClaude/Private/UnrealClaudeModule.cpp
    UnrealClaude/Source/UnrealClaude/Public/UnrealClaudeModule.h
  </files>
  <action>
**Step 1: Rename widget files** via `git mv` (per D-03):
- `Public/ClaudeEditorWidget.h` → `Public/ChatEditorWidget.h`
- `Private/ClaudeEditorWidget.cpp` → `Private/ChatEditorWidget.cpp`
- `Private/Widgets/SClaudeInputArea.h` → `Private/Widgets/SChatInputArea.h`
- `Private/Widgets/SClaudeInputArea.cpp` → `Private/Widgets/SChatInputArea.cpp`
- `Private/Widgets/SClaudeToolbar.h` → `Private/Widgets/SChatToolbar.h`
- `Private/Widgets/SClaudeToolbar.cpp` → `Private/Widgets/SChatToolbar.cpp`

**Step 2: Update ChatEditorWidget.h** (per D-03):
- `#include "IChatBackend.h"` (was `IClaudeRunner.h`)
- Forward declare: `class SChatInputArea;` (was `SClaudeInputArea`)
- Rename class: `SClaudeEditorWidget` → `SChatEditorWidget` (per D-03)
- `SChatMessage` can keep its name (it was already `SChatMessage`, not `SClaudeMessage`)
- Update all internal type references: `FClaudeStreamEvent` → `FChatStreamEvent`, `EClaudeStreamEventType` → `EChatStreamEventType`
- Keep tab label as "Claude Assistant" (per D-05)

**Step 3: Update ChatEditorWidget.cpp**:
- Update includes: `"ChatEditorWidget.h"`, `"ChatSubsystem.h"` (was `ClaudeSubsystem.h`), `"Widgets/SChatInputArea.h"`, `"Widgets/SChatToolbar.h"`
- Remove `#include "ClaudeCodeRunner.h"` if present
- `FClaudeCodeSubsystem::Get()` → `FChatSubsystem::Get()` throughout
- **Replace IsClaudeAvailable() static call** (per D-25, D-26):
  - Was: `FClaudeCodeRunner::IsClaudeAvailable()` 
  - Now: `FChatSubsystem::Get().IsBackendAvailable()`
- Replace hardcoded `"Claude"` role labels with `FChatSubsystem::Get().GetBackendDisplayName()` where appropriate (per D-08). BUT keep "Claude Assistant" as the tab/window title per D-05
- Update all type references: `FClaudeStreamEvent` → `FChatStreamEvent`, etc.
- `SClaudeEditorWidget` → `SChatEditorWidget` throughout
- `FClaudePromptOptions` → `FChatPromptOptions`

**Step 4: Update SChatInputArea.h/.cpp**:
- Rename class: `SClaudeInputArea` → `SChatInputArea` (per D-03)
- Update includes and type references

**Step 5: Update SChatToolbar.h/.cpp**:
- Rename class: `SClaudeToolbar` → `SChatToolbar` (per D-03)
- Update includes and type references

**Step 6: Update UnrealClaudeModule.cpp** (per D-04 — module name stays):
- Update includes: `"ChatEditorWidget.h"` (was `ClaudeEditorWidget.h`), `"ChatSubsystem.h"` (was `ClaudeSubsystem.h`), remove `"ClaudeCodeRunner.h"` if present
- `FClaudeCodeSubsystem::Get()` → `FChatSubsystem::Get()` throughout
- `SClaudeEditorWidget` → `SChatEditorWidget` in tab spawner lambda
- **Keep all of:** `IMPLEMENT_MODULE(FUnrealClaudeModule, UnrealClaude)`, `LogUnrealClaude`, `ClaudeTabName("ClaudeAssistant")`, `FUnrealClaudeCommands` (per D-04, D-05)
- If StartupModule() directly references `FClaudeCodeRunner::IsClaudeAvailable()`, replace with `FChatBackendFactory::IsBinaryAvailable(EChatBackendType::Claude)` or `FChatSubsystem::Get().IsBackendAvailable()`

**Step 7: Update UnrealClaudeModule.h** if it has any of the renamed type references (check includes).

**Step 8: Update UnrealClaudeConstants.h** — add a `Backend` namespace (per research):
```cpp
namespace Backend
{
    /** Default port for OpenCode server */
    constexpr uint32 DefaultOpenCodePort = 4096;
    
    /** Default backend if no preference set */
    constexpr const TCHAR* DefaultBackendString = TEXT("Claude");
}
```
  </action>
  <verify>
    <automated>git status shows all 6 widget files renamed. Grep entire Source/ for "SClaudeEditorWidget\|SClaudeInputArea\|SClaudeToolbar" returns 0 matches (old class names gone). Grep for "FClaudeCodeSubsystem" returns 0 matches. UnrealClaudeModule.cpp contains "SChatEditorWidget" and "FChatSubsystem". Grep UnrealClaudeConstants.h for "Backend" namespace shows match.</automated>
  </verify>
  <done>All 3 widget classes renamed (SChatEditorWidget, SChatInputArea, SChatToolbar). Module updated to use FChatSubsystem. Widget no longer directly references FClaudeCodeRunner — uses subsystem for availability checks and display names. Constants updated with Backend namespace.</done>
</task>

<!-- ============================================================ -->
<!-- WAVE 3: Update ALL test files to use new names + add new     -->
<!-- abstraction tests. This is the final wave that ensures all   -->
<!-- 281 existing tests pass under new names, plus adds new tests -->
<!-- for the abstraction constructs.                              -->
<!-- ============================================================ -->

<task type="auto">
  <name>Task 8: Rename test files + update TestUtils.h + update all test references (Wave 3)</name>
  <files>
    UnrealClaude/Source/UnrealClaude/Private/Tests/TestUtils.h
    UnrealClaude/Source/UnrealClaude/Private/Tests/ClaudeRunnerTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/ChatBackendTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/ClaudeSubsystemTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/ChatSubsystemTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/SessionManagerTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/SlateWidgetTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/ModuleTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/ClipboardImageTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/MCPServerTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/MCPToolTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/MCPTaskQueueTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/MCPParamValidatorTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/MCPAssetToolTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/MCPAnimBlueprintTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/MaterialAssetToolTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/CharacterToolTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/ConstantsTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/ProjectContextTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/ScriptExecutionTests.cpp
  </files>
  <action>
**This is the largest task — every test file must be updated to remove all references to old type names.** Per D-07: "All 281 Phase 1 tests updated as part of the rename. FMockClaudeRunner -> FMockChatBackend. Every test file touched. No stale references left."

**Step 1: Update TestUtils.h**:
- `#include "IClaudeRunner.h"` → `#include "IChatBackend.h"` and `#include "ClaudeRequestConfig.h"`
- Rename: `FMockClaudeRunner` → `FMockChatBackend` (per D-07)
- Change base class: `: public IClaudeRunner` → `: public IChatBackend`
- Update all method signatures to use renamed types:
  - `const FClaudeRequestConfig&` → `const FChatRequestConfig&`
  - `FOnClaudeResponse` → `FOnChatResponse`
  - `FOnClaudeProgress` → `FOnChatProgress`
- **Add 4 new interface method implementations** to FMockChatBackend:
  ```cpp
  virtual FString GetDisplayName() const override { return TEXT("MockBackend"); }
  virtual EChatBackendType GetBackendType() const override { return EChatBackendType::Claude; }
  virtual FString FormatPrompt(
      const TArray<TPair<FString, FString>>& History,
      const FString& SystemPrompt,
      const FString& ProjectContext) const override
  {
      return History.Num() > 0 ? History.Last().Key : TEXT("");
  }
  virtual TUniquePtr<FChatRequestConfig> CreateConfig() const override
  {
      return MakeUnique<FChatRequestConfig>();
  }
  ```

**Step 2: Rename test files** via `git mv` (per D-07):
- `Private/Tests/ClaudeRunnerTests.cpp` → `Private/Tests/ChatBackendTests.cpp`
- `Private/Tests/ClaudeSubsystemTests.cpp` → `Private/Tests/ChatSubsystemTests.cpp`

**Step 3: Update ChatBackendTests.cpp** (was ClaudeRunnerTests.cpp):
- Update all `#include` directives: `"IChatBackend.h"`, `"ChatBackendTypes.h"`, `"ClaudeCodeRunner.h"`, `"ClaudeRequestConfig.h"`
- Replace ALL old type names with new ones throughout the file:
  - `FClaudeStreamEvent` → `FChatStreamEvent`
  - `EClaudeStreamEventType` → `EChatStreamEventType`
  - `FClaudeRequestConfig` → use `FChatRequestConfig` for base tests, `FClaudeRequestConfig` where testing Claude-specific fields
  - `IClaudeRunner` → `IChatBackend`
  - `FMockClaudeRunner` → `FMockChatBackend`
- Update test string identifiers:
  - `"UnrealClaude.ClaudeRunner.*"` → `"UnrealClaude.ChatBackend.*"` (per research Pitfall 7)
- Update test class names: `FClaudeRunner_*` → `FChatBackend_*`

**Step 4: Update ChatSubsystemTests.cpp** (was ClaudeSubsystemTests.cpp):
- Update includes: `"ChatSubsystem.h"` (was `ClaudeSubsystem.h`)
- Replace: `FClaudeCodeSubsystem` → `FChatSubsystem` throughout
- Replace: `FClaudePromptOptions` → `FChatPromptOptions`
- Replace: `IClaudeRunner` → `IChatBackend`
- Update test string identifiers: `"UnrealClaude.ClaudeSubsystem.*"` → `"UnrealClaude.ChatSubsystem.*"`
- Update test class names: `FClaudeSubsystem_*` → `FChatSubsystem_*`

**Step 5: Update SessionManagerTests.cpp**:
- Replace: `FClaudeSessionManager` → `FChatSessionManager`
- Update includes: `"ChatSessionManager.h"` (was `ClaudeSessionManager.h`)
- Update test identifiers if they reference "ClaudeSession" → "ChatSession"

**Step 6: Update SlateWidgetTests.cpp**:
- Replace: `SClaudeEditorWidget` → `SChatEditorWidget`, `SClaudeInputArea` → `SChatInputArea`, `SClaudeToolbar` → `SChatToolbar`
- Update includes: `"ChatEditorWidget.h"`, `"Widgets/SChatInputArea.h"`, `"Widgets/SChatToolbar.h"`, `"ChatSubsystem.h"`
- Replace: `FClaudeCodeSubsystem` → `FChatSubsystem`

**Step 7: Update ModuleTests.cpp**:
- Replace: `FClaudeCodeSubsystem` → `FChatSubsystem`
- Update includes: `"ChatSubsystem.h"`
- Any references to `IClaudeRunner` → `IChatBackend`

**Step 8: Update ClipboardImageTests.cpp**:
- Replace: `FClaudeRequestConfig` → `FClaudeRequestConfig` (this file tests Claude-specific config fields, so it keeps using the derived type)
- Replace: `FClaudeStreamEvent` → `FChatStreamEvent`
- Replace: `FClaudePromptOptions` → `FChatPromptOptions`
- Update includes: `"IChatBackend.h"`, `"ChatBackendTypes.h"`, `"ClaudeRequestConfig.h"`

**Step 9: Update remaining test files** (MCPServerTests, MCPToolTests, etc.):
- Most MCP test files do NOT reference the types being renamed — they work with `FMCPToolResult`, `IMCPTool`, etc.
- Scan each file for any `#include "IClaudeRunner.h"`, `#include "ClaudeSubsystem.h"`, etc. and update if found
- Scan for any `FClaudeCodeSubsystem`, `FMockClaudeRunner`, etc. and replace
- If a test file has zero references to renamed types, it needs no changes — but verify by grepping

**CRITICAL: After all test updates, do a comprehensive grep for every old type name across the ENTIRE Source/ directory:**
- `IClaudeRunner` — should be 0 matches (except comments that explain the rename history)
- `FClaudeStreamEvent` — should be 0 matches
- `EClaudeStreamEventType` — should be 0 matches
- `FOnClaudeResponse` — should be 0 matches
- `FOnClaudeProgress` — should be 0 matches
- `FOnClaudeStreamEvent` — should be 0 matches
- `FClaudeCodeSubsystem` — should be 0 matches
- `FClaudeSessionManager` — should be 0 matches
- `FClaudePromptOptions` — should be 0 matches
- `FMockClaudeRunner` — should be 0 matches
- `SClaudeEditorWidget` — should be 0 matches
- `SClaudeInputArea` — should be 0 matches
- `SClaudeToolbar` — should be 0 matches
- `#include "IClaudeRunner.h"` — should be 0 matches
- `#include "ClaudeSubsystem.h"` — should be 0 matches
- `#include "ClaudeSessionManager.h"` — should be 0 matches
- `#include "ClaudeEditorWidget.h"` — should be 0 matches

**NOTE on FClaudeRequestConfig:** This name is INTENTIONALLY kept (it's the Claude-specific derived config per D-11). Files that use `FClaudeRequestConfig` directly (like ClipboardImageTests, ClaudeCodeRunner) are correct.
**NOTE on FClaudeCodeRunner:** This name is INTENTIONALLY kept per D-06. References to it are correct.
  </action>
  <verify>
    <automated>Comprehensive grep of Source/ directory for all 17 old type names listed above — each returns 0 matches (excluding comments). git status shows ClaudeRunnerTests.cpp and ClaudeSubsystemTests.cpp deleted, ChatBackendTests.cpp and ChatSubsystemTests.cpp exist. TestUtils.h contains "FMockChatBackend" and "IChatBackend".</automated>
  </verify>
  <done>All 17+ test files updated. FMockChatBackend implements IChatBackend with all 9 methods. Test files renamed (ClaudeRunnerTests → ChatBackendTests, ClaudeSubsystemTests → ChatSubsystemTests). Zero references to old type names remain in Source/. All test string identifiers updated.</done>
</task>

<task type="auto" tdd="true">
  <name>Task 9: Add new abstraction tests — enum, factory, config, backend swap (Wave 3)</name>
  <files>
    UnrealClaude/Source/UnrealClaude/Private/Tests/ChatBackendTests.cpp
    UnrealClaude/Source/UnrealClaude/Private/Tests/ChatSubsystemTests.cpp
  </files>
  <behavior>
    - Test: EChatBackendType::Claude converts to "Claude" string and back
    - Test: EChatBackendType::OpenCode converts to "OpenCode" string and back
    - Test: StringToEnum with unknown string returns Claude (default fallback)
    - Test: FChatRequestConfig has virtual destructor (polymorphic deletion safe)
    - Test: FClaudeRequestConfig derives from FChatRequestConfig (inheritance works)
    - Test: FClaudeRequestConfig has Claude-specific fields (bUseJsonOutput, bSkipPermissions, AllowedTools)
    - Test: FMockChatBackend implements all 9 IChatBackend methods
    - Test: FMockChatBackend::GetDisplayName returns "MockBackend"
    - Test: FMockChatBackend::CreateConfig returns valid TUniquePtr
    - Test: FChatBackendFactory::Create(Claude) returns non-null backend
    - Test: FChatBackendFactory::Create(OpenCode) returns non-null backend (falls back to Claude for now)
    - Test: FChatBackendFactory::IsBinaryAvailable(Claude) delegates correctly
    - Test: FChatSubsystem::GetActiveBackendType returns the currently active type
    - Test: FChatSubsystem::IsBackendAvailable delegates to backend
    - Test: FChatSubsystem::GetBackendDisplayName returns backend name
  </behavior>
  <action>
**Add new test cases to ChatBackendTests.cpp** (the file renamed in Task 8). These tests verify the new abstraction constructs introduced in this phase. Add them after the existing (renamed) tests.

**New test section: EChatBackendType Enum** (tests for ABST-02):
- `FChatBackend_Enum_ClaudeToString` — verify `ChatBackendUtils::EnumToString(EChatBackendType::Claude)` returns `"Claude"`
- `FChatBackend_Enum_OpenCodeToString` — verify returns `"OpenCode"`
- `FChatBackend_Enum_StringToClaude` — verify `ChatBackendUtils::StringToEnum(TEXT("Claude"))` returns `EChatBackendType::Claude`
- `FChatBackend_Enum_StringToOpenCode` — verify returns `EChatBackendType::OpenCode`
- `FChatBackend_Enum_UnknownStringDefaultsToClaude` — verify `StringToEnum(TEXT("Garbage"))` returns `EChatBackendType::Claude`

**New test section: Config Inheritance** (tests for ABST-01):
- `FChatBackend_Config_BaseHasVirtualDestructor` — create `FChatRequestConfig*` pointing to `FClaudeRequestConfig`, delete via base pointer (no crash = pass). Use TUniquePtr<FChatRequestConfig> holding a FClaudeRequestConfig.
- `FChatBackend_Config_ClaudeSpecificFields` — create `FClaudeRequestConfig`, set `bUseJsonOutput=true`, `bSkipPermissions=false`, verify fields
- `FChatBackend_Config_BaseFieldsAccessibleViaBasePtr` — create `FClaudeRequestConfig`, access `Prompt`, `SystemPrompt`, `TimeoutSeconds` via `FChatRequestConfig*`

**New test section: Mock Backend Interface** (tests for ABST-01):
- `FChatBackend_Mock_ImplementsGetDisplayName` — verify `FMockChatBackend::GetDisplayName()` returns non-empty
- `FChatBackend_Mock_ImplementsGetBackendType` — verify returns a valid enum value
- `FChatBackend_Mock_ImplementsCreateConfig` — verify returns non-null TUniquePtr
- `FChatBackend_Mock_ImplementsFormatPrompt` — verify returns FString (not crashing)

**New test section: Factory** (tests for ABST-03):
- `FChatBackend_Factory_CreateClaude` — verify `FChatBackendFactory::Create(EChatBackendType::Claude)` returns non-null, `GetBackendType() == Claude`
- `FChatBackend_Factory_CreateOpenCode` — verify returns non-null (falls back to Claude for now), note in test comment this will change in Phase 4
- `FChatBackend_Factory_IsBinaryAvailable` — verify `IsBinaryAvailable(EChatBackendType::Claude)` returns a boolean without crashing

**Add new test cases to ChatSubsystemTests.cpp** (tests for ABST-03):
- `FChatSubsystem_Backend_GetActiveBackendType` — verify `FChatSubsystem::Get().GetActiveBackendType()` returns a valid EChatBackendType value
- `FChatSubsystem_Backend_IsBackendAvailable` — verify `IsBackendAvailable()` returns boolean without crash
- `FChatSubsystem_Backend_GetDisplayName` — verify `GetBackendDisplayName()` returns non-empty string
- `FChatSubsystem_Backend_GetBackendNotNull` — verify `GetBackend()` returns non-null pointer

**CRITICAL:** Check that ChatBackendTests.cpp stays under 500 lines after additions. If it exceeds 500 lines, split the new abstraction tests into a separate file `ChatBackendAbstractionTests.cpp`. Monitor total line count.

**Test identifiers follow pattern:** `"UnrealClaude.ChatBackend.{SubCategory}.{TestName}"` and `"UnrealClaude.ChatSubsystem.Backend.{TestName}"`
**Test class names follow pattern:** `FChatBackend_{Category}_{Scenario}` and `FChatSubsystem_Backend_{Scenario}`
**All tests use:** `EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter`
  </action>
  <verify>
    <automated>Grep ChatBackendTests.cpp for "EChatBackendType\|FChatBackendFactory\|CreateConfig\|FormatPrompt\|GetDisplayName" shows multiple matches for new tests. Grep ChatSubsystemTests.cpp for "GetActiveBackendType\|IsBackendAvailable\|GetBackendDisplayName" shows matches. Line count of ChatBackendTests.cpp is ≤ 500 lines (or split file exists). Total new test count: ~19 new tests.</automated>
  </verify>
  <done>~19 new tests covering EChatBackendType enum serialization (5), config inheritance (3), mock backend interface (4), factory creation (3), and subsystem backend routing (4). All new tests use standard IMPLEMENT_SIMPLE_AUTOMATION_TEST pattern.</done>
</task>

</tasks>

<verification>
After ALL tasks complete, run these verification steps:

1. **No stale references** — comprehensive grep of the entire `Source/` directory for all old type names:
   ```
   grep -rn "IClaudeRunner\|FClaudeStreamEvent\|EClaudeStreamEventType\|FOnClaudeResponse\|FOnClaudeProgress\|FOnClaudeStreamEvent\|FClaudeCodeSubsystem\|FClaudeSessionManager\|FClaudePromptOptions\|FMockClaudeRunner\|SClaudeEditorWidget\|SClaudeInputArea\|SClaudeToolbar" Source/
   ```
   Expected: 0 matches (excluding intentional comments documenting the rename)
   
   Separately verify KEPT names still exist:
   ```
   grep -rn "FClaudeCodeRunner\|FClaudeRequestConfig" Source/
   ```
   Expected: Multiple matches (these are intentionally kept per D-06, D-11)

2. **No stale includes** — grep for old filenames in include directives:
   ```
   grep -rn '#include.*IClaudeRunner\|#include.*ClaudeSubsystem\|#include.*ClaudeSessionManager\|#include.*ClaudeEditorWidget\|#include.*SClaudeInputArea\|#include.*SClaudeToolbar' Source/
   ```
   Expected: 0 matches

3. **New files exist** — verify all new abstraction files are present:
   - `Public/ChatBackendTypes.h` — EChatBackendType, delegates, FChatStreamEvent, FChatRequestConfig, FChatPromptOptions
   - `Public/IChatBackend.h` — 9-method pure virtual interface
   - `Public/ClaudeRequestConfig.h` — FClaudeRequestConfig derived config
   - `Public/ChatSubsystem.h` — FChatSubsystem with backend abstraction
   - `Public/ChatSessionManager.h` — FChatSessionManager
   - `Public/ChatEditorWidget.h` — SChatEditorWidget
   - `Private/ChatBackendFactory.h` + `.cpp` — factory with auto-detect
   - `Private/ChatSubsystem.cpp`, `Private/ChatSessionManager.cpp`, `Private/ChatEditorWidget.cpp`
   - `Private/Widgets/SChatInputArea.h/.cpp`, `Private/Widgets/SChatToolbar.h/.cpp`
   - `Private/Tests/ChatBackendTests.cpp`, `Private/Tests/ChatSubsystemTests.cpp`

4. **Old files removed** — verify deleted files are gone from git:
   - `Public/IClaudeRunner.h` — DELETED
   - `Public/ClaudeSubsystem.h` — DELETED (renamed)
   - `Public/ClaudeSessionManager.h` — DELETED (renamed)
   - `Public/ClaudeEditorWidget.h` — DELETED (renamed)
   - `Private/ClaudeSubsystem.cpp` — DELETED (renamed)
   - `Private/ClaudeSessionManager.cpp` — DELETED (renamed)
   - `Private/ClaudeEditorWidget.cpp` — DELETED (renamed)
   - `Private/Widgets/SClaudeInputArea.h/.cpp` — DELETED (renamed)
   - `Private/Widgets/SClaudeToolbar.h/.cpp` — DELETED (renamed)
   - `Private/Tests/ClaudeRunnerTests.cpp` — DELETED (renamed)
   - `Private/Tests/ClaudeSubsystemTests.cpp` — DELETED (renamed)

5. **Module unchanged** — verify these are NOT renamed:
   - Plugin name: `UnrealClaude` in .uplugin and Build.cs
   - Log category: `LogUnrealClaude`
   - Tab name: `ClaudeAssistant`
   - Module class: `FUnrealClaudeModule`
   - Runner class: `FClaudeCodeRunner` (per D-06)

6. **Compilation check** — Build the plugin:
   ```
   "C:/Program Files/Epic Games/UE_5.7/Engine/Build/BatchFiles/RunUAT.bat" BuildPlugin -Plugin="path/to/UnrealClaude.uplugin" -Package="path/to/PluginBuild" -TargetPlatforms=Win64 -Rocket
   ```

7. **Test run** — All 281+ tests pass (281 existing renamed + ~19 new):
   ```
   Automation RunTests UnrealClaude
   ```
</verification>

<success_criteria>
1. **ABST-01 satisfied:** IChatBackend interface exists with 9 pure virtual methods (ExecuteAsync, ExecuteSync, Cancel, IsExecuting, IsAvailable, GetDisplayName, GetBackendType, FormatPrompt, CreateConfig) — no Claude-specific types in method signatures. FChatRequestConfig is the base; FClaudeRequestConfig derives with Claude-specific fields. FClaudeCodeRunner implements IChatBackend.

2. **ABST-02 satisfied:** EChatBackendType enum class with Claude and OpenCode values. ChatBackendUtils::EnumToString/StringToEnum for persistence. Stored as string in config (D-29). Tests verify round-trip serialization.

3. **ABST-03 satisfied:** FChatSubsystem (renamed from FClaudeCodeSubsystem) routes chat requests through IChatBackend pointer. SetActiveBackend() swaps the backend at runtime via FChatBackendFactory. Backend swap blocked during active requests (D-14). Shared conversation history across swaps (D-19). Widgets use subsystem for availability checks (D-25/D-26).

4. **Zero regressions:** All 281 existing tests pass under new names + ~19 new tests pass. No stale type references anywhere in Source/.

5. **Conventions followed:** All files ≤500 lines, all functions ≤50 lines, copyright headers on all new files, Doxygen on all public types/methods, TEXT() macro on all string literals, b prefix on booleans.
</success_criteria>

<output>
After completion, create `.planning/phases/02-backend-abstraction/02-01-SUMMARY.md`
</output>
