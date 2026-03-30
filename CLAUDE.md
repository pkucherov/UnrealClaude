<!-- GSD:project-start source:PROJECT.md -->
## Project

**UnrealClaude — OpenCode Integration**

An Unreal Engine 5.7 Editor plugin that integrates AI coding assistants directly into the editor. Currently supports Claude Code CLI as a chat backend with 30+ MCP tools for editor manipulation. This project adds OpenCode CLI as a second chat backend, so users can switch between Claude and OpenCode from the same chat panel.

**Core Value:** Users can get AI coding assistance inside the Unreal Editor from either Claude Code or OpenCode, choosing whichever backend suits their workflow, without leaving the editor or losing conversation context.

### Constraints

- **Tech stack**: Must stay within UE 5.7 C++ (Slate, HTTP module, FRunnable) — no new external runtime dependencies beyond OpenCode CLI itself
- **Backwards compatible**: Existing Claude Code CLI integration must continue working exactly as before
- **No UE module split**: Plugin remains a single `UnrealClaude` module
- **OpenCode install**: Users must have `opencode` CLI installed separately (similar to existing Claude CLI requirement)
- **Platform parity**: OpenCode backend must work on all three platforms (Win64, Linux, Mac) where the plugin currently works
<!-- GSD:project-end -->

<!-- GSD:stack-start source:codebase/STACK.md -->
## Technology Stack

## Languages
- C++ (C++20) - All plugin source code; Unreal Engine module, MCP server, editor widget, session management, tool implementations
- JavaScript (ES modules) - MCP bridge (`UnrealClaude/Resources/mcp-bridge/`); runs under Node.js
- C# - Build configuration (`UnrealClaude/Source/UnrealClaude/UnrealClaude.Build.cs`); Unreal Build Tool rules
## Runtime
- Unreal Engine 5.7 — plugin runs exclusively as a UE Editor module (`"LoadingPhase": "PostEngineInit"`)
- Node.js — required to run the MCP bridge (`mcp-bridge/index.js`)
- npm — used for MCP bridge and Claude Code CLI installation
- Lockfile: present in submodule (`UnrealClaude/Resources/mcp-bridge/`)
## Frameworks
- Unreal Engine 5.7 — host framework; plugin is a single `Editor`-type module with `EnabledByDefault: true`
- Model Context Protocol (MCP) — communication protocol between the Node.js bridge and Claude Code CLI
- Vitest — MCP bridge JavaScript unit/integration tests (in `ue5-mcp-bridge` submodule)
- UE Automation Test Framework — C++ tests via `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros; run with `Automation RunTests UnrealClaude` from editor console
- Unreal Build Tool (UBT) — C++ compilation and dependency graph
- RunUAT (`BuildPlugin`) — packages the plugin for distribution; invoked via `RunUAT.bat` (Windows) or `RunUAT.sh` (Linux/Mac)
- Live Coding — hot-reload compilation on Windows editor builds (linked via `LiveCoding` UE module, Win64 only)
## Key Dependencies
- `Core`, `CoreUObject`, `Engine` — foundational UE runtime
- `InputCore` — input handling
- `Slate`, `SlateCore` — editor UI (chat panel, widgets)
- `EditorStyle`, `UnrealEd`, `EditorFramework` — editor integration
- `ToolMenus` — Tools menu registration (`Tools > Claude Assistant`)
- `Projects`, `WorkspaceMenuStructure` — project/plugin metadata access
- `Json`, `JsonUtilities` — JSON serialization/deserialization for NDJSON stream parsing and MCP protocol
- `HTTP`, `HTTPServer` — built-in HTTP server on port 3000; serves MCP REST endpoints
- `Sockets`, `Networking` — low-level network support for HTTP server
- `ImageWrapper` — screenshot/viewport capture image encoding
- `Kismet`, `KismetCompiler`, `BlueprintGraph`, `GraphEditor` — Blueprint creation and modification tools
- `AssetRegistry`, `AssetTools` — asset search, dependencies, referencers
- `AnimGraph`, `AnimGraphRuntime` — Animation Blueprint state machine editing
- `EditorScriptingUtilities` — asset saving utilities
- `EnhancedInput` — Enhanced Input action and mapping context management
- `ApplicationCore` — clipboard support (`FPlatformApplicationMisc`) on all platforms
- `LiveCoding` — hot-reload on Windows editor builds only (conditional dependency)
- `@anthropic-ai/claude-code` — Claude Code CLI; installed globally via `npm install -g @anthropic-ai/claude-code`; shell-executed by the plugin as a subprocess
## Configuration
- No `.env` file in the C++ plugin; environment variables are read at runtime via `FPlatformMisc::GetEnvironmentVariable()`
- `UNREAL_MCP_URL` — env var consumed by the MCP bridge to locate the in-editor HTTP server (default: `http://localhost:3000`)
- Claude authentication is managed externally via `claude auth login`; no API keys stored in project
- `UnrealClaude/Source/UnrealClaude/UnrealClaude.Build.cs` — UBT module rules
- `UnrealClaude/UnrealClaude.uplugin` — plugin descriptor (version 1.4.1, engine 5.7.0)
- `UnrealClaude/Config/FilterPlugin.ini` — plugin filter configuration
- MCP config written at runtime to `<Project>/Saved/UnrealClaude/mcp-config.json`; temp system prompt and prompt files also written to `<Project>/Saved/UnrealClaude/`
## Platform Requirements
- Unreal Engine 5.7 installed (Windows, Linux, or macOS Apple Silicon)
- Node.js (for MCP bridge; `brew install node` on macOS, `dnf install nodejs` on Linux)
- Claude Code CLI (`npm install -g @anthropic-ai/claude-code`) and authenticated session
- Windows: winsock/platform APIs included via UE; clipboard via `ApplicationCore`
- Linux: additional system libs required (`nss`, `mesa-libgbm`, `libXcomposite`, etc.) plus clipboard tool (`wl-clipboard` or `xclip`)
- Deployment target: Unreal Engine Editor (Editor-only plugin; not for packaged games)
- Supported platforms: `Win64`, `Linux`, `Mac` (declared in `uplugin` and `Build.cs`)
- No prebuilt binaries distributed; must be built from source with `RunUAT BuildPlugin`
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

## Style Configuration
- No `.editorconfig`, `.clang-format`, `.eslintrc`, or `.prettierrc` detected — no automated formatter enforced at the repo level
- Style is manually maintained, guided by `UnrealClaude/CLAUDE.md` documentation
- C++ targets **Unreal Engine 5.7 coding standards** exclusively
## Naming Conventions
- C++ implementation files: `PascalCase.cpp` / `PascalCase.h` (e.g., `BlueprintEditor.cpp`, `JsonUtils.h`)
- MCP tool files: `MCPTool_ToolName.cpp` / `MCPTool_ToolName.h` (e.g., `MCPTool_SpawnActor.cpp`)
- Test files: `CategoryTests.cpp` (e.g., `MCPToolTests.cpp`, `MCPParamValidatorTests.cpp`, `CharacterToolTests.cpp`)
- Plugin classes: `FPascalCase` for plain structs/utility classes (e.g., `FMCPToolBase`, `FMCPParamValidator`, `FMCPErrors`)
- Interfaces: `IPascalCase` (e.g., `IMCPTool` in `MCPToolRegistry.h`)
- UE UObject-derived: standard UE prefix (`UClass`, `AActor`, etc.)
- Enums: `EPascalCase` (e.g., `EMCPErrorCode` in `MCPErrors.h`)
- Structs: `FPascalCase` (e.g., `FMCPToolInfo`, `FMCPToolResult`, `FMCPToolParameter`, `FMCPToolAnnotations`)
- `PascalCase` for all methods (UE convention): `GetInfo()`, `Execute()`, `ValidateActorName()`, `ExtractRequiredString()`
- Boolean-returning validators prefixed with `Validate` or `Extract`: `ValidateBlueprintPath()`, `ExtractActorName()`
- Member variables: `PascalCase` without `m_` prefix (e.g., `bSuccess`, `Tools`, `TaskQueue`)
- Boolean members: `b` prefix (e.g., `bReadOnlyHint`, `bDestructiveHint`, `bRequired`, `bSuccess`)
- Local variables: `PascalCase` for complex objects, lowercase for primitives in loops
- Out-parameters: `Out` prefix (e.g., `OutWorld`, `OutValue`, `OutError`)
- String literals: wrapped in `TEXT()` macro throughout (e.g., `TEXT("spawn_actor")`)
- Log categories: `DEFINE_LOG_CATEGORY(LogUnrealClaude)` in `UnrealClaudeModule.cpp`
- UI text: `LOCTEXT("Key", "Text")` macro for Slate widgets
- Snake_case strings: `"spawn_actor"`, `"blueprint_modify"`, `"anim_blueprint_modify"`
## Code Patterns
- Vectors: `{ "x": 0, "y": 0, "z": 0 }`
- Rotators: `{ "pitch": 0, "yaw": 0, "roll": 0 }`
- Helpers: `UnrealClaudeJsonUtils::ExtractVector()`, `ExtractRotator()` in `UnrealClaudeUtils.h`
- Maximum 500 lines per file
- Maximum 50 lines per function
## Documentation
- **File header**: Every `.cpp` and `.h` opens with `// Copyright Natali Caggiano. All Rights Reserved.`
- **Class documentation**: Doxygen-style `/** ... */` block comments above all classes and public methods
- **Parameter docs**: `@param Name - Description` format inside `/** */` blocks (see `MCPToolBase.h`)
- **Section comments**: `// ===== Section Name =====` separators group related code in large files (e.g., `MCPToolBase.h`, `MCPToolTests.cpp`)
- **Known issues**: Documented inline with detailed `// KNOWN ISSUE:` blocks including root cause, attempted workarounds, and references (see `MCPTaskQueueTests.cpp` lines 41–119)
- **Test comments**: Tests use descriptive assertion strings: `TestTrue("Description of what should be true", condition)`
## Error Handling
- All MCP tools return `FMCPToolResult` — never throw exceptions
- Centralized error factory class `FMCPErrors` in `MCPErrors.h` provides consistent message formatting:
- `TOptional<FMCPToolResult>` used as out-parameter for early-return validation chains
- Null-check pattern: `if (!Tool) return false;` in tests after `TestNotNull`
- Editor context validated before world operations via `ValidateEditorContext(OutWorld)` helper
## Commit Conventions
- `feat:` — New MCP tools or features
- `fix:` — Bug fixes
- `test:` — Test additions or fixes
- `docs:` — Documentation updates
- `refactor:` — Code refactoring without behavior changes
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

## Pattern Overview
- The plugin exposes a REST API (MCP server on port 3000) consumed by an external Node.js bridge, which Claude Code CLI uses as MCP tools
- Claude CLI is also invoked directly as a subprocess for the chat panel (two separate Claude integration paths)
- All MCP tool implementations inherit from a shared `FMCPToolBase`, are registered at startup in `FMCPToolRegistry`, and dispatched by name via the HTTP server
- Async/long-running tool executions are routed through `FMCPTaskQueue` (a background `FRunnable` thread), while synchronous tools execute on the UE game thread
- The Slate UI (`SClaudeEditorWidget`) is completely decoupled from the MCP server; it talks only to `FClaudeCodeSubsystem`
## Components
### FUnrealClaudeModule (Module Entry Point)
- **Responsibility:** UE module lifecycle: startup/shutdown, menu/toolbar registration, MCP server lifecycle, keyboard shortcut binding
- **Location:** `UnrealClaude/Source/UnrealClaude/`
- **Key files:**
### FUnrealClaudeMCPServer (REST API Server)
- **Responsibility:** HTTP server on port 3000 (configurable via `UnrealClaudeConstants::MCPServer::DefaultPort`). Exposes three routes: `GET /mcp/tools`, `POST /mcp/tool/{name}`, `GET /mcp/status`. Owns `FMCPToolRegistry`.
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/MCP/`
- **Key files:**
### FMCPToolRegistry (Tool Registration & Dispatch)
- **Responsibility:** Owns all `IMCPTool` instances, provides O(1) lookup by name, caches tool metadata list, owns `FMCPTaskQueue`.
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/MCP/`
- **Key files:**
### FMCPToolBase / IMCPTool (Tool Abstraction)
- **Responsibility:** `IMCPTool` is the pure-virtual interface every tool implements (`GetInfo()` + `Execute()`). `FMCPToolBase` is a concrete helper base that adds parameter extraction helpers, transform utilities, actor finders, validation wrappers, and JSON serialization helpers.
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/MCP/`
- **Key files:**
### MCP Tool Implementations (30+ tools)
- **Responsibility:** Domain-specific editor operations exposed over REST. Grouped by concern:
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/MCP/Tools/`
### FMCPTaskQueue (Async Execution Engine)
- **Responsibility:** Background `FRunnable` thread that manages up to 4 concurrent tool executions. Tools can be submitted by name+params, polled by `FGuid` task ID, and cancelled. Handles timeouts and result retention (5-minute window).
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/MCP/`
- **Key files:**
### FClaudeCodeSubsystem (Chat Orchestrator)
- **Responsibility:** Singleton that orchestrates chat: builds prompts with conversation history and context injections, delegates execution to `IClaudeRunner`, and manages `FClaudeSessionManager`.
- **Location:** `UnrealClaude/Source/UnrealClaude/`
- **Key files:**
### IClaudeRunner / FClaudeCodeRunner (Claude CLI Subprocess)
- **Responsibility:** `IClaudeRunner` is the abstract interface for Claude execution (supports mock/test injection). `FClaudeCodeRunner` implements it: spawns `claude -p --stream-json` as a subprocess via `FRunnable`, captures NDJSON output, parses structured stream events (`FClaudeStreamEvent`), and fires callbacks on the game thread.
- **Location:** `UnrealClaude/Source/UnrealClaude/Public/` and `Private/`
- **Key files:**
### FClaudeSessionManager (Session Persistence)
- **Responsibility:** Maintains in-memory conversation history as `TArray<TPair<FString, FString>>`, serializes to `Saved/UnrealClaude/session.json`, enforces max-history size.
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/`
- **Key files:**
### FProjectContextManager (Project Introspection)
- **Responsibility:** Gathers UE project metadata (source files, UCLASS scan, level actors, asset counts) and formats it as a system prompt injection. Thread-safe via `FCriticalSection`.
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/`
- **Key files:**
### FScriptExecutionManager (Script Pipeline)
- **Responsibility:** Permission-gated script execution pipeline for four script types: C++ (via Live Coding), Python, Console commands, and Editor Utility Blueprints. Maintains a persistent history log.
- **Location:** `UnrealClaude/Source/UnrealClaude/`
- **Key files:**
### Slate UI Layer
- **Responsibility:** Chat UI rendered directly in the editor as a docked tab.
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/Widgets/`
- **Key files:**
### Utilities
- **Location:** `UnrealClaude/Source/UnrealClaude/Private/`
- **Key files:**
### Node.js MCP Bridge (External Submodule)
- **Responsibility:** Translates between Claude Code's MCP protocol (JSON-RPC 2.0) and the plugin's HTTP REST API. Runs as a separate process.
- **Location:** `UnrealClaude/Resources/mcp-bridge/` (git submodule → `Natfii/ue5-mcp-bridge`)
## Data Flow
### Chat Flow (User → Claude CLI → Chat Panel)
### MCP Tool Flow (Claude Code → Node.js Bridge → UE Plugin)
### Async Tool Flow
## Entry Points
- `FUnrealClaudeModule::StartupModule()` — `Private/UnrealClaudeModule.cpp:30` — module initialization: registers menus, starts MCP server, initializes `FProjectContextManager` and `FScriptExecutionManager`
- `FUnrealClaudeMCPServer::Start()` — starts HTTP listener and registers routes
- `FClaudeCodeSubsystem::Get()` — singleton accessor; constructed on first access
- Tab spawner lambda in `StartupModule()` — creates `SClaudeEditorWidget` when user opens the tab
## Key Abstractions
- **`IMCPTool`** (`Private/MCP/MCPToolRegistry.h`) — pure interface all MCP tools implement; `GetInfo()` returns metadata, `Execute()` returns `FMCPToolResult`
- **`FMCPToolBase`** (`Private/MCP/MCPToolBase.h`) — concrete base with shared helpers; all 30+ tools extend this
- **`IClaudeRunner`** (`Public/IClaudeRunner.h`) — abstraction over Claude CLI execution; allows mock substitution in tests
- **`FMCPToolAnnotations`** (`Private/MCP/MCPToolRegistry.h`) — read-only/modifying/destructive hints communicated to Claude as tool metadata
- **`FClaudeStreamEvent`** / `EClaudeStreamEventType` (`Public/IClaudeRunner.h`) — typed NDJSON event model bridging raw subprocess output to UI
- **`FClaudePromptOptions`** (`Public/ClaudeSubsystem.h`) — options struct for prompt sending; consolidates context flags, progress callbacks, and image paths
## Error Handling
- All tool parameters validated via `FMCPParamValidator` (path traversal blocks, character blocklists, bounds checks) before execution
- `TOptional<FMCPToolResult>` used as early-return error accumulator in `FMCPToolBase` helpers
- `ValidateEditorContext()` guard on every modifying tool before touching `UWorld`
- Claude subprocess errors bubble up via `FOnClaudeResponse` callback with `bSuccess=false`
- MCP server returns structured JSON error responses for unknown tools and bad parameters
## Cross-Cutting Concerns
<!-- GSD:architecture-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd:quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd:debug` for investigation and bug fixing
- `/gsd:execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->



<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd:profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
