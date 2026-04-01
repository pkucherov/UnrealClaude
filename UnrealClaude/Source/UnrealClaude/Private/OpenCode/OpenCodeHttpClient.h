// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"

// ===== IHttpClient =====

/**
 * Interface for HTTP operations used by the OpenCode backend (D-22, D-27).
 * Abstracts FHttpModule calls to enable mock injection in tests.
 * Production: FOpenCodeHttpClient. Tests: FMockHttpClient (TestUtils.h).
 */
class IHttpClient
{
public:
	virtual ~IHttpClient() = default;

	/**
	 * Synchronous-style GET request.
	 * @param Url - Full URL to request
	 * @param OutBody - Receives response body on success
	 * @return HTTP status code, or -1 on connection failure
	 */
	virtual int32 Get(const FString& Url, FString& OutBody) = 0;

	/**
	 * Synchronous-style POST request.
	 * @param Url - Full URL to post to
	 * @param Body - Request body (JSON string)
	 * @param OutResponse - Receives response body
	 * @return HTTP status code, or -1 on connection failure
	 */
	virtual int32 Post(const FString& Url, const FString& Body, FString& OutResponse) = 0;

	/**
	 * Begin a long-lived SSE stream connection.
	 * StreamCallback fires per-chunk on the HTTP thread; return false to abort.
	 * OnDisconnected fires when the connection closes (normal or error).
	 * @param Url - SSE endpoint URL
	 * @param StreamCallback - Called with raw chunk data; return false to abort stream
	 * @param OnDisconnected - Called when stream ends
	 */
	virtual void BeginSSEStream(const FString& Url,
		TFunction<bool(void* Ptr, int64 Length)> StreamCallback,
		TFunction<void()> OnDisconnected) = 0;

	/**
	 * Abort the active SSE stream connection.
	 * Safe to call even if no stream is active.
	 */
	virtual void AbortSSEStream() = 0;
};

// ===== FOpenCodeHttpClient =====

/**
 * Production HTTP client using UE's FHttpModule (D-20, D-22).
 * Synchronous Get/Post use FEvent to block until response received.
 * SSE streaming uses SetResponseBodyReceiveStreamDelegate with SetTimeout(0.0f).
 */
class FOpenCodeHttpClient : public IHttpClient
{
public:
	FOpenCodeHttpClient() = default;
	virtual ~FOpenCodeHttpClient() = default;

	// IHttpClient interface
	virtual int32 Get(const FString& Url, FString& OutBody) override;
	virtual int32 Post(const FString& Url, const FString& Body, FString& OutResponse) override;
	virtual void BeginSSEStream(const FString& Url,
		TFunction<bool(void* Ptr, int64 Length)> StreamCallback,
		TFunction<void()> OnDisconnected) override;
	virtual void AbortSSEStream() override;

	/**
	 * Convenience: check OpenCode server health endpoint.
	 * @param BaseUrl - Server base URL (e.g. http://127.0.0.1:4096)
	 * @param OutVersion - Receives version string if healthy
	 * @return true if server responds with { "healthy": true }
	 */
	bool CheckHealth(const FString& BaseUrl, FString& OutVersion);

private:
	/** Active SSE request handle for cancellation */
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveSSERequest;
};
