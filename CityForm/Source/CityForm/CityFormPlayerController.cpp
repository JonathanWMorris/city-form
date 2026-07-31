// Copyright Epic Games, Inc. All Rights Reserved.

#include "CityFormPlayerController.h"

#include "CityFormRoadPlacementComponent.h"
#include "CityFormToolPaletteWidget.h"

ACityFormPlayerController::ACityFormPlayerController()
{
	RoadPlacementTool = CreateDefaultSubobject<UCityFormRoadPlacementComponent>(TEXT("RoadPlacementTool"));
}

void ACityFormPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(
		EKeys::LeftMouseButton,
		IE_Pressed,
		this,
		&ACityFormPlayerController::HandlePrimaryToolAction);
	InputComponent->BindKey(
		EKeys::RightMouseButton,
		IE_Pressed,
		this,
		&ACityFormPlayerController::HandleCancelToolAction);
	InputComponent->BindKey(
		EKeys::Escape,
		IE_Pressed,
		this,
		&ACityFormPlayerController::HandleCancelToolAction);
}

void ACityFormPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	ToolPalette = CreateWidget<UCityFormToolPaletteWidget>(
		this,
		UCityFormToolPaletteWidget::StaticClass());
	if (ToolPalette != nullptr)
	{
		ToolPalette->InitializeForController(this);
		ToolPalette->AddToViewport(100);
		ToolPalette->SetAnchorsInViewport(FAnchors(0.5f, 1.0f));
		ToolPalette->SetAlignmentInViewport(FVector2D(0.5, 1.0));
		ToolPalette->SetPositionInViewport(FVector2D(0.0, -24.0), false);
		ToolPalette->SetRoadCategoryOpen(false);
	}
}

void ACityFormPlayerController::SetToolMode(const ECityFormToolMode ToolMode)
{
	if (RoadPlacementTool != nullptr)
	{
		RoadPlacementTool->SetToolMode(ToolMode);
	}
	if (ToolPalette != nullptr)
	{
		ToolPalette->SetSelectedTool(ToolMode);
	}
}

void ACityFormPlayerController::ToggleRoadCategory()
{
	SetRoadCategoryOpen(!bRoadCategoryOpen);
}

void ACityFormPlayerController::SetToolStatus(const FString& Message, const bool bIsError)
{
	if (ToolPalette != nullptr)
	{
		ToolPalette->SetStatus(Message, bIsError);
	}
	if (bIsError)
	{
		UE_LOG(LogTemp, Warning, TEXT("Road tool: %s"), *Message);
	}
}

bool ACityFormPlayerController::IsPointerOverToolPalette() const
{
	return ToolPalette != nullptr && ToolPalette->IsPointerOverPalette();
}

void ACityFormPlayerController::HandlePrimaryToolAction()
{
	if (!IsPointerOverToolPalette() && RoadPlacementTool != nullptr)
	{
		RoadPlacementTool->HandlePrimaryAction();
	}
}

void ACityFormPlayerController::HandleCancelToolAction()
{
	if (RoadPlacementTool != nullptr && RoadPlacementTool->CancelPendingPlacement())
	{
		return;
	}
	if (RoadPlacementTool != nullptr && RoadPlacementTool->GetToolMode() != ECityFormToolMode::None)
	{
		SetToolMode(ECityFormToolMode::None);
		SetToolStatus(TEXT("Basic Two-Way Road deselected. Choose a road type."));
		return;
	}
	if (bRoadCategoryOpen)
	{
		SetRoadCategoryOpen(false);
	}
}

void ACityFormPlayerController::SetRoadCategoryOpen(const bool bOpen)
{
	if (bRoadCategoryOpen == bOpen)
	{
		if (ToolPalette != nullptr)
		{
			ToolPalette->SetRoadCategoryOpen(bOpen);
		}
		return;
	}

	bRoadCategoryOpen = bOpen;
	if (!bRoadCategoryOpen && RoadPlacementTool != nullptr &&
		RoadPlacementTool->GetToolMode() != ECityFormToolMode::None)
	{
		SetToolMode(ECityFormToolMode::None);
	}
	if (ToolPalette != nullptr)
	{
		ToolPalette->SetRoadCategoryOpen(bRoadCategoryOpen);
	}
	SetToolStatus(
		bRoadCategoryOpen
			? TEXT("Choose Basic Two-Way Road to begin placing roads.")
			: TEXT("Choose Roads to open the road-building tools."));
}
