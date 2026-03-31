// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Integration tests for MCP Task Queue system
 * Tests async task submission, status tracking, cancellation, and lifecycle
 */

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "MCP/MCPToolRegistry.h"
#include "MCP/MCPTaskQueue.h"
#include "MCP/MCPAsyncTask.h"
#include "Dom/JsonObject.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== Task Queue Initialization Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPTaskQueue_Create,
	"UnrealClaude.MCP.TaskQueue.Create",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPTaskQueue_Create::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;
	TSharedPtr<FMCPTaskQueue> Queue = Registry.GetTaskQueue();

	TestTrue("Task queue should be created", Queue.IsValid());

	// Check default config values
	TestEqual("Default max concurrent tasks should be 4", Queue->Config.MaxConcurrentTasks, 4);
	TestEqual("Default max history size should be 100", Queue->Config.MaxHistorySize, 100);
	TestEqual("Default timeout should be 120000ms", Queue->Config.DefaultTimeoutMs, 120000u);
	TestEqual("Default result retention should be 300s", Queue->Config.ResultRetentionSeconds, 300);

	return true;
}

// ============================================================================
// KNOWN ISSUE: FRunnableThread tests and the GameThread deadlock.
//
// ROOT CAUSE: The UE automation test framework runs tests on the game thread.
// When FMCPTaskQueue::Start() creates an FRunnableThread, the worker thread
// dispatches tool execution back to the game thread via FTSTicker / AsyncTask.
// With IMPLEMENT_SIMPLE_AUTOMATION_TEST (synchronous), the GameThread is
// blocked inside RunTest(), so the dispatch never completes — DEADLOCK.
//
// LATENT TEST APPROACH (attempted 2026-03-31):
// Using IMPLEMENT_COMPLEX_AUTOMATION_TEST with ADD_LATENT_AUTOMATION_COMMAND,
// the test's RunTest() enqueues latent commands and returns immediately.
// The GameThread is then free to process ticks between latent command
// Update() calls, which should allow FTSTicker dispatches from the worker
// thread to execute. Shutdown() via latent command also runs on a tick
// boundary where the GameThread is responsive.
//
// This approach should resolve the original deadlock because:
//   1. RunTest() returns immediately after queuing latent commands
//   2. GameThread processes ticks (FTSTicker, AsyncTask) between updates
//   3. Worker thread dispatches to GameThread complete normally
//   4. Shutdown latent command calls StopTaskQueue on a tick boundary
//
// RISK: If tool execution during tests causes editor-state side effects
// (e.g., get_level_actors accessing the editor world), results may vary
// depending on test environment state. Timeout protection is included.
//
// VERIFICATION: The task queue works correctly in normal editor operation.
// The MCP bridge uses the task queue successfully for async tool execution.
//
// REFERENCES:
//   - UE Forums: FRunnable thread freezes editor
//     https://forums.unrealengine.com/t/frunnable-thread-freezes-editor/365196
//   - UE Bug UE-352373: Deadlock in animation evaluation with threads
//   - UE Docs: Automation Driver warns against synchronous GameThread blocking
//     https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-driver-in-unreal-engine
// ============================================================================

// ===== Latent Commands for Threading Tests =====

/** Latent command: stop the task queue on a tick boundary */
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
	FStopTaskQueue, TSharedPtr<FMCPToolRegistry>, Registry);

bool FStopTaskQueue::Update()
{
	if (Registry.IsValid())
	{
		Registry->StopTaskQueue();
	}
	return true;
}

/** Latent command: start the task queue on a tick boundary */
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
	FStartTaskQueue, TSharedPtr<FMCPToolRegistry>, Registry);

bool FStartTaskQueue::Update()
{
	if (Registry.IsValid())
	{
		Registry->StartTaskQueue();
	}
	return true;
}

// ===== Task Queue Start/Stop Latent Test =====

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FMCPTaskQueue_StartStop_Latent,
	"UnrealClaude.MCP.TaskQueue.StartStopLatent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

void FMCPTaskQueue_StartStop_Latent::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("StartStopLifecycle"));
	OutTestCommands.Add(TEXT(""));
}

bool FMCPTaskQueue_StartStop_Latent::RunTest(const FString& Parameters)
{
	TSharedPtr<FMCPToolRegistry> Registry = MakeShared<FMCPToolRegistry>();

	// Start -> Start (double start safe) -> Stop -> Stop (double stop safe)
	ADD_LATENT_AUTOMATION_COMMAND(FStartTaskQueue(Registry));
	ADD_LATENT_AUTOMATION_COMMAND(FStartTaskQueue(Registry));
	ADD_LATENT_AUTOMATION_COMMAND(FStopTaskQueue(Registry));
	ADD_LATENT_AUTOMATION_COMMAND(FStopTaskQueue(Registry));

	// Restart cycle
	ADD_LATENT_AUTOMATION_COMMAND(FStartTaskQueue(Registry));
	ADD_LATENT_AUTOMATION_COMMAND(FStopTaskQueue(Registry));

	return true;
}

// ===== Task Submission Latent Tests =====

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FMCPTaskQueue_SubmitValidTool_Latent,
	"UnrealClaude.MCP.TaskQueue.SubmitValidToolLatent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

void FMCPTaskQueue_SubmitValidTool_Latent::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("SubmitValidTool"));
	OutTestCommands.Add(TEXT(""));
}

bool FMCPTaskQueue_SubmitValidTool_Latent::RunTest(
	const FString& Parameters)
{
	TSharedPtr<FMCPToolRegistry> Registry = MakeShared<FMCPToolRegistry>();
	Registry->StartTaskQueue();
	TSharedPtr<FMCPTaskQueue> Queue = Registry->GetTaskQueue();

	TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
	FGuid TaskId = Queue->SubmitTask(TEXT("get_level_actors"), Params);

	TestTrue(TEXT("Should return valid task ID"), TaskId.IsValid());

	TSharedPtr<FMCPAsyncTask> Task = Queue->GetTask(TaskId);
	TestNotNull(TEXT("Task should be retrievable"), Task.Get());
	if (Task.IsValid())
	{
		TestEqual(TEXT("Tool name should match"),
			Task->ToolName, TEXT("get_level_actors"));
	}

	ADD_LATENT_AUTOMATION_COMMAND(FStopTaskQueue(Registry));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FMCPTaskQueue_SubmitInvalidTool_Latent,
	"UnrealClaude.MCP.TaskQueue.SubmitInvalidToolLatent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

void FMCPTaskQueue_SubmitInvalidTool_Latent::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("SubmitInvalidTool"));
	OutTestCommands.Add(TEXT(""));
}

bool FMCPTaskQueue_SubmitInvalidTool_Latent::RunTest(
	const FString& Parameters)
{
	TSharedPtr<FMCPToolRegistry> Registry = MakeShared<FMCPToolRegistry>();
	Registry->StartTaskQueue();
	TSharedPtr<FMCPTaskQueue> Queue = Registry->GetTaskQueue();

	TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
	FGuid TaskId = Queue->SubmitTask(TEXT("nonexistent_tool_xyz"), Params);

	TestFalse(TEXT("Should return invalid task ID for unknown tool"),
		TaskId.IsValid());

	ADD_LATENT_AUTOMATION_COMMAND(FStopTaskQueue(Registry));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FMCPTaskQueue_SubmitWithTimeout_Latent,
	"UnrealClaude.MCP.TaskQueue.SubmitWithTimeoutLatent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

void FMCPTaskQueue_SubmitWithTimeout_Latent::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("SubmitWithTimeout"));
	OutTestCommands.Add(TEXT(""));
}

bool FMCPTaskQueue_SubmitWithTimeout_Latent::RunTest(
	const FString& Parameters)
{
	TSharedPtr<FMCPToolRegistry> Registry = MakeShared<FMCPToolRegistry>();
	Registry->StartTaskQueue();
	TSharedPtr<FMCPTaskQueue> Queue = Registry->GetTaskQueue();

	TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
	FGuid TaskId = Queue->SubmitTask(
		TEXT("get_level_actors"), Params, 30000);

	TestTrue(TEXT("Should return valid task ID"), TaskId.IsValid());

	TSharedPtr<FMCPAsyncTask> Task = Queue->GetTask(TaskId);
	TestNotNull(TEXT("Task should be retrievable"), Task.Get());
	if (Task.IsValid())
	{
		TestEqual(TEXT("Custom timeout should be set"),
			Task->TimeoutMs, 30000u);
	}

	ADD_LATENT_AUTOMATION_COMMAND(FStopTaskQueue(Registry));
	return true;
}

// ===== Task Status Latent Tests =====

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FMCPTaskQueue_GetTaskStatus_Latent,
	"UnrealClaude.MCP.TaskQueue.GetTaskStatusLatent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

void FMCPTaskQueue_GetTaskStatus_Latent::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("GetTaskStatus"));
	OutTestCommands.Add(TEXT(""));
}

bool FMCPTaskQueue_GetTaskStatus_Latent::RunTest(
	const FString& Parameters)
{
	TSharedPtr<FMCPToolRegistry> Registry = MakeShared<FMCPToolRegistry>();
	Registry->StartTaskQueue();
	TSharedPtr<FMCPTaskQueue> Queue = Registry->GetTaskQueue();

	TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
	FGuid TaskId = Queue->SubmitTask(TEXT("get_level_actors"), Params);

	TSharedPtr<FMCPAsyncTask> Task = Queue->GetTask(TaskId);
	TestNotNull(TEXT("Task should exist"), Task.Get());

	if (Task.IsValid())
	{
		EMCPTaskStatus Status = Task->Status.Load();
		TestTrue(TEXT("Initial status should be pending or running"),
			Status == EMCPTaskStatus::Pending
			|| Status == EMCPTaskStatus::Running);
		TestTrue(TEXT("SubmittedTime should be set"),
			Task->SubmittedTime > FDateTime::MinValue());
	}

	ADD_LATENT_AUTOMATION_COMMAND(FStopTaskQueue(Registry));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FMCPTaskQueue_GetNonExistentTask_Latent,
	"UnrealClaude.MCP.TaskQueue.GetNonExistentTaskLatent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

void FMCPTaskQueue_GetNonExistentTask_Latent::GetTests(
	TArray<FString>& OutBeautifiedNames,
	TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("GetNonExistentTask"));
	OutTestCommands.Add(TEXT(""));
}

bool FMCPTaskQueue_GetNonExistentTask_Latent::RunTest(
	const FString& Parameters)
{
	TSharedPtr<FMCPToolRegistry> Registry = MakeShared<FMCPToolRegistry>();
	Registry->StartTaskQueue();
	TSharedPtr<FMCPTaskQueue> Queue = Registry->GetTaskQueue();

	FGuid FakeId = FGuid::NewGuid();
	TSharedPtr<FMCPAsyncTask> Task = Queue->GetTask(FakeId);

	TestNull(TEXT("Non-existent task should return nullptr"), Task.Get());

	ADD_LATENT_AUTOMATION_COMMAND(FStopTaskQueue(Registry));
	return true;
}

// ===== Task Cancellation Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPTaskQueue_CancelPendingTask,
	"UnrealClaude.MCP.TaskQueue.CancelPendingTask",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPTaskQueue_CancelPendingTask::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;
	// Don't start queue - tasks will stay pending
	TSharedPtr<FMCPTaskQueue> Queue = Registry.GetTaskQueue();

	TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
	FGuid TaskId = Queue->SubmitTask(TEXT("get_level_actors"), Params);

	// Task should be pending since queue isn't running
	TSharedPtr<FMCPAsyncTask> Task = Queue->GetTask(TaskId);
	TestTrue("Task should be pending", Task->Status.Load() == EMCPTaskStatus::Pending);

	// Cancel should succeed for pending tasks
	bool bCancelled = Queue->CancelTask(TaskId);
	TestTrue("Should be able to cancel pending task", bCancelled);

	// Check status changed
	TestTrue("Status should be cancelled", Task->Status.Load() == EMCPTaskStatus::Cancelled);
	TestTrue("Task should be complete", Task->IsComplete());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPTaskQueue_CancelNonExistentTask,
	"UnrealClaude.MCP.TaskQueue.CancelNonExistentTask",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPTaskQueue_CancelNonExistentTask::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;
	TSharedPtr<FMCPTaskQueue> Queue = Registry.GetTaskQueue();

	// Try to cancel a task that doesn't exist
	FGuid FakeId = FGuid::NewGuid();
	bool bCancelled = Queue->CancelTask(FakeId);

	TestFalse("Should not be able to cancel non-existent task", bCancelled);

	return true;
}

// ===== Task Result Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPTaskQueue_GetResultIncomplete,
	"UnrealClaude.MCP.TaskQueue.GetResultIncomplete",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPTaskQueue_GetResultIncomplete::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;
	// Don't start queue so task stays pending
	TSharedPtr<FMCPTaskQueue> Queue = Registry.GetTaskQueue();

	TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
	FGuid TaskId = Queue->SubmitTask(TEXT("get_level_actors"), Params);

	// Try to get result before completion
	FMCPToolResult Result;
	bool bGotResult = Queue->GetTaskResult(TaskId, Result);

	TestFalse("Should not get result for incomplete task", bGotResult);

	return true;
}

// ===== Task Listing Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPTaskQueue_GetAllTasks,
	"UnrealClaude.MCP.TaskQueue.GetAllTasks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPTaskQueue_GetAllTasks::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;
	TSharedPtr<FMCPTaskQueue> Queue = Registry.GetTaskQueue();

	// Submit multiple tasks
	TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
	Queue->SubmitTask(TEXT("get_level_actors"), Params);
	Queue->SubmitTask(TEXT("get_output_log"), Params);

	// Get all tasks
	TArray<TSharedPtr<FMCPAsyncTask>> Tasks = Queue->GetAllTasks(true);

	TestTrue("Should have at least 2 tasks", Tasks.Num() >= 2);

	// Verify tasks are sorted by submitted time (newest first)
	if (Tasks.Num() >= 2)
	{
		TestTrue("Tasks should be sorted by submitted time (newest first)",
			Tasks[0]->SubmittedTime >= Tasks[1]->SubmittedTime);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPTaskQueue_GetStats,
	"UnrealClaude.MCP.TaskQueue.GetStats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPTaskQueue_GetStats::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;
	TSharedPtr<FMCPTaskQueue> Queue = Registry.GetTaskQueue();

	// Submit tasks without starting queue (they'll be pending)
	TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
	Queue->SubmitTask(TEXT("get_level_actors"), Params);
	Queue->SubmitTask(TEXT("get_output_log"), Params);

	// Get stats
	int32 Pending, Running, Completed;
	Queue->GetStats(Pending, Running, Completed);

	TestTrue("Should have pending tasks", Pending >= 2);
	TestEqual("Should have no running tasks (queue not started)", Running, 0);
	TestEqual("Should have no completed tasks", Completed, 0);

	return true;
}

// ===== Task Status String Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPAsyncTask_StatusToString,
	"UnrealClaude.MCP.TaskQueue.StatusToString",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPAsyncTask_StatusToString::RunTest(const FString& Parameters)
{
	TestEqual("Pending status string", FMCPAsyncTask::StatusToString(EMCPTaskStatus::Pending), TEXT("pending"));
	TestEqual("Running status string", FMCPAsyncTask::StatusToString(EMCPTaskStatus::Running), TEXT("running"));
	TestEqual("Completed status string", FMCPAsyncTask::StatusToString(EMCPTaskStatus::Completed), TEXT("completed"));
	TestEqual("Failed status string", FMCPAsyncTask::StatusToString(EMCPTaskStatus::Failed), TEXT("failed"));
	TestEqual("Cancelled status string", FMCPAsyncTask::StatusToString(EMCPTaskStatus::Cancelled), TEXT("cancelled"));
	TestEqual("TimedOut status string", FMCPAsyncTask::StatusToString(EMCPTaskStatus::TimedOut), TEXT("timed_out"));

	return true;
}

// ===== Task JSON Serialization Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPAsyncTask_ToJson,
	"UnrealClaude.MCP.TaskQueue.TaskToJson",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPAsyncTask_ToJson::RunTest(const FString& Parameters)
{
	FMCPAsyncTask Task;
	Task.ToolName = TEXT("test_tool");
	Task.Status.Store(EMCPTaskStatus::Pending);
	Task.Progress.Store(50);
	Task.ProgressMessage = TEXT("Processing...");

	TSharedPtr<FJsonObject> Json = Task.ToJson(false);

	TestTrue("JSON should have task_id", Json->HasField(TEXT("task_id")));
	TestEqual("JSON tool_name should match", Json->GetStringField(TEXT("tool_name")), TEXT("test_tool"));
	TestEqual("JSON status should be pending", Json->GetStringField(TEXT("status")), TEXT("pending"));
	TestEqual("JSON progress should be 50", Json->GetIntegerField(TEXT("progress")), 50);
	TestEqual("JSON progress_message should match", Json->GetStringField(TEXT("progress_message")), TEXT("Processing..."));
	TestTrue("JSON should have submitted_at", Json->HasField(TEXT("submitted_at")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPAsyncTask_ToJsonWithResult,
	"UnrealClaude.MCP.TaskQueue.TaskToJsonWithResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPAsyncTask_ToJsonWithResult::RunTest(const FString& Parameters)
{
	FMCPAsyncTask Task;
	Task.ToolName = TEXT("test_tool");
	Task.Status.Store(EMCPTaskStatus::Completed);
	Task.StartedTime = FDateTime::UtcNow() - FTimespan::FromSeconds(1);
	Task.CompletedTime = FDateTime::UtcNow();
	Task.Result = FMCPToolResult::Success(TEXT("Test completed"));

	TSharedPtr<FJsonObject> Json = Task.ToJson(true);

	TestTrue("JSON should have completed_at", Json->HasField(TEXT("completed_at")));
	TestTrue("JSON should have duration_ms", Json->HasField(TEXT("duration_ms")));
	TestTrue("JSON should have success field", Json->GetBoolField(TEXT("success")));
	TestEqual("JSON message should match", Json->GetStringField(TEXT("message")), TEXT("Test completed"));

	return true;
}

// ===== Task IsComplete Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPAsyncTask_IsComplete,
	"UnrealClaude.MCP.TaskQueue.TaskIsComplete",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPAsyncTask_IsComplete::RunTest(const FString& Parameters)
{
	FMCPAsyncTask Task;

	// Non-terminal states
	Task.Status.Store(EMCPTaskStatus::Pending);
	TestFalse("Pending should not be complete", Task.IsComplete());

	Task.Status.Store(EMCPTaskStatus::Running);
	TestFalse("Running should not be complete", Task.IsComplete());

	// Terminal states
	Task.Status.Store(EMCPTaskStatus::Completed);
	TestTrue("Completed should be complete", Task.IsComplete());

	Task.Status.Store(EMCPTaskStatus::Failed);
	TestTrue("Failed should be complete", Task.IsComplete());

	Task.Status.Store(EMCPTaskStatus::Cancelled);
	TestTrue("Cancelled should be complete", Task.IsComplete());

	Task.Status.Store(EMCPTaskStatus::TimedOut);
	TestTrue("TimedOut should be complete", Task.IsComplete());

	return true;
}

// ===== Queue Capacity Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMCPTaskQueue_ConfigOverride,
	"UnrealClaude.MCP.TaskQueue.ConfigOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FMCPTaskQueue_ConfigOverride::RunTest(const FString& Parameters)
{
	FMCPToolRegistry Registry;
	TSharedPtr<FMCPTaskQueue> Queue = Registry.GetTaskQueue();

	// Override config
	Queue->Config.MaxConcurrentTasks = 2;
	Queue->Config.MaxHistorySize = 10;
	Queue->Config.DefaultTimeoutMs = 5000;

	TestEqual("Config should be overridable - MaxConcurrentTasks", Queue->Config.MaxConcurrentTasks, 2);
	TestEqual("Config should be overridable - MaxHistorySize", Queue->Config.MaxHistorySize, 10);
	TestEqual("Config should be overridable - DefaultTimeoutMs", Queue->Config.DefaultTimeoutMs, 5000u);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
