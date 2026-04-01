// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformProcess.h"

/**
 * Interface for process management operations.
 * Abstracts FPlatformProcess calls to enable mock injection in tests (D-27).
 * Production implementation wraps FPlatformProcess directly.
 * Test implementation (FMockProcessLauncher in TestUtils.h) uses controllable flags.
 */
class IProcessLauncher
{
public:
	virtual ~IProcessLauncher() = default;

	/**
	 * Launch a process. Returns FProcHandle (check IsValid()).
	 * @param URL - Path to the executable
	 * @param Params - Command-line parameters
	 * @param bLaunchDetached - Launch without inheriting parent console
	 * @param bLaunchHidden - Hide the process window
	 * @param bLaunchReallyHidden - Hide even more aggressively
	 * @param OutProcessID - Receives the OS-assigned process ID
	 * @param PriorityModifier - Process priority offset (-2 to +2)
	 * @param WorkingDirectory - Initial working directory for the process
	 * @param PipeWriteChild - Optional write pipe handle for stdin
	 */
	virtual FProcHandle LaunchProcess(
		const TCHAR* URL, const TCHAR* Params,
		bool bLaunchDetached, bool bLaunchHidden, bool bLaunchReallyHidden,
		uint32* OutProcessID, int32 PriorityModifier,
		const TCHAR* WorkingDirectory, void* PipeWriteChild) = 0;

	/**
	 * Check if a process handle is still running.
	 * @param Handle - Process handle to query
	 */
	virtual bool IsProcRunning(FProcHandle& Handle) = 0;

	/**
	 * Terminate a process.
	 * @param Handle - Process handle to terminate
	 * @param bKillTree - Also kill all child processes
	 */
	virtual void TerminateProc(FProcHandle& Handle, bool bKillTree = false) = 0;

	/**
	 * Check if an application with the given PID is running.
	 * @param ProcessId - OS process ID to query
	 */
	virtual bool IsApplicationRunning(uint32 ProcessId) = 0;
};
