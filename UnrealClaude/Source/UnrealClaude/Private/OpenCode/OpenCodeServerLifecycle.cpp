// Copyright Natali Caggiano. All Rights Reserved.

#include "OpenCode/OpenCodeServerLifecycle.h"
#include "OpenCode/IProcessLauncher.h"
#include "OpenCode/OpenCodeHttpClient.h"
#include "UnrealClaudeConstants.h"
#include "UnrealClaudeModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"

// ===== Constructor / Destructor =====

FOpenCodeServerLifecycle::FOpenCodeServerLifecycle(
	TSharedPtr<IProcessLauncher> InProcessLauncher,
	TSharedPtr<IHttpClient> InHttpClient,
	const FString& InOpenCodePath)
	: ProcessLauncher(MoveTemp(InProcessLauncher))
	, HttpClient(MoveTemp(InHttpClient))
	, OpenCodePath(InOpenCodePath)
	, CurrentBackoffSeconds(UnrealClaudeConstants::Backend::SSEReconnectBackoffInitialSeconds)
{
}

FOpenCodeServerLifecycle::~FOpenCodeServerLifecycle()
{
}

// ===== Public API =====

void FOpenCodeServerLifecycle::EnsureServerReady()
{
	// Idempotent: only act when Disconnected
	if (CurrentState != EOpenCodeConnectionState::Disconnected)
	{
		return;
	}

	SetState(EOpenCodeConnectionState::Detecting);

	if (DetectExistingServer())
	{
		SetState(EOpenCodeConnectionState::Connecting);
		return;
	}

	if (TrySpawnServer())
	{
		SetState(EOpenCodeConnectionState::Connecting);
	}
	else
	{
		SetState(EOpenCodeConnectionState::Disconnected);
	}
}

void FOpenCodeServerLifecycle::Shutdown()
{
	if (bIsManagedServer && ManagedPID > 0)
	{
		ProcessLauncher->TerminateProc(ManagedHandle, true);
		DeletePidFile();
		ManagedPID = 0;
		bIsManagedServer = false;
	}

	ActivePort = 0;
	SetState(EOpenCodeConnectionState::Disconnected);
}

EOpenCodeConnectionState FOpenCodeServerLifecycle::GetConnectionState() const
{
	return CurrentState;
}

FString FOpenCodeServerLifecycle::GetServerBaseUrl() const
{
	if (ActivePort == 0)
	{
		return FString();
	}
	return FString::Printf(TEXT("http://127.0.0.1:%u"), ActivePort);
}

float FOpenCodeServerLifecycle::GetNextBackoffSeconds() const
{
	return CurrentBackoffSeconds;
}

void FOpenCodeServerLifecycle::IncrementBackoff()
{
	const float MaxBackoff = UnrealClaudeConstants::Backend::SSEReconnectBackoffMaxSeconds;
	CurrentBackoffSeconds = FMath::Min(CurrentBackoffSeconds * 2.0f, MaxBackoff);
	++BackoffAttempts;
}

void FOpenCodeServerLifecycle::ResetBackoff()
{
	CurrentBackoffSeconds = UnrealClaudeConstants::Backend::SSEReconnectBackoffInitialSeconds;
	BackoffAttempts = 0;
}

void FOpenCodeServerLifecycle::MarkReconnecting()
{
	SetState(EOpenCodeConnectionState::Reconnecting);
}

void FOpenCodeServerLifecycle::MarkConnected()
{
	ResetBackoff();
	SetState(EOpenCodeConnectionState::Connected);
}

void FOpenCodeServerLifecycle::MarkConnecting()
{
	SetState(EOpenCodeConnectionState::Connecting);
}

void FOpenCodeServerLifecycle::SetConfiguredPort(uint32 Port)
{
	ConfiguredPort = Port;
}

// ===== State Machine =====

void FOpenCodeServerLifecycle::SetState(EOpenCodeConnectionState NewState)
{
	CurrentState = NewState;
	OnStateChanged.ExecuteIfBound(NewState);
}

// ===== Detection =====

bool FOpenCodeServerLifecycle::DetectExistingServer()
{
	uint32 PID = 0;
	if (!ReadPidFile(PID))
	{
		return false;
	}

	// Check if the process is still alive
	if (!ProcessLauncher->IsApplicationRunning(PID))
	{
		// Stale PID — clean up
		DeletePidFile();
		return false;
	}

	// Process alive — check server health
	const uint32 Port = ResolvePort();
	const FString HealthUrl = FString::Printf(
		TEXT("http://127.0.0.1:%u/global/health"), Port);

	FString ResponseBody;
	const int32 StatusCode = HttpClient->Get(HealthUrl, ResponseBody);

	if (StatusCode == 200 && ResponseBody.Contains(TEXT("\"healthy\":true")))
	{
		ActivePort = Port;
		// Server was found alive — treat as managed (we have the PID file)
		ManagedPID = PID;
		bIsManagedServer = true;
		return true;
	}

	// Alive but unhealthy — kill it and re-spawn
	FProcHandle DummyHandle;
	ProcessLauncher->TerminateProc(DummyHandle, true);
	DeletePidFile();
	return false;
}

// ===== Spawning =====

bool FOpenCodeServerLifecycle::TrySpawnServer()
{
	const uint32 BasePort = ResolvePort();
	const int32 MaxRetries = UnrealClaudeConstants::Backend::MaxPortRetryCount;

	for (int32 i = 0; i < MaxRetries; ++i)
	{
		const uint32 Port = BasePort + static_cast<uint32>(i);
		const FString Params = FString::Printf(
			TEXT("serve --port %u --hostname 127.0.0.1"), Port);
		const FString WorkDir = FPaths::ProjectDir();

		uint32 OutPID = 0;
		FProcHandle Handle = ProcessLauncher->LaunchProcess(
			*OpenCodePath, *Params,
			/*bLaunchDetached=*/true, /*bLaunchHidden=*/true, /*bLaunchReallyHidden=*/false,
			&OutPID, /*PriorityModifier=*/0,
			*WorkDir, /*PipeWriteChild=*/nullptr);

		if (OutPID != 0)
		{
			ManagedHandle = Handle;
			ManagedPID = OutPID;
			ActivePort = Port;
			bIsManagedServer = true;
			WritePidFile(OutPID);
			return true;
		}
	}

	FireError(EOpenCodeErrorType::ServerUnreachable,
		TEXT("Failed to spawn OpenCode server: all ports exhausted"));
	return false;
}

uint32 FOpenCodeServerLifecycle::ResolvePort() const
{
	// 1. Caller-configured override
	if (ConfiguredPort > 0)
	{
		return ConfiguredPort;
	}

	// 2. Environment variable
	const FString EnvValue = FPlatformMisc::GetEnvironmentVariable(
		UnrealClaudeConstants::Backend::OpenCodePortEnvVar);
	if (!EnvValue.IsEmpty())
	{
		const int32 EnvPort = FCString::Atoi(*EnvValue);
		if (EnvPort > 0)
		{
			return static_cast<uint32>(EnvPort);
		}
	}

	// 3. Compile-time default
	return UnrealClaudeConstants::Backend::DefaultOpenCodePort;
}

// ===== PID File =====

FString FOpenCodeServerLifecycle::GetPidFilePath() const
{
	return FPaths::ProjectSavedDir()
		/ TEXT("UnrealClaude")
		/ UnrealClaudeConstants::Backend::OpenCodePidFilename;
}

bool FOpenCodeServerLifecycle::ReadPidFile(uint32& OutPID) const
{
	FString Content;
	if (!FFileHelper::LoadFileToString(Content, *GetPidFilePath()))
	{
		return false;
	}

	const int32 PID = FCString::Atoi(*Content.TrimStartAndEnd());
	if (PID <= 0)
	{
		return false;
	}

	OutPID = static_cast<uint32>(PID);
	return true;
}

void FOpenCodeServerLifecycle::WritePidFile(uint32 PID)
{
	const FString PidPath = GetPidFilePath();
	IFileManager::Get().MakeDirectory(
		*FPaths::GetPath(PidPath), /*Tree=*/true);
	FFileHelper::SaveStringToFile(
		FString::FromInt(static_cast<int32>(PID)), *PidPath);
}

void FOpenCodeServerLifecycle::DeletePidFile()
{
	IFileManager::Get().Delete(*GetPidFilePath());
}

// ===== Error Helpers =====

void FOpenCodeServerLifecycle::FireError(
	EOpenCodeErrorType ErrorType, const FString& Message)
{
	FChatStreamEvent ErrorEvent;
	ErrorEvent.Type = EChatStreamEventType::Unknown;
	ErrorEvent.bIsError = true;
	ErrorEvent.ErrorType = ErrorType;
	ErrorEvent.Text = Message;
	OnError.ExecuteIfBound(ErrorEvent);
}
