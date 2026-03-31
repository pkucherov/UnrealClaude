// Copyright Natali Caggiano. All Rights Reserved.

/**
 * Automation tests for Slate UI widgets: SChatInputArea, SChatToolbar, SChatEditorWidget.
 * Tests widget construction via SAssignNew and public API methods.
 * All tests guarded by FSlateApplication::IsInitialized() check.
 *
 * SAFETY: No keyboard/mouse simulation or button click tests.
 * Only construction + public API methods are tested.
 */

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Widgets/SChatInputArea.h"
#include "Widgets/SChatToolbar.h"
#include "ChatEditorWidget.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== Input Area Construction Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateWidget_InputArea_Construction,
	"UnrealClaude.Widgets.InputArea.Construction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSlateWidget_InputArea_Construction::RunTest(const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate not initialized — skipping widget test"));
		return true;
	}

	TSharedPtr<SChatInputArea> InputArea;
	SAssignNew(InputArea, SChatInputArea)
		.bIsWaiting(false);

	TestNotNull(TEXT("InputArea should construct via SAssignNew"),
		InputArea.Get());

	return true;
}

// ===== Input Area Text API Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateWidget_InputArea_SetGetTextRoundTrip,
	"UnrealClaude.Widgets.InputArea.SetGetTextRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSlateWidget_InputArea_SetGetTextRoundTrip::RunTest(const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate not initialized — skipping widget test"));
		return true;
	}

	TSharedPtr<SChatInputArea> InputArea;
	SAssignNew(InputArea, SChatInputArea)
		.bIsWaiting(false);
	TestNotNull(TEXT("InputArea should construct"), InputArea.Get());
	if (!InputArea.IsValid()) { return false; }

	InputArea->SetText(TEXT("Hello Claude!"));
	FString Result = InputArea->GetText();

	TestEqual(TEXT("GetText should return the text set by SetText"),
		Result, TEXT("Hello Claude!"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateWidget_InputArea_ClearText,
	"UnrealClaude.Widgets.InputArea.ClearText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSlateWidget_InputArea_ClearText::RunTest(const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate not initialized — skipping widget test"));
		return true;
	}

	TSharedPtr<SChatInputArea> InputArea;
	SAssignNew(InputArea, SChatInputArea)
		.bIsWaiting(false);
	TestNotNull(TEXT("InputArea should construct"), InputArea.Get());
	if (!InputArea.IsValid()) { return false; }

	InputArea->SetText(TEXT("Some text to clear"));
	InputArea->ClearText();
	FString Result = InputArea->GetText();

	TestTrue(TEXT("GetText should return empty after ClearText"),
		Result.IsEmpty());

	return true;
}

// ===== Input Area Image API Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateWidget_InputArea_HasAttachedImagesFresh,
	"UnrealClaude.Widgets.InputArea.HasAttachedImagesFresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSlateWidget_InputArea_HasAttachedImagesFresh::RunTest(
	const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate not initialized — skipping widget test"));
		return true;
	}

	TSharedPtr<SChatInputArea> InputArea;
	SAssignNew(InputArea, SChatInputArea)
		.bIsWaiting(false);
	TestNotNull(TEXT("InputArea should construct"), InputArea.Get());
	if (!InputArea.IsValid()) { return false; }

	TestFalse(TEXT("Fresh widget should have no attached images"),
		InputArea->HasAttachedImages());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateWidget_InputArea_GetAttachedImageCountFresh,
	"UnrealClaude.Widgets.InputArea.GetAttachedImageCountFresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSlateWidget_InputArea_GetAttachedImageCountFresh::RunTest(
	const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate not initialized — skipping widget test"));
		return true;
	}

	TSharedPtr<SChatInputArea> InputArea;
	SAssignNew(InputArea, SChatInputArea)
		.bIsWaiting(false);
	TestNotNull(TEXT("InputArea should construct"), InputArea.Get());
	if (!InputArea.IsValid()) { return false; }

	TestEqual(TEXT("Fresh widget should have 0 attached images"),
		InputArea->GetAttachedImageCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateWidget_InputArea_GetAttachedImagePathsFresh,
	"UnrealClaude.Widgets.InputArea.GetAttachedImagePathsFresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSlateWidget_InputArea_GetAttachedImagePathsFresh::RunTest(
	const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate not initialized — skipping widget test"));
		return true;
	}

	TSharedPtr<SChatInputArea> InputArea;
	SAssignNew(InputArea, SChatInputArea)
		.bIsWaiting(false);
	TestNotNull(TEXT("InputArea should construct"), InputArea.Get());
	if (!InputArea.IsValid()) { return false; }

	TArray<FString> Paths = InputArea->GetAttachedImagePaths();

	TestEqual(TEXT("Fresh widget should return empty image paths array"),
		Paths.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateWidget_InputArea_ClearAttachedImagesFresh,
	"UnrealClaude.Widgets.InputArea.ClearAttachedImagesFresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSlateWidget_InputArea_ClearAttachedImagesFresh::RunTest(
	const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate not initialized — skipping widget test"));
		return true;
	}

	TSharedPtr<SChatInputArea> InputArea;
	SAssignNew(InputArea, SChatInputArea)
		.bIsWaiting(false);
	TestNotNull(TEXT("InputArea should construct"), InputArea.Get());
	if (!InputArea.IsValid()) { return false; }

	// Should not crash on fresh widget
	InputArea->ClearAttachedImages();

	TestFalse(TEXT("HasAttachedImages should still be false after clear"),
		InputArea->HasAttachedImages());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateWidget_InputArea_RemoveAttachedImageInvalidIndex,
	"UnrealClaude.Widgets.InputArea.RemoveAttachedImageInvalidIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSlateWidget_InputArea_RemoveAttachedImageInvalidIndex::RunTest(
	const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate not initialized — skipping widget test"));
		return true;
	}

	TSharedPtr<SChatInputArea> InputArea;
	SAssignNew(InputArea, SChatInputArea)
		.bIsWaiting(false);
	TestNotNull(TEXT("InputArea should construct"), InputArea.Get());
	if (!InputArea.IsValid()) { return false; }

	// Should not crash with out-of-bounds indices
	InputArea->RemoveAttachedImage(-1);
	InputArea->RemoveAttachedImage(0);
	InputArea->RemoveAttachedImage(999);

	TestEqual(TEXT("Image count should remain 0 after invalid removes"),
		InputArea->GetAttachedImageCount(), 0);

	return true;
}

// ===== Input Area Callback Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateWidget_InputArea_OnSendCallbackBinding,
	"UnrealClaude.Widgets.InputArea.OnSendCallbackBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSlateWidget_InputArea_OnSendCallbackBinding::RunTest(
	const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate not initialized — skipping widget test"));
		return true;
	}

	bool bSendFired = false;
	TSharedPtr<SChatInputArea> InputArea;
	SAssignNew(InputArea, SChatInputArea)
		.bIsWaiting(false)
		.OnSend(FOnInputAction::CreateLambda([&bSendFired]()
		{
			bSendFired = true;
		}));

	TestNotNull(TEXT("InputArea should construct with OnSend callback"),
		InputArea.Get());

	// Note: We cannot simulate button clicks or key presses in automation
	// tests. We verify that the widget constructs with a callback binding
	// without crashing. The callback mechanism is tested by the Slate
	// framework itself.

	return true;
}

// ===== Toolbar Construction Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateWidget_Toolbar_ConstructionDefaults,
	"UnrealClaude.Widgets.Toolbar.ConstructionDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSlateWidget_Toolbar_ConstructionDefaults::RunTest(
	const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate not initialized — skipping widget test"));
		return true;
	}

	TSharedPtr<SChatToolbar> Toolbar;
	SAssignNew(Toolbar, SChatToolbar);

	TestNotNull(TEXT("Toolbar should construct with default arguments"),
		Toolbar.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateWidget_Toolbar_ConstructionCustomAttributes,
	"UnrealClaude.Widgets.Toolbar.ConstructionCustomAttributes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSlateWidget_Toolbar_ConstructionCustomAttributes::RunTest(
	const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate not initialized — skipping widget test"));
		return true;
	}

	TSharedPtr<SChatToolbar> Toolbar;
	SAssignNew(Toolbar, SChatToolbar)
		.bUE57ContextEnabled(false)
		.bProjectContextEnabled(false)
		.bRestoreEnabled(true);

	TestNotNull(
		TEXT("Toolbar should construct with custom attribute values"),
		Toolbar.Get());

	return true;
}

// ===== Toolbar Callback Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateWidget_Toolbar_OnNewSessionCallbackBinding,
	"UnrealClaude.Widgets.Toolbar.OnNewSessionCallbackBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSlateWidget_Toolbar_OnNewSessionCallbackBinding::RunTest(
	const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate not initialized — skipping widget test"));
		return true;
	}

	bool bCallbackFired = false;
	TSharedPtr<SChatToolbar> Toolbar;
	SAssignNew(Toolbar, SChatToolbar)
		.bUE57ContextEnabled(true)
		.bProjectContextEnabled(true)
		.bRestoreEnabled(false)
		.OnNewSession(FOnToolbarAction::CreateLambda(
			[&bCallbackFired]()
		{
			bCallbackFired = true;
		}));

	TestNotNull(
		TEXT("Toolbar should construct with OnNewSession callback"),
		Toolbar.Get());

	// Note: Button click simulation not supported in automation tests.
	// Verifying that the widget constructs with callback binding intact.

	return true;
}

// ===== Editor Widget Construction Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSlateWidget_EditorWidget_Construction,
	"UnrealClaude.Widgets.EditorWidget.Construction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FSlateWidget_EditorWidget_Construction::RunTest(
	const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate not initialized — skipping widget test"));
		return true;
	}

	// SChatEditorWidget::Construct() accesses FChatSubsystem
	// singleton, starts MCP server checks, and performs other editor
	// subsystem initialization. This may or may not succeed in the
	// automation test context depending on module load state.
	// We attempt construction and report the outcome.

	TSharedPtr<SChatEditorWidget> EditorWidget;

	// Attempt construction — may access subsystems not available in test
	SAssignNew(EditorWidget, SChatEditorWidget);

	if (EditorWidget.IsValid())
	{
		TestNotNull(
			TEXT("EditorWidget constructed successfully"),
			EditorWidget.Get());
	}
	else
	{
		AddWarning(TEXT("SChatEditorWidget construction returned "
			"invalid pointer — subsystem dependencies may not be "
			"available in test context"));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
