// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Unit tests for FChatSubsystem.
 * Tests singleton access, system prompts, history management,
 * session persistence, and FChatPromptOptions defaults.
 * 
 * SAFETY: No tests call SendPrompt (spawns subprocess).
 * ClearHistory test documents state modification.
 */

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "ChatSubsystem.h"
#include "IChatBackend.h"
#include "ChatBackendTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== Singleton Access Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatSubsystem_Singleton_GetReturnsSameInstance,
	"UnrealClaude.ChatSubsystem.Singleton.GetReturnsSameInstance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_Singleton_GetReturnsSameInstance::RunTest(const FString& Parameters)
{
	FChatSubsystem& Instance1 = FChatSubsystem::Get();
	FChatSubsystem& Instance2 = FChatSubsystem::Get();

	TestEqual("Get() should return same address on repeated calls",
		&Instance1, &Instance2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatSubsystem_Singleton_GetBackendNotNull,
	"UnrealClaude.ChatSubsystem.Singleton.GetBackendNotNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_Singleton_GetBackendNotNull::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();
	IChatBackend* Backend = Subsystem.GetBackend();

	TestNotNull("GetBackend() should return a valid backend pointer", Backend);

	return true;
}

// ===== System Prompt Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatSubsystem_Prompt_UE57SystemPromptNotEmpty,
	"UnrealClaude.ChatSubsystem.Prompt.UE57SystemPromptNotEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_Prompt_UE57SystemPromptNotEmpty::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();
	FString SystemPrompt = Subsystem.GetUE57SystemPrompt();

	TestTrue("System prompt should not be empty", !SystemPrompt.IsEmpty());
	TestTrue("System prompt should mention Unreal Engine",
		SystemPrompt.Contains(TEXT("Unreal Engine")));
	TestTrue("System prompt should mention UE5.7",
		SystemPrompt.Contains(TEXT("5.7")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatSubsystem_Prompt_ProjectContextNoCrash,
	"UnrealClaude.ChatSubsystem.Prompt.ProjectContextNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_Prompt_ProjectContextNoCrash::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();

	// Project context may be empty if context hasn't been gathered yet.
	// The important thing is that this call doesn't crash.
	FString ProjectContext = Subsystem.GetProjectContextPrompt();

	// Just verify it returns a valid FString (non-null)
	TestTrue("GetProjectContextPrompt() should return valid string (possibly empty)",
		ProjectContext.Len() >= 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatSubsystem_Prompt_SetCustomNoCrash,
	"UnrealClaude.ChatSubsystem.Prompt.SetCustomNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_Prompt_SetCustomNoCrash::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();

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
	FChatSubsystem_History_GetHistoryReturnsValidArray,
	"UnrealClaude.ChatSubsystem.History.GetHistoryReturnsValidArray",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_History_GetHistoryReturnsValidArray::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();
	const TArray<TPair<FString, FString>>& History = Subsystem.GetHistory();

	TestTrue("GetHistory() should return array with non-negative count",
		History.Num() >= 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatSubsystem_History_ClearHistoryResetsToEmpty,
	"UnrealClaude.ChatSubsystem.History.ClearHistoryResetsToEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_History_ClearHistoryResetsToEmpty::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();

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
	FChatSubsystem_Session_FilePathNotEmpty,
	"UnrealClaude.ChatSubsystem.Session.FilePathNotEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_Session_FilePathNotEmpty::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();
	FString SessionPath = Subsystem.GetSessionFilePath();

	TestTrue("Session file path should not be empty", !SessionPath.IsEmpty());
	TestTrue("Session file path should contain UnrealClaude",
		SessionPath.Contains(TEXT("UnrealClaude")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatSubsystem_Session_HasSavedSessionNoCrash,
	"UnrealClaude.ChatSubsystem.Session.HasSavedSessionNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_Session_HasSavedSessionNoCrash::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();

	// HasSavedSession depends on filesystem state — we just verify no crash
	bool bHasSession = Subsystem.HasSavedSession();

	TestTrue("HasSavedSession() should return a valid bool",
		bHasSession || !bHasSession);

	return true;
}

// ===== Safety Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatSubsystem_Safety_CancelWhenIdleNoCrash,
	"UnrealClaude.ChatSubsystem.Safety.CancelWhenIdleNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_Safety_CancelWhenIdleNoCrash::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();

	// CancelCurrentRequest when nothing is running should be safe
	Subsystem.CancelCurrentRequest();

	TestTrue("CancelCurrentRequest() should not crash when idle", true);

	return true;
}

// ===== Regression Guards =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatSubsystem_Regression_PromptOptionsDefaults,
	"UnrealClaude.ChatSubsystem.Regression.PromptOptionsDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_Regression_PromptOptionsDefaults::RunTest(const FString& Parameters)
{
	FChatPromptOptions Options;

	TestTrue("bIncludeEngineContext should default to true",
		Options.bIncludeEngineContext);
	TestTrue("bIncludeProjectContext should default to true",
		Options.bIncludeProjectContext);
	TestEqual("AttachedImagePaths should default to empty",
		Options.AttachedImagePaths.Num(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
