// Copyright Natali Caggiano. All Rights Reserved.

#include "ChatBackendFactory.h"
#include "IChatBackend.h"
#include "ClaudeCodeRunner.h"
#include "UnrealClaudeModule.h"
#include "HAL/PlatformProcess.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

TUniquePtr<IChatBackend> FChatBackendFactory::Create(EChatBackendType Type)
{
	switch (Type)
	{
	case EChatBackendType::Claude:
		return MakeUnique<FClaudeCodeRunner>();
	case EChatBackendType::OpenCode:
		// Phase 4 will add FOpenCodeRunner here
		UE_LOG(LogUnrealClaude, Warning,
			TEXT("OpenCode backend not yet implemented, falling back to Claude"));
		return MakeUnique<FClaudeCodeRunner>();
	default:
		return MakeUnique<FClaudeCodeRunner>();
	}
}

EChatBackendType FChatBackendFactory::DetectPreferredBackend()
{
	// Check if OpenCode is available — prefer it if found
	FString OpenCodePath = GetOpenCodePath();
	if (!OpenCodePath.IsEmpty())
	{
		UE_LOG(LogUnrealClaude, Log,
			TEXT("OpenCode binary detected at: %s — preferring OpenCode backend"),
			*OpenCodePath);
		return EChatBackendType::OpenCode;
	}

	return EChatBackendType::Claude;
}

bool FChatBackendFactory::IsBinaryAvailable(EChatBackendType Type)
{
	switch (Type)
	{
	case EChatBackendType::Claude:
		return FClaudeCodeRunner::IsClaudeAvailable();
	case EChatBackendType::OpenCode:
		return !GetOpenCodePath().IsEmpty();
	default:
		return false;
	}
}

FString FChatBackendFactory::GetOpenCodePath()
{
	// Cache the path to avoid repeated lookups
	static FString CachedOpenCodePath;
	static bool bHasSearched = false;

	if (bHasSearched && !CachedOpenCodePath.IsEmpty())
	{
		return CachedOpenCodePath;
	}
	bHasSearched = true;

	TArray<FString> PossiblePaths;

#if PLATFORM_WINDOWS
	// Check PATH for opencode.cmd or opencode.exe
	FString PathEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
	TArray<FString> PathDirs;
	PathEnv.ParseIntoArray(PathDirs, TEXT(";"), true);

	for (const FString& Dir : PathDirs)
	{
		PossiblePaths.Add(FPaths::Combine(Dir, TEXT("opencode.cmd")));
		PossiblePaths.Add(FPaths::Combine(Dir, TEXT("opencode.exe")));
	}

	// npm global install locations
	FString AppData = FPlatformMisc::GetEnvironmentVariable(TEXT("APPDATA"));
	if (!AppData.IsEmpty())
	{
		PossiblePaths.Add(FPaths::Combine(AppData, TEXT("npm"), TEXT("opencode.cmd")));
	}
#else
	// Linux/Mac: check PATH for opencode
	FString PathEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
	TArray<FString> PathDirs;
	PathEnv.ParseIntoArray(PathDirs, TEXT(":"), true);

	for (const FString& Dir : PathDirs)
	{
		PossiblePaths.Add(FPaths::Combine(Dir, TEXT("opencode")));
	}

	// Common system paths
	PossiblePaths.Add(TEXT("/usr/local/bin/opencode"));
	PossiblePaths.Add(TEXT("/usr/bin/opencode"));
#endif

	// Check each path
	for (const FString& Path : PossiblePaths)
	{
		if (IFileManager::Get().FileExists(*Path))
		{
			UE_LOG(LogUnrealClaude, Log, TEXT("Found OpenCode at: %s"), *Path);
			CachedOpenCodePath = Path;
			return CachedOpenCodePath;
		}
	}

	return FString();
}
