// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Unit tests for FClaudeSessionManager.
 * Tests standalone instances (NOT the singleton subsystem).
 * Covers construction defaults, history state machine, configuration,
 * and persistence (save/load/round-trip).
 *
 * IMPORTANT: Every test that calls SaveSession deletes the file at the end.
 */

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "ClaudeSessionManager.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== Construction Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_Construction_EmptyHistory,
	"UnrealClaude.Session.Construction.EmptyHistory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_Construction_EmptyHistory::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;

	TestEqual("Fresh manager should have empty history",
		Manager.GetHistory().Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_Construction_PositiveMaxHistorySize,
	"UnrealClaude.Session.Construction.PositiveMaxHistorySize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_Construction_PositiveMaxHistorySize::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;

	TestTrue("Default MaxHistorySize should be positive",
		Manager.GetMaxHistorySize() > 0);

	return true;
}

// ===== History State Machine Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_History_AddExchangeStoresData,
	"UnrealClaude.Session.History.AddExchangeStoresData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_History_AddExchangeStoresData::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;

	Manager.AddExchange(TEXT("Hello"), TEXT("World"));

	TestEqual("History should have 1 exchange after AddExchange",
		Manager.GetHistory().Num(), 1);
	TestEqual("Prompt should match",
		Manager.GetHistory()[0].Key, TEXT("Hello"));
	TestEqual("Response should match",
		Manager.GetHistory()[0].Value, TEXT("World"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_History_MultipleAddsFIFOOrder,
	"UnrealClaude.Session.History.MultipleAddsFIFOOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_History_MultipleAddsFIFOOrder::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;

	Manager.AddExchange(TEXT("First"), TEXT("Response1"));
	Manager.AddExchange(TEXT("Second"), TEXT("Response2"));
	Manager.AddExchange(TEXT("Third"), TEXT("Response3"));

	TestEqual("History should have 3 exchanges",
		Manager.GetHistory().Num(), 3);
	TestEqual("First exchange prompt should be at index 0",
		Manager.GetHistory()[0].Key, TEXT("First"));
	TestEqual("Second exchange prompt should be at index 1",
		Manager.GetHistory()[1].Key, TEXT("Second"));
	TestEqual("Third exchange prompt should be at index 2",
		Manager.GetHistory()[2].Key, TEXT("Third"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_History_ClearHistoryEmpties,
	"UnrealClaude.Session.History.ClearHistoryEmpties",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_History_ClearHistoryEmpties::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;

	Manager.AddExchange(TEXT("Prompt1"), TEXT("Response1"));
	Manager.AddExchange(TEXT("Prompt2"), TEXT("Response2"));
	TestEqual("History should have 2 exchanges before clear",
		Manager.GetHistory().Num(), 2);

	Manager.ClearHistory();

	TestEqual("History should be empty after ClearHistory",
		Manager.GetHistory().Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_History_MaxSizeEnforcement,
	"UnrealClaude.Session.History.MaxSizeEnforcement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_History_MaxSizeEnforcement::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;
	Manager.SetMaxHistorySize(3);

	// Add 5 exchanges — only last 3 should remain
	Manager.AddExchange(TEXT("P1"), TEXT("R1"));
	Manager.AddExchange(TEXT("P2"), TEXT("R2"));
	Manager.AddExchange(TEXT("P3"), TEXT("R3"));
	Manager.AddExchange(TEXT("P4"), TEXT("R4"));
	Manager.AddExchange(TEXT("P5"), TEXT("R5"));

	TestEqual("History should be clamped to MaxHistorySize=3",
		Manager.GetHistory().Num(), 3);
	TestEqual("Oldest surviving exchange should be P3",
		Manager.GetHistory()[0].Key, TEXT("P3"));
	TestEqual("Middle surviving exchange should be P4",
		Manager.GetHistory()[1].Key, TEXT("P4"));
	TestEqual("Newest exchange should be P5",
		Manager.GetHistory()[2].Key, TEXT("P5"));

	return true;
}

// ===== Configuration Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_Config_GetMaxHistorySize,
	"UnrealClaude.Session.Config.GetMaxHistorySize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_Config_GetMaxHistorySize::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;

	// Default should match the constant
	int32 DefaultMax = Manager.GetMaxHistorySize();
	TestTrue("Default MaxHistorySize should be positive",
		DefaultMax > 0);
	TestEqual("Default MaxHistorySize should be 50",
		DefaultMax, 50);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_Config_SetMaxHistorySize,
	"UnrealClaude.Session.Config.SetMaxHistorySize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_Config_SetMaxHistorySize::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;

	Manager.SetMaxHistorySize(25);
	TestEqual("MaxHistorySize should be set to 25",
		Manager.GetMaxHistorySize(), 25);

	Manager.SetMaxHistorySize(100);
	TestEqual("MaxHistorySize should be set to 100",
		Manager.GetMaxHistorySize(), 100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_Config_ClampZeroToMinOne,
	"UnrealClaude.Session.Config.ClampZeroToMinOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_Config_ClampZeroToMinOne::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;

	Manager.SetMaxHistorySize(0);
	TestEqual("Setting MaxHistorySize=0 should clamp to 1",
		Manager.GetMaxHistorySize(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_Config_ClampNegativeToMinOne,
	"UnrealClaude.Session.Config.ClampNegativeToMinOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_Config_ClampNegativeToMinOne::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;

	Manager.SetMaxHistorySize(-10);
	TestEqual("Setting MaxHistorySize=-10 should clamp to 1",
		Manager.GetMaxHistorySize(), 1);

	Manager.SetMaxHistorySize(-1);
	TestEqual("Setting MaxHistorySize=-1 should clamp to 1",
		Manager.GetMaxHistorySize(), 1);

	return true;
}

// ===== Persistence Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_Persistence_SaveSessionWritesFile,
	"UnrealClaude.Session.Persistence.SaveSessionWritesFile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_Persistence_SaveSessionWritesFile::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;
	Manager.AddExchange(TEXT("TestPrompt"), TEXT("TestResponse"));

	bool bSaved = Manager.SaveSession();
	TestTrue("SaveSession should return true", bSaved);

	FString SessionPath = Manager.GetSessionFilePath();
	TestTrue("Session file should exist on disk after save",
		IFileManager::Get().FileExists(*SessionPath));

	// Cleanup
	IFileManager::Get().Delete(*Manager.GetSessionFilePath());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_Persistence_LoadSessionRoundTrip,
	"UnrealClaude.Session.Persistence.LoadSessionRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_Persistence_LoadSessionRoundTrip::RunTest(const FString& Parameters)
{
	// Save with one manager
	FClaudeSessionManager SaveManager;
	SaveManager.AddExchange(TEXT("RoundTripPrompt"), TEXT("RoundTripResponse"));
	SaveManager.AddExchange(TEXT("SecondPrompt"), TEXT("SecondResponse"));
	bool bSaved = SaveManager.SaveSession();
	TestTrue("SaveSession should succeed", bSaved);

	// Load with a fresh manager
	FClaudeSessionManager LoadManager;
	bool bLoaded = LoadManager.LoadSession();
	TestTrue("LoadSession should return true", bLoaded);

	TestEqual("Loaded history should have 2 exchanges",
		LoadManager.GetHistory().Num(), 2);
	TestEqual("First loaded prompt should match",
		LoadManager.GetHistory()[0].Key, TEXT("RoundTripPrompt"));
	TestEqual("First loaded response should match",
		LoadManager.GetHistory()[0].Value, TEXT("RoundTripResponse"));
	TestEqual("Second loaded prompt should match",
		LoadManager.GetHistory()[1].Key, TEXT("SecondPrompt"));
	TestEqual("Second loaded response should match",
		LoadManager.GetHistory()[1].Value, TEXT("SecondResponse"));

	// Cleanup
	IFileManager::Get().Delete(*LoadManager.GetSessionFilePath());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_Persistence_GetSessionFilePathContainsUnrealClaude,
	"UnrealClaude.Session.Persistence.GetSessionFilePathContainsUnrealClaude",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_Persistence_GetSessionFilePathContainsUnrealClaude::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;

	FString SessionPath = Manager.GetSessionFilePath();
	TestTrue("Session file path should contain 'UnrealClaude'",
		SessionPath.Contains(TEXT("UnrealClaude")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_Persistence_HasSavedSessionTrueAfterSave,
	"UnrealClaude.Session.Persistence.HasSavedSessionTrueAfterSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_Persistence_HasSavedSessionTrueAfterSave::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;

	// Ensure no leftover file
	IFileManager::Get().Delete(*Manager.GetSessionFilePath());
	TestFalse("HasSavedSession should be false before save",
		Manager.HasSavedSession());

	Manager.AddExchange(TEXT("Prompt"), TEXT("Response"));
	Manager.SaveSession();

	TestTrue("HasSavedSession should be true after save",
		Manager.HasSavedSession());

	// Cleanup
	IFileManager::Get().Delete(*Manager.GetSessionFilePath());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSessionManager_Persistence_LoadSessionNoFileReturnsFalse,
	"UnrealClaude.Session.Persistence.LoadSessionNoFileReturnsFalse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSessionManager_Persistence_LoadSessionNoFileReturnsFalse::RunTest(const FString& Parameters)
{
	FClaudeSessionManager Manager;

	// Ensure no file exists
	IFileManager::Get().Delete(*Manager.GetSessionFilePath());

	bool bLoaded = Manager.LoadSession();
	TestFalse("LoadSession should return false when no file exists",
		bLoaded);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
