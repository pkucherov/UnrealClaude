// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChatBackendTypes.h"

class IChatBackend;

/**
 * Factory for creating chat backend instances by type.
 * Supports auto-detection of preferred backend based on binary availability.
 */
class FChatBackendFactory
{
public:
	/** Create a backend instance for the given type */
	static TUniquePtr<IChatBackend> Create(EChatBackendType Type);

	/** Detect preferred backend at startup based on available binaries */
	static EChatBackendType DetectPreferredBackend();

	/** Check if a backend's binary/server is available */
	static bool IsBinaryAvailable(EChatBackendType Type);

private:
	/** Check if OpenCode binary exists on PATH */
	static FString GetOpenCodePath();
};
