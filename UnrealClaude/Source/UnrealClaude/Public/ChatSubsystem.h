// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IChatBackend.h"
#include "ChatBackendTypes.h"

// Forward declarations
class FChatSessionManager;
class FChatBackendFactory;

/**
 * Subsystem for managing chat backend interactions.
 * Orchestrates backend, session management, and prompt building.
 * Routes all requests through IChatBackend — supports runtime backend swapping.
 */
class UNREALCLAUDE_API FChatSubsystem
{
public:
	static FChatSubsystem& Get();

	/** Destructor - must be defined in cpp where full types are available */
	~FChatSubsystem();

	/** Send a prompt to the active backend with optional context */
	void SendPrompt(
		const FString& Prompt,
		FOnChatResponse OnComplete,
		const FChatPromptOptions& Options = FChatPromptOptions()
	);

	/** Send a prompt (legacy API for backward compatibility) */
	void SendPrompt(
		const FString& Prompt,
		FOnChatResponse OnComplete,
		bool bIncludeUE57Context,
		FOnChatProgress OnProgress,
		bool bIncludeProjectContext = true
	);

	/** Get the default UE5.7 system prompt */
	FString GetUE57SystemPrompt() const;

	/** Get the project context prompt */
	FString GetProjectContextPrompt() const;

	/** Set custom system prompt additions */
	void SetCustomSystemPrompt(const FString& InCustomPrompt);

	/** Get conversation history (delegates to session manager) */
	const TArray<TPair<FString, FString>>& GetHistory() const;

	/** Clear conversation history */
	void ClearHistory();

	/** Cancel current request */
	void CancelCurrentRequest();

	/** Save current session to disk */
	bool SaveSession();

	/** Load previous session from disk */
	bool LoadSession();

	/** Check if a previous session exists */
	bool HasSavedSession() const;

	/** Get session file path */
	FString GetSessionFilePath() const;

	/** Get the active backend interface */
	IChatBackend* GetBackend() const;

	// ===== Backend Abstraction Methods =====

	/** Switch the active backend (blocked if request active per D-14) */
	void SetActiveBackend(EChatBackendType Type);

	/** Get the active backend type */
	EChatBackendType GetActiveBackendType() const;

	/** Check if the active backend is available */
	bool IsBackendAvailable() const;

	/** Get the active backend's display name */
	FString GetBackendDisplayName() const;

private:
	FChatSubsystem();

	TUniquePtr<IChatBackend> Backend;
	EChatBackendType ActiveBackendType;
	TUniquePtr<FChatSessionManager> SessionManager;
	FString CustomSystemPrompt;
};
