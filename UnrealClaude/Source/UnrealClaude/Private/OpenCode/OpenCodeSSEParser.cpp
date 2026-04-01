// Copyright Natali Caggiano. All Rights Reserved.

#include "OpenCode/OpenCodeSSEParser.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// ===== Public API =====

void FOpenCodeSSEParser::ProcessChunk(const FString& RawChunk)
{
	Buffer += RawChunk;

	int32 LineEnd;
	while (Buffer.FindChar(TEXT('\n'), LineEnd))
	{
		FString Line = Buffer.Left(LineEnd);
		Buffer.RemoveAt(0, LineEnd + 1);

		if (Line.IsEmpty())
		{
			// Blank line = SSE event boundary
			FlushEvent();
		}
		else if (Line.StartsWith(TEXT("data:")))
		{
			FString Data = Line.Mid(5).TrimStart();
			if (!CurrentData.IsEmpty())
			{
				CurrentData += TEXT("\n");
			}
			CurrentData += Data;
		}
		else if (Line.StartsWith(TEXT("event:")))
		{
			CurrentEventName = Line.Mid(6).TrimStart();
		}
		else if (Line.StartsWith(TEXT("retry:")))
		{
			LastRetryMs = (uint32)FCString::Atoi(*Line.Mid(6).TrimStart());
		}
		// Lines starting with ':' are comments — silently ignored
	}
}

void FOpenCodeSSEParser::Reset()
{
	Buffer.Reset();
	CurrentData.Reset();
	CurrentEventName.Reset();
}

// ===== Private: FlushEvent =====

void FOpenCodeSSEParser::FlushEvent()
{
	if (CurrentData.IsEmpty())
	{
		CurrentEventName.Reset();
		return;
	}

	FChatStreamEvent Event = ParseEventPayload(CurrentEventName, CurrentData);

	CurrentData.Reset();
	CurrentEventName.Reset();

	OnEvent.ExecuteIfBound(Event);
}

// ===== Private: ParseEventPayload =====

FChatStreamEvent FOpenCodeSSEParser::ParseEventPayload(
	const FString& EventName,
	const FString& Data)
{
	FChatStreamEvent Event;
	Event.RawJson = Data;

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Data);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		// Malformed JSON — surface as error (D-03)
		Event.bIsError = true;
		Event.Type = EChatStreamEventType::Unknown;
		return Event;
	}

	// OpenCode wraps payload in { "directory": "...", "payload": { "type": ... } }
	// Fall back to root if no "payload" field (D-36 flat event fallback)
	TSharedPtr<FJsonObject> Payload = Root->GetObjectField(TEXT("payload"));
	if (!Payload.IsValid())
	{
		Payload = Root;
	}

	FString TypeStr;
	Payload->TryGetStringField(TEXT("type"), TypeStr);

	// Route to type-specific mapper
	if (TypeStr == TEXT("server.connected"))
	{
		Event.Type = EChatStreamEventType::SessionInit;
	}
	else if (TypeStr == TEXT("message.part.updated"))
	{
		Event = MapMessagePartUpdated(Payload);
		Event.RawJson = Data; // preserve raw regardless of mapping
	}
	else if (TypeStr == TEXT("message.updated"))
	{
		Event.Type = EChatStreamEventType::AssistantMessage;
	}
	else if (TypeStr == TEXT("permission.updated"))
	{
		Event.Type = EChatStreamEventType::PermissionRequest;
		// RawJson already set above; consumer can inspect permission details
	}
	else if (TypeStr == TEXT("session.status"))
	{
		Event = MapSessionStatus(Payload);
		Event.RawJson = Data;
	}
	else if (TypeStr == TEXT("session.error"))
	{
		Event = MapSessionError(Payload);
		Event.RawJson = Data;
	}
	else
	{
		// Unknown type — not an error, just unknown (TEST per spec)
		Event.Type = EChatStreamEventType::Unknown;
	}

	return Event;
}

// ===== Private: MapMessagePartUpdated =====

FChatStreamEvent FOpenCodeSSEParser::MapMessagePartUpdated(
	const TSharedPtr<FJsonObject>& Payload)
{
	FChatStreamEvent Event;
	Event.Type = EChatStreamEventType::Unknown;

	const TSharedPtr<FJsonObject>* PropsPtr = nullptr;
	if (!Payload->TryGetObjectField(TEXT("properties"), PropsPtr) || !PropsPtr)
	{
		return Event;
	}
	const TSharedPtr<FJsonObject>& Props = *PropsPtr;

	const TSharedPtr<FJsonObject>* PartPtr = nullptr;
	if (!Props->TryGetObjectField(TEXT("part"), PartPtr) || !PartPtr)
	{
		return Event;
	}
	const TSharedPtr<FJsonObject>& Part = *PartPtr;

	// Check for step/reasoning part type first
	FString PartType;
	Part->TryGetStringField(TEXT("type"), PartType);

	if (PartType == TEXT("step-start") || PartType == TEXT("step-finish"))
	{
		Event.Type = EChatStreamEventType::AgentThinking;
		return Event;
	}

	if (PartType == TEXT("reasoning"))
	{
		Event.Type = EChatStreamEventType::AgentThinking;
		Part->TryGetStringField(TEXT("reasoning"), Event.Text);
		return Event;
	}

	// TextPart: has "content" and/or "delta" fields
	FString Delta;
	FString Content;
	bool bHasContent = Part->TryGetStringField(TEXT("content"), Content);
	bool bHasDelta = Part->TryGetStringField(TEXT("delta"), Delta);

	if (bHasContent || bHasDelta)
	{
		Event.Type = EChatStreamEventType::TextContent;
		// Prefer delta for streaming; fall back to content
		Event.Text = bHasDelta ? Delta : Content;
		return Event;
	}

	// ToolPart: has "callID" and "tool" fields
	FString CallId;
	FString ToolName;
	bool bHasCallId = Part->TryGetStringField(TEXT("callID"), CallId);
	bool bHasTool = Part->TryGetStringField(TEXT("tool"), ToolName);

	if (bHasCallId || bHasTool)
	{
		FString State;
		Part->TryGetStringField(TEXT("state"), State);

		if (State == TEXT("completed"))
		{
			Event.Type = EChatStreamEventType::ToolResult;
			Event.ToolCallId = CallId;
			Event.ToolName = ToolName;
			Part->TryGetStringField(TEXT("output"), Event.ToolResultContent);
		}
		else
		{
			Event.Type = EChatStreamEventType::ToolUse;
			Event.ToolCallId = CallId;
			Event.ToolName = ToolName;
		}
		return Event;
	}

	return Event;
}

// ===== Private: MapSessionStatus =====

FChatStreamEvent FOpenCodeSSEParser::MapSessionStatus(
	const TSharedPtr<FJsonObject>& Payload)
{
	FChatStreamEvent Event;
	Event.Type = EChatStreamEventType::StatusUpdate;
	Payload->TryGetStringField(TEXT("status"), Event.Text);
	return Event;
}

// ===== Private: MapSessionError =====

FChatStreamEvent FOpenCodeSSEParser::MapSessionError(
	const TSharedPtr<FJsonObject>& Payload)
{
	FChatStreamEvent Event;
	Event.bIsError = true;
	Event.Type = EChatStreamEventType::Unknown;

	int32 Code = 0;
	Payload->TryGetNumberField(TEXT("code"), Code);

	if (Code == 401 || Code == 403)
	{
		Event.ErrorType = EOpenCodeErrorType::AuthFailure;
	}
	else if (Code == 429)
	{
		Event.ErrorType = EOpenCodeErrorType::RateLimit;
	}
	else if (Code == 400)
	{
		Event.ErrorType = EOpenCodeErrorType::BadRequest;
	}
	else if (Code >= 500 && Code < 600)
	{
		Event.ErrorType = EOpenCodeErrorType::InternalError;
	}
	else if (Code == 0)
	{
		// No code field — network-level failure
		Event.ErrorType = EOpenCodeErrorType::ServerUnreachable;
	}
	else
	{
		Event.ErrorType = EOpenCodeErrorType::Unknown;
	}

	return Event;
}
