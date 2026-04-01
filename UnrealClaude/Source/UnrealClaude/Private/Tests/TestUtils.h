// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Shared test utilities for UnrealClaude automation tests.
 * Provides JSON factories, temp file helpers, and FMockChatBackend
 * for use across all UnrealClaude test files.
 */

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "IChatBackend.h"
#include "ChatBackendTypes.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== JSON Helpers =====

/** Create a JSON object with a single string field */
static TSharedRef<FJsonObject> MakeJsonWithString(const FString& Key, const FString& Value)
{
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(Key, Value);
	return Json;
}

/** Create an empty JSON object */
static TSharedRef<FJsonObject> MakeEmptyJson()
{
	return MakeShared<FJsonObject>();
}

/** Create a JSON object with a single integer field */
static TSharedRef<FJsonObject> MakeJsonWithInt(const FString& Key, int32 Value)
{
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetNumberField(Key, static_cast<double>(Value));
	return Json;
}

/** Create a JSON object with a single boolean field */
static TSharedRef<FJsonObject> MakeJsonWithBool(const FString& Key, bool Value)
{
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetBoolField(Key, Value);
	return Json;
}

// ===== Temp File Helpers =====

/** Get the test temporary directory path */
static FString GetTestTempDir()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UnrealClaude"), TEXT("Tests"));
}

/** Ensure the test temporary directory exists */
static void EnsureTestTempDir()
{
	IFileManager::Get().MakeDirectory(*GetTestTempDir(), true);
}

/** Create a temporary text file and return its full path */
static FString CreateTempTextFile(const FString& FileName, const FString& Content)
{
	EnsureTestTempDir();
	FString FilePath = FPaths::Combine(GetTestTempDir(), FileName);
	FFileHelper::SaveStringToFile(Content, *FilePath);
	return FilePath;
}

/** Delete a temporary file */
static void DeleteTempFile(const FString& Path)
{
	IFileManager::Get().Delete(*Path);
}

/** Delete the entire test temporary directory */
static void CleanupTestTempDir()
{
	IFileManager::Get().DeleteDirectory(*GetTestTempDir(), false, true);
}

// ===== Mock Backend =====

/**
 * Mock implementation of IChatBackend for testing.
 * Does not spawn subprocesses or threads — all calls are synchronous fakes.
 */
class FMockChatBackend : public IChatBackend
{
public:
	/** Whether ExecuteAsync was called */
	bool bExecuteAsyncCalled = false;

	/** Whether Cancel was called */
	bool bCancelCalled = false;

	/** Whether the mock should report success */
	bool bShouldSucceed = true;

	/** The mock response to return */
	FString MockResponse = TEXT("mock response");

	/** The last prompt received */
	FString LastPrompt;

	// IChatBackend interface

	virtual bool ExecuteAsync(
		const FChatRequestConfig& Config,
		FOnChatResponse OnComplete,
		FOnChatProgress OnProgress = FOnChatProgress()) override
	{
		bExecuteAsyncCalled = true;
		LastPrompt = Config.Prompt;
		OnComplete.ExecuteIfBound(MockResponse, bShouldSucceed);
		return true;
	}

	virtual bool ExecuteSync(
		const FChatRequestConfig& Config,
		FString& OutResponse) override
	{
		LastPrompt = Config.Prompt;
		OutResponse = MockResponse;
		return bShouldSucceed;
	}

	virtual void Cancel() override
	{
		bCancelCalled = true;
	}

	virtual bool IsExecuting() const override
	{
		return false;
	}

	virtual bool IsAvailable() const override
	{
		return true;
	}

	virtual FString GetDisplayName() const override
	{
		return TEXT("MockBackend");
	}

	virtual EChatBackendType GetBackendType() const override
	{
		return EChatBackendType::Claude;
	}

	virtual FString FormatPrompt(
		const TArray<TPair<FString, FString>>& History,
		const FString& SystemPrompt,
		const FString& ProjectContext) const override
	{
		return History.Num() > 0 ? History.Last().Key : TEXT("");
	}

	virtual TUniquePtr<FChatRequestConfig> CreateConfig() const override
	{
		return MakeUnique<FChatRequestConfig>();
	}
};

// ===== OpenCode Test Mocks =====

#include "OpenCode/IProcessLauncher.h"
#include "OpenCode/OpenCodeHttpClient.h"
#include "OpenCode/OpenCodeTypes.h"

/**
 * Mock process launcher for testing FOpenCodeServerLifecycle without real process spawning (D-27).
 * All behavior controlled via public bool flags and call counters.
 */
class FMockProcessLauncher : public IProcessLauncher
{
public:
	/** Whether LaunchProcess should simulate a successful spawn */
	bool bLaunchShouldSucceed = true;
	/** Whether IsProcRunning returns true */
	bool bProcessIsRunning = true;
	/** Whether IsApplicationRunning returns true */
	bool bApplicationIsRunning = true;
	/** Simulated PID written to OutProcessID */
	uint32 MockPID = 12345;
	/** Number of times LaunchProcess was called */
	int32 LaunchCallCount = 0;
	/** Number of times TerminateProc was called */
	int32 TerminateCallCount = 0;
	/** Last Params string passed to LaunchProcess */
	FString LastLaunchParams;

	virtual FProcHandle LaunchProcess(
		const TCHAR* URL, const TCHAR* Params,
		bool bLaunchDetached, bool bLaunchHidden, bool bLaunchReallyHidden,
		uint32* OutProcessID, int32 PriorityModifier,
		const TCHAR* WorkingDirectory, void* PipeWriteChild) override
	{
		++LaunchCallCount;
		LastLaunchParams = Params;
		if (OutProcessID)
		{
			*OutProcessID = bLaunchShouldSucceed ? MockPID : 0;
		}
		// UE FProcHandle default-constructs as invalid.
		// Lifecycle checks bLaunchShouldSucceed / OutProcessID != 0 rather than handle validity.
		return FProcHandle();
	}

	virtual bool IsProcRunning(FProcHandle& Handle) override
	{
		return bProcessIsRunning;
	}

	virtual void TerminateProc(FProcHandle& Handle, bool bKillTree = false) override
	{
		++TerminateCallCount;
	}

	virtual bool IsApplicationRunning(uint32 ProcessId) override
	{
		return bApplicationIsRunning;
	}
};

/**
 * Mock HTTP client for testing without real network calls (D-27).
 * Provides SimulateSSEData / SimulateSSEDisconnect helpers for SSE tests.
 */
class FMockHttpClient : public IHttpClient
{
public:
	/** HTTP status code to return from Get/Post */
	int32 MockStatusCode = 200;
	/** Response body to return from Get/Post */
	FString MockResponseBody = TEXT("{\"healthy\":true,\"version\":\"0.1.0\"}");
	/** Number of times Get was called */
	int32 GetCallCount = 0;
	/** Number of times Post was called */
	int32 PostCallCount = 0;
	/** Last URL passed to Get or BeginSSEStream */
	FString LastGetUrl;
	/** Last URL passed to Post */
	FString LastPostUrl;
	/** Last body passed to Post */
	FString LastPostBody;
	/** Last stream callback passed to BeginSSEStream */
	TFunction<bool(void*, int64)> LastStreamCallback;
	/** Last disconnect callback passed to BeginSSEStream */
	TFunction<void()> LastDisconnectCallback;
	/** Whether SSE stream is currently active */
	bool bSSEStreamActive = false;

	virtual int32 Get(const FString& Url, FString& OutBody) override
	{
		++GetCallCount;
		LastGetUrl = Url;
		OutBody = MockResponseBody;
		return MockStatusCode;
	}

	virtual int32 Post(const FString& Url, const FString& Body, FString& OutResponse) override
	{
		++PostCallCount;
		LastPostUrl = Url;
		LastPostBody = Body;
		OutResponse = MockResponseBody;
		return MockStatusCode;
	}

	virtual void BeginSSEStream(const FString& Url,
		TFunction<bool(void* Ptr, int64 Length)> StreamCallback,
		TFunction<void()> OnDisconnected) override
	{
		LastGetUrl = Url;
		LastStreamCallback = StreamCallback;
		LastDisconnectCallback = OnDisconnected;
		bSSEStreamActive = true;
	}

	virtual void AbortSSEStream() override
	{
		bSSEStreamActive = false;
	}

	/** Test helper: simulate receiving a chunk of SSE data through the stream callback */
	void SimulateSSEData(const FString& Data)
	{
		if (LastStreamCallback)
		{
			auto Utf8 = FTCHARToUTF8(*Data);
			LastStreamCallback((void*)Utf8.Get(), Utf8.Length());
		}
	}

	/** Test helper: simulate SSE stream disconnect firing the disconnect callback */
	void SimulateSSEDisconnect()
	{
		bSSEStreamActive = false;
		if (LastDisconnectCallback)
		{
			LastDisconnectCallback();
		}
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
