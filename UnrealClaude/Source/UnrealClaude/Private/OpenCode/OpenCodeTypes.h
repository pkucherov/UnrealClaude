// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChatBackendTypes.h"

// ===== EOpenCodeConnectionState =====

/**
 * Connection states for the OpenCode server lifecycle state machine (D-34).
 * Transitions: Disconnected → Detecting → Spawning/Connecting → Connected.
 * Reconnecting is entered on unexpected disconnect from Connected.
 */
enum class EOpenCodeConnectionState : uint8
{
	/** No server connection, not attempting */
	Disconnected,
	/** Reading PID file and checking for existing server */
	Detecting,
	/** Launching a new opencode serve process */
	Spawning,
	/** Server process found/spawned, establishing HTTP connection */
	Connecting,
	/** SSE stream active, server health confirmed */
	Connected,
	/** Connection lost, waiting for exponential backoff before retry */
	Reconnecting
};

// ===== Delegates =====

/**
 * Delegate fired on every connection state transition (D-35).
 * Consumers (FOpenCodeRunner, UI) bind this to update status indicators.
 */
DECLARE_DELEGATE_OneParam(FOnConnectionStateChanged, EOpenCodeConnectionState);
