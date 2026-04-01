// Copyright Natali Caggiano. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/TestUtils.h"

// ===== FMockHttpClient interface contract tests =====
// These tests verify the IHttpClient interface contract using FMockHttpClient.
// Production FOpenCodeHttpClient requires real HTTP and is tested during integration.

// ---- Get returns configured response ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeHttpClient_Get_ReturnsConfiguredResponse,
	"UnrealClaude.OpenCode.HttpClient.Get_ReturnsConfiguredResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeHttpClient_Get_ReturnsConfiguredResponse::RunTest(const FString& Parameters)
{
	FMockHttpClient Client;
	Client.MockStatusCode = 200;
	Client.MockResponseBody = TEXT("{\"healthy\":true,\"version\":\"1.2.3\"}");

	FString OutBody;
	int32 Status = Client.Get(TEXT("http://127.0.0.1:4096/global/health"), OutBody);

	TestEqual("Get: returns configured status code", Status, 200);
	TestEqual("Get: returns configured body", OutBody,
		FString(TEXT("{\"healthy\":true,\"version\":\"1.2.3\"}")));
	return true;
}

// ---- Get tracks call count and URL ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeHttpClient_Get_TracksCallCount,
	"UnrealClaude.OpenCode.HttpClient.Get_TracksCallCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeHttpClient_Get_TracksCallCount::RunTest(const FString& Parameters)
{
	FMockHttpClient Client;
	FString OutBody;

	Client.Get(TEXT("http://127.0.0.1:4096/global/health"), OutBody);
	Client.Get(TEXT("http://127.0.0.1:4096/session"), OutBody);

	TestEqual("Get: call count is 2", Client.GetCallCount, 2);
	TestEqual("Get: LastGetUrl is second URL",
		Client.LastGetUrl, FString(TEXT("http://127.0.0.1:4096/session")));
	return true;
}

// ---- Post sends body and returns response ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeHttpClient_Post_SendsBodyAndReturnsResponse,
	"UnrealClaude.OpenCode.HttpClient.Post_SendsBodyAndReturnsResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeHttpClient_Post_SendsBodyAndReturnsResponse::RunTest(const FString& Parameters)
{
	FMockHttpClient Client;
	Client.MockStatusCode = 201;
	Client.MockResponseBody = TEXT("{\"id\":\"session_abc\"}");

	FString OutResponse;
	int32 Status = Client.Post(
		TEXT("http://127.0.0.1:4096/session"),
		TEXT("{\"projectRoot\":\"/project\"}"),
		OutResponse);

	TestEqual("Post: returns configured status", Status, 201);
	TestEqual("Post: returns configured response", OutResponse,
		FString(TEXT("{\"id\":\"session_abc\"}")));
	TestEqual("Post: captures request body", Client.LastPostBody,
		FString(TEXT("{\"projectRoot\":\"/project\"}")));
	TestEqual("Post: captures URL", Client.LastPostUrl,
		FString(TEXT("http://127.0.0.1:4096/session")));
	return true;
}

// ---- Post tracks call count ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeHttpClient_Post_TracksCallCount,
	"UnrealClaude.OpenCode.HttpClient.Post_TracksCallCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeHttpClient_Post_TracksCallCount::RunTest(const FString& Parameters)
{
	FMockHttpClient Client;
	FString OutResponse;

	Client.Post(TEXT("http://127.0.0.1:4096/session"), TEXT("{}"), OutResponse);
	Client.Post(TEXT("http://127.0.0.1:4096/session/123/message"), TEXT("{}"), OutResponse);

	TestEqual("Post: call count is 2", Client.PostCallCount, 2);
	return true;
}

// ---- BeginSSEStream activates stream and stores callbacks ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeHttpClient_SSEStream_StoresCallbacks,
	"UnrealClaude.OpenCode.HttpClient.SSEStream_StoresCallbacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeHttpClient_SSEStream_StoresCallbacks::RunTest(const FString& Parameters)
{
	FMockHttpClient Client;

	Client.BeginSSEStream(
		TEXT("http://127.0.0.1:4096/event"),
		[](void*, int64) -> bool { return true; },
		[]() {});

	TestTrue("SSEStream: bSSEStreamActive is true", Client.bSSEStreamActive);
	TestEqual("SSEStream: URL captured", Client.LastGetUrl,
		FString(TEXT("http://127.0.0.1:4096/event")));
	TestTrue("SSEStream: stream callback stored",
		Client.LastStreamCallback != nullptr);
	return true;
}

// ---- SimulateSSEData invokes stream callback ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeHttpClient_SimulateSSEData_InvokesCallback,
	"UnrealClaude.OpenCode.HttpClient.SimulateSSEData_InvokesCallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeHttpClient_SimulateSSEData_InvokesCallback::RunTest(const FString& Parameters)
{
	FMockHttpClient Client;

	FString ReceivedData;
	bool bCallbackFired = false;

	Client.BeginSSEStream(
		TEXT("http://127.0.0.1:4096/event"),
		[&ReceivedData, &bCallbackFired](void* Ptr, int64 Length) -> bool
		{
			bCallbackFired = true;
			// Convert UTF-8 bytes back to FString for verification
			ReceivedData = FString(Length, (const UTF8CHAR*)Ptr);
			return true;
		},
		[]() {});

	Client.SimulateSSEData(TEXT("data: {\"type\":\"server.connected\"}\n\n"));

	TestTrue("SimulateSSE: callback was fired", bCallbackFired);
	TestFalse("SimulateSSE: received data non-empty", ReceivedData.IsEmpty());
	return true;
}

// ---- SimulateSSEDisconnect fires disconnect callback ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeHttpClient_SimulateSSEDisconnect_FiresCallback,
	"UnrealClaude.OpenCode.HttpClient.SimulateSSEDisconnect_FiresCallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeHttpClient_SimulateSSEDisconnect_FiresCallback::RunTest(const FString& Parameters)
{
	FMockHttpClient Client;
	bool bDisconnectFired = false;

	Client.BeginSSEStream(
		TEXT("http://127.0.0.1:4096/event"),
		[](void*, int64) -> bool { return true; },
		[&bDisconnectFired]() { bDisconnectFired = true; });

	Client.SimulateSSEDisconnect();

	TestTrue("SimulateDisconnect: callback fired", bDisconnectFired);
	TestFalse("SimulateDisconnect: stream no longer active", Client.bSSEStreamActive);
	return true;
}

// ---- AbortSSEStream clears active flag ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeHttpClient_AbortSSEStream_ClearsActive,
	"UnrealClaude.OpenCode.HttpClient.AbortSSEStream_ClearsActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeHttpClient_AbortSSEStream_ClearsActive::RunTest(const FString& Parameters)
{
	FMockHttpClient Client;

	Client.BeginSSEStream(
		TEXT("http://127.0.0.1:4096/event"),
		[](void*, int64) -> bool { return true; },
		[]() {});

	TestTrue("BeforeAbort: stream active", Client.bSSEStreamActive);

	Client.AbortSSEStream();

	TestFalse("AfterAbort: stream no longer active", Client.bSSEStreamActive);
	return true;
}

// ---- Error status code is returned correctly ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeHttpClient_ErrorStatus_ReturnsCode,
	"UnrealClaude.OpenCode.HttpClient.ErrorStatus_ReturnsCode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeHttpClient_ErrorStatus_ReturnsCode::RunTest(const FString& Parameters)
{
	FMockHttpClient Client;
	Client.MockStatusCode = 500;
	Client.MockResponseBody = TEXT("{\"error\":\"Internal Server Error\"}");

	FString OutBody;
	int32 Status = Client.Get(TEXT("http://127.0.0.1:4096/global/health"), OutBody);

	TestEqual("ErrorStatus: 500 returned correctly", Status, 500);
	return true;
}

// ---- AbortSSEStream safe when no stream active ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOpenCodeHttpClient_AbortSSEStream_SafeWhenInactive,
	"UnrealClaude.OpenCode.HttpClient.AbortSSEStream_SafeWhenInactive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FOpenCodeHttpClient_AbortSSEStream_SafeWhenInactive::RunTest(const FString& Parameters)
{
	FMockHttpClient Client;

	// Calling Abort with no active stream should not crash
	Client.AbortSSEStream();

	TestFalse("AbortInactive: stream not active", Client.bSSEStreamActive);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
