# External Integrations

**Analysis Date:** 2026-03-30

## APIs & Services

**Anthropic Claude Code CLI:**
- The plugin shells out to the `claude` CLI binary (installed via `npm install -g @anthropic-ai/claude-code`)
- Invoked as a subprocess by `FClaudeCodeRunner` (`UnrealClaude/Source/UnrealClaude/Private/ClaudeCodeRunner.cpp`)
- Communication uses `--output-format stream-json` and `--input-format stream-json` (NDJSON over stdin/stdout)
- System prompt and user prompt are written to temp files under `<Project>/Saved/UnrealClaude/` to avoid command-line length limits
- MCP config is dynamically written to `<Project>/Saved/UnrealClaude/mcp-config.json` at runtime and passed to Claude via `--mcp-config`
- Allowed tools passed via `--allowedTools` flag; default set: `Read, Write, Edit, Grep, Glob, Bash` plus `mcp__unrealclaude__*`

**Unreal Engine HTTP Server (internal):**
- The C++ plugin runs an HTTP server on port 3000 (default) using UE's built-in `HTTPServer` module
- Exposes REST endpoints at `GET /mcp/tools`, `POST /mcp/tool/{name}`, `GET /mcp/status`
- Serves the MCP tool registry to the Node.js bridge; consumed exclusively by the bridge process
- CORS restricted to `http://localhost` for security

## SDKs

**`@anthropic-ai/claude-code`** — Claude Code CLI (npm global package)
- Version: 2.1.52+ supported; 2.1.71 verified on Windows; latest generally supported
- Purpose: Executes AI prompts with tool access (file ops, shell commands) in the project working directory
- Authentication: handled externally via `claude auth login` (browser OAuth flow to Anthropic/Claude Pro/Max account); no API keys stored in the plugin

**MCP Bridge (Node.js submodule):**
- Source: git submodule at `UnrealClaude/Resources/mcp-bridge/` → `https://github.com/Natfii/ue5-mcp-bridge`
- Entry point: `mcp-bridge/index.js`
- Purpose: Translates MCP protocol tool calls from Claude Code CLI into HTTP POST requests to the C++ HTTP server at `http://localhost:3000`
- Configuration: `UNREAL_MCP_URL` environment variable (default: `http://localhost:3000`)
- Started by Claude Code CLI as a child process using the MCP config JSON; launched via `node index.js`

## Data Sources

**UE Asset Registry:**
- Type: In-process UE runtime API
- Used by: `MCPTool_AssetSearch`, `MCPTool_AssetDependencies`, `MCPTool_AssetReferencers` (in `UnrealClaude/Source/UnrealClaude/Private/MCP/Tools/`)
- Purpose: Searching project assets, querying asset dependency graphs and referencers

**UE Editor World/Level State:**
- Type: In-process UE Editor runtime state
- Used by: `MCPTool_GetLevelActors`, `MCPTool_SpawnActor`, `MCPTool_MoveActor`, `MCPTool_SetProperty`, `MCPTool_DeleteActors`, `MCPTool_OpenLevel`
- Purpose: Querying and mutating the live editor level/world

**Blueprint Graph:**
- Type: In-process UE Kismet/BlueprintGraph APIs
- Used by: `MCPTool_BlueprintModify`, `MCPTool_BlueprintQuery`, `MCPTool_AnimBlueprintModify`
- Purpose: Creating/modifying Blueprint nodes, variables, functions, and Animation Blueprint state machines

**UE Output Log:**
- Type: In-process UE log buffer
- Used by: `MCPTool_GetOutputLog`
- Purpose: Exposing editor output log to Claude for context and debugging

**Session Persistence (local file):**
- Type: Local JSON file
- Location: `<Project>/Saved/UnrealClaude/session.json` (managed by `FClaudeSessionManager`)
- Purpose: Persisting conversation history across editor sessions; loaded on startup

**Dynamic UE 5.7 Context Files:**
- Type: Local Markdown files
- Location: `UnrealClaude/Resources/mcp-bridge/contexts/` (part of `ue5-mcp-bridge` submodule)
- Files: `actor.md`, `animation.md`, `assets.md`, `blueprint.md`, `character.md`, `enhanced_input.md`, `material.md`, `parallel_workflows.md`, `replication.md`, `slate.md`
- Purpose: On-demand UE 5.7 API documentation injected into Claude's context via `unreal_get_ue_context` MCP tool

**Project CLAUDE.md:**
- Type: Local Markdown file
- Location: Project root `CLAUDE.md` (user-created)
- Purpose: Custom system prompt extension; loaded by `FProjectContextManager` and appended to Claude's system prompt

## Authentication Providers

**Anthropic (via Claude Code CLI):**
- Auth method: OAuth browser flow via `claude auth login`
- Credentials stored: by Claude Code CLI in user profile (`~/.claude/` or equivalent); not managed by the plugin
- Required subscription: Claude Pro or Max, or Anthropic API access
- No API keys, tokens, or credentials are stored in the plugin repository

## Notable Dependencies

**`FPlatformProcess` (UE HAL):**
- Central to the plugin's operation; used to spawn the `claude` CLI as a subprocess, create stdin/stdout pipes, and read NDJSON output
- File: `UnrealClaude/Source/UnrealClaude/Private/ClaudeCodeRunner.cpp`

**`FHttpServerModule` / `IHttpRouter` (UE):**
- Used to host the in-editor MCP REST server; the entire MCP tool surface is exposed through this
- File: `UnrealClaude/Source/UnrealClaude/Private/MCP/UnrealClaudeMCPServer.cpp`

**`FBase64` (UE):**
- Used to base64-encode viewport screenshots (PNG) for inclusion in Claude's multimodal prompt payloads
- Images are validated against a whitelist directory (`<Project>/Saved/UnrealClaude/screenshots/`) before encoding

**`FMCPTaskQueue`:**
- Internal async queue (max 4 concurrent tasks) that wraps tool execution to prevent blocking the game thread
- File: `UnrealClaude/Source/UnrealClaude/Private/MCP/MCPTaskQueue.cpp`

**Node.js (external runtime):**
- Required to run the MCP bridge; not bundled — must be installed separately (`brew install node`, system package manager, or nvm)
- Bridge communicates with the C++ HTTP server over localhost HTTP

---

*Integration audit: 2026-03-30*
