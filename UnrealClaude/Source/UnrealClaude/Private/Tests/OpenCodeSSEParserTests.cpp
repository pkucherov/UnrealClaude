// Copyright Natali Caggiano. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "OpenCode/OpenCodeSSEParser.h"
#include "ChatBackendTypes.h"

// ===== Helper: Capture events into a TArray =====

static TArray<FChatStreamEvent> CaptureEvents(
	FOpenCodeSSEParser& Parser,
	const TArray<FString>& Chunks)
{
	TArray<FChatStreamEvent> Captured;
	Parser.OnEvent.BindLambda([&Captured](const FChatStreamEvent& Evt)
	{
		Captured.Add(Evt);
	});
	for (const FString& Chunk : Chunks)
	{
		Parser.ProcessChunk(Chunk);
	}
	return Captured;
}

static TArray<FChatStreamEvent> CaptureEvents(
	FOpenCodeSSEParser& Parser,
	const FString& SingleChunk)
{
	return CaptureEvents(Parser, TArray<FString>{ SingleChunk });
}

// ===== SSE Wire-format helpers =====

/** Build a complete SSE event string (blank-line terminated) */
static FString MakeSSEEvent(const FString& DataJson)
{
	return FString::Printf(TEXT("data: %s\n\n"), *DataJson);
}

static FString MakeSSEEventWithEventField(const FString& EventName, const FString& DataJson)
{
	return FString::Printf(TEXT("event: %s\ndata: %s\n\n"), *EventName, *DataJson);
}

// ===== GlobalEvent wrapper helpers =====

/** Wrap a payload JSON string in OpenCode's GlobalEvent shape */
static FString WrapGlobal(const FString& PayloadJson)
{
	return FString::Printf(
		TEXT("{\"directory\":\"/project\",\"payload\":%s}"),
		*PayloadJson);
}

/** Payload JSON string for a given type */
static FString SimplePayload(const FString& TypeStr)
{
	return FString::Printf(TEXT("{\"type\":\"%s\"}"), *TypeStr);
}

// ===== Tests =====

// ---- Single well-formed server.connected → SessionInit ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_ServerConnected,
	"UnrealClaude.OpenCode.SSEParser.ServerConnected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_ServerConnected::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Chunk = MakeSSEEvent(WrapGlobal(SimplePayload(TEXT("server.connected"))));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("ServerConnected: one event emitted", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("ServerConnected: type is SessionInit",
			Events[0].Type, EChatStreamEventType::SessionInit);
		TestFalse("ServerConnected: not an error", Events[0].bIsError);
	}
	return true;
}

// ---- Flat payload (no wrapper) falls back to root object ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_FlatPayloadFallback,
	"UnrealClaude.OpenCode.SSEParser.FlatPayloadFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_FlatPayloadFallback::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Chunk = MakeSSEEvent(SimplePayload(TEXT("server.connected")));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("FlatFallback: one event emitted", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("FlatFallback: type is SessionInit",
			Events[0].Type, EChatStreamEventType::SessionInit);
	}
	return true;
}

// ---- permission.updated → PermissionRequest ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_PermissionRequest,
	"UnrealClaude.OpenCode.SSEParser.PermissionRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_PermissionRequest::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Payload = TEXT("{\"type\":\"permission.updated\",\"permission\":{\"tool\":\"bash\"}}");
	FString Chunk = MakeSSEEvent(WrapGlobal(Payload));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("Permission: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("Permission: type is PermissionRequest",
			Events[0].Type, EChatStreamEventType::PermissionRequest);
		TestFalse("Permission: RawJson preserved (non-empty)", Events[0].RawJson.IsEmpty());
	}
	return true;
}

// ---- session.status → StatusUpdate ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_SessionStatus,
	"UnrealClaude.OpenCode.SSEParser.SessionStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_SessionStatus::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Payload = TEXT("{\"type\":\"session.status\",\"status\":\"idle\"}");
	FString Chunk = MakeSSEEvent(WrapGlobal(Payload));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("SessionStatus: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("SessionStatus: type is StatusUpdate",
			Events[0].Type, EChatStreamEventType::StatusUpdate);
		TestEqual("SessionStatus: Text contains status",
			Events[0].Text, FString(TEXT("idle")));
	}
	return true;
}

// ---- session.error → bIsError=true ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_SessionError,
	"UnrealClaude.OpenCode.SSEParser.SessionError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_SessionError::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Payload = TEXT("{\"type\":\"session.error\",\"code\":401,\"message\":\"Unauthorized\"}");
	FString Chunk = MakeSSEEvent(WrapGlobal(Payload));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("SessionError: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestTrue("SessionError: bIsError is true", Events[0].bIsError);
		TestEqual("SessionError: ErrorType is AuthFailure",
			Events[0].ErrorType, EOpenCodeErrorType::AuthFailure);
	}
	return true;
}

// ---- message.updated → AssistantMessage ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_MessageUpdated,
	"UnrealClaude.OpenCode.SSEParser.MessageUpdated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_MessageUpdated::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Payload = TEXT("{\"type\":\"message.updated\",\"message\":{\"id\":\"msg_01\"}}");
	FString Chunk = MakeSSEEvent(WrapGlobal(Payload));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("MessageUpdated: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("MessageUpdated: type is AssistantMessage",
			Events[0].Type, EChatStreamEventType::AssistantMessage);
	}
	return true;
}

// ---- message.part.updated (TextPart) → TextContent ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_TextPart,
	"UnrealClaude.OpenCode.SSEParser.TextPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_TextPart::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Payload = TEXT(
		"{\"type\":\"message.part.updated\","
		"\"properties\":{\"part\":{\"content\":\"Hello\",\"delta\":\"Hello\"}}}"
	);
	FString Chunk = MakeSSEEvent(WrapGlobal(Payload));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("TextPart: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("TextPart: type is TextContent",
			Events[0].Type, EChatStreamEventType::TextContent);
		TestEqual("TextPart: Text contains delta",
			Events[0].Text, FString(TEXT("Hello")));
	}
	return true;
}

// ---- message.part.updated (ToolPart, active) → ToolUse ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_ToolPart_Active,
	"UnrealClaude.OpenCode.SSEParser.ToolPart_Active",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_ToolPart_Active::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Payload = TEXT(
		"{\"type\":\"message.part.updated\","
		"\"properties\":{\"part\":{\"callID\":\"call_01\",\"tool\":\"bash\",\"state\":\"running\"}}}"
	);
	FString Chunk = MakeSSEEvent(WrapGlobal(Payload));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("ToolUse: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("ToolUse: type is ToolUse",
			Events[0].Type, EChatStreamEventType::ToolUse);
		TestEqual("ToolUse: ToolName is bash",
			Events[0].ToolName, FString(TEXT("bash")));
		TestEqual("ToolUse: ToolCallId populated",
			Events[0].ToolCallId, FString(TEXT("call_01")));
	}
	return true;
}

// ---- message.part.updated (ToolPart, state=completed) → ToolResult ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_ToolPart_Completed,
	"UnrealClaude.OpenCode.SSEParser.ToolPart_Completed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_ToolPart_Completed::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Payload = TEXT(
		"{\"type\":\"message.part.updated\","
		"\"properties\":{\"part\":{\"callID\":\"call_02\",\"tool\":\"read_file\","
		"\"state\":\"completed\",\"output\":\"file contents here\"}}}"
	);
	FString Chunk = MakeSSEEvent(WrapGlobal(Payload));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("ToolResult: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("ToolResult: type is ToolResult",
			Events[0].Type, EChatStreamEventType::ToolResult);
		TestEqual("ToolResult: content populated",
			Events[0].ToolResultContent, FString(TEXT("file contents here")));
	}
	return true;
}

// ---- message.part.updated (StepStartPart) → AgentThinking ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_StepStartPart,
	"UnrealClaude.OpenCode.SSEParser.StepStartPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_StepStartPart::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Payload = TEXT(
		"{\"type\":\"message.part.updated\","
		"\"properties\":{\"part\":{\"type\":\"step-start\"}}}"
	);
	FString Chunk = MakeSSEEvent(WrapGlobal(Payload));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("StepStart: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("StepStart: type is AgentThinking",
			Events[0].Type, EChatStreamEventType::AgentThinking);
	}
	return true;
}

// ---- message.part.updated (ReasoningPart) → AgentThinking with text ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_ReasoningPart,
	"UnrealClaude.OpenCode.SSEParser.ReasoningPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_ReasoningPart::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Payload = TEXT(
		"{\"type\":\"message.part.updated\","
		"\"properties\":{\"part\":{\"type\":\"reasoning\",\"reasoning\":\"thinking...\"}}}"
	);
	FString Chunk = MakeSSEEvent(WrapGlobal(Payload));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("Reasoning: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("Reasoning: type is AgentThinking",
			Events[0].Type, EChatStreamEventType::AgentThinking);
		TestEqual("Reasoning: Text contains reasoning",
			Events[0].Text, FString(TEXT("thinking...")));
	}
	return true;
}

// ---- Unknown type → Type=Unknown, RawJson preserved ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_UnknownType,
	"UnrealClaude.OpenCode.SSEParser.UnknownType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_UnknownType::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Payload = TEXT("{\"type\":\"future.event.v99\",\"data\":\"x\"}");
	FString Chunk = MakeSSEEvent(WrapGlobal(Payload));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("Unknown: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("Unknown: type is Unknown",
			Events[0].Type, EChatStreamEventType::Unknown);
		TestFalse("Unknown: not flagged as error", Events[0].bIsError);
		TestFalse("Unknown: RawJson non-empty", Events[0].RawJson.IsEmpty());
	}
	return true;
}

// ---- Malformed JSON → bIsError=true, RawJson preserved ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_MalformedJson,
	"UnrealClaude.OpenCode.SSEParser.MalformedJson",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_MalformedJson::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Chunk = MakeSSEEvent(TEXT("{not valid json}"));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("Malformed: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestTrue("Malformed: bIsError is true", Events[0].bIsError);
		TestFalse("Malformed: RawJson preserved (non-empty)", Events[0].RawJson.IsEmpty());
	}
	return true;
}

// ---- Valid JSON, missing type field → Type=Unknown, not error ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_MissingTypeField,
	"UnrealClaude.OpenCode.SSEParser.MissingTypeField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_MissingTypeField::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Chunk = MakeSSEEvent(TEXT("{\"someField\":\"someValue\"}"));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("MissingType: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("MissingType: type is Unknown",
			Events[0].Type, EChatStreamEventType::Unknown);
		TestFalse("MissingType: not an error", Events[0].bIsError);
	}
	return true;
}

// ---- Comment line (:) silently ignored ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_CommentIgnored,
	"UnrealClaude.OpenCode.SSEParser.CommentIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_CommentIgnored::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	// A comment-only "event" (blank line after comment) should emit nothing
	FString Chunk = TEXT(": this is a comment\n\n");
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("Comment: no events emitted", Events.Num(), 0);
	return true;
}

// ---- retry: field stored, accessible via GetLastRetryMs ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_RetryField,
	"UnrealClaude.OpenCode.SSEParser.RetryField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_RetryField::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Chunk = TEXT("retry: 5000\n\n");
	Parser.ProcessChunk(Chunk);

	TestEqual("Retry: GetLastRetryMs returns 5000",
		(int32)Parser.GetLastRetryMs(), 5000);
	return true;
}

// ---- Multi-event batch in a single chunk ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_MultiEventBatch,
	"UnrealClaude.OpenCode.SSEParser.MultiEventBatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_MultiEventBatch::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString E1 = MakeSSEEvent(WrapGlobal(SimplePayload(TEXT("server.connected"))));
	FString E2 = MakeSSEEvent(WrapGlobal(SimplePayload(TEXT("permission.updated"))));
	FString E3 = MakeSSEEvent(WrapGlobal(SimplePayload(TEXT("message.updated"))));
	auto Events = CaptureEvents(Parser, E1 + E2 + E3);

	TestEqual("Batch: three events emitted", Events.Num(), 3);
	if (Events.Num() == 3)
	{
		TestEqual("Batch[0]: SessionInit",
			Events[0].Type, EChatStreamEventType::SessionInit);
		TestEqual("Batch[1]: PermissionRequest",
			Events[1].Type, EChatStreamEventType::PermissionRequest);
		TestEqual("Batch[2]: AssistantMessage",
			Events[2].Type, EChatStreamEventType::AssistantMessage);
	}
	return true;
}

// ---- Event split across two chunks ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_SplitAcrossChunks,
	"UnrealClaude.OpenCode.SSEParser.SplitAcrossChunks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_SplitAcrossChunks::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Full = MakeSSEEvent(WrapGlobal(SimplePayload(TEXT("server.connected"))));
	// Split at an arbitrary character mid-way
	int32 SplitAt = Full.Len() / 2;
	FString Part1 = Full.Left(SplitAt);
	FString Part2 = Full.Mid(SplitAt);

	auto Events = CaptureEvents(Parser, TArray<FString>{ Part1, Part2 });

	TestEqual("Split: one event emitted after two chunks", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("Split: type is SessionInit",
			Events[0].Type, EChatStreamEventType::SessionInit);
	}
	return true;
}

// ---- Partial line across chunks ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_PartialLineAcrossChunks,
	"UnrealClaude.OpenCode.SSEParser.PartialLineAcrossChunks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_PartialLineAcrossChunks::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	// Send exactly the data: line without the trailing newline, then complete it
	FString JsonPayload = WrapGlobal(SimplePayload(TEXT("server.connected")));
	FString DataLine = FString::Printf(TEXT("data: %s"), *JsonPayload);
	FString Part1 = DataLine.Left(10);
	FString Part2 = DataLine.Mid(10) + TEXT("\n\n");

	auto Events = CaptureEvents(Parser, TArray<FString>{ Part1, Part2 });

	TestEqual("PartialLine: one event after reassembly", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestEqual("PartialLine: type is SessionInit",
			Events[0].Type, EChatStreamEventType::SessionInit);
	}
	return true;
}

// ---- Reset() discards partial buffer ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_Reset,
	"UnrealClaude.OpenCode.SSEParser.Reset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_Reset::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	TArray<FChatStreamEvent> Captured;
	Parser.OnEvent.BindLambda([&Captured](const FChatStreamEvent& Evt)
	{
		Captured.Add(Evt);
	});

	// Feed partial data that hasn't completed yet
	Parser.ProcessChunk(TEXT("data: {\"type\":\"server.connect"));

	// Reset — discards the partial buffer
	Parser.Reset();

	// Now send a complete new event
	Parser.ProcessChunk(MakeSSEEvent(WrapGlobal(SimplePayload(TEXT("session.status")))));

	// Should only have the post-reset event, not one for the partial old data
	TestEqual("Reset: one event (post-reset only)", Captured.Num(), 1);
	if (Captured.Num() > 0)
	{
		TestEqual("Reset: type is StatusUpdate",
			Captured[0].Type, EChatStreamEventType::StatusUpdate);
	}
	return true;
}

// ---- event: field ignored when JSON type field is present ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_EventFieldIgnored,
	"UnrealClaude.OpenCode.SSEParser.EventFieldIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_EventFieldIgnored::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	// SSE event: field says "garbage" but JSON type says "server.connected"
	FString Chunk = MakeSSEEventWithEventField(
		TEXT("garbage"),
		WrapGlobal(SimplePayload(TEXT("server.connected"))));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("EventField: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		// JSON type wins — should be SessionInit, not Unknown
		TestEqual("EventField: JSON type wins, SessionInit",
			Events[0].Type, EChatStreamEventType::SessionInit);
	}
	return true;
}

// ---- session.error with 500 → InternalError ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_SessionError_500,
	"UnrealClaude.OpenCode.SSEParser.SessionError_500",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_SessionError_500::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Payload = TEXT("{\"type\":\"session.error\",\"code\":500,\"message\":\"Internal\"}");
	FString Chunk = MakeSSEEvent(WrapGlobal(Payload));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("Error500: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestTrue("Error500: bIsError", Events[0].bIsError);
		TestEqual("Error500: ErrorType is InternalError",
			Events[0].ErrorType, EOpenCodeErrorType::InternalError);
	}
	return true;
}

// ---- session.error with 429 → RateLimit ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_SessionError_429,
	"UnrealClaude.OpenCode.SSEParser.SessionError_429",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_SessionError_429::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	FString Payload = TEXT("{\"type\":\"session.error\",\"code\":429,\"message\":\"Rate limited\"}");
	FString Chunk = MakeSSEEvent(WrapGlobal(Payload));
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("Error429: one event", Events.Num(), 1);
	if (Events.Num() > 0)
	{
		TestTrue("Error429: bIsError", Events[0].bIsError);
		TestEqual("Error429: ErrorType is RateLimit",
			Events[0].ErrorType, EOpenCodeErrorType::RateLimit);
	}
	return true;
}

// ---- Empty data line with blank line → no event ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeSSEParser_EmptyDataLine,
	"UnrealClaude.OpenCode.SSEParser.EmptyDataLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeSSEParser_EmptyDataLine::RunTest(const FString& Parameters)
{
	FOpenCodeSSEParser Parser;
	// A blank line with no preceding data should emit nothing
	FString Chunk = TEXT("\n");
	auto Events = CaptureEvents(Parser, Chunk);

	TestEqual("EmptyData: no events emitted", Events.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
