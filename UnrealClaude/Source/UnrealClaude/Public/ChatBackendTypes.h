// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// ===== EChatBackendType =====

/**
 * Identifies which chat backend is active.
 * Used by factory, subsystem, and config persistence.
 */
enum class EChatBackendType : uint8
{
	/** Claude Code CLI backend */
	Claude,

	/** OpenCode CLI backend */
	OpenCode
};

/**
 * Utility functions for EChatBackendType string conversion.
 * Used for config serialization/persistence (D-28, D-29).
 */
namespace ChatBackendUtils
{
	/** Convert EChatBackendType to display/config string */
	inline FString EnumToString(EChatBackendType Type)
	{
		switch (Type)
		{
		case EChatBackendType::Claude:   return TEXT("Claude");
		case EChatBackendType::OpenCode: return TEXT("OpenCode");
		default:                         return TEXT("Claude");
		}
	}

	/** Convert string to EChatBackendType. Unknown strings default to Claude. */
	inline EChatBackendType StringToEnum(const FString& Str)
	{
		if (Str.Equals(TEXT("OpenCode"), ESearchCase::IgnoreCase))
		{
			return EChatBackendType::OpenCode;
		}
		return EChatBackendType::Claude;
	}
}

// ===== Delegates =====

/** Callback when a chat request completes */
DECLARE_DELEGATE_TwoParams(FOnChatResponse, const FString& /*Response*/, bool /*bSuccess*/);

/** Callback for streaming output progress */
DECLARE_DELEGATE_OneParam(FOnChatProgress, const FString& /*PartialOutput*/);

// ===== EChatStreamEventType =====

/**
 * Types of structured events from CLI stream-json NDJSON output.
 * Backend-agnostic — each backend maps its wire format to these types.
 */
enum class EChatStreamEventType : uint8
{
	/** Session initialization */
	SessionInit,
	/** Text content from assistant */
	TextContent,
	/** Tool use block from assistant (tool invocation) */
	ToolUse,
	/** Tool result returned to assistant */
	ToolResult,
	/** Final result with stats and cost */
	Result,
	/** Raw assistant message (full message, not parsed into sub-events) */
	AssistantMessage,
	/** Unknown or unparsed event type */
	Unknown
};

// ===== FChatStreamEvent =====

/**
 * Structured event parsed from CLI stream-json NDJSON output.
 * Each NDJSON line becomes one of these events.
 * Flat struct — no inheritance, no metadata map (D-09, D-10).
 */
struct UNREALCLAUDE_API FChatStreamEvent
{
	/** Event type */
	EChatStreamEventType Type = EChatStreamEventType::Unknown;

	/** Text content (for TextContent events) */
	FString Text;

	/** Tool name (for ToolUse events) */
	FString ToolName;

	/** Tool input JSON string (for ToolUse events) */
	FString ToolInput;

	/** Tool call ID (for ToolUse/ToolResult events) */
	FString ToolCallId;

	/** Tool result content (for ToolResult events) */
	FString ToolResultContent;

	/** Session ID (for SessionInit/Result events) */
	FString SessionId;

	/** Whether this is an error event */
	bool bIsError = false;

	/** Duration in ms (for Result events) */
	int32 DurationMs = 0;

	/** Number of turns (for Result events) */
	int32 NumTurns = 0;

	/** Total cost in USD (for Result events) */
	float TotalCostUsd = 0.0f;

	/** Result text (for Result events) */
	FString ResultText;

	/** Raw JSON line for debugging */
	FString RawJson;
};

/** Delegate for structured stream events */
DECLARE_DELEGATE_OneParam(FOnChatStreamEvent, const FChatStreamEvent& /*Event*/);

// ===== FChatRequestConfig =====

/**
 * Base configuration for chat backend execution.
 * Contains only fields common to ALL backends.
 * Backend-specific fields live in derived structs (e.g. FClaudeRequestConfig).
 * Virtual destructor enables safe polymorphic deletion (D-11).
 */
struct UNREALCLAUDE_API FChatRequestConfig
{
	/** The prompt to send to the backend */
	FString Prompt;

	/** Optional system prompt to append (for UE5.7 context) */
	FString SystemPrompt;

	/** Working directory for the backend CLI (usually project root) */
	FString WorkingDirectory;

	/** Timeout in seconds (0 = no timeout) */
	float TimeoutSeconds = 300.0f;

	/** Optional paths to attached clipboard images (PNG) */
	TArray<FString> AttachedImagePaths;

	/** Optional callback for structured NDJSON stream events */
	FOnChatStreamEvent OnStreamEvent;

	/** Virtual destructor for polymorphic deletion */
	virtual ~FChatRequestConfig() = default;
};

// ===== FChatPromptOptions =====

/**
 * Options for sending a prompt through FChatSubsystem.
 * Reduces parameter count in SendPrompt method.
 */
struct UNREALCLAUDE_API FChatPromptOptions
{
	/** Include UE5.7 engine context in system prompt */
	bool bIncludeEngineContext = true;

	/** Include project-specific context in system prompt */
	bool bIncludeProjectContext = true;

	/** Optional callback for streaming output progress */
	FOnChatProgress OnProgress;

	/** Optional callback for structured NDJSON stream events */
	FOnChatStreamEvent OnStreamEvent;

	/** Optional paths to attached clipboard images (PNG) */
	TArray<FString> AttachedImagePaths;

	/** Default constructor with sensible defaults */
	FChatPromptOptions() = default;

	/** Convenience constructor for common case */
	FChatPromptOptions(bool bEngineContext, bool bProjectContext)
		: bIncludeEngineContext(bEngineContext)
		, bIncludeProjectContext(bProjectContext)
	{}
};
