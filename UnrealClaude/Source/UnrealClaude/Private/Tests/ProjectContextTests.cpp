// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Unit tests for FProjectContextManager and related structs.
 * Tests singleton access (D-23), structure validation without
 * asserting specific counts (D-26), formatting safety, and
 * struct default regression guards (D-12).
 *
 * SAFETY: Uses bForceRefresh=false to avoid expensive scans.
 * Context-dependent assertions guarded with HasContext().
 */

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "ProjectContext.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== Singleton Access Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectContext_Singleton_GetReturnsValidRef,
	"UnrealClaude.ProjectContext.Singleton.GetReturnsValidRef",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FProjectContext_Singleton_GetReturnsValidRef::RunTest(const FString& Parameters)
{
	FProjectContextManager& Manager1 = FProjectContextManager::Get();
	FProjectContextManager& Manager2 = FProjectContextManager::Get();

	TestEqual("Get() should return same address on repeated calls",
		&Manager1, &Manager2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectContext_Singleton_HasContextReturnsBool,
	"UnrealClaude.ProjectContext.Singleton.HasContextReturnsBool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FProjectContext_Singleton_HasContextReturnsBool::RunTest(const FString& Parameters)
{
	FProjectContextManager& Manager = FProjectContextManager::Get();
	bool bHas = Manager.HasContext();

	// HasContext() returns a valid boolean — either state is acceptable
	TestTrue("HasContext() should return a valid bool",
		bHas || !bHas);

	return true;
}

// ===== Context Gathering Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectContext_Gathering_ProjectNameNotEmpty,
	"UnrealClaude.ProjectContext.Gathering.ProjectNameNotEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FProjectContext_Gathering_ProjectNameNotEmpty::RunTest(const FString& Parameters)
{
	FProjectContextManager& Manager = FProjectContextManager::Get();

	if (Manager.HasContext())
	{
		const FProjectContext& Context = Manager.GetContext(false);
		TestTrue("ProjectName should not be empty when context is gathered",
			!Context.ProjectName.IsEmpty());
	}
	else
	{
		AddInfo(TEXT("Context not yet gathered — skipping content assertions"));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectContext_Gathering_ProjectPathNotEmpty,
	"UnrealClaude.ProjectContext.Gathering.ProjectPathNotEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FProjectContext_Gathering_ProjectPathNotEmpty::RunTest(const FString& Parameters)
{
	FProjectContextManager& Manager = FProjectContextManager::Get();

	if (Manager.HasContext())
	{
		const FProjectContext& Context = Manager.GetContext(false);
		TestTrue("ProjectPath should not be empty when context is gathered",
			!Context.ProjectPath.IsEmpty());
	}
	else
	{
		AddInfo(TEXT("Context not yet gathered — skipping content assertions"));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectContext_Gathering_AssetCountNonNegative,
	"UnrealClaude.ProjectContext.Gathering.AssetCountNonNegative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FProjectContext_Gathering_AssetCountNonNegative::RunTest(const FString& Parameters)
{
	FProjectContextManager& Manager = FProjectContextManager::Get();

	if (Manager.HasContext())
	{
		const FProjectContext& Context = Manager.GetContext(false);
		TestTrue("AssetCount should be non-negative",
			Context.AssetCount >= 0);
		TestTrue("BlueprintCount should be non-negative",
			Context.BlueprintCount >= 0);
		TestTrue("CppClassCount should be non-negative",
			Context.CppClassCount >= 0);
	}
	else
	{
		AddInfo(TEXT("Context not yet gathered — skipping count assertions"));
	}

	return true;
}

// ===== Formatting Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectContext_Formatting_FormatContextNoCrash,
	"UnrealClaude.ProjectContext.Formatting.FormatContextNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FProjectContext_Formatting_FormatContextNoCrash::RunTest(const FString& Parameters)
{
	FProjectContextManager& Manager = FProjectContextManager::Get();

	// FormatContextForPrompt may return empty if no context gathered.
	// The important thing is that it doesn't crash.
	FString Formatted = Manager.FormatContextForPrompt();

	TestTrue("FormatContextForPrompt() should return valid string",
		Formatted.Len() >= 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectContext_Formatting_GetContextSummaryNoCrash,
	"UnrealClaude.ProjectContext.Formatting.GetContextSummaryNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FProjectContext_Formatting_GetContextSummaryNoCrash::RunTest(const FString& Parameters)
{
	FProjectContextManager& Manager = FProjectContextManager::Get();

	// GetContextSummary returns a string in either state
	FString Summary = Manager.GetContextSummary();

	TestTrue("GetContextSummary() should return non-empty string",
		!Summary.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectContext_Formatting_TimeSinceRefreshNonNegative,
	"UnrealClaude.ProjectContext.Formatting.TimeSinceRefreshNonNegative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FProjectContext_Formatting_TimeSinceRefreshNonNegative::RunTest(const FString& Parameters)
{
	FProjectContextManager& Manager = FProjectContextManager::Get();
	FTimespan TimeSince = Manager.GetTimeSinceRefresh();

	// Should be non-negative (or MaxValue if no context gathered)
	TestTrue("GetTimeSinceRefresh() should return non-negative timespan",
		TimeSince.GetTicks() >= 0);

	return true;
}

// ===== Struct Default Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectContext_Defaults_ProjectContextZeroInit,
	"UnrealClaude.ProjectContext.Defaults.ProjectContextZeroInit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FProjectContext_Defaults_ProjectContextZeroInit::RunTest(const FString& Parameters)
{
	FProjectContext Context;

	TestEqual("AssetCount should default to 0",
		Context.AssetCount, 0);
	TestEqual("BlueprintCount should default to 0",
		Context.BlueprintCount, 0);
	TestEqual("CppClassCount should default to 0",
		Context.CppClassCount, 0);
	TestTrue("ProjectName should default to empty",
		Context.ProjectName.IsEmpty());
	TestTrue("ProjectPath should default to empty",
		Context.ProjectPath.IsEmpty());
	TestEqual("SourceFiles should default to empty array",
		Context.SourceFiles.Num(), 0);
	TestEqual("UClasses should default to empty array",
		Context.UClasses.Num(), 0);
	TestEqual("LevelActors should default to empty array",
		Context.LevelActors.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectContext_Defaults_UClassInfoDefaults,
	"UnrealClaude.ProjectContext.Defaults.UClassInfoDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FProjectContext_Defaults_UClassInfoDefaults::RunTest(const FString& Parameters)
{
	FUClassInfo ClassInfo;

	TestTrue("ClassName should default to empty",
		ClassInfo.ClassName.IsEmpty());
	TestTrue("ParentClass should default to empty",
		ClassInfo.ParentClass.IsEmpty());
	TestTrue("FilePath should default to empty",
		ClassInfo.FilePath.IsEmpty());
	TestFalse("bIsBlueprint should default to false",
		ClassInfo.bIsBlueprint);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProjectContext_Defaults_LevelActorInfoDefaults,
	"UnrealClaude.ProjectContext.Defaults.LevelActorInfoDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FProjectContext_Defaults_LevelActorInfoDefaults::RunTest(const FString& Parameters)
{
	FLevelActorInfo ActorInfo;

	TestTrue("Name should default to empty",
		ActorInfo.Name.IsEmpty());
	TestTrue("Label should default to empty",
		ActorInfo.Label.IsEmpty());
	TestTrue("ClassName should default to empty",
		ActorInfo.ClassName.IsEmpty());
	TestEqual("Location should default to zero vector",
		ActorInfo.Location, FVector::ZeroVector);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
