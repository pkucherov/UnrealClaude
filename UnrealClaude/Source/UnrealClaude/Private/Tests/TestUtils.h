// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Shared test utilities for UnrealClaude automation tests.
 * Provides JSON factories, temp file helpers, and FMockClaudeRunner
 * for use across all UnrealClaude test files.
 */

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "IClaudeRunner.h"
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

// ===== Mock Runner =====

/**
 * Mock implementation of IClaudeRunner for testing.
 * Does not spawn subprocesses or threads — all calls are synchronous fakes.
 */
class FMockClaudeRunner : public IClaudeRunner
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

	// IClaudeRunner interface

	virtual bool ExecuteAsync(
		const FClaudeRequestConfig& Config,
		FOnClaudeResponse OnComplete,
		FOnClaudeProgress OnProgress = FOnClaudeProgress()) override
	{
		bExecuteAsyncCalled = true;
		LastPrompt = Config.Prompt;
		OnComplete.ExecuteIfBound(MockResponse, bShouldSucceed);
		return true;
	}

	virtual bool ExecuteSync(
		const FClaudeRequestConfig& Config,
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
};

#endif // WITH_DEV_AUTOMATION_TESTS
