// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Unit tests for FUnrealClaudeMCPServer and FMCPToolRegistry.
 * Tests tool registration, metadata, result types, annotation factories,
 * and server construction (without calling Start/Stop to avoid port conflicts).
 */

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "MCP/UnrealClaudeMCPServer.h"
#include "MCP/MCPToolRegistry.h"
#include "UnrealClaudeConstants.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== Tool Registry Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPServer_Registry_Construction,
	"UnrealClaude.MCPServer.Registry.Construction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_Registry_Construction::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;

	// Construction should not crash and should register builtin tools
	TArray<FMCPToolInfo> AllTools = Registry.GetAllTools();
	TestTrue("Registry should have tools after construction",
		AllTools.Num() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPServer_Registry_ToolCountPositive,
	"UnrealClaude.MCPServer.Registry.ToolCountPositive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_Registry_ToolCountPositive::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;

	TArray<FMCPToolInfo> AllTools = Registry.GetAllTools();
	TestTrue("Tool count should be greater than zero",
		AllTools.Num() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPServer_Registry_FindToolValid,
	"UnrealClaude.MCPServer.Registry.FindToolValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_Registry_FindToolValid::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;

	// spawn_actor should always be registered
	IMCPTool* Tool = Registry.FindTool(TEXT("spawn_actor"));
	TestNotNull("FindTool('spawn_actor') should return a valid tool", Tool);

	if (Tool)
	{
		FMCPToolInfo Info = Tool->GetInfo();
		TestEqual("Tool name should be 'spawn_actor'",
			Info.Name, TEXT("spawn_actor"));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPServer_Registry_FindToolInvalid,
	"UnrealClaude.MCPServer.Registry.FindToolInvalid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_Registry_FindToolInvalid::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;

	IMCPTool* Tool = Registry.FindTool(TEXT("nonexistent_tool_xyz"));
	TestNull("FindTool for nonexistent tool should return nullptr", Tool);

	TestFalse("HasTool should return false for nonexistent tool",
		Registry.HasTool(TEXT("nonexistent_tool_xyz")));

	return true;
}

// ===== Tool Registry Snapshot Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPServer_Snapshot_ExpectedToolsRegistered,
	"UnrealClaude.MCPServer.Snapshot.ExpectedToolsRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_Snapshot_ExpectedToolsRegistered::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;

	for (const FString& ToolName : UnrealClaudeConstants::MCPServer::ExpectedTools)
	{
		TestTrue(
			FString::Printf(TEXT("Expected tool '%s' should be registered"), *ToolName),
			Registry.HasTool(ToolName));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPServer_Snapshot_AllToolsHaveNonEmptyName,
	"UnrealClaude.MCPServer.Snapshot.AllToolsHaveNonEmptyName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_Snapshot_AllToolsHaveNonEmptyName::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;
	TArray<FMCPToolInfo> AllTools = Registry.GetAllTools();

	for (const FMCPToolInfo& ToolInfo : AllTools)
	{
		TestFalse(
			FString::Printf(TEXT("Tool name should not be empty (found: '%s')"), *ToolInfo.Name),
			ToolInfo.Name.IsEmpty());
	}

	return true;
}

// ===== Tool Metadata Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPServer_Metadata_AllToolsHaveDescription,
	"UnrealClaude.MCPServer.Metadata.AllToolsHaveDescription",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_Metadata_AllToolsHaveDescription::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;
	TArray<FMCPToolInfo> AllTools = Registry.GetAllTools();

	for (const FMCPToolInfo& ToolInfo : AllTools)
	{
		TestFalse(
			FString::Printf(TEXT("Tool '%s' should have a non-empty description"), *ToolInfo.Name),
			ToolInfo.Description.IsEmpty());
	}

	return true;
}

// ===== Result Type Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPServer_Result_SuccessIsTrue,
	"UnrealClaude.MCPServer.Result.SuccessIsTrue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_Result_SuccessIsTrue::RunTest(const FString& Parameters)
{
	FMCPToolResult Result = FMCPToolResult::Success(TEXT("Operation completed"));

	TestTrue("Success result should have bSuccess=true", Result.bSuccess);
	TestEqual("Success result message should match",
		Result.Message, TEXT("Operation completed"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPServer_Result_ErrorIsFalse,
	"UnrealClaude.MCPServer.Result.ErrorIsFalse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_Result_ErrorIsFalse::RunTest(const FString& Parameters)
{
	FMCPToolResult Result = FMCPToolResult::Error(TEXT("Something went wrong"));

	TestFalse("Error result should have bSuccess=false", Result.bSuccess);
	TestEqual("Error result message should match",
		Result.Message, TEXT("Something went wrong"));

	return true;
}

// ===== Annotation Factory Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPServer_Annotations_ReadOnlyFactory,
	"UnrealClaude.MCPServer.Annotations.ReadOnlyFactory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_Annotations_ReadOnlyFactory::RunTest(const FString& Parameters)
{
	FMCPToolAnnotations Annotations = FMCPToolAnnotations::ReadOnly();

	TestTrue("ReadOnly should have bReadOnlyHint=true",
		Annotations.bReadOnlyHint);
	TestFalse("ReadOnly should have bDestructiveHint=false",
		Annotations.bDestructiveHint);
	TestTrue("ReadOnly should have bIdempotentHint=true",
		Annotations.bIdempotentHint);
	TestFalse("ReadOnly should have bOpenWorldHint=false",
		Annotations.bOpenWorldHint);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPServer_Annotations_ModifyingFactory,
	"UnrealClaude.MCPServer.Annotations.ModifyingFactory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_Annotations_ModifyingFactory::RunTest(const FString& Parameters)
{
	FMCPToolAnnotations Annotations = FMCPToolAnnotations::Modifying();

	TestFalse("Modifying should have bReadOnlyHint=false",
		Annotations.bReadOnlyHint);
	TestFalse("Modifying should have bDestructiveHint=false",
		Annotations.bDestructiveHint);
	TestFalse("Modifying should have bIdempotentHint=false",
		Annotations.bIdempotentHint);
	TestFalse("Modifying should have bOpenWorldHint=false",
		Annotations.bOpenWorldHint);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPServer_Annotations_DestructiveFactory,
	"UnrealClaude.MCPServer.Annotations.DestructiveFactory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_Annotations_DestructiveFactory::RunTest(const FString& Parameters)
{
	FMCPToolAnnotations Annotations = FMCPToolAnnotations::Destructive();

	TestFalse("Destructive should have bReadOnlyHint=false",
		Annotations.bReadOnlyHint);
	TestTrue("Destructive should have bDestructiveHint=true",
		Annotations.bDestructiveHint);
	TestFalse("Destructive should have bIdempotentHint=false",
		Annotations.bIdempotentHint);
	TestFalse("Destructive should have bOpenWorldHint=false",
		Annotations.bOpenWorldHint);

	return true;
}

// ===== Server Construction Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPServer_Server_ConstructWithoutCrash,
	"UnrealClaude.MCPServer.Server.ConstructWithoutCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPServer_Server_ConstructWithoutCrash::RunTest(const FString& Parameters)
{
	// Construct without calling Start — should not crash
	FUnrealClaudeMCPServer Server;

	TestFalse("Server should not be running after construction",
		Server.IsRunning());
	TestEqual("Server default port should be DefaultPort",
		Server.GetPort(), UnrealClaudeConstants::MCPServer::DefaultPort);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
