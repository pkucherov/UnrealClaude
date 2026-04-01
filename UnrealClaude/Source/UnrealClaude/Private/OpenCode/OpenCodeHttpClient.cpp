// Copyright Natali Caggiano. All Rights Reserved.

#include "OpenCode/OpenCodeHttpClient.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "HAL/Event.h"
#include "Async/Async.h"
#include "UnrealClaudeModule.h"

// ===== Helper: synchronous HTTP request via FEvent =====

namespace
{
	/**
	 * Execute a pre-configured request synchronously by blocking on an FEvent.
	 * Must NOT be called from the game thread (would deadlock HTTP callbacks).
	 * @param Request - Configured request (verb, URL, headers, body already set)
	 * @param OutBody - Receives response body
	 * @return HTTP status code, or -1 on failure / timeout
	 */
	int32 ExecuteSync(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request, FString& OutBody)
	{
		if (!FHttpModule::IsAvailable())
		{
			return -1;
		}

		int32 StatusCode = -1;
		FEvent* DoneEvent = FPlatformProcess::GetSynchEventFromPool(false);

		Request->OnProcessRequestComplete().BindLambda(
			[DoneEvent, &StatusCode, &OutBody](
				FHttpRequestPtr /*Req*/,
				FHttpResponsePtr Response,
				bool bConnected)
			{
				if (bConnected && Response.IsValid())
				{
					StatusCode = Response->GetResponseCode();
					OutBody = Response->GetContentAsString();
				}
				DoneEvent->Trigger();
			});

		Request->ProcessRequest();

		// 30-second timeout for synchronous HTTP calls
		constexpr float TimeoutSeconds = 30.0f;
		DoneEvent->Wait(FTimespan::FromSeconds(TimeoutSeconds));
		FPlatformProcess::ReturnSynchEventToPool(DoneEvent);

		return StatusCode;
	}
} // anonymous namespace

// ===== IHttpClient: Get =====

int32 FOpenCodeHttpClient::Get(const FString& Url, FString& OutBody)
{
	if (!FHttpModule::IsAvailable())
	{
		return -1;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));

	int32 Status = ExecuteSync(Request, OutBody);
	UE_LOG(LogUnrealClaude, Verbose, TEXT("OpenCodeHTTP GET %s → %d"), *Url, Status);
	return Status;
}

// ===== IHttpClient: Post =====

int32 FOpenCodeHttpClient::Post(
	const FString& Url,
	const FString& Body,
	FString& OutResponse)
{
	if (!FHttpModule::IsAvailable())
	{
		return -1;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body);

	int32 Status = ExecuteSync(Request, OutResponse);
	UE_LOG(LogUnrealClaude, Verbose, TEXT("OpenCodeHTTP POST %s → %d"), *Url, Status);
	return Status;
}

// ===== IHttpClient: BeginSSEStream =====

void FOpenCodeHttpClient::BeginSSEStream(
	const FString& Url,
	TFunction<bool(void* Ptr, int64 Length)> StreamCallback,
	TFunction<void()> OnDisconnected)
{
	if (!FHttpModule::IsAvailable())
	{
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Accept"), TEXT("text/event-stream"));
	Request->SetHeader(TEXT("Cache-Control"), TEXT("no-cache"));

	// CRITICAL: SetTimeout(0.0f) prevents UE from closing the long-lived SSE connection
	// (see RESEARCH.md Pitfall 3)
	Request->SetTimeout(0.0f);

	// Stream delegate fires per-chunk on the HTTP thread
	Request->SetResponseBodyReceiveStreamDelegate(
		FHttpRequestStreamDelegate::CreateLambda(
			[StreamCallback](void* Ptr, int64 Length) -> bool
			{
				if (StreamCallback)
				{
					return StreamCallback(Ptr, Length);
				}
				return true; // true = continue stream
			}));

	// Fires when the SSE connection closes (normal disconnect or error)
	Request->OnProcessRequestComplete().BindLambda(
		[OnDisconnected](
			FHttpRequestPtr /*Req*/,
			FHttpResponsePtr /*Response*/,
			bool /*bConnected*/)
		{
			// Dispatch disconnect notification to game thread
			if (OnDisconnected)
			{
				AsyncTask(ENamedThreads::GameThread,
					[OnDisconnected]() { OnDisconnected(); });
			}
		});

	ActiveSSERequest = Request;
	Request->ProcessRequest();

	UE_LOG(LogUnrealClaude, Verbose, TEXT("OpenCodeHTTP SSE started: %s"), *Url);
}

// ===== IHttpClient: AbortSSEStream =====

void FOpenCodeHttpClient::AbortSSEStream()
{
	if (ActiveSSERequest.IsValid())
	{
		ActiveSSERequest->CancelRequest();
		ActiveSSERequest.Reset();
		UE_LOG(LogUnrealClaude, Verbose, TEXT("OpenCodeHTTP SSE aborted"));
	}
}

// ===== CheckHealth convenience =====

bool FOpenCodeHttpClient::CheckHealth(const FString& BaseUrl, FString& OutVersion)
{
	FString Body;
	FString HealthUrl = BaseUrl / TEXT("global/health");
	int32 Code = Get(HealthUrl, Body);
	if (Code != 200)
	{
		return false;
	}

	TSharedPtr<FJsonObject> Json;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
	{
		return false;
	}

	bool bHealthy = false;
	Json->TryGetBoolField(TEXT("healthy"), bHealthy);
	Json->TryGetStringField(TEXT("version"), OutVersion);
	return bHealthy;
}
