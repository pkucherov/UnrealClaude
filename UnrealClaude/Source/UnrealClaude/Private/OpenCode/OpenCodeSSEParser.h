// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChatBackendTypes.h"

// ===== FOpenCodeSSEParser =====

/**
 * Stateful parser that converts raw SSE byte chunks into FChatStreamEvent objects.
 *
 * Design (D-02, D-03, COMM-02, COMM-03):
 * - Accumulates partial chunks in an internal FString buffer
 * - Splits on newline boundaries; flushes complete events on blank lines
 * - Maps OpenCode GlobalEvent JSON payload types to EChatStreamEventType
 * - Malformed JSON emits FChatStreamEvent with bIsError=true and RawJson preserved
 * - Pure data transformer — no FRunnable, no AsyncTask. Caller is responsible
 *   for thread dispatching. Tests call ProcessChunk synchronously.
 *
 * Usage:
 *   FOpenCodeSSEParser Parser;
 *   Parser.OnEvent.BindLambda(...);
 *   Parser.ProcessChunk(ReceivedBytes);
 */
class FOpenCodeSSEParser
{
public:
	/** Delegate fires once per complete SSE event. Called on the same thread as ProcessChunk. */
	FOnChatStreamEvent OnEvent;

	/**
	 * Feed raw bytes from HTTP stream delegate.
	 * Accumulates data; emits events via OnEvent when blank-line boundaries are found.
	 */
	void ProcessChunk(const FString& RawChunk);

	/** Reset buffer state — call on reconnect to discard partial data. */
	void Reset();

	/** Get last retry interval from SSE retry: field (0 if none received). */
	uint32 GetLastRetryMs() const { return LastRetryMs; }

private:
	/** Accumulated bytes not yet parsed into a complete line. */
	FString Buffer;

	/** SSE event: field value for the current event (may be empty). */
	FString CurrentEventName;

	/** Accumulated data: field content for the current event. */
	FString CurrentData;

	/** Last retry: field value received, in milliseconds. */
	uint32 LastRetryMs = 0;

	/** Called on blank line — parse accumulated CurrentData and fire OnEvent. */
	void FlushEvent();

	/**
	 * Parse a complete SSE data string into a FChatStreamEvent.
	 * @param EventName - value from the SSE event: field (may be empty)
	 * @param Data - value from the SSE data: field (JSON string)
	 * @return Populated FChatStreamEvent
	 */
	FChatStreamEvent ParseEventPayload(const FString& EventName, const FString& Data);

	/**
	 * Map an OpenCode event type string to EChatStreamEventType.
	 * @param TypeStr - the "type" field from the payload JSON
	 * @param Payload - the payload JSON object for field inspection
	 * @return Matching EChatStreamEventType
	 */
	EChatStreamEventType MapEventType(
		const FString& TypeStr,
		const TSharedPtr<FJsonObject>& Payload);

	/**
	 * Map a message.part.updated event to the appropriate FChatStreamEvent.
	 * Inspects the properties.part shape to distinguish TextPart/ToolPart/StepPart/ReasoningPart.
	 */
	FChatStreamEvent MapMessagePartUpdated(const TSharedPtr<FJsonObject>& Payload);

	/**
	 * Map a session.status event.
	 * Sets Type=StatusUpdate and Text to the status value.
	 */
	FChatStreamEvent MapSessionStatus(const TSharedPtr<FJsonObject>& Payload);

	/**
	 * Map a session.error event.
	 * Sets bIsError=true and classifies EOpenCodeErrorType from the error code field.
	 */
	FChatStreamEvent MapSessionError(const TSharedPtr<FJsonObject>& Payload);
};
