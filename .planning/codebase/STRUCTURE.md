# Project Structure

**Analysis Date:** 2026-03-30

## Root Layout

```
UnrealClaude/                        # Repository root
├── UnrealClaude/                    # Plugin root (UE plugin format)
│   ├── Config/                      # UE plugin config files
│   ├── Resources/                   # Non-code resources
│   │   ├── mcp-bridge/              # Git submodule: Node.js MCP bridge (Natfii/ue5-mcp-bridge)
│   │   ├── mcp-config.json          # MCP bridge configuration
│   │   ├── CLAUDE.md.example        # Template for project-level CLAUDE.md
│   │   └── icon128.png              # Plugin icon
│   └── Source/
│       └── UnrealClaude/            # Single UE module
│           ├── Public/              # Headers exposed to other modules
│           └── Private/             # Implementation files (not exposed)
│               ├── MCP/             # MCP server + tool infrastructure
│               │   └── Tools/       # Individual MCP tool implementations
│               ├── Scripting/       # Script executor interfaces
│               ├── Tests/           # Automation test files
│               └── Widgets/         # Sub-widgets for the chat panel
├── .planning/                       # GSD planning documents
│   └── codebase/                    # Codebase analysis docs (this file)
├── README.md
├── INSTALL_LINUX.md
├── INSTALL_MAC.md
└── .gitmodules                      # Declares mcp-bridge submodule
```

---

## Key Directories

### `UnrealClaude/Source/UnrealClaude/Public/`
- **Purpose:** Public API headers for the plugin module. These are the headers other modules (or tests) can include.
- **Contents:** Module class, all singleton subsystem headers, public interfaces, shared types
- **Key files:**
  - `UnrealClaudeModule.h` — `FUnrealClaudeModule : IModuleInterface`, declares `LogUnrealClaude`
  - `ClaudeSubsystem.h` — `FClaudeCodeSubsystem` singleton (chat orchestrator)
  - `IClaudeRunner.h` — `IClaudeRunner` interface + `FClaudeRequestConfig` + `FClaudeStreamEvent` types + all Claude-related delegates
  - `ClaudeCodeRunner.h` — `FClaudeCodeRunner : IClaudeRunner, FRunnable`
  - `ClaudeEditorWidget.h` — `SClaudeEditorWidget` (main chat panel Slate widget)
  - `ClaudeSessionManager.h` — Session history persistence
  - `ScriptExecutionManager.h` — Script pipeline singleton
  - `ScriptTypes.h` — `EScriptType`, `FScriptExecutionResult`, `FScriptHistoryEntry`
  - `CharacterDataTypes.h` — Shared character data structs
  - `UnrealClaudeConstants.h` — All magic numbers in namespaced `constexpr`
  - `UnrealClaudeUtils.h` — JSON vector/rotator helpers (`UnrealClaudeJsonUtils` namespace)
  - `UnrealClaudeCommands.h` — Keyboard shortcut command definitions

### `UnrealClaude/Source/UnrealClaude/Private/`
- **Purpose:** Implementation files and private headers not exported from the module.
- **Contents:** Module startup/shutdown, context gathering, animation helpers, blueprint utilities, JSON utils, and all private implementation `.cpp` files
- **Key files:**
  - `UnrealClaudeModule.cpp` — `StartupModule()` / `ShutdownModule()`, menu & toolbar registration
  - `ClaudeSubsystem.cpp` — Prompt building with history/context injection
  - `ClaudeCodeRunner.cpp` — Subprocess spawning, NDJSON stream parsing
  - `ClaudeEditorWidget.cpp` — Chat rendering, streaming event handling, code-block parsing
  - `ProjectContext.h/.cpp` — `FProjectContextManager` singleton (source scan, UClass parser, asset count)
  - `JsonUtils.h/.cpp` — Thin UE JSON wrappers
  - `ClipboardImageUtils.h/.cpp` — Clipboard PNG capture
  - `AnimAssetManager.h/.cpp`, `AnimGraphEditor.h/.cpp`, `AnimGraphFinder.h/.cpp` etc. — Animation Blueprint editing helpers used by `MCPTool_AnimBlueprintModify`
  - `BlueprintEditor.h/.cpp`, `BlueprintGraphEditor.h/.cpp`, `BlueprintUtils.h/.cpp` etc. — Blueprint editing helpers used by `MCPTool_BlueprintModify`

### `UnrealClaude/Source/UnrealClaude/Private/MCP/`
- **Purpose:** MCP server infrastructure: HTTP server, tool registry, task queue, and shared tool base.
- **Contents:** Core MCP framework classes
- **Key files:**
  - `UnrealClaudeMCPServer.h/.cpp` — HTTP server, route handlers (`/mcp/tools`, `/mcp/tool/{name}`, `/mcp/status`)
  - `MCPToolRegistry.h/.cpp` — `IMCPTool` interface, `FMCPToolRegistry`, `FMCPToolResult`, `FMCPToolInfo`, `FMCPToolAnnotations`
  - `MCPToolBase.h/.cpp` — `FMCPToolBase : IMCPTool` with shared parameter extraction and validation helpers
  - `MCPTaskQueue.h/.cpp` — Background worker thread for async tool execution
  - `MCPAsyncTask.h` — `FMCPAsyncTask` struct with status/result/progress tracking
  - `MCPParamValidator.h/.cpp` — Security-focused input validation (path traversal, injection chars, bounds)
  - `MCPErrors.h` — Shared error message constants
  - `MCPBlueprintLoadContext.h` — RAII context for Blueprint load operations

### `UnrealClaude/Source/UnrealClaude/Private/MCP/Tools/`
- **Purpose:** One `.h`/`.cpp` pair per MCP tool (or header-only for task queue tools).
- **Contents:** 30+ tool implementations inheriting `FMCPToolBase`
- **Naming pattern:** `MCPTool_{ToolName}.h/.cpp` (e.g., `MCPTool_SpawnActor.h`, `MCPTool_BlueprintModify.h`)
- **Header-only tools** (task queue tools): `MCPTool_TaskSubmit.h`, `MCPTool_TaskStatus.h`, `MCPTool_TaskResult.h`, `MCPTool_TaskList.h`, `MCPTool_TaskCancel.h`

### `UnrealClaude/Source/UnrealClaude/Private/Scripting/`
- **Purpose:** Script executor abstraction.
- **Key files:**
  - `IScriptExecutor.h` — Pure interface `IScriptExecutor` for the four script types (C++, Python, Console, EditorUtility)

### `UnrealClaude/Source/UnrealClaude/Private/Widgets/`
- **Purpose:** Sub-widget components for the chat panel, split out from `ClaudeEditorWidget`.
- **Key files:**
  - `SClaudeInputArea.h/.cpp` — Multi-line text input, clipboard image paste, thumbnail strip
  - `SClaudeToolbar.h/.cpp` — Toolbar/header widget for the Claude panel

### `UnrealClaude/Source/UnrealClaude/Private/Tests/`
- **Purpose:** UE Automation Framework test files.
- **Key files:**
  - `MCPToolTests.cpp` — Core MCP tool registration/execution tests
  - `MCPAssetToolTests.cpp` — Asset tool tests
  - `MCPParamValidatorTests.cpp` — Parameter validation tests
  - `MCPTaskQueueTests.cpp` — Task queue tests
  - `MCPAnimBlueprintTests.cpp` — Animation Blueprint tool tests
  - `CharacterToolTests.cpp` — Character tool tests
  - `MaterialAssetToolTests.cpp` — Material tool tests
  - `ClipboardImageTests.cpp` — Clipboard image capture tests

### `UnrealClaude/Resources/mcp-bridge/`
- **Purpose:** External Node.js process that bridges Claude Code's MCP protocol (JSON-RPC 2.0) to the plugin's HTTP REST API.
- **Generated:** No (committed submodule reference)
- **Note:** Empty in this repo clone (submodule not initialized). Run `git submodule update --init` to populate. Separate repo: `https://github.com/Natfii/ue5-mcp-bridge`

### `UnrealClaude/Config/`
- **Purpose:** UE plugin configuration.
- **Key files:**
  - `FilterPlugin.ini` — Build filter configuration

---

## Module Organization

The entire plugin is a **single UE module** named `UnrealClaude`. There are no sub-modules.

- Module type: `Editor` (editor-only plugin, not shipped with game builds)
- Module entry: `FUnrealClaudeModule : IModuleInterface` in `Public/UnrealClaudeModule.h`
- `IMPLEMENT_MODULE(FUnrealClaudeModule, UnrealClaude)` in `Private/UnrealClaudeModule.cpp`

---

## Naming Conventions

**Files:**
- Public headers: `PascalCase.h` — e.g., `ClaudeSubsystem.h`, `IClaudeRunner.h`
- Private headers: `PascalCase.h` co-located with `.cpp` — e.g., `ProjectContext.h`, `JsonUtils.h`
- Private sub-widget headers: `SWidgetName.h` — e.g., `SClaudeInputArea.h`, `SClaudeToolbar.h`
- MCP tools: `MCPTool_FeatureName.h/.cpp` — e.g., `MCPTool_SpawnActor.h`, `MCPTool_BlueprintModify.h`
- MCP infrastructure: `MCP{Component}.h/.cpp` — e.g., `MCPToolBase.h`, `MCPTaskQueue.h`
- Test files: `{Subject}Tests.cpp` — e.g., `MCPToolTests.cpp`, `MCPParamValidatorTests.cpp`
- Animation helpers: `Anim{Purpose}.h/.cpp` — e.g., `AnimGraphEditor.h`, `AnimAssetManager.h`
- Blueprint helpers: `Blueprint{Purpose}.h/.cpp` — e.g., `BlueprintEditor.h`, `BlueprintUtils.h`

**Classes:**
- Module: `FUnrealClaudeModule`
- Singletons: `F{Name}` with `static Get()` — e.g., `FClaudeCodeSubsystem::Get()`, `FProjectContextManager::Get()`
- Slate widgets: `S{Name} : SCompoundWidget` — e.g., `SClaudeEditorWidget`, `SClaudeInputArea`
- MCP tools: `FMCPTool_{Name} : FMCPToolBase` — e.g., `FMCPTool_SpawnActor`
- Interfaces: `I{Name}` — e.g., `IClaudeRunner`, `IMCPTool`, `IScriptExecutor`
- Data structs: `F{Name}` — e.g., `FClaudeStreamEvent`, `FMCPToolResult`, `FProjectContext`

---

## Notable Files

- `Public/UnrealClaudeConstants.h` — All numeric constants; **always use these instead of magic numbers** in new code
- `Public/IClaudeRunner.h` — All Claude streaming delegates and event types live here; essential for understanding Claude CLI integration
- `Private/MCP/MCPToolRegistry.h` — Defines `IMCPTool`, `FMCPToolResult`, `FMCPToolAnnotations`; the contract every tool implements
- `Private/MCP/MCPToolBase.h` — 541-line base class with every helper a tool author needs; read before writing a new tool
- `Private/MCP/MCPParamValidator.h` — Security boundary; all external input must pass through here
- `Private/UnrealClaudeModule.cpp` — Shows complete plugin initialization order and what singletons are started when
- `UnrealClaude/CLAUDE.md` (inside the plugin, loaded by editor) — Runtime instructions for Claude Code when working in this repo

---

## Where to Add New Code

**New MCP tool:**
1. Create `Private/MCP/Tools/MCPTool_{YourTool}.h` and `.cpp`
2. Inherit from `FMCPToolBase`, implement `GetInfo()` and `Execute()`
3. Register in `FMCPToolRegistry::RegisterBuiltinTools()` in `Private/MCP/MCPToolRegistry.cpp`
4. Add tool name to `UnrealClaudeConstants::MCPServer::ExpectedTools` in `Public/UnrealClaudeConstants.h`
5. Add tests in `Private/Tests/MCPToolTests.cpp` or a new `Private/Tests/{YourTool}Tests.cpp`

**New Slate sub-widget:**
- Implementation: `Private/Widgets/SYourWidget.h/.cpp`
- Pattern: inherit `SCompoundWidget`, use `SLATE_BEGIN_ARGS` / `SLATE_END_ARGS`

**New shared utility:**
- Headers shared across modules: `Public/UnrealClaudeUtils.h` (extend existing namespace) or new `Public/{Name}Utils.h`
- Internal helpers: `Private/{Name}Utils.h/.cpp`

**New constants:**
- Add a new namespace block in `Public/UnrealClaudeConstants.h`

**New animation/blueprint helper:**
- Animation editing helpers: `Private/Anim{Name}.h/.cpp`
- Blueprint editing helpers: `Private/Blueprint{Name}.h/.cpp`

---

*Structure analysis: 2026-03-30*
