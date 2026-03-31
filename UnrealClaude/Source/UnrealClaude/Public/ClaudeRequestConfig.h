// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChatBackendTypes.h"

/**
 * Claude-specific request configuration.
 * Extends FChatRequestConfig with fields unique to the Claude Code CLI.
 * Created by FClaudeCodeRunner::CreateConfig() and populated by the subsystem.
 */
struct UNREALCLAUDE_API FClaudeRequestConfig : public FChatRequestConfig
{
	/** Use JSON output format for structured responses (Claude CLI flag) */
	bool bUseJsonOutput = false;

	/** Skip permission prompts (--dangerously-skip-permissions) (Claude CLI flag) */
	bool bSkipPermissions = true;

	/** Allow Claude to use tools (Read, Write, Bash, etc.) (Claude CLI flag) */
	TArray<FString> AllowedTools;
};
