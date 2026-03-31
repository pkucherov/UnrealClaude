// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Unit tests for FScriptExecutionManager and related types.
 * Tests safe read-only methods: history queries, path accessors,
 * format helpers, and type/struct regression guards (D-12).
 *
 * SAFETY: NEVER calls ExecuteScript() (shows modal dialog — Pitfall 10).
 * NEVER calls ClearHistory/SaveHistory/LoadHistory on the singleton
 * (state leak). Only read-only methods tested on the live singleton.
 * Struct and enum tests use stack-local instances.
 */

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "ScriptExecutionManager.h"
#include "ScriptTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== Singleton Access Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScriptExecution_Singleton_GetNoCrash,
	"UnrealClaude.ScriptExecution.Singleton.GetNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FScriptExecution_Singleton_GetNoCrash::RunTest(const FString& Parameters)
{
	FScriptExecutionManager& Manager1 = FScriptExecutionManager::Get();
	FScriptExecutionManager& Manager2 = FScriptExecutionManager::Get();

	TestEqual("Get() should return same address on repeated calls",
		&Manager1, &Manager2);

	return true;
}

// ===== History Query Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScriptExecution_History_GetRecentZeroReturnsEmpty,
	"UnrealClaude.ScriptExecution.History.GetRecentZeroReturnsEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FScriptExecution_History_GetRecentZeroReturnsEmpty::RunTest(const FString& Parameters)
{
	FScriptExecutionManager& Manager = FScriptExecutionManager::Get();
	TArray<FScriptHistoryEntry> Recent = Manager.GetRecentScripts(0);

	TestEqual("GetRecentScripts(0) should return empty array",
		Recent.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScriptExecution_History_GetRecentTenNonNegative,
	"UnrealClaude.ScriptExecution.History.GetRecentTenNonNegative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FScriptExecution_History_GetRecentTenNonNegative::RunTest(const FString& Parameters)
{
	FScriptExecutionManager& Manager = FScriptExecutionManager::Get();
	TArray<FScriptHistoryEntry> Recent = Manager.GetRecentScripts(10);

	TestTrue("GetRecentScripts(10) should return non-negative count",
		Recent.Num() >= 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScriptExecution_History_FormatZeroNoCrash,
	"UnrealClaude.ScriptExecution.History.FormatZeroNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FScriptExecution_History_FormatZeroNoCrash::RunTest(const FString& Parameters)
{
	FScriptExecutionManager& Manager = FScriptExecutionManager::Get();
	FString Formatted = Manager.FormatHistoryForContext(0);

	// FormatHistoryForContext(0) may return empty string — that's fine
	TestTrue("FormatHistoryForContext(0) should return valid string",
		Formatted.Len() >= 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScriptExecution_History_FormatTenNoCrash,
	"UnrealClaude.ScriptExecution.History.FormatTenNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FScriptExecution_History_FormatTenNoCrash::RunTest(const FString& Parameters)
{
	FScriptExecutionManager& Manager = FScriptExecutionManager::Get();
	FString Formatted = Manager.FormatHistoryForContext(10);

	TestTrue("FormatHistoryForContext(10) should return valid string",
		Formatted.Len() >= 0);

	return true;
}

// ===== Path Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScriptExecution_Paths_HistoryFilePathNotEmpty,
	"UnrealClaude.ScriptExecution.Paths.HistoryFilePathNotEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FScriptExecution_Paths_HistoryFilePathNotEmpty::RunTest(const FString& Parameters)
{
	FScriptExecutionManager& Manager = FScriptExecutionManager::Get();
	FString Path = Manager.GetHistoryFilePath();

	TestTrue("GetHistoryFilePath() should not be empty",
		!Path.IsEmpty());
	TestTrue("History path should contain UnrealClaude",
		Path.Contains(TEXT("UnrealClaude")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScriptExecution_Paths_CppScriptDirNotEmpty,
	"UnrealClaude.ScriptExecution.Paths.CppScriptDirNotEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FScriptExecution_Paths_CppScriptDirNotEmpty::RunTest(const FString& Parameters)
{
	FScriptExecutionManager& Manager = FScriptExecutionManager::Get();
	FString Dir = Manager.GetCppScriptDirectory();

	TestTrue("GetCppScriptDirectory() should not be empty",
		!Dir.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScriptExecution_Paths_ContentScriptDirNotEmpty,
	"UnrealClaude.ScriptExecution.Paths.ContentScriptDirNotEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FScriptExecution_Paths_ContentScriptDirNotEmpty::RunTest(const FString& Parameters)
{
	FScriptExecutionManager& Manager = FScriptExecutionManager::Get();
	FString Dir = Manager.GetContentScriptDirectory();

	TestTrue("GetContentScriptDirectory() should not be empty",
		!Dir.IsEmpty());

	return true;
}

// ===== Type Regression Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScriptExecution_Types_EnumHasFourValues,
	"UnrealClaude.ScriptExecution.Types.EnumHasFourValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FScriptExecution_Types_EnumHasFourValues::RunTest(const FString& Parameters)
{
	// Verify all 4 enum values exist and convert to distinct strings
	FString CppStr = ScriptTypeToString(EScriptType::Cpp);
	FString PythonStr = ScriptTypeToString(EScriptType::Python);
	FString ConsoleStr = ScriptTypeToString(EScriptType::Console);
	FString EditorStr = ScriptTypeToString(EScriptType::EditorUtility);

	TestEqual("Cpp should convert to 'cpp'", CppStr, FString(TEXT("cpp")));
	TestEqual("Python should convert to 'python'",
		PythonStr, FString(TEXT("python")));
	TestEqual("Console should convert to 'console'",
		ConsoleStr, FString(TEXT("console")));
	TestEqual("EditorUtility should convert to 'editor_utility'",
		EditorStr, FString(TEXT("editor_utility")));

	// Verify round-trip
	TestEqual("StringToScriptType('cpp') round-trip",
		StringToScriptType(TEXT("cpp")), EScriptType::Cpp);
	TestEqual("StringToScriptType('python') round-trip",
		StringToScriptType(TEXT("python")), EScriptType::Python);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScriptExecution_Types_ExecutionResultDefaults,
	"UnrealClaude.ScriptExecution.Types.ExecutionResultDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FScriptExecution_Types_ExecutionResultDefaults::RunTest(const FString& Parameters)
{
	FScriptExecutionResult Result;

	TestFalse("bSuccess should default to false", Result.bSuccess);
	TestTrue("Message should default to empty", Result.Message.IsEmpty());
	TestTrue("Output should default to empty", Result.Output.IsEmpty());
	TestTrue("ErrorOutput should default to empty",
		Result.ErrorOutput.IsEmpty());
	TestEqual("RetryCount should default to 0", Result.RetryCount, 0);

	// Test static factory methods
	FScriptExecutionResult SuccessResult =
		FScriptExecutionResult::Success(TEXT("OK"), TEXT("output"));
	TestTrue("Success factory sets bSuccess=true", SuccessResult.bSuccess);
	TestEqual("Success factory sets message",
		SuccessResult.Message, FString(TEXT("OK")));

	FScriptExecutionResult ErrorResult =
		FScriptExecutionResult::Error(TEXT("Fail"), TEXT("err"));
	TestFalse("Error factory sets bSuccess=false", ErrorResult.bSuccess);
	TestEqual("Error factory sets error output",
		ErrorResult.ErrorOutput, FString(TEXT("err")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScriptExecution_Types_HistoryEntryFieldsAccessible,
	"UnrealClaude.ScriptExecution.Types.HistoryEntryFieldsAccessible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FScriptExecution_Types_HistoryEntryFieldsAccessible::RunTest(const FString& Parameters)
{
	FScriptHistoryEntry Entry;

	TestTrue("ScriptId should be valid GUID", Entry.ScriptId.IsValid());
	TestEqual("ScriptType should default to Console",
		Entry.ScriptType, EScriptType::Console);
	TestTrue("Filename should default to empty",
		Entry.Filename.IsEmpty());
	TestTrue("Description should default to empty",
		Entry.Description.IsEmpty());
	TestFalse("bSuccess should default to false", Entry.bSuccess);
	TestTrue("ResultMessage should default to empty",
		Entry.ResultMessage.IsEmpty());
	TestTrue("FilePath should default to empty",
		Entry.FilePath.IsEmpty());

	// Verify ToJson doesn't crash
	TSharedPtr<FJsonObject> Json = Entry.ToJson();
	TestNotNull("ToJson() should return valid JSON object", Json.Get());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
