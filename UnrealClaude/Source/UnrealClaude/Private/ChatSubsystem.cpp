// Copyright Natali Caggiano. All Rights Reserved.

#include "ChatSubsystem.h"
#include "IChatBackend.h"
#include "ChatSessionManager.h"
#include "ChatBackendFactory.h"
#include "ClaudeRequestConfig.h"
#include "ProjectContext.h"
#include "ScriptExecutionManager.h"
#include "UnrealClaudeModule.h"
#include "UnrealClaudeConstants.h"

// Cached system prompt - static to avoid recreation on each call
static const FString CachedUE57SystemPrompt = TEXT(R"(You are an expert Unreal Engine 5.7 developer assistant integrated directly into the UE Editor.

CONTEXT:
- You are helping with an Unreal Engine 5.7 project
- The user is working in the Unreal Editor and expects UE5.7-specific guidance
- Focus on current UE5.7 APIs, patterns, and best practices

KEY UE5.7 FEATURES TO BE AWARE OF:
- Enhanced Nanite and Lumen for next-gen rendering
- World Partition for open world streaming
- Mass Entity (experimental) for large-scale simulations
- Enhanced Input System (preferred over legacy input)
- Common UI for cross-platform interfaces
- Gameplay Ability System (GAS) for complex ability systems
- MetaSounds for procedural audio
- Chaos physics engine (default)
- Control Rig for animation
- Niagara for VFX

CODING STANDARDS:
- Use UPROPERTY, UFUNCTION, UCLASS macros properly
- Follow Unreal naming conventions (F for structs, U for UObject, A for Actor, E for enums)
- Prefer BlueprintCallable/BlueprintPure for BP-exposed functions
- Use TObjectPtr<> for object pointers in headers (UE5+)
- Use Forward declarations in headers, includes in cpp
- Properly use GENERATED_BODY() macro

WHEN PROVIDING CODE:
- Always specify the correct includes
- Use proper UE5.7 API calls (not deprecated ones)
- Include both .h and .cpp when showing class implementations
- Explain any engine-specific gotchas or limitations

TOOL USAGE GUIDELINES:
- You have dedicated MCP tools for common Unreal Editor operations. ALWAYS prefer these over execute_script:
  * spawn_actor, move_actor, delete_actors, get_level_actors, set_property - Actor manipulation
  * open_level (open/new/list_templates) - Level management: open maps, create new levels, list templates
  * blueprint_query, blueprint_modify - Blueprint inspection and editing
  * anim_blueprint_modify - Animation blueprint state machines
  * asset_search, asset_dependencies, asset_referencers - Asset discovery and dependency tracking
  * capture_viewport - Screenshot the editor viewport
  * run_console_command - Run editor console commands
  * enhanced_input - Input action and mapping context management
  * character, character_data - Character and movement configuration
  * material - Material and material instance operations
  * task_submit, task_status, task_result, task_list, task_cancel - Async task management
- Only use execute_script when NO dedicated tool can accomplish the task
- Use open_level to switch levels instead of console commands (the 'open' command is blocked for security)
- Use get_ue_context to look up UE5.7 API patterns before writing scripts

RESPONSE FORMAT:
- Be concise but thorough
- Provide code examples when helpful
- Mention relevant documentation or resources
- Warn about common pitfalls)");

FChatSubsystem& FChatSubsystem::Get()
{
	static FChatSubsystem Instance;
	return Instance;
}

FChatSubsystem::FChatSubsystem()
{
	// Detect preferred backend and create it via factory
	ActiveBackendType = FChatBackendFactory::DetectPreferredBackend();
	Backend = FChatBackendFactory::Create(ActiveBackendType);
	SessionManager = MakeUnique<FChatSessionManager>();
}

FChatSubsystem::~FChatSubsystem()
{
	// Destructor defined here where full types are available
	// TUniquePtr will properly destroy the objects
}

IChatBackend* FChatSubsystem::GetBackend() const
{
	return Backend.Get();
}

void FChatSubsystem::SendPrompt(
	const FString& Prompt,
	FOnChatResponse OnComplete,
	const FChatPromptOptions& Options)
{
	// Use backend to create the right config type
	TUniquePtr<FChatRequestConfig> Config = Backend->CreateConfig();

	// Format prompt with history using backend-specific format
	const TArray<TPair<FString, FString>>& History = GetHistory();
	FString SystemPrompt;

	if (Options.bIncludeEngineContext)
	{
		SystemPrompt = GetUE57SystemPrompt();
	}

	FString ProjectContext;
	if (Options.bIncludeProjectContext)
	{
		ProjectContext = GetProjectContextPrompt();
		SystemPrompt += ProjectContext;
	}

	if (!CustomSystemPrompt.IsEmpty())
	{
		SystemPrompt += TEXT("\n\n") + CustomSystemPrompt;
	}

	// Build prompt with conversation history via backend
	FString FormattedHistory = Backend->FormatPrompt(History, SystemPrompt, ProjectContext);
	if (!FormattedHistory.IsEmpty())
	{
		Config->Prompt = FormattedHistory + Prompt;
	}
	else
	{
		Config->Prompt = Prompt;
	}

	Config->SystemPrompt = SystemPrompt;
	Config->WorkingDirectory = FPaths::ProjectDir();
	Config->AttachedImagePaths = Options.AttachedImagePaths;
	Config->OnStreamEvent = Options.OnStreamEvent;

	// Wrap completion to store history and save session
	FOnChatResponse WrappedComplete;
	WrappedComplete.BindLambda([this, Prompt, OnComplete](const FString& Response, bool bSuccess)
	{
		if (bSuccess && SessionManager.IsValid())
		{
			SessionManager->AddExchange(Prompt, Response);
			SessionManager->SaveSession();
		}
		OnComplete.ExecuteIfBound(Response, bSuccess);
	});

	Backend->ExecuteAsync(*Config, WrappedComplete, Options.OnProgress);
}

void FChatSubsystem::SendPrompt(
	const FString& Prompt,
	FOnChatResponse OnComplete,
	bool bIncludeUE57Context,
	FOnChatProgress OnProgress,
	bool bIncludeProjectContext)
{
	// Legacy API - delegate to new API
	FChatPromptOptions Options;
	Options.bIncludeEngineContext = bIncludeUE57Context;
	Options.bIncludeProjectContext = bIncludeProjectContext;
	Options.OnProgress = OnProgress;
	SendPrompt(Prompt, OnComplete, Options);
}

FString FChatSubsystem::GetUE57SystemPrompt() const
{
	// Return cached static prompt to avoid string recreation
	return CachedUE57SystemPrompt;
}

FString FChatSubsystem::GetProjectContextPrompt() const
{
	FString Context = FProjectContextManager::Get().FormatContextForPrompt();

	// Add script execution history (last 10 scripts)
	FString ScriptHistory = FScriptExecutionManager::Get().FormatHistoryForContext(10);
	if (!ScriptHistory.IsEmpty())
	{
		Context += TEXT("\n\n") + ScriptHistory;
	}

	return Context;
}

void FChatSubsystem::SetCustomSystemPrompt(const FString& InCustomPrompt)
{
	CustomSystemPrompt = InCustomPrompt;
}

const TArray<TPair<FString, FString>>& FChatSubsystem::GetHistory() const
{
	static TArray<TPair<FString, FString>> EmptyHistory;
	if (SessionManager.IsValid())
	{
		return SessionManager->GetHistory();
	}
	return EmptyHistory;
}

void FChatSubsystem::ClearHistory()
{
	if (SessionManager.IsValid())
	{
		SessionManager->ClearHistory();
	}
}

void FChatSubsystem::CancelCurrentRequest()
{
	if (Backend.IsValid())
	{
		Backend->Cancel();
	}
}

bool FChatSubsystem::SaveSession()
{
	if (SessionManager.IsValid())
	{
		return SessionManager->SaveSession();
	}
	return false;
}

bool FChatSubsystem::LoadSession()
{
	if (SessionManager.IsValid())
	{
		return SessionManager->LoadSession();
	}
	return false;
}

bool FChatSubsystem::HasSavedSession() const
{
	if (SessionManager.IsValid())
	{
		return SessionManager->HasSavedSession();
	}
	return false;
}

FString FChatSubsystem::GetSessionFilePath() const
{
	if (SessionManager.IsValid())
	{
		return SessionManager->GetSessionFilePath();
	}
	return FString();
}

// ===== Backend Abstraction Methods =====

void FChatSubsystem::SetActiveBackend(EChatBackendType Type)
{
	// Block swap during active request (per D-14)
	if (Backend.IsValid() && Backend->IsExecuting())
	{
		UE_LOG(LogUnrealClaude, Warning, TEXT("Cannot switch backend while a request is active"));
		return;
	}

	ActiveBackendType = Type;
	Backend = FChatBackendFactory::Create(Type);
	UE_LOG(LogUnrealClaude, Log, TEXT("Active backend switched to: %s"),
		*ChatBackendUtils::EnumToString(Type));
}

EChatBackendType FChatSubsystem::GetActiveBackendType() const
{
	return ActiveBackendType;
}

bool FChatSubsystem::IsBackendAvailable() const
{
	return Backend.IsValid() && Backend->IsAvailable();
}

FString FChatSubsystem::GetBackendDisplayName() const
{
	if (Backend.IsValid())
	{
		return Backend->GetDisplayName();
	}
	return TEXT("Unknown");
}
