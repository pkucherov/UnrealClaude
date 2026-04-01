// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformProcess.h"
#include "OpenCode/OpenCodeTypes.h"
#include "ChatBackendTypes.h"

class IProcessLauncher;
class IHttpClient;

/**
 * Manages the OpenCode server lifecycle: detect, spawn, monitor, and shutdown.
 * Owns the six-state connection machine (D-34, D-35):
 *   Disconnected → Detecting → Spawning / Connecting → Connected → Reconnecting
 *
 * Dependencies (IProcessLauncher, IHttpClient) are constructor-injected for testability (D-10, D-27).
 * This class is owned by FOpenCodeRunner (Phase 4) and is not thread-safe by itself.
 */
class FOpenCodeServerLifecycle
{
public:
	/**
	 * @param InProcessLauncher - Process launcher (mock in tests, real in production)
	 * @param InHttpClient      - HTTP client for health checks (mock in tests)
	 * @param InOpenCodePath    - Filesystem path to the opencode binary
	 */
	FOpenCodeServerLifecycle(
		TSharedPtr<IProcessLauncher> InProcessLauncher,
		TSharedPtr<IHttpClient> InHttpClient,
		const FString& InOpenCodePath);

	~FOpenCodeServerLifecycle();

	// ===== Public API =====

	/**
	 * Start the detection/spawn sequence.
	 * No-op when state is not Disconnected (idempotent guard).
	 * Sequence: Detecting → (Connecting if existing server found) or (Spawning → Connecting).
	 */
	void EnsureServerReady();

	/**
	 * Shut down the managed OpenCode server process.
	 * Only terminates a server that was spawned by this instance (bIsManagedServer).
	 * Transitions to Disconnected regardless.
	 */
	void Shutdown();

	/** Returns the current connection state. */
	EOpenCodeConnectionState GetConnectionState() const;

	/**
	 * Returns the base URL of the active server, e.g. "http://127.0.0.1:4096".
	 * Empty string when not connected/connecting.
	 */
	FString GetServerBaseUrl() const;

	/** Returns the current backoff delay in seconds (before incrementing). */
	float GetNextBackoffSeconds() const;

	/** Double the backoff delay, capped at SSEReconnectBackoffMaxSeconds. */
	void IncrementBackoff();

	/** Reset backoff delay to SSEReconnectBackoffInitialSeconds. */
	void ResetBackoff();

	/** Transition to Reconnecting state (called by FOpenCodeRunner on lost connection). */
	void MarkReconnecting();

	/** Transition to Connected state and reset backoff (called when SSE stream established). */
	void MarkConnected();

	/** Transition to Connecting state. */
	void MarkConnecting();

	/** Override the port used for the next EnsureServerReady() call. 0 = use default resolution. */
	void SetConfiguredPort(uint32 Port);

	// ===== Delegates =====

	/** Fires on every connection state transition. */
	FOnConnectionStateChanged OnStateChanged;

	/** Fires when a lifecycle error occurs (e.g. port exhaustion). */
	FOnChatStreamEvent OnError;

private:
	// ===== State Machine =====

	/** Set state and fire OnStateChanged delegate. */
	void SetState(EOpenCodeConnectionState NewState);

	// ===== Detection =====

	/**
	 * Read PID file; check process liveness and server health.
	 * @return true if an existing healthy server was found (sets ActivePort).
	 */
	bool DetectExistingServer();

	// ===== Spawning =====

	/**
	 * Try spawning opencode serve on ports 4096..4099.
	 * @return true if a process was successfully started.
	 */
	bool TrySpawnServer();

	/**
	 * Resolve the port to use: ConfiguredPort → env var OPENCODE_PORT → DefaultOpenCodePort.
	 * @return The resolved port number.
	 */
	uint32 ResolvePort() const;

	// ===== PID File =====

	/** Returns the canonical PID file path. */
	FString GetPidFilePath() const;

	/**
	 * Read PID from the PID file.
	 * @param OutPID - Receives the PID on success.
	 * @return true if the file existed and contained a valid non-zero integer.
	 */
	bool ReadPidFile(uint32& OutPID) const;

	/** Write PID integer to the PID file, creating directories as needed. */
	void WritePidFile(uint32 PID);

	/** Delete the PID file if it exists. */
	void DeletePidFile();

	// ===== Error Helpers =====

	/** Build and fire a FChatStreamEvent error through OnError. */
	void FireError(EOpenCodeErrorType ErrorType, const FString& Message);

	// ===== Dependencies =====

	TSharedPtr<IProcessLauncher> ProcessLauncher;
	TSharedPtr<IHttpClient> HttpClient;
	FString OpenCodePath;

	// ===== State =====

	EOpenCodeConnectionState CurrentState = EOpenCodeConnectionState::Disconnected;

	/** Port the active server is listening on (0 = none). */
	uint32 ActivePort = 0;

	/** Caller-configured port override (0 = no override). */
	uint32 ConfiguredPort = 0;

	/** PID of the managed server process (0 = none). */
	uint32 ManagedPID = 0;

	/** Handle to the managed server process. */
	FProcHandle ManagedHandle;

	/** True when this instance owns the server process (spawned it via TrySpawnServer). */
	bool bIsManagedServer = false;

	// ===== Backoff =====

	float CurrentBackoffSeconds = 1.0f;
	int32 BackoffAttempts = 0;
};
