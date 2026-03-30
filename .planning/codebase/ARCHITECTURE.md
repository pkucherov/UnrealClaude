# Architecture

**Analysis Date:** 2026-03-30

## Pattern Overview

**Overall:** Layered Plugin Architecture — an Unreal Engine 5.7 Editor plugin that combines a local HTTP/REST server (MCP layer) with a CLI subprocess runner (Claude layer) and a Slate UI layer, all wired together at module startup.

**Key Characteristics:**
- The plugin exposes a REST API (MCP server on port 3000) consumed by an external Node.js bridge, which Claude Code CLI uses as MCP tools
- Claude CLI is also invoked directly as a subprocess for the chat panel (two separate Claude integration paths)
- All MCP tool implementations inherit from a shared `FMCPToolBase`, are registered at startup in `FMCPToolRegistry`, and dispatched by name via the HTTP server
- Async/long-running tool executions are routed through `FMCPTaskQueue` (a background `FRunnable` thread), while synchronous tools execute on the UE game thread
- The Slate UI (`SClaudeEditorWidget`) is completely decoupled from the MCP server; it talks only to `FClaudeCodeSubsystem`

---

## Components

### FUnrealClaudeModule (Module Entry Point)
- **Responsibility:** UE module lifecycle: startup/shutdown, menu/toolbar registration, MCP server lifecycle, keyboard shortcut binding
- **Location:** `UnrealClaude/Source/UnrealClaude/`
- **Key files:**
  - `Public/UnrealClaudeModule.h`
  - `Private/UnrealClaudeModule.cpp`

### FUnrealClaudeMCPServer (REST API Server)
- **Responsibility:** HTTP server on port 3000 (configurable via `UnrealClaudeConstants::MCPServer::DefaultPort`). Exposes three routes: `GET /mcp/tools`, `POST /mcp/tool/{name}`, `GET /mcp/status`. Owns `FMCPToolRegistry`.
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/MCP/`
- **Key files:**
  - `Private/MCP/UnrealClaudeMCPServer.h`
  - `Private/MCP/UnrealClaudeMCPServer.cpp`

### FMCPToolRegistry (Tool Registration & Dispatch)
- **Responsibility:** Owns all `IMCPTool` instances, provides O(1) lookup by name, caches tool metadata list, owns `FMCPTaskQueue`.
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/MCP/`
- **Key files:**
  - `Private/MCP/MCPToolRegistry.h`
  - `Private/MCP/MCPToolRegistry.cpp`

### FMCPToolBase / IMCPTool (Tool Abstraction)
- **Responsibility:** `IMCPTool` is the pure-virtual interface every tool implements (`GetInfo()` + `Execute()`). `FMCPToolBase` is a concrete helper base that adds parameter extraction helpers, transform utilities, actor finders, validation wrappers, and JSON serialization helpers.
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/MCP/`
- **Key files:**
  - `Private/MCP/MCPToolBase.h`
  - `Private/MCP/MCPToolBase.cpp`
  - `Private/MCP/MCPToolRegistry.h` (contains `IMCPTool`, `FMCPToolResult`, `FMCPToolInfo`, `FMCPToolAnnotations`)

### MCP Tool Implementations (30+ tools)
- **Responsibility:** Domain-specific editor operations exposed over REST. Grouped by concern:
  - **Actor**: `MCPTool_SpawnActor`, `MCPTool_GetLevelActors`, `MCPTool_DeleteActors`, `MCPTool_MoveActor`, `MCPTool_SetProperty`
  - **Blueprint**: `MCPTool_BlueprintQuery`, `MCPTool_BlueprintModify`, `MCPTool_BlueprintQueryList`
  - **Animation Blueprint**: `MCPTool_AnimBlueprintModify`
  - **Asset**: `MCPTool_Asset`, `MCPTool_AssetSearch`, `MCPTool_AssetDependencies`, `MCPTool_AssetReferencers`
  - **Character**: `MCPTool_Character`, `MCPTool_CharacterData`
  - **Material**: `MCPTool_Material`
  - **Enhanced Input**: `MCPTool_EnhancedInput`
  - **Level**: `MCPTool_OpenLevel`
  - **Utility**: `MCPTool_RunConsoleCommand`, `MCPTool_GetOutputLog`, `MCPTool_CaptureViewport`, `MCPTool_ExecuteScript`, `MCPTool_CleanupScripts`, `MCPTool_GetScriptHistory`
  - **Task Queue**: `MCPTool_TaskSubmit`, `MCPTool_TaskStatus`, `MCPTool_TaskResult`, `MCPTool_TaskList`, `MCPTool_TaskCancel`
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/MCP/Tools/`

### FMCPTaskQueue (Async Execution Engine)
- **Responsibility:** Background `FRunnable` thread that manages up to 4 concurrent tool executions. Tools can be submitted by name+params, polled by `FGuid` task ID, and cancelled. Handles timeouts and result retention (5-minute window).
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/MCP/`
- **Key files:**
  - `Private/MCP/MCPTaskQueue.h`
  - `Private/MCP/MCPTaskQueue.cpp`
  - `Private/MCP/MCPAsyncTask.h`

### FClaudeCodeSubsystem (Chat Orchestrator)
- **Responsibility:** Singleton that orchestrates chat: builds prompts with conversation history and context injections, delegates execution to `IClaudeRunner`, and manages `FClaudeSessionManager`.
- **Location:** `UnrealClaude/Source/UnrealClaude/`
- **Key files:**
  - `Public/ClaudeSubsystem.h`
  - `Private/ClaudeSubsystem.cpp`

### IClaudeRunner / FClaudeCodeRunner (Claude CLI Subprocess)
- **Responsibility:** `IClaudeRunner` is the abstract interface for Claude execution (supports mock/test injection). `FClaudeCodeRunner` implements it: spawns `claude -p --stream-json` as a subprocess via `FRunnable`, captures NDJSON output, parses structured stream events (`FClaudeStreamEvent`), and fires callbacks on the game thread.
- **Location:** `UnrealClaude/Source/UnrealClaude/Public/` and `Private/`
- **Key files:**
  - `Public/IClaudeRunner.h` (interface, delegates, event types)
  - `Public/ClaudeCodeRunner.h`
  - `Private/ClaudeCodeRunner.cpp`

### FClaudeSessionManager (Session Persistence)
- **Responsibility:** Maintains in-memory conversation history as `TArray<TPair<FString, FString>>`, serializes to `Saved/UnrealClaude/session.json`, enforces max-history size.
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/`
- **Key files:**
  - `Private/ClaudeSessionManager.h` (private header)
  - `Private/ClaudeSessionManager.cpp`

### FProjectContextManager (Project Introspection)
- **Responsibility:** Gathers UE project metadata (source files, UCLASS scan, level actors, asset counts) and formats it as a system prompt injection. Thread-safe via `FCriticalSection`.
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/`
- **Key files:**
  - `Private/ProjectContext.h`
  - `Private/ProjectContext.cpp`

### FScriptExecutionManager (Script Pipeline)
- **Responsibility:** Permission-gated script execution pipeline for four script types: C++ (via Live Coding), Python, Console commands, and Editor Utility Blueprints. Maintains a persistent history log.
- **Location:** `UnrealClaude/Source/UnrealClaude/`
- **Key files:**
  - `Public/ScriptExecutionManager.h`
  - `Private/ScriptExecutionManager.cpp`
  - `Private/Scripting/IScriptExecutor.h`

### Slate UI Layer
- **Responsibility:** Chat UI rendered directly in the editor as a docked tab.
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/Widgets/`
- **Key files:**
  - `Public/ClaudeEditorWidget.h` + `Private/ClaudeEditorWidget.cpp` — main chat widget (`SClaudeEditorWidget`), streaming NDJSON event rendering, tool call collapsible groups, code block parsing
  - `Private/Widgets/SClaudeInputArea.h/.cpp` — multi-line input with clipboard image paste, thumbnail strip, send/cancel buttons
  - `Private/Widgets/SClaudeToolbar.h/.cpp` — toolbar/header for the chat panel

### Utilities
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/`
- **Key files:**
  - `Private/JsonUtils.h/.cpp` — thin wrappers around UE JSON serialization
  - `Private/ClipboardImageUtils.h/.cpp` — clipboard bitmap capture → PNG temp file
  - `Public/UnrealClaudeUtils.h` — shared JSON helpers (vector/rotator conversion)
  - `Public/UnrealClaudeConstants.h` — all magic numbers in named namespaces
  - `Private/MCP/MCPParamValidator.h/.cpp` — security-first parameter validation (path traversal, injection, bounds)

### Node.js MCP Bridge (External Submodule)
- **Responsibility:** Translates between Claude Code's MCP protocol (JSON-RPC 2.0) and the plugin's HTTP REST API. Runs as a separate process.
- **Location:** `UnrealClaude/Resources/mcp-bridge/` (git submodule → `Natfii/ue5-mcp-bridge`)

---

## Data Flow

### Chat Flow (User → Claude CLI → Chat Panel)
1. User types prompt in `SClaudeEditorWidget` → clicks Send
2. `SClaudeEditorWidget::SendMessage()` calls `FClaudeCodeSubsystem::Get().SendPrompt()`
3. `FClaudeCodeSubsystem` prepends conversation history + injects UE5.7 system prompt + project context (`FProjectContextManager`)
4. `FClaudeCodeSubsystem` delegates to `FClaudeCodeRunner::ExecuteAsync()`
5. `FClaudeCodeRunner` writes prompts to temp files, spawns `claude -p --stream-json` subprocess
6. Worker thread reads NDJSON from stdout, calls `ParseAndEmitNdjsonLine()` → fires `FOnClaudeStreamEvent` delegate per event
7. `SClaudeEditorWidget::OnClaudeStreamEvent()` handles events on game thread: `TextContent` appends text, `ToolUse`/`ToolResult` update collapsible tool indicators, `Result` appends stats footer
8. On completion, `FClaudeCodeSubsystem` adds exchange to `FClaudeSessionManager` history

### MCP Tool Flow (Claude Code → Node.js Bridge → UE Plugin)
1. Claude Code invokes an MCP tool (e.g., `unreal_spawn_actor`) via MCP protocol
2. Node.js bridge (`mcp-bridge`) translates to `POST http://localhost:3000/mcp/tool/spawn_actor` with JSON body
3. `FUnrealClaudeMCPServer::HandleExecuteTool()` parses request, calls `FMCPToolRegistry::ExecuteTool(name, params)`
4. Registry looks up `IMCPTool` by name, calls `Execute(params)` on game thread (or `FMCPTaskQueue::SubmitTask()` for async)
5. Tool returns `FMCPToolResult` (success flag + message + optional JSON data)
6. Server serializes result to JSON response → bridge returns to Claude Code

### Async Tool Flow
1. Claude Code calls `task_submit` tool with `tool_name` + `parameters`
2. `MCPTool_TaskSubmit` calls `FMCPTaskQueue::SubmitTask()` → returns `FGuid` task ID immediately
3. Worker thread in `FMCPTaskQueue` dequeues task, calls `FMCPToolRegistry::ExecuteTool()` in background
4. Claude Code polls `task_status` with task ID; retrieves result via `task_result` when complete

---

## Entry Points

- `FUnrealClaudeModule::StartupModule()` — `Private/UnrealClaudeModule.cpp:30` — module initialization: registers menus, starts MCP server, initializes `FProjectContextManager` and `FScriptExecutionManager`
- `FUnrealClaudeMCPServer::Start()` — starts HTTP listener and registers routes
- `FClaudeCodeSubsystem::Get()` — singleton accessor; constructed on first access
- Tab spawner lambda in `StartupModule()` — creates `SClaudeEditorWidget` when user opens the tab

---

## Key Abstractions

- **`IMCPTool`** (`Private/MCP/MCPToolRegistry.h`) — pure interface all MCP tools implement; `GetInfo()` returns metadata, `Execute()` returns `FMCPToolResult`
- **`FMCPToolBase`** (`Private/MCP/MCPToolBase.h`) — concrete base with shared helpers; all 30+ tools extend this
- **`IClaudeRunner`** (`Public/IClaudeRunner.h`) — abstraction over Claude CLI execution; allows mock substitution in tests
- **`FMCPToolAnnotations`** (`Private/MCP/MCPToolRegistry.h`) — read-only/modifying/destructive hints communicated to Claude as tool metadata
- **`FClaudeStreamEvent`** / `EClaudeStreamEventType` (`Public/IClaudeRunner.h`) — typed NDJSON event model bridging raw subprocess output to UI
- **`FClaudePromptOptions`** (`Public/ClaudeSubsystem.h`) — options struct for prompt sending; consolidates context flags, progress callbacks, and image paths

---

## Error Handling

**Strategy:** Two-phase — synchronous validation before any editor mutation; structured `FMCPToolResult` for all tool outcomes.

**Patterns:**
- All tool parameters validated via `FMCPParamValidator` (path traversal blocks, character blocklists, bounds checks) before execution
- `TOptional<FMCPToolResult>` used as early-return error accumulator in `FMCPToolBase` helpers
- `ValidateEditorContext()` guard on every modifying tool before touching `UWorld`
- Claude subprocess errors bubble up via `FOnClaudeResponse` callback with `bSuccess=false`
- MCP server returns structured JSON error responses for unknown tools and bad parameters

---

## Cross-Cutting Concerns

**Logging:** `LogUnrealClaude` log category (defined in `UnrealClaudeModule.h`, via `DECLARE_LOG_CATEGORY_EXTERN`)

**Validation:** Centralized in `FMCPParamValidator` (`Private/MCP/MCPParamValidator.h`); called from `FMCPToolBase` validation helpers — never skip in new tools

**Constants:** All numeric constants live in `Public/UnrealClaudeConstants.h` namespaced by concern (`MCPServer`, `Session`, `Context`, `ScriptExecution`, `ClipboardImage`, etc.)

**Thread Safety:** `FMCPTaskQueue` uses `FCriticalSection TasksLock` for task map access; `FProjectContextManager` uses `FCriticalSection ContextLock`; Claude runner is a `FRunnable` on its own thread that marshals results back via game-thread delegates

---

*Architecture analysis: 2026-03-30*
