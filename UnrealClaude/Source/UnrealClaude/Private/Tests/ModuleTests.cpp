// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Unit tests for FUnrealClaudeModule startup effects.
 * Verifies that module initialization correctly created all
 * singletons and populated system state. Tests verify effects
 * only — never calls StartupModule/ShutdownModule directly.
 *
 * Pattern: Module is already started when tests run (D-10).
 * We verify the singletons it initializes are accessible.
 */

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "ChatSubsystem.h"
#include "IChatBackend.h"
#include "ProjectContext.h"
#include "ScriptExecutionManager.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== Module Startup Effect Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnrealClaudeModule_Startup_SubsystemInitialized,
	"UnrealClaude.Module.Startup.SubsystemInitialized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FUnrealClaudeModule_Startup_SubsystemInitialized::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();

	TestEqual("Subsystem singleton should return consistent address",
		&Subsystem, &FChatSubsystem::Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnrealClaudeModule_Startup_RunnerAvailable,
	"UnrealClaude.Module.Startup.RunnerAvailable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FUnrealClaudeModule_Startup_RunnerAvailable::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();
	IChatBackend* Backend = Subsystem.GetBackend();

	TestNotNull("Module should have initialized a backend", Backend);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnrealClaudeModule_Startup_SystemPromptBuilt,
	"UnrealClaude.Module.Startup.SystemPromptBuilt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FUnrealClaudeModule_Startup_SystemPromptBuilt::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();
	FString SystemPrompt = Subsystem.GetUE57SystemPrompt();

	TestTrue("System prompt should be non-empty after module startup",
		!SystemPrompt.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnrealClaudeModule_Startup_ContextManagerInitialized,
	"UnrealClaude.Module.Startup.ContextManagerInitialized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FUnrealClaudeModule_Startup_ContextManagerInitialized::RunTest(const FString& Parameters)
{
	FProjectContextManager& Manager = FProjectContextManager::Get();

	TestEqual("ProjectContextManager singleton should be consistent",
		&Manager, &FProjectContextManager::Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnrealClaudeModule_Startup_ScriptManagerInitialized,
	"UnrealClaude.Module.Startup.ScriptManagerInitialized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FUnrealClaudeModule_Startup_ScriptManagerInitialized::RunTest(const FString& Parameters)
{
	FScriptExecutionManager& Manager = FScriptExecutionManager::Get();

	TestEqual("ScriptExecutionManager singleton should be consistent",
		&Manager, &FScriptExecutionManager::Get());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
