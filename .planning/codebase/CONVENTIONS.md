# Coding Conventions

**Analysis Date:** 2026-03-30

## Style Configuration

- No `.editorconfig`, `.clang-format`, `.eslintrc`, or `.prettierrc` detected — no automated formatter enforced at the repo level
- Style is manually maintained, guided by `UnrealClaude/CLAUDE.md` documentation
- C++ targets **Unreal Engine 5.7 coding standards** exclusively

## Naming Conventions

**Files:**
- C++ implementation files: `PascalCase.cpp` / `PascalCase.h` (e.g., `BlueprintEditor.cpp`, `JsonUtils.h`)
- MCP tool files: `MCPTool_ToolName.cpp` / `MCPTool_ToolName.h` (e.g., `MCPTool_SpawnActor.cpp`)
- Test files: `CategoryTests.cpp` (e.g., `MCPToolTests.cpp`, `MCPParamValidatorTests.cpp`, `CharacterToolTests.cpp`)

**Classes:**
- Plugin classes: `FPascalCase` for plain structs/utility classes (e.g., `FMCPToolBase`, `FMCPParamValidator`, `FMCPErrors`)
- Interfaces: `IPascalCase` (e.g., `IMCPTool` in `MCPToolRegistry.h`)
- UE UObject-derived: standard UE prefix (`UClass`, `AActor`, etc.)
- Enums: `EPascalCase` (e.g., `EMCPErrorCode` in `MCPErrors.h`)
- Structs: `FPascalCase` (e.g., `FMCPToolInfo`, `FMCPToolResult`, `FMCPToolParameter`, `FMCPToolAnnotations`)

**Functions:**
- `PascalCase` for all methods (UE convention): `GetInfo()`, `Execute()`, `ValidateActorName()`, `ExtractRequiredString()`
- Boolean-returning validators prefixed with `Validate` or `Extract`: `ValidateBlueprintPath()`, `ExtractActorName()`

**Variables:**
- Member variables: `PascalCase` without `m_` prefix (e.g., `bSuccess`, `Tools`, `TaskQueue`)
- Boolean members: `b` prefix (e.g., `bReadOnlyHint`, `bDestructiveHint`, `bRequired`, `bSuccess`)
- Local variables: `PascalCase` for complex objects, lowercase for primitives in loops
- Out-parameters: `Out` prefix (e.g., `OutWorld`, `OutValue`, `OutError`)

**Constants / Macros:**
- String literals: wrapped in `TEXT()` macro throughout (e.g., `TEXT("spawn_actor")`)
- Log categories: `DEFINE_LOG_CATEGORY(LogUnrealClaude)` in `UnrealClaudeModule.cpp`
- UI text: `LOCTEXT("Key", "Text")` macro for Slate widgets

**MCP tool names (runtime identifiers):**
- Snake_case strings: `"spawn_actor"`, `"blueprint_modify"`, `"anim_blueprint_modify"`

## Code Patterns

**Parameter validation pattern** — used in every MCP tool `Execute()`:
```cpp
FMCPToolResult FMCPTool_YourTool::Execute(const TSharedRef<FJsonObject>& Params)
{
    FString RequiredParam;
    TOptional<FMCPToolResult> Error;
    if (!ExtractRequiredString(Params, TEXT("param_name"), RequiredParam, Error))
    {
        return Error.GetValue();
    }
    if (!ValidateBlueprintPathParam(BlueprintPath, Error))
    {
        return Error.GetValue();
    }
    // ... do work ...
    return FMCPToolResult::Success(TEXT("Operation completed"));
}
```
Defined in `MCPToolBase.h` helper methods; enforced by documentation in `CLAUDE.md`.

**Result type pattern** — `FMCPToolResult` (defined in `MCPToolRegistry.h`) used throughout:
```cpp
FMCPToolResult::Success(TEXT("message"), OptionalDataJson)
FMCPToolResult::Error(TEXT("message"))
```

**Tool annotation pattern** — every tool marks its behavior category:
```cpp
Info.Annotations = FMCPToolAnnotations::ReadOnly();    // no side effects
Info.Annotations = FMCPToolAnnotations::Modifying();   // reversible changes
Info.Annotations = FMCPToolAnnotations::Destructive(); // cannot undo
```

**TOptional<FMCPToolResult> error propagation** — used in FMCPToolBase helper signatures so callers return early with `OutError.GetValue()` on validation failure.

**JSON vector/rotator convention**:
- Vectors: `{ "x": 0, "y": 0, "z": 0 }`
- Rotators: `{ "pitch": 0, "yaw": 0, "roll": 0 }`
- Helpers: `UnrealClaudeJsonUtils::ExtractVector()`, `ExtractRotator()` in `UnrealClaudeUtils.h`

**Inheritance hierarchy for tools**:
```
IMCPTool (interface, MCPToolRegistry.h)
  └── FMCPToolBase (base with helpers, MCPToolBase.h)
        └── FMCPTool_SpawnActor, FMCPTool_MoveActor, etc. (MCPTool_*.h)
```

**File size limits** (enforced by CLAUDE.md):
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
  - Messages start with uppercase
  - No trailing periods (JSON convention)
  - Grouped by HTTP-like numeric codes (`1xx` params, `2xx` validation, `3xx` not-found, `4xx` operation, `5xx` context)
- `TOptional<FMCPToolResult>` used as out-parameter for early-return validation chains
- Null-check pattern: `if (!Tool) return false;` in tests after `TestNotNull`
- Editor context validated before world operations via `ValidateEditorContext(OutWorld)` helper

## Commit Conventions

Conventional Commits style, documented in `CLAUDE.md`:
- `feat:` — New MCP tools or features
- `fix:` — Bug fixes
- `test:` — Test additions or fixes
- `docs:` — Documentation updates
- `refactor:` — Code refactoring without behavior changes

Pre-commit requirement (from `CLAUDE.md`): Run `Automation RunTests UnrealClaude` before committing.
