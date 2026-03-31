// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Unit tests for UnrealClaudeConstants.
 * Validates compile-time constants have sensible values: positive sizes,
 * valid port ranges, consistent bounds relationships.
 * One test per namespace, multiple assertions per test.
 */

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "UnrealClaudeConstants.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== MCP Server Constants =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FConstants_MCPServer_ValidValues,
	"UnrealClaude.Constants.MCPServer.ValidValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FConstants_MCPServer_ValidValues::RunTest(const FString& Parameters)
{
	TestEqual("DefaultPort should be 3000",
		UnrealClaudeConstants::MCPServer::DefaultPort, 3000u);
	TestTrue("DefaultPort should be >= 1024 (non-privileged)",
		UnrealClaudeConstants::MCPServer::DefaultPort >= 1024u);
	TestTrue("DefaultPort should be <= 65535 (valid port range)",
		UnrealClaudeConstants::MCPServer::DefaultPort <= 65535u);
	TestTrue("GameThreadTimeoutMs should be positive",
		UnrealClaudeConstants::MCPServer::GameThreadTimeoutMs > 0u);
	TestTrue("MaxRequestBodySize should be positive",
		UnrealClaudeConstants::MCPServer::MaxRequestBodySize > 0);

	return true;
}

// ===== Session Constants =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FConstants_Session_ValidValues,
	"UnrealClaude.Constants.Session.ValidValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FConstants_Session_ValidValues::RunTest(const FString& Parameters)
{
	TestTrue("MaxHistorySize should be positive",
		UnrealClaudeConstants::Session::MaxHistorySize > 0);
	TestTrue("MaxHistoryInPrompt should be positive",
		UnrealClaudeConstants::Session::MaxHistoryInPrompt > 0);
	TestTrue("MaxHistoryInPrompt should be <= MaxHistorySize",
		UnrealClaudeConstants::Session::MaxHistoryInPrompt <=
		UnrealClaudeConstants::Session::MaxHistorySize);

	return true;
}

// ===== Process Constants =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FConstants_Process_ValidValues,
	"UnrealClaude.Constants.Process.ValidValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FConstants_Process_ValidValues::RunTest(const FString& Parameters)
{
	TestTrue("DefaultTimeoutSeconds should be positive",
		UnrealClaudeConstants::Process::DefaultTimeoutSeconds > 0.0f);
	TestTrue("OutputBufferSize should be positive",
		UnrealClaudeConstants::Process::OutputBufferSize > 0);

	return true;
}

// ===== Validation Constants =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FConstants_Validation_ValidValues,
	"UnrealClaude.Constants.Validation.ValidValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FConstants_Validation_ValidValues::RunTest(const FString& Parameters)
{
	TestTrue("MaxActorNameLength should be positive",
		UnrealClaudeConstants::MCPValidation::MaxActorNameLength > 0);
	TestTrue("MaxPropertyPathLength should be positive",
		UnrealClaudeConstants::MCPValidation::MaxPropertyPathLength > 0);
	TestTrue("MaxPropertyPathLength should be >= MaxActorNameLength",
		UnrealClaudeConstants::MCPValidation::MaxPropertyPathLength >=
		UnrealClaudeConstants::MCPValidation::MaxActorNameLength);
	TestTrue("MaxClassPathLength should be positive",
		UnrealClaudeConstants::MCPValidation::MaxClassPathLength > 0);
	TestTrue("MaxCommandLength should be positive",
		UnrealClaudeConstants::MCPValidation::MaxCommandLength > 0);

	return true;
}

// ===== Numeric Bounds =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FConstants_NumericBounds_ValidValues,
	"UnrealClaude.Constants.NumericBounds.ValidValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FConstants_NumericBounds_ValidValues::RunTest(const FString& Parameters)
{
	TestTrue("MaxCoordinateValue should be positive",
		UnrealClaudeConstants::NumericBounds::MaxCoordinateValue > 0.0);

	return true;
}

// ===== UI Constants =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FConstants_UI_ValidValues,
	"UnrealClaude.Constants.UI.ValidValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FConstants_UI_ValidValues::RunTest(const FString& Parameters)
{
	TestTrue("MinInputHeight should be less than MaxInputHeight",
		UnrealClaudeConstants::UI::MinInputHeight <
		UnrealClaudeConstants::UI::MaxInputHeight);
	TestTrue("MinInputHeight should be positive",
		UnrealClaudeConstants::UI::MinInputHeight > 0.0f);
	TestTrue("MaxInputHeight should be positive",
		UnrealClaudeConstants::UI::MaxInputHeight > 0.0f);

	return true;
}

// ===== Clipboard Constants =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FConstants_Clipboard_ValidValues,
	"UnrealClaude.Constants.Clipboard.ValidValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FConstants_Clipboard_ValidValues::RunTest(const FString& Parameters)
{
	TestTrue("MaxImagesPerMessage should be positive",
		UnrealClaudeConstants::ClipboardImage::MaxImagesPerMessage > 0);
	TestTrue("MaxImageFileSize should be positive",
		UnrealClaudeConstants::ClipboardImage::MaxImageFileSize > 0);
	TestTrue("MaxTotalImagePayloadSize should be positive",
		UnrealClaudeConstants::ClipboardImage::MaxTotalImagePayloadSize > 0);
	TestTrue("MaxImageFileSize should be less than MaxTotalImagePayloadSize",
		UnrealClaudeConstants::ClipboardImage::MaxImageFileSize <
		UnrealClaudeConstants::ClipboardImage::MaxTotalImagePayloadSize);

	return true;
}

// ===== Script Execution Constants =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FConstants_ScriptExecution_ValidValues,
	"UnrealClaude.Constants.ScriptExecution.ValidValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FConstants_ScriptExecution_ValidValues::RunTest(const FString& Parameters)
{
	TestTrue("ScriptExecution MaxHistorySize should be positive",
		UnrealClaudeConstants::ScriptExecution::MaxHistorySize > 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
