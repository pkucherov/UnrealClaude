# Technology Stack

**Analysis Date:** 2026-03-30

## Languages

**Primary:**
- C++ (C++20) - All plugin source code; Unreal Engine module, MCP server, editor widget, session management, tool implementations
- JavaScript (ES modules) - MCP bridge (`UnrealClaude/Resources/mcp-bridge/`); runs under Node.js

**Secondary:**
- C# - Build configuration (`UnrealClaude/Source/UnrealClaude/UnrealClaude.Build.cs`); Unreal Build Tool rules

## Runtime

**Environment:**
- Unreal Engine 5.7 — plugin runs exclusively as a UE Editor module (`"LoadingPhase": "PostEngineInit"`)
- Node.js — required to run the MCP bridge (`mcp-bridge/index.js`)

**Package Manager:**
- npm — used for MCP bridge and Claude Code CLI installation
- Lockfile: present in submodule (`UnrealClaude/Resources/mcp-bridge/`)

## Frameworks

**Core:**
- Unreal Engine 5.7 — host framework; plugin is a single `Editor`-type module with `EnabledByDefault: true`
- Model Context Protocol (MCP) — communication protocol between the Node.js bridge and Claude Code CLI

**Testing:**
- Vitest — MCP bridge JavaScript unit/integration tests (in `ue5-mcp-bridge` submodule)
- UE Automation Test Framework — C++ tests via `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros; run with `Automation RunTests UnrealClaude` from editor console

**Build/Dev:**
- Unreal Build Tool (UBT) — C++ compilation and dependency graph
- RunUAT (`BuildPlugin`) — packages the plugin for distribution; invoked via `RunUAT.bat` (Windows) or `RunUAT.sh` (Linux/Mac)
- Live Coding — hot-reload compilation on Windows editor builds (linked via `LiveCoding` UE module, Win64 only)

## Key Dependencies

**UE Public Module Dependencies** (declared in `UnrealClaude.Build.cs`):
- `Core`, `CoreUObject`, `Engine` — foundational UE runtime
- `InputCore` — input handling
- `Slate`, `SlateCore` — editor UI (chat panel, widgets)
- `EditorStyle`, `UnrealEd`, `EditorFramework` — editor integration
- `ToolMenus` — Tools menu registration (`Tools > Claude Assistant`)
- `Projects`, `WorkspaceMenuStructure` — project/plugin metadata access

**UE Private Module Dependencies** (declared in `UnrealClaude.Build.cs`):
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

**External CLI Dependency:**
- `@anthropic-ai/claude-code` — Claude Code CLI; installed globally via `npm install -g @anthropic-ai/claude-code`; shell-executed by the plugin as a subprocess

## Configuration

**Environment:**
- No `.env` file in the C++ plugin; environment variables are read at runtime via `FPlatformMisc::GetEnvironmentVariable()`
- `UNREAL_MCP_URL` — env var consumed by the MCP bridge to locate the in-editor HTTP server (default: `http://localhost:3000`)
- Claude authentication is managed externally via `claude auth login`; no API keys stored in project

**Build:**
- `UnrealClaude/Source/UnrealClaude/UnrealClaude.Build.cs` — UBT module rules
- `UnrealClaude/UnrealClaude.uplugin` — plugin descriptor (version 1.4.1, engine 5.7.0)
- `UnrealClaude/Config/FilterPlugin.ini` — plugin filter configuration
- MCP config written at runtime to `<Project>/Saved/UnrealClaude/mcp-config.json`; temp system prompt and prompt files also written to `<Project>/Saved/UnrealClaude/`

## Platform Requirements

**Development:**
- Unreal Engine 5.7 installed (Windows, Linux, or macOS Apple Silicon)
- Node.js (for MCP bridge; `brew install node` on macOS, `dnf install nodejs` on Linux)
- Claude Code CLI (`npm install -g @anthropic-ai/claude-code`) and authenticated session
- Windows: winsock/platform APIs included via UE; clipboard via `ApplicationCore`
- Linux: additional system libs required (`nss`, `mesa-libgbm`, `libXcomposite`, etc.) plus clipboard tool (`wl-clipboard` or `xclip`)

**Production:**
- Deployment target: Unreal Engine Editor (Editor-only plugin; not for packaged games)
- Supported platforms: `Win64`, `Linux`, `Mac` (declared in `uplugin` and `Build.cs`)
- No prebuilt binaries distributed; must be built from source with `RunUAT BuildPlugin`

---

*Stack analysis: 2026-03-30*
