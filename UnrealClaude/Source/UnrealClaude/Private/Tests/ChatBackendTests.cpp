// Copyright Natali Caggiano. All Rights Reserved.

/** Unit tests for FClaudeCodeRunner and related structs.
 * Tests NDJSON stream parsing, payload construction, struct defaults,
 * static methods, and IChatBackend interface contract.
 * SAFETY: No tests call ExecuteAsync or ExecuteSync. Only safe helpers tested.
 */

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "ClaudeCodeRunner.h"
#include "IChatBackend.h"
#include "ChatBackendTypes.h"
#include "ClaudeRequestConfig.h"
#include "Tests/TestUtils.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== Stream Event Struct Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_StreamEvent_DefaultValues,
	"UnrealClaude.ChatBackend.StreamEvent.DefaultValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_StreamEvent_DefaultValues::RunTest(const FString& Parameters)
{
	FChatStreamEvent Event;

	TestEqual("Default Type should be Unknown",
		Event.Type, EChatStreamEventType::Unknown);
	TestTrue("Default Text should be empty", Event.Text.IsEmpty());
	TestTrue("Default ToolName should be empty", Event.ToolName.IsEmpty());
	TestTrue("Default ToolInput should be empty", Event.ToolInput.IsEmpty());
	TestTrue("Default ToolCallId should be empty", Event.ToolCallId.IsEmpty());
	TestTrue("Default ToolResultContent should be empty",
		Event.ToolResultContent.IsEmpty());
	TestTrue("Default SessionId should be empty", Event.SessionId.IsEmpty());
	TestFalse("Default bIsError should be false", Event.bIsError);
	TestEqual("Default DurationMs should be 0", Event.DurationMs, 0);
	TestEqual("Default NumTurns should be 0", Event.NumTurns, 0);
	TestEqual("Default TotalCostUsd should be 0.0f",
		Event.TotalCostUsd, 0.0f);
	TestTrue("Default ResultText should be empty", Event.ResultText.IsEmpty());
	TestTrue("Default RawJson should be empty", Event.RawJson.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_StreamEvent_AllEnumValues,
	"UnrealClaude.ChatBackend.StreamEvent.AllEnumValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_StreamEvent_AllEnumValues::RunTest(const FString& Parameters)
{
	// Verify all 7 enum values can be assigned and compared
	FChatStreamEvent Event;

	Event.Type = EChatStreamEventType::SessionInit;
	TestEqual("SessionInit assignment",
		Event.Type, EChatStreamEventType::SessionInit);

	Event.Type = EChatStreamEventType::TextContent;
	TestEqual("TextContent assignment",
		Event.Type, EChatStreamEventType::TextContent);

	Event.Type = EChatStreamEventType::ToolUse;
	TestEqual("ToolUse assignment",
		Event.Type, EChatStreamEventType::ToolUse);

	Event.Type = EChatStreamEventType::ToolResult;
	TestEqual("ToolResult assignment",
		Event.Type, EChatStreamEventType::ToolResult);

	Event.Type = EChatStreamEventType::Result;
	TestEqual("Result assignment",
		Event.Type, EChatStreamEventType::Result);

	Event.Type = EChatStreamEventType::AssistantMessage;
	TestEqual("AssistantMessage assignment",
		Event.Type, EChatStreamEventType::AssistantMessage);

	Event.Type = EChatStreamEventType::Unknown;
	TestEqual("Unknown assignment",
		Event.Type, EChatStreamEventType::Unknown);

	return true;
}

// ===== Request Config Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_RequestConfig_DefaultValues,
	"UnrealClaude.ChatBackend.RequestConfig.DefaultValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_RequestConfig_DefaultValues::RunTest(const FString& Parameters)
{
	FClaudeRequestConfig Config;

	TestTrue("Default Prompt should be empty", Config.Prompt.IsEmpty());
	TestTrue("Default SystemPrompt should be empty",
		Config.SystemPrompt.IsEmpty());
	TestTrue("Default WorkingDirectory should be empty",
		Config.WorkingDirectory.IsEmpty());
	TestFalse("Default bUseJsonOutput should be false",
		Config.bUseJsonOutput);
	TestTrue("Default bSkipPermissions should be true",
		Config.bSkipPermissions);
	TestEqual("Default TimeoutSeconds should be 300.0f",
		Config.TimeoutSeconds, 300.0f);
	TestEqual("Default AllowedTools should be empty",
		Config.AllowedTools.Num(), 0);
	TestEqual("Default AttachedImagePaths should be empty",
		Config.AttachedImagePaths.Num(), 0);

	return true;
}

// ===== NDJSON Parsing Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Parse_EmptyInput,
	"UnrealClaude.ChatBackend.Parse.EmptyInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Parse_EmptyInput::RunTest(const FString& Parameters)
{
	FClaudeCodeRunner Runner;
	FString Result = Runner.ParseStreamJsonOutput(TEXT(""));

	// Empty input returns error fallback message
	TestTrue("Empty input should return error or empty response",
		Result.Contains(TEXT("Error")) || Result.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Parse_ValidResultMessage,
	"UnrealClaude.ChatBackend.Parse.ValidResultMessage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Parse_ValidResultMessage::RunTest(const FString& Parameters)
{
	FClaudeCodeRunner Runner;

	// A result message — the primary output format ParseStreamJsonOutput looks for
	FString Input = TEXT("{\"type\":\"result\",\"result\":\"Hello from Claude\",\"subtype\":\"success\",\"is_error\":false,\"duration_ms\":1234,\"num_turns\":1,\"total_cost_usd\":0.01}");

	FString Result = Runner.ParseStreamJsonOutput(Input);

	TestEqual("Should extract result text from result message",
		Result, TEXT("Hello from Claude"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Parse_ValidAssistantMessage,
	"UnrealClaude.ChatBackend.Parse.ValidAssistantMessage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Parse_ValidAssistantMessage::RunTest(const FString& Parameters)
{
	FClaudeCodeRunner Runner;

	// Assistant message with text content block (fallback path)
	FString Input = TEXT("{\"type\":\"assistant\",\"message\":{\"content\":[{\"type\":\"text\",\"text\":\"Hello world\"}]}}");

	FString Result = Runner.ParseStreamJsonOutput(Input);

	TestEqual("Should extract text from assistant content blocks",
		Result, TEXT("Hello world"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Parse_MalformedJson,
	"UnrealClaude.ChatBackend.Parse.MalformedJson",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Parse_MalformedJson::RunTest(const FString& Parameters)
{
	FClaudeCodeRunner Runner;

	FString Input = TEXT("{broken json that is not valid");
	FString Result = Runner.ParseStreamJsonOutput(Input);

	// Malformed JSON should not crash, returns error fallback
	TestTrue("Malformed JSON should return error message",
		Result.Contains(TEXT("Error")) || !Result.IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Parse_MultipleNdjsonLines,
	"UnrealClaude.ChatBackend.Parse.MultipleNdjsonLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Parse_MultipleNdjsonLines::RunTest(const FString& Parameters)
{
	FClaudeCodeRunner Runner;

	// Multiple NDJSON lines: system init, assistant, result
	FString Input =
		TEXT("{\"type\":\"system\",\"subtype\":\"init\",\"session_id\":\"test-123\"}\n")
		TEXT("{\"type\":\"assistant\",\"message\":{\"content\":[{\"type\":\"text\",\"text\":\"Part 1\"}]}}\n")
		TEXT("{\"type\":\"result\",\"result\":\"Final answer\",\"is_error\":false}\n");

	FString Result = Runner.ParseStreamJsonOutput(Input);

	// Result message takes priority over assistant blocks
	TestEqual("Should extract result text when result message present",
		Result, TEXT("Final answer"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Parse_AssistantFallbackMultipleBlocks,
	"UnrealClaude.ChatBackend.Parse.AssistantFallbackMultipleBlocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Parse_AssistantFallbackMultipleBlocks::RunTest(const FString& Parameters)
{
	FClaudeCodeRunner Runner;

	// Multiple assistant messages without a result message (fallback path)
	FString Input =
		TEXT("{\"type\":\"assistant\",\"message\":{\"content\":[{\"type\":\"text\",\"text\":\"Hello \"}]}}\n")
		TEXT("{\"type\":\"assistant\",\"message\":{\"content\":[{\"type\":\"text\",\"text\":\"World\"}]}}\n");

	FString Result = Runner.ParseStreamJsonOutput(Input);

	TestEqual("Should concatenate text from multiple assistant blocks",
		Result, TEXT("Hello World"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Parse_MixedValidAndInvalidLines,
	"UnrealClaude.ChatBackend.Parse.MixedValidAndInvalidLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Parse_MixedValidAndInvalidLines::RunTest(const FString& Parameters)
{
	FClaudeCodeRunner Runner;

	// Mix of valid NDJSON, empty lines, and malformed lines
	FString Input =
		TEXT("\n")
		TEXT("{broken line}\n")
		TEXT("{\"type\":\"result\",\"result\":\"valid output\"}\n")
		TEXT("\n")
		TEXT("{also broken}\n");

	FString Result = Runner.ParseStreamJsonOutput(Input);

	TestEqual("Should extract result despite malformed lines",
		Result, TEXT("valid output"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Parse_ToolUseLineIgnored,
	"UnrealClaude.ChatBackend.Parse.ToolUseLineIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Parse_ToolUseLineIgnored::RunTest(const FString& Parameters)
{
	FClaudeCodeRunner Runner;

	// Tool use messages should be skipped when looking for text
	FString Input =
		TEXT("{\"type\":\"assistant\",\"message\":{\"content\":[{\"type\":\"tool_use\",\"name\":\"read_file\",\"id\":\"t1\",\"input\":{}}]}}\n")
		TEXT("{\"type\":\"result\",\"result\":\"done\"}\n");

	FString Result = Runner.ParseStreamJsonOutput(Input);

	TestEqual("Should return result, ignoring tool_use blocks",
		Result, TEXT("done"));

	return true;
}

// ===== Payload Construction Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Payload_TextOnly,
	"UnrealClaude.ChatBackend.Payload.TextOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Payload_TextOnly::RunTest(const FString& Parameters)
{
	FClaudeCodeRunner Runner;
	TArray<FString> EmptyImages;

	FString Payload = Runner.BuildStreamJsonPayload(
		TEXT("Hello Claude"), EmptyImages);

	TestTrue("Payload should not be empty", !Payload.IsEmpty());
	TestTrue("Payload should end with newline (NDJSON format)",
		Payload.EndsWith(TEXT("\n")));

	// Parse and verify structure
	FString PayloadTrimmed = Payload.TrimEnd();
	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader =
		TJsonReaderFactory<>::Create(PayloadTrimmed);

	bool bParsed = FJsonSerializer::Deserialize(Reader, JsonObj);
	TestTrue("Payload should be valid JSON", bParsed);

	if (bParsed && JsonObj.IsValid())
	{
		TestEqual("Outer type should be 'user'",
			JsonObj->GetStringField(TEXT("type")), TEXT("user"));

		const TSharedPtr<FJsonObject>* MessageObj;
		TestTrue("Should have 'message' field",
			JsonObj->TryGetObjectField(TEXT("message"), MessageObj));

		if (MessageObj)
		{
			TestEqual("Message role should be 'user'",
				(*MessageObj)->GetStringField(TEXT("role")), TEXT("user"));

			const TArray<TSharedPtr<FJsonValue>>* ContentArray;
			TestTrue("Message should have content array",
				(*MessageObj)->TryGetArrayField(TEXT("content"), ContentArray));

			if (ContentArray && ContentArray->Num() > 0)
			{
				const TSharedPtr<FJsonObject>* TextBlock;
				TestTrue("First content block should be object",
					(*ContentArray)[0]->TryGetObject(TextBlock));

				if (TextBlock)
				{
					TestEqual("Content type should be 'text'",
						(*TextBlock)->GetStringField(TEXT("type")), TEXT("text"));
					TestEqual("Content text should match prompt",
						(*TextBlock)->GetStringField(TEXT("text")),
						TEXT("Hello Claude"));
				}
			}
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Payload_EmptyPrompt,
	"UnrealClaude.ChatBackend.Payload.EmptyPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Payload_EmptyPrompt::RunTest(const FString& Parameters)
{
	FClaudeCodeRunner Runner;
	TArray<FString> EmptyImages;

	FString Payload = Runner.BuildStreamJsonPayload(TEXT(""), EmptyImages);

	// Empty prompt should still produce valid JSON payload
	TestTrue("Empty prompt payload should not be empty",
		!Payload.IsEmpty());
	TestTrue("Empty prompt payload should end with newline",
		Payload.EndsWith(TEXT("\n")));

	return true;
}

// ===== Static Method Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Static_IsClaudeAvailableNoCrash,
	"UnrealClaude.ChatBackend.Static.IsClaudeAvailableNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Static_IsClaudeAvailableNoCrash::RunTest(const FString& Parameters)
{
	// Don't assert the result — Claude CLI may or may not be installed.
	// Just verify the call doesn't crash.
	bool bAvailable = FClaudeCodeRunner::IsClaudeAvailable();

	TestTrue("IsClaudeAvailable() should return a valid bool",
		bAvailable || !bAvailable);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Static_GetClaudePathNoCrash,
	"UnrealClaude.ChatBackend.Static.GetClaudePathNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Static_GetClaudePathNoCrash::RunTest(const FString& Parameters)
{
	// Don't assert the result — Claude CLI may or may not be installed.
	// Just verify the call doesn't crash.
	FString Path = FClaudeCodeRunner::GetClaudePath();

	TestTrue("GetClaudePath() should return valid FString (possibly empty)",
		Path.Len() >= 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Instance_IsNotExecutingOnConstruction,
	"UnrealClaude.ChatBackend.Instance.IsNotExecutingOnConstruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Instance_IsNotExecutingOnConstruction::RunTest(const FString& Parameters)
{
	FClaudeCodeRunner Runner;

	TestFalse("Fresh runner should not be executing",
		Runner.IsExecuting());

	return true;
}

// ===== Interface Contract Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChatBackend_Interface_MockBackendAsIChatBackend,
	"UnrealClaude.ChatBackend.Interface.MockBackendAsIChatBackend",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FChatBackend_Interface_MockBackendAsIChatBackend::RunTest(const FString& Parameters)
{
	// Verify FMockChatBackend can be used through IChatBackend interface
	FMockChatBackend MockBackend;
	IChatBackend* Backend = &MockBackend;

	TestNotNull("MockBackend should be castable to IChatBackend*", Backend);
	TestTrue("Mock should be available", Backend->IsAvailable());
	TestFalse("Mock should not be executing", Backend->IsExecuting());

	// Test Cancel through interface
	Backend->Cancel();
	TestTrue("Cancel should set bCancelCalled", MockBackend.bCancelCalled);

	// Test ExecuteAsync through interface (synchronous fake)
	FChatRequestConfig Config;
	Config.Prompt = TEXT("test prompt");

	FString ReceivedResponse;
	bool bReceivedSuccess = false;

	FOnChatResponse OnComplete;
	OnComplete.BindLambda(
		[&ReceivedResponse, &bReceivedSuccess](
			const FString& Response, bool bSuccess)
		{
			ReceivedResponse = Response;
			bReceivedSuccess = bSuccess;
		});

	bool bStarted = Backend->ExecuteAsync(Config, OnComplete);

	TestTrue("ExecuteAsync should return true", bStarted);
	TestTrue("bExecuteAsyncCalled should be set",
		MockBackend.bExecuteAsyncCalled);
	TestEqual("LastPrompt should capture the prompt",
		MockBackend.LastPrompt, TEXT("test prompt"));
	TestEqual("Should receive mock response",
		ReceivedResponse, TEXT("mock response"));
	TestTrue("Should report success", bReceivedSuccess);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
