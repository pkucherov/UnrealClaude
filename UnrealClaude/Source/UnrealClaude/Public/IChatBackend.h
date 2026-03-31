// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChatBackendTypes.h"

/**
 * Abstract interface for chat backend implementations.
 * Each backend (Claude, OpenCode, etc.) implements this interface.
 * The subsystem routes all chat requests through this abstraction,
 * enabling runtime backend swapping without UI or subsystem changes.
 *
 * 5 methods carried from IClaudeRunner + 4 new methods for backend identity
 * and polymorphic config/prompt handling.
 */
class UNREALCLAUDE_API IChatBackend
{
public:
	virtual ~IChatBackend() = default;

	// ===== Carried from IClaudeRunner (5 methods) =====

	/**
	 * Execute a chat request asynchronously
	 * @param Config - Request configuration (base type; backend may cast internally)
	 * @param OnComplete - Callback when execution completes
	 * @param OnProgress - Optional callback for streaming output
	 * @return true if execution started successfully
	 */
	virtual bool ExecuteAsync(
		const FChatRequestConfig& Config,
		FOnChatResponse OnComplete,
		FOnChatProgress OnProgress = FOnChatProgress()) = 0;

	/**
	 * Execute a chat request synchronously (blocking)
	 * @param Config - Request configuration
	 * @param OutResponse - Output response string
	 * @return true if execution succeeded
	 */
	virtual bool ExecuteSync(const FChatRequestConfig& Config, FString& OutResponse) = 0;

	/** Cancel the current execution */
	virtual void Cancel() = 0;

	/** Check if currently executing a request */
	virtual bool IsExecuting() const = 0;

	/**
	 * Check if the backend is available (CLI installed, server reachable, etc.)
	 * @return true if the backend is ready to accept requests
	 */
	virtual bool IsAvailable() const = 0;

	// ===== New methods (4 methods) =====

	/**
	 * Get the human-readable display name for this backend (e.g. "Claude", "OpenCode")
	 * @return Display name string
	 */
	virtual FString GetDisplayName() const = 0;

	/**
	 * Get the backend type enum value
	 * @return The EChatBackendType identifying this backend
	 */
	virtual EChatBackendType GetBackendType() const = 0;

	/**
	 * Format a prompt with conversation history using backend-specific conventions.
	 * Claude uses "Human:/Assistant:" format; OpenCode may use a different format.
	 * @param History - Array of (user prompt, assistant response) pairs
	 * @param SystemPrompt - System prompt to include
	 * @param ProjectContext - Project context string to include
	 * @return Formatted prompt string ready for the backend CLI
	 */
	virtual FString FormatPrompt(
		const TArray<TPair<FString, FString>>& History,
		const FString& SystemPrompt,
		const FString& ProjectContext) const = 0;

	/**
	 * Create a new config instance of the appropriate derived type for this backend.
	 * Enables polymorphic config creation without the caller knowing the concrete type.
	 * @return A TUniquePtr to a new config instance (e.g. FClaudeRequestConfig for Claude)
	 */
	virtual TUniquePtr<FChatRequestConfig> CreateConfig() const = 0;
};
