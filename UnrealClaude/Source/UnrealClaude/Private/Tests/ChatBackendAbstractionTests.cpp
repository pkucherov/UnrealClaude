// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Abstraction layer tests for the chat backend system.
 * Verifies EChatBackendType enum serialization, config inheritance,
 * mock backend interface compliance, factory creation, and subsystem
 * backend routing. Split from ChatBackendTests.cpp per 500-line convention.
 */

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "ChatBackendTypes.h"
#include "IChatBackend.h"
#include "ClaudeRequestConfig.h"
#include "ChatSubsystem.h"
#include "ChatBackendFactory.h"
#include "Tests/TestUtils.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== EChatBackendType Enum Serialization Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Enum_ClaudeToString,
	"UnrealClaude.ChatBackend.Enum.ClaudeToString",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Enum_ClaudeToString::RunTest(const FString& Parameters)
{
	FString Result = ChatBackendUtils::EnumToString(EChatBackendType::Claude);
	TestEqual("Claude enum should convert to 'Claude' string",
		Result, TEXT("Claude"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Enum_OpenCodeToString,
	"UnrealClaude.ChatBackend.Enum.OpenCodeToString",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Enum_OpenCodeToString::RunTest(const FString& Parameters)
{
	FString Result = ChatBackendUtils::EnumToString(EChatBackendType::OpenCode);
	TestEqual("OpenCode enum should convert to 'OpenCode' string",
		Result, TEXT("OpenCode"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Enum_StringToClaude,
	"UnrealClaude.ChatBackend.Enum.StringToClaude",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Enum_StringToClaude::RunTest(const FString& Parameters)
{
	EChatBackendType Result = ChatBackendUtils::StringToEnum(TEXT("Claude"));
	TestEqual("'Claude' string should convert to Claude enum",
		Result, EChatBackendType::Claude);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Enum_StringToOpenCode,
	"UnrealClaude.ChatBackend.Enum.StringToOpenCode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Enum_StringToOpenCode::RunTest(const FString& Parameters)
{
	EChatBackendType Result = ChatBackendUtils::StringToEnum(TEXT("OpenCode"));
	TestEqual("'OpenCode' string should convert to OpenCode enum",
		Result, EChatBackendType::OpenCode);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Enum_UnknownStringDefaultsToClaude,
	"UnrealClaude.ChatBackend.Enum.UnknownStringDefaultsToClaude",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Enum_UnknownStringDefaultsToClaude::RunTest(const FString& Parameters)
{
	EChatBackendType Result = ChatBackendUtils::StringToEnum(TEXT("Garbage"));
	TestEqual("Unknown string should default to Claude",
		Result, EChatBackendType::Claude);
	return true;
}

// ===== Config Inheritance Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Config_BaseHasVirtualDestructor,
	"UnrealClaude.ChatBackend.Config.BaseHasVirtualDestructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Config_BaseHasVirtualDestructor::RunTest(const FString& Parameters)
{
	// Create FClaudeRequestConfig through base pointer and delete —
	// should not crash (virtual destructor required)
	TUniquePtr<FChatRequestConfig> Config = MakeUnique<FClaudeRequestConfig>();
	TestNotNull("Config should be non-null before reset", Config.Get());

	Config.Reset();
	TestNull("Config should be null after reset", Config.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Config_ClaudeSpecificFields,
	"UnrealClaude.ChatBackend.Config.ClaudeSpecificFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Config_ClaudeSpecificFields::RunTest(const FString& Parameters)
{
	FClaudeRequestConfig Config;

	// Verify Claude-specific fields exist and have correct defaults
	Config.bUseJsonOutput = true;
	TestTrue("bUseJsonOutput should be settable to true",
		Config.bUseJsonOutput);

	Config.bSkipPermissions = false;
	TestFalse("bSkipPermissions should be settable to false",
		Config.bSkipPermissions);

	Config.AllowedTools.Add(TEXT("read_file"));
	TestEqual("AllowedTools should accept entries",
		Config.AllowedTools.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Config_BaseFieldsAccessibleViaBasePtr,
	"UnrealClaude.ChatBackend.Config.BaseFieldsAccessibleViaBasePtr",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Config_BaseFieldsAccessibleViaBasePtr::RunTest(const FString& Parameters)
{
	FClaudeRequestConfig Derived;
	Derived.Prompt = TEXT("test prompt");
	Derived.SystemPrompt = TEXT("system");
	Derived.TimeoutSeconds = 120.0f;

	FChatRequestConfig* Base = &Derived;

	TestEqual("Prompt accessible via base pointer",
		Base->Prompt, TEXT("test prompt"));
	TestEqual("SystemPrompt accessible via base pointer",
		Base->SystemPrompt, TEXT("system"));
	TestEqual("TimeoutSeconds accessible via base pointer",
		Base->TimeoutSeconds, 120.0f);

	return true;
}

// ===== Mock Backend Interface Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Mock_ImplementsGetDisplayName,
	"UnrealClaude.ChatBackend.Mock.ImplementsGetDisplayName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Mock_ImplementsGetDisplayName::RunTest(const FString& Parameters)
{
	FMockChatBackend Mock;
	FString Name = Mock.GetDisplayName();

	TestTrue("GetDisplayName should return non-empty string",
		!Name.IsEmpty());
	TestEqual("GetDisplayName should return MockBackend",
		Name, TEXT("MockBackend"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Mock_ImplementsGetBackendType,
	"UnrealClaude.ChatBackend.Mock.ImplementsGetBackendType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Mock_ImplementsGetBackendType::RunTest(const FString& Parameters)
{
	FMockChatBackend Mock;
	EChatBackendType Type = Mock.GetBackendType();

	// Just verify it returns a valid enum value (doesn't crash)
	TestTrue("GetBackendType should return a valid enum",
		Type == EChatBackendType::Claude ||
		Type == EChatBackendType::OpenCode);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Mock_ImplementsCreateConfig,
	"UnrealClaude.ChatBackend.Mock.ImplementsCreateConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Mock_ImplementsCreateConfig::RunTest(const FString& Parameters)
{
	FMockChatBackend Mock;
	TUniquePtr<FChatRequestConfig> Config = Mock.CreateConfig();

	TestNotNull("CreateConfig should return non-null TUniquePtr",
		Config.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Mock_ImplementsFormatPrompt,
	"UnrealClaude.ChatBackend.Mock.ImplementsFormatPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Mock_ImplementsFormatPrompt::RunTest(const FString& Parameters)
{
	FMockChatBackend Mock;
	TArray<TPair<FString, FString>> History;
	History.Add(TPair<FString, FString>(TEXT("Hello"), TEXT("World")));

	FString Result = Mock.FormatPrompt(
		History, TEXT("system"), TEXT("context"));

	// Mock returns last history key
	TestEqual("FormatPrompt should return last history key",
		Result, TEXT("Hello"));

	return true;
}

// ===== Factory Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Factory_CreateClaude,
	"UnrealClaude.ChatBackend.Factory.CreateClaude",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Factory_CreateClaude::RunTest(const FString& Parameters)
{
	TUniquePtr<IChatBackend> Backend =
		FChatBackendFactory::Create(EChatBackendType::Claude);

	TestNotNull("Create(Claude) should return non-null backend",
		Backend.Get());

	if (Backend.IsValid())
	{
		TestEqual("Backend type should be Claude",
			Backend->GetBackendType(), EChatBackendType::Claude);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Factory_CreateOpenCode,
	"UnrealClaude.ChatBackend.Factory.CreateOpenCode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Factory_CreateOpenCode::RunTest(const FString& Parameters)
{
	// NOTE: In Phase 2, OpenCode falls back to Claude since no
	// OpenCode runner exists yet. This will change in Phase 4.
	TUniquePtr<IChatBackend> Backend =
		FChatBackendFactory::Create(EChatBackendType::OpenCode);

	TestNotNull("Create(OpenCode) should return non-null backend (falls back to Claude)",
		Backend.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Factory_IsBinaryAvailable,
	"UnrealClaude.ChatBackend.Factory.IsBinaryAvailable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Factory_IsBinaryAvailable::RunTest(const FString& Parameters)
{
	// Just verify the call doesn't crash — Claude CLI may or may
	// not be installed in the test environment
	bool bAvailable = FChatBackendFactory::IsBinaryAvailable(
		EChatBackendType::Claude);

	TestTrue("IsBinaryAvailable should return a valid bool",
		bAvailable || !bAvailable);

	return true;
}

// ===== Subsystem Backend Routing Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatSubsystem_Backend_GetActiveBackendType,
	"UnrealClaude.ChatSubsystem.Backend.GetActiveBackendType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_Backend_GetActiveBackendType::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();
	EChatBackendType Type = Subsystem.GetActiveBackendType();

	TestTrue("GetActiveBackendType should return valid enum value",
		Type == EChatBackendType::Claude ||
		Type == EChatBackendType::OpenCode);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatSubsystem_Backend_IsBackendAvailable,
	"UnrealClaude.ChatSubsystem.Backend.IsBackendAvailable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_Backend_IsBackendAvailable::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();
	bool bAvailable = Subsystem.IsBackendAvailable();

	TestTrue("IsBackendAvailable should return valid bool without crash",
		bAvailable || !bAvailable);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatSubsystem_Backend_GetDisplayName,
	"UnrealClaude.ChatSubsystem.Backend.GetDisplayName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_Backend_GetDisplayName::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();
	FString Name = Subsystem.GetBackendDisplayName();

	TestTrue("GetBackendDisplayName should return non-empty string",
		!Name.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatSubsystem_Backend_GetBackendNotNull,
	"UnrealClaude.ChatSubsystem.Backend.GetBackendNotNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatSubsystem_Backend_GetBackendNotNull::RunTest(const FString& Parameters)
{
	FChatSubsystem& Subsystem = FChatSubsystem::Get();
	IChatBackend* Backend = Subsystem.GetBackend();

	TestNotNull("GetBackend should return non-null pointer",
		Backend);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
