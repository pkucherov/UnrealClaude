// Copyright Natali Caggiano. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/TestUtils.h"
#include "OpenCode/OpenCodeServerLifecycle.h"
#include "ChatBackendTypes.h"

// ===== Test fixture helper =====

/** Create a lifecycle instance with mock dependencies and a fake opencode path. */
static FOpenCodeServerLifecycle MakeLifecycle(
	TSharedPtr<FMockProcessLauncher> Launcher,
	TSharedPtr<FMockHttpClient> Http)
{
	return FOpenCodeServerLifecycle(Launcher, Http, TEXT("/fake/opencode"));
}

// ===== State machine basics =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_InitialState_IsDisconnected,
	"UnrealClaude.OpenCode.ServerLifecycle.InitialState_IsDisconnected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_InitialState_IsDisconnected::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	TestEqual("Initial state is Disconnected",
		LC.GetConnectionState(),
		EOpenCodeConnectionState::Disconnected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_OnStateChanged_FiredOnTransition,
	"UnrealClaude.OpenCode.ServerLifecycle.OnStateChanged_FiredOnTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_OnStateChanged_FiredOnTransition::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	TArray<EOpenCodeConnectionState> StateHistory;
	LC.OnStateChanged.BindLambda([&StateHistory](EOpenCodeConnectionState S)
	{
		StateHistory.Add(S);
	});

	LC.MarkConnecting();

	TestEqual("OnStateChanged fired", StateHistory.Num(), 1);
	if (StateHistory.Num() > 0)
	{
		TestEqual("State is Connecting",
			StateHistory[0], EOpenCodeConnectionState::Connecting);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_GetConnectionState_ReturnsCurrentState,
	"UnrealClaude.OpenCode.ServerLifecycle.GetConnectionState_ReturnsCurrentState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_GetConnectionState_ReturnsCurrentState::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	LC.MarkConnected();
	TestEqual("GetConnectionState returns Connected",
		LC.GetConnectionState(),
		EOpenCodeConnectionState::Connected);

	LC.MarkReconnecting();
	TestEqual("GetConnectionState returns Reconnecting",
		LC.GetConnectionState(),
		EOpenCodeConnectionState::Reconnecting);
	return true;
}

// ===== Detection: No PID file → Spawning =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_NoPidFile_ProceedsToSpawning,
	"UnrealClaude.OpenCode.ServerLifecycle.NoPidFile_ProceedsToSpawning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_NoPidFile_ProceedsToSpawning::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	// No PID file exists; spawn should succeed
	Launcher->bLaunchShouldSucceed = true;

	TArray<EOpenCodeConnectionState> States;
	LC.OnStateChanged.BindLambda([&States](EOpenCodeConnectionState S)
	{
		States.Add(S);
	});

	LC.EnsureServerReady();

	// Should have transitioned: Detecting → Spawning (if no PID, goes to spawn)
	// then Connecting (after successful spawn)
	bool bHasConnecting = States.Contains(EOpenCodeConnectionState::Connecting);
	TestTrue("EnsureServerReady: reaches Connecting after spawn", bHasConnecting);
	TestTrue("EnsureServerReady: launch was called",
		Launcher->LaunchCallCount > 0);
	return true;
}

// ===== Detection: valid PID + healthy → Connecting (skip Spawning) =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_ValidPidAndHealthy_SkipsSpawning,
	"UnrealClaude.OpenCode.ServerLifecycle.ValidPidAndHealthy_SkipsSpawning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_ValidPidAndHealthy_SkipsSpawning::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	// Write a PID file so detection finds it
	FString PidPath = FPaths::ProjectSavedDir() / TEXT("UnrealClaude") / TEXT("opencode-server.pid");
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("UnrealClaude")), true);
	FFileHelper::SaveStringToFile(TEXT("12345"), *PidPath);

	// Process is alive
	Launcher->bApplicationIsRunning = true;
	// Health check returns 200 with healthy=true
	Http->MockStatusCode = 200;
	Http->MockResponseBody = TEXT("{\"healthy\":true,\"version\":\"1.0\"}");

	TArray<EOpenCodeConnectionState> States;
	LC.OnStateChanged.BindLambda([&States](EOpenCodeConnectionState S)
	{
		States.Add(S);
	});

	LC.EnsureServerReady();

	// Cleanup PID file
	IFileManager::Get().Delete(*PidPath);

	// Should reach Connecting without spawning
	bool bHasConnecting = States.Contains(EOpenCodeConnectionState::Connecting);
	TestTrue("ValidPid: reaches Connecting", bHasConnecting);
	TestEqual("ValidPid: no spawn attempt", Launcher->LaunchCallCount, 0);
	return true;
}

// ===== Detection: stale PID (process not alive) → Spawning =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_StalePid_ProceedsToSpawning,
	"UnrealClaude.OpenCode.ServerLifecycle.StalePid_ProceedsToSpawning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_StalePid_ProceedsToSpawning::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	// Write a stale PID file
	FString PidPath = FPaths::ProjectSavedDir() / TEXT("UnrealClaude") / TEXT("opencode-server.pid");
	IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("UnrealClaude")), true);
	FFileHelper::SaveStringToFile(TEXT("99999"), *PidPath);

	// Process is NOT alive (stale PID)
	Launcher->bApplicationIsRunning = false;
	Launcher->bLaunchShouldSucceed = true;

	LC.EnsureServerReady();

	// PID file should be gone (cleaned up) and spawn should have been attempted
	TestFalse("StalePid: PID file deleted", IFileManager::Get().FileExists(*PidPath));
	TestTrue("StalePid: spawn was attempted", Launcher->LaunchCallCount > 0);

	// Cleanup just in case
	IFileManager::Get().Delete(*PidPath);
	return true;
}

// ===== Spawning: default port succeeds → PID file written → Connecting =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_DefaultPortSpawns_WritesPidFile,
	"UnrealClaude.OpenCode.ServerLifecycle.DefaultPortSpawns_WritesPidFile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_DefaultPortSpawns_WritesPidFile::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	Launcher->bLaunchShouldSucceed = true;
	Launcher->MockPID = 54321;

	FString PidPath = FPaths::ProjectSavedDir() / TEXT("UnrealClaude") / TEXT("opencode-server.pid");
	// Ensure no leftover file
	IFileManager::Get().Delete(*PidPath);

	LC.EnsureServerReady();

	// PID file should have been written
	FString PidContent;
	bool bFileExists = FFileHelper::LoadFileToString(PidContent, *PidPath);

	TestTrue("PidFile: file was written", bFileExists);
	if (bFileExists)
	{
		int32 WrittenPID = FCString::Atoi(*PidContent);
		TestEqual("PidFile: contains correct PID", WrittenPID, (int32)54321);
	}

	// Cleanup
	IFileManager::Get().Delete(*PidPath);
	return true;
}

// ===== Spawning: spawn params include correct format =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_SpawnParams_UseCorrectFormat,
	"UnrealClaude.OpenCode.ServerLifecycle.SpawnParams_UseCorrectFormat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_SpawnParams_UseCorrectFormat::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	Launcher->bLaunchShouldSucceed = true;

	LC.EnsureServerReady();

	// Spawn params should contain "serve --port" and "127.0.0.1"
	TestTrue("SpawnParams: contains 'serve'",
		Launcher->LastLaunchParams.Contains(TEXT("serve")));
	TestTrue("SpawnParams: contains '--port'",
		Launcher->LastLaunchParams.Contains(TEXT("--port")));
	TestTrue("SpawnParams: contains '127.0.0.1'",
		Launcher->LastLaunchParams.Contains(TEXT("127.0.0.1")));

	FString PidPath = FPaths::ProjectSavedDir() / TEXT("UnrealClaude") / TEXT("opencode-server.pid");
	IFileManager::Get().Delete(*PidPath);
	return true;
}

// ===== Spawning: port retry on failure (4096→4097→success) =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_PortRetry_TriesNextPort,
	"UnrealClaude.OpenCode.ServerLifecycle.PortRetry_TriesNextPort",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_PortRetry_TriesNextPort::RunTest(const FString& Parameters)
{
	// Custom mock: first call fails, second succeeds
	class FFailFirstLauncher : public FMockProcessLauncher
	{
	public:
		virtual FProcHandle LaunchProcess(
			const TCHAR* URL, const TCHAR* Params,
			bool bLaunchDetached, bool bLaunchHidden, bool bLaunchReallyHidden,
			uint32* OutProcessID, int32 PriorityModifier,
			const TCHAR* WorkingDirectory, void* PipeWriteChild) override
		{
			++LaunchCallCount;
			LastLaunchParams = Params;
			// Only succeed on the second call
			bool bSucceed = (LaunchCallCount >= 2);
			if (OutProcessID)
			{
				*OutProcessID = bSucceed ? MockPID : 0;
			}
			return FProcHandle();
		}
	};

	auto Launcher = MakeShared<FFailFirstLauncher>();
	Launcher->MockPID = 44444;
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	LC.EnsureServerReady();

	TestTrue("PortRetry: at least 2 launch attempts", Launcher->LaunchCallCount >= 2);
	// Second attempt should have used port 4097
	TestTrue("PortRetry: second attempt used 4097",
		Launcher->LastLaunchParams.Contains(TEXT("4097")));

	FString PidPath = FPaths::ProjectSavedDir() / TEXT("UnrealClaude") / TEXT("opencode-server.pid");
	IFileManager::Get().Delete(*PidPath);
	return true;
}

// ===== Spawning: all ports exhausted → Disconnected + error =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_AllPortsFail_DisconnectedWithError,
	"UnrealClaude.OpenCode.ServerLifecycle.AllPortsFail_DisconnectedWithError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_AllPortsFail_DisconnectedWithError::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	// All spawns fail
	Launcher->bLaunchShouldSucceed = false;

	FChatStreamEvent ErrorEvent;
	bool bErrorFired = false;
	LC.OnError.BindLambda([&ErrorEvent, &bErrorFired](const FChatStreamEvent& Evt)
	{
		ErrorEvent = Evt;
		bErrorFired = true;
	});

	LC.EnsureServerReady();

	TestTrue("AllFail: error event fired", bErrorFired);
	TestTrue("AllFail: error has bIsError=true", ErrorEvent.bIsError);
	TestEqual("AllFail: error type is ServerUnreachable",
		ErrorEvent.ErrorType, EOpenCodeErrorType::ServerUnreachable);
	TestEqual("AllFail: state is Disconnected",
		LC.GetConnectionState(), EOpenCodeConnectionState::Disconnected);
	TestEqual("AllFail: tried all 4 ports", Launcher->LaunchCallCount, 4);
	return true;
}

// ===== Shutdown: terminates managed server =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_Shutdown_TerminatesManagedServer,
	"UnrealClaude.OpenCode.ServerLifecycle.Shutdown_TerminatesManagedServer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_Shutdown_TerminatesManagedServer::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	Launcher->bLaunchShouldSucceed = true;
	Launcher->MockPID = 77777;

	// Spawn first
	LC.EnsureServerReady();
	TestTrue("Setup: launch was called", Launcher->LaunchCallCount > 0);

	int32 TermCountBefore = Launcher->TerminateCallCount;

	LC.Shutdown();

	TestEqual("Shutdown: TerminateProc called",
		Launcher->TerminateCallCount, TermCountBefore + 1);
	TestEqual("Shutdown: state is Disconnected",
		LC.GetConnectionState(), EOpenCodeConnectionState::Disconnected);

	FString PidPath = FPaths::ProjectSavedDir() / TEXT("UnrealClaude") / TEXT("opencode-server.pid");
	TestFalse("Shutdown: PID file deleted", IFileManager::Get().FileExists(*PidPath));

	IFileManager::Get().Delete(*PidPath);
	return true;
}

// ===== Shutdown: does NOT terminate unmanaged server =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_Shutdown_LeavesUnmanagedServer,
	"UnrealClaude.OpenCode.ServerLifecycle.Shutdown_LeavesUnmanagedServer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_Shutdown_LeavesUnmanagedServer::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	// This lifecycle never spawned anything — call Shutdown directly
	// (simulates a lifecycle that detected an externally-running server via health check only)
	LC.Shutdown();

	TestEqual("UnmanagedShutdown: no TerminateProc called",
		Launcher->TerminateCallCount, 0);
	return true;
}

// ===== Backoff: initial value is SSEReconnectBackoffInitialSeconds =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_Backoff_InitialValue,
	"UnrealClaude.OpenCode.ServerLifecycle.Backoff_InitialValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_Backoff_InitialValue::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	TestEqual("Backoff: initial value is 1.0s",
		LC.GetNextBackoffSeconds(), 1.0f);
	return true;
}

// ===== Backoff: doubles on each increment, caps at 30s =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_Backoff_DoublesAndCaps,
	"UnrealClaude.OpenCode.ServerLifecycle.Backoff_DoublesAndCaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_Backoff_DoublesAndCaps::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	// Sequence: 1 → 2 → 4 → 8 → 16 → 30 (capped)
	TArray<float> Expected = { 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 30.0f, 30.0f };
	for (int32 i = 0; i < Expected.Num(); ++i)
	{
		float Val = LC.GetNextBackoffSeconds();
		TestEqual(
			FString::Printf(TEXT("Backoff[%d]: expected %.0f"), i, Expected[i]),
			Val, Expected[i]);
		LC.IncrementBackoff();
	}
	return true;
}

// ===== Backoff: ResetBackoff returns to 1.0s =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_Backoff_ResetReturnsToInitial,
	"UnrealClaude.OpenCode.ServerLifecycle.Backoff_ResetReturnsToInitial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_Backoff_ResetReturnsToInitial::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	// Increase backoff several times
	LC.IncrementBackoff();
	LC.IncrementBackoff();
	LC.IncrementBackoff();
	TestTrue("Backoff: after increments > 1.0f",
		LC.GetNextBackoffSeconds() > 1.0f);

	// Reset
	LC.ResetBackoff();
	TestEqual("Backoff: after reset is 1.0f",
		LC.GetNextBackoffSeconds(), 1.0f);
	return true;
}

// ===== State transitions: MarkConnected resets backoff =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_MarkConnected_ResetsBackoff,
	"UnrealClaude.OpenCode.ServerLifecycle.MarkConnected_ResetsBackoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_MarkConnected_ResetsBackoff::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	LC.IncrementBackoff();
	LC.IncrementBackoff();
	TestTrue("Setup: backoff > 1.0f", LC.GetNextBackoffSeconds() > 1.0f);

	LC.MarkConnected();

	TestEqual("MarkConnected: state is Connected",
		LC.GetConnectionState(), EOpenCodeConnectionState::Connected);
	TestEqual("MarkConnected: backoff reset to 1.0f",
		LC.GetNextBackoffSeconds(), 1.0f);
	return true;
}

// ===== MarkReconnecting transitions state =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_MarkReconnecting_SetsState,
	"UnrealClaude.OpenCode.ServerLifecycle.MarkReconnecting_SetsState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_MarkReconnecting_SetsState::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	LC.MarkReconnecting();

	TestEqual("MarkReconnecting: state is Reconnecting",
		LC.GetConnectionState(), EOpenCodeConnectionState::Reconnecting);
	return true;
}

// ===== SetConfiguredPort overrides default =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_ConfiguredPort_OverridesDefault,
	"UnrealClaude.OpenCode.ServerLifecycle.ConfiguredPort_OverridesDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_ConfiguredPort_OverridesDefault::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	LC.SetConfiguredPort(5000);
	Launcher->bLaunchShouldSucceed = true;

	LC.EnsureServerReady();

	// Spawn params should use port 5000
	TestTrue("ConfigPort: params contain 5000",
		Launcher->LastLaunchParams.Contains(TEXT("5000")));

	FString PidPath = FPaths::ProjectSavedDir() / TEXT("UnrealClaude") / TEXT("opencode-server.pid");
	IFileManager::Get().Delete(*PidPath);
	return true;
}

// ===== GetServerBaseUrl returns correct URL =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_GetServerBaseUrl_ReturnsCorrectUrl,
	"UnrealClaude.OpenCode.ServerLifecycle.GetServerBaseUrl_ReturnsCorrectUrl",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_GetServerBaseUrl_ReturnsCorrectUrl::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	Launcher->bLaunchShouldSucceed = true;
	LC.EnsureServerReady();

	FString BaseUrl = LC.GetServerBaseUrl();

	// Should be in format http://127.0.0.1:4096 (or higher port if retried)
	TestTrue("BaseUrl: starts with http://",
		BaseUrl.StartsWith(TEXT("http://")));
	TestTrue("BaseUrl: contains 127.0.0.1",
		BaseUrl.Contains(TEXT("127.0.0.1")));

	FString PidPath = FPaths::ProjectSavedDir() / TEXT("UnrealClaude") / TEXT("opencode-server.pid");
	IFileManager::Get().Delete(*PidPath);
	return true;
}

// ===== PID file cleanup on Shutdown =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_PidFile_DeletedOnShutdown,
	"UnrealClaude.OpenCode.ServerLifecycle.PidFile_DeletedOnShutdown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_PidFile_DeletedOnShutdown::RunTest(const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	Launcher->bLaunchShouldSucceed = true;
	Launcher->MockPID = 11111;
	LC.EnsureServerReady();

	FString PidPath = FPaths::ProjectSavedDir() / TEXT("UnrealClaude") / TEXT("opencode-server.pid");
	TestTrue("Setup: PID file created", IFileManager::Get().FileExists(*PidPath));

	LC.Shutdown();

	TestFalse("AfterShutdown: PID file deleted", IFileManager::Get().FileExists(*PidPath));

	// Safety cleanup
	IFileManager::Get().Delete(*PidPath);
	return true;
}

// ===== EnsureServerReady is idempotent when not Disconnected =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLifecycle_EnsureServerReady_IdempotentWhenNotDisconnected,
	"UnrealClaude.OpenCode.ServerLifecycle.EnsureServerReady_IdempotentWhenNotDisconnected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FServerLifecycle_EnsureServerReady_IdempotentWhenNotDisconnected::RunTest(
	const FString& Parameters)
{
	auto Launcher = MakeShared<FMockProcessLauncher>();
	auto Http = MakeShared<FMockHttpClient>();
	auto LC = MakeLifecycle(Launcher, Http);

	Launcher->bLaunchShouldSucceed = true;
	LC.EnsureServerReady();

	int32 LaunchCountAfterFirst = Launcher->LaunchCallCount;

	// Calling again while in Connecting state should do nothing
	LC.EnsureServerReady();

	TestEqual("Idempotent: no additional spawn on second call",
		Launcher->LaunchCallCount, LaunchCountAfterFirst);

	FString PidPath = FPaths::ProjectSavedDir() / TEXT("UnrealClaude") / TEXT("opencode-server.pid");
	IFileManager::Get().Delete(*PidPath);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
