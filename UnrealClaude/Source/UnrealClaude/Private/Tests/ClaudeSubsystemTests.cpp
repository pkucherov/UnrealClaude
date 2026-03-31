// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Unit tests for FClaudeCodeSubsystem
 * Tests singleton access, system prompts, history management,
 * session persistence, and FClaudePromptOptions defaults.
 * 
 * SAFETY: No tests call SendPrompt (spawns subprocess).
 * ClearHistory test documents state modification.
 */

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "ClaudeSubsystem.h"
#include "IClaudeRunner.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== Singleton Access Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClaudeSubsystem_Singleton_GetReturnsSameInstance,
	"UnrealClaude.ClaudeSubsystem.Singleton.GetReturnsSameInstance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeSubsystem_Singleton_GetReturnsSameInstance::RunTest(const FString& Parameters)
{
	FClaudeCodeSubsystem& Instance1 = FClaudeCodeSubsystem::Get();
	FClaudeCodeSubsystem& Instance2 = FClaudeCodeSubsystem::Get();

	TestEqual("Get() should return same address on repeated calls",
		&Instance1, &Instance2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClaudeSubsystem_Singleton_GetRunnerNotNull,
	"UnrealClaude.ClaudeSubsystem.Singleton.GetRunnerNotNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeSubsystem_Singleton_GetRunnerNotNull::RunTest(const FString& Parameters)
{
	FClaudeCodeSubsystem& Subsystem = FClaudeCodeSubsystem::Get();
	IClaudeRunner* Runner = Subsystem.GetRunner();

	TestNotNull("GetRunner() should return a valid runner pointer", Runner);

	return true;
}

// ===== System Prompt Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClaudeSubsystem_Prompt_UE57SystemPromptNotEmpty,
	"UnrealClaude.ClaudeSubsystem.Prompt.UE57SystemPromptNotEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeSubsystem_Prompt_UE57SystemPromptNotEmpty::RunTest(const FString& Parameters)
{
	FClaudeCodeSubsystem& Subsystem = FClaudeCodeSubsystem::Get();
	FString SystemPrompt = Subsystem.GetUE57SystemPrompt();

	TestTrue("System prompt should not be empty", !SystemPrompt.IsEmpty());
	TestTrue("System prompt should mention Unreal Engine",
		SystemPrompt.Contains(TEXT("Unreal Engine")));
	TestTrue("System prompt should mention UE5.7",
		SystemPrompt.Contains(TEXT("5.7")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClaudeSubsystem_Prompt_ProjectContextNoCrash,
	"UnrealClaude.ClaudeSubsystem.Prompt.ProjectContextNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeSubsystem_Prompt_ProjectContextNoCrash::RunTest(const FString& Parameters)
{
	FClaudeCodeSubsystem& Subsystem = FClaudeCodeSubsystem::Get();

	// Project context may be empty if context hasn't been gathered yet.
	// The important thing is that this call doesn't crash.
	FString ProjectContext = Subsystem.GetProjectContextPrompt();

	// Just verify it returns a valid FString (non-null)
	TestTrue("GetProjectContextPrompt() should return valid string (possibly empty)",
		ProjectContext.Len() >= 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClaudeSubsystem_Prompt_SetCustomNoCrash,
	"UnrealClaude.ClaudeSubsystem.Prompt.SetCustomNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeSubsystem_Prompt_SetCustomNoCrash::RunTest(const FString& Parameters)
{
	FClaudeCodeSubsystem& Subsystem = FClaudeCodeSubsystem::Get();

	// SetCustomSystemPrompt modifies private state that isn't directly observable.
	// We only verify it doesn't crash when called with various inputs.
	Subsystem.SetCustomSystemPrompt(TEXT("Test custom prompt"));
	Subsystem.SetCustomSystemPrompt(TEXT(""));
	Subsystem.SetCustomSystemPrompt(TEXT("Multi\nLine\nPrompt"));

	// Restore to empty to avoid affecting other tests
	Subsystem.SetCustomSystemPrompt(TEXT(""));

	TestTrue("SetCustomSystemPrompt should not crash with various inputs", true);

	return true;
}

// ===== History Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClaudeSubsystem_History_GetHistoryReturnsValidArray,
	"UnrealClaude.ClaudeSubsystem.History.GetHistoryReturnsValidArray",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeSubsystem_History_GetHistoryReturnsValidArray::RunTest(const FString& Parameters)
{
	FClaudeCodeSubsystem& Subsystem = FClaudeCodeSubsystem::Get();
	const TArray<TPair<FString, FString>>& History = Subsystem.GetHistory();

	TestTrue("GetHistory() should return array with non-negative count",
		History.Num() >= 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClaudeSubsystem_History_ClearHistoryResetsToEmpty,
	"UnrealClaude.ClaudeSubsystem.History.ClearHistoryResetsToEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeSubsystem_History_ClearHistoryResetsToEmpty::RunTest(const FString& Parameters)
{
	FClaudeCodeSubsystem& Subsystem = FClaudeCodeSubsystem::Get();

	// Save the current history count before modification
	int32 OriginalCount = Subsystem.GetHistory().Num();

	// ClearHistory should reset to zero entries
	Subsystem.ClearHistory();

	TestEqual("History should be empty after ClearHistory()",
		Subsystem.GetHistory().Num(), 0);

	// NOTE: This test modifies singleton state (history was cleared).
	// Original history count was saved but cannot be restored without
	// calling SendPrompt (which spawns a subprocess). Document this.
	AddInfo(FString::Printf(TEXT("History had %d entries before clear (state modified)"),
		OriginalCount));

	return true;
}

// ===== Session Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClaudeSubsystem_Session_FilePathNotEmpty,
	"UnrealClaude.ClaudeSubsystem.Session.FilePathNotEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeSubsystem_Session_FilePathNotEmpty::RunTest(const FString& Parameters)
{
	FClaudeCodeSubsystem& Subsystem = FClaudeCodeSubsystem::Get();
	FString SessionPath = Subsystem.GetSessionFilePath();

	TestTrue("Session file path should not be empty", !SessionPath.IsEmpty());
	TestTrue("Session file path should contain UnrealClaude",
		SessionPath.Contains(TEXT("UnrealClaude")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClaudeSubsystem_Session_HasSavedSessionNoCrash,
	"UnrealClaude.ClaudeSubsystem.Session.HasSavedSessionNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeSubsystem_Session_HasSavedSessionNoCrash::RunTest(const FString& Parameters)
{
	FClaudeCodeSubsystem& Subsystem = FClaudeCodeSubsystem::Get();

	// HasSavedSession depends on filesystem state — we just verify no crash
	bool bHasSession = Subsystem.HasSavedSession();

	TestTrue("HasSavedSession() should return a valid bool",
		bHasSession || !bHasSession);

	return true;
}

// ===== Safety Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClaudeSubsystem_Safety_CancelWhenIdleNoCrash,
	"UnrealClaude.ClaudeSubsystem.Safety.CancelWhenIdleNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeSubsystem_Safety_CancelWhenIdleNoCrash::RunTest(const FString& Parameters)
{
	FClaudeCodeSubsystem& Subsystem = FClaudeCodeSubsystem::Get();

	// CancelCurrentRequest when nothing is running should be safe
	Subsystem.CancelCurrentRequest();

	TestTrue("CancelCurrentRequest() should not crash when idle", true);

	return true;
}

// ===== Regression Guards =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClaudeSubsystem_Regression_PromptOptionsDefaults,
	"UnrealClaude.ClaudeSubsystem.Regression.PromptOptionsDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FClaudeSubsystem_Regression_PromptOptionsDefaults::RunTest(const FString& Parameters)
{
	FClaudePromptOptions Options;

	TestTrue("bIncludeEngineContext should default to true",
		Options.bIncludeEngineContext);
	TestTrue("bIncludeProjectContext should default to true",
		Options.bIncludeProjectContext);
	TestEqual("AttachedImagePaths should default to empty",
		Options.AttachedImagePaths.Num(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
