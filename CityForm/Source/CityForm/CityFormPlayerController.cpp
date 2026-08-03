// Copyright Epic Games, Inc. All Rights Reserved.

#include "CityFormPlayerController.h"

#include "CityFormDevelopmentVisualizationActor.h"
#include "CityFormGameMode.h"
#include "CityFormRoadPlacementComponent.h"
#include "CityFormSimulationSubsystem.h"
#include "CityFormToolPaletteWidget.h"
#include "CityFormZoningToolComponent.h"
#include "Engine/GameInstance.h"

ACityFormPlayerController::ACityFormPlayerController()
{
	RoadPlacementTool = CreateDefaultSubobject<UCityFormRoadPlacementComponent>(TEXT("RoadPlacementTool"));
	ZoningTool = CreateDefaultSubobject<UCityFormZoningToolComponent>(TEXT("ZoningTool"));
}

void ACityFormPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(
		EKeys::LeftMouseButton, IE_Pressed, this, &ACityFormPlayerController::HandlePrimaryToolAction);
	InputComponent->BindKey(
		EKeys::RightMouseButton, IE_Pressed, this, &ACityFormPlayerController::HandleCancelToolAction);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ACityFormPlayerController::HandleCancelToolAction);
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

	ToolPalette = CreateWidget<UCityFormToolPaletteWidget>(this, UCityFormToolPaletteWidget::StaticClass());
	if (ensureMsgf(ToolPalette != nullptr, TEXT("City Form could not create its tool palette.")))
	{
		ToolPalette->InitializeForController(this);
		// Unreal 5.8's viewport position helper resets anchors to the top-left, so
		// establish the offset first and the intended bottom-center anchor afterward.
		ToolPalette->SetPositionInViewport(FVector2D(0.0, -24.0), false);
		ToolPalette->SetAnchorsInViewport(FAnchors(0.5f, 1.0f));
		ToolPalette->SetAlignmentInViewport(FVector2D(0.5, 1.0));
		ToolPalette->AddToViewport(100);
		ensureMsgf(ToolPalette->IsInViewport(), TEXT("City Form could not attach its tool palette to the viewport."));
		ToolPalette->SetOpenCategory(ECityFormToolCategory::None);
	}

	if (ACityFormGameMode* GameMode = GetWorld()->GetAuthGameMode<ACityFormGameMode>())
	{
		DevelopmentVisualization = GameMode->GetDevelopmentVisualization();
	}
}

void ACityFormPlayerController::SetToolMode(const ECityFormToolMode ToolMode)
{
	ActiveToolMode = ToolMode;
	if (RoadPlacementTool != nullptr)
	{
		RoadPlacementTool->SetToolMode(ToolMode);
	}
	if (ZoningTool != nullptr)
	{
		ZoningTool->SetToolMode(ToolMode);
	}
	if (ToolPalette != nullptr)
	{
		ToolPalette->SetSelectedTool(ToolMode);
	}
}

void ACityFormPlayerController::ToggleRoadCategory()
{
	SetOpenCategory(
		OpenCategory == ECityFormToolCategory::Roads ? ECityFormToolCategory::None : ECityFormToolCategory::Roads);
}

void ACityFormPlayerController::ToggleZoningCategory()
{
	SetOpenCategory(
		OpenCategory == ECityFormToolCategory::Zoning ? ECityFormToolCategory::None : ECityFormToolCategory::Zoning);
}

void ACityFormPlayerController::AdvanceFiveMinutes()
{
	UGameInstance* GameInstance = GetGameInstance();
	UCityFormSimulationSubsystem* Simulation =
		GameInstance != nullptr ? GameInstance->GetSubsystem<UCityFormSimulationSubsystem>() : nullptr;
	if (Simulation == nullptr)
	{
		SetToolStatus(TEXT("The city simulation is unavailable."), true);
		return;
	}
	const CityForm::Simulation::FAdvanceTimeResult Result =
		Simulation->AdvanceSimulation(CityForm::Simulation::FSimulationDuration(300000));
	if (!Result.IsSuccess())
	{
		SetToolStatus(Result.Error.Message, true);
		return;
	}
	const CityForm::Simulation::FCitySummary Summary = Simulation->GetCitySummary();
	SetToolStatus(FString::Printf(TEXT("Advanced 5 minutes. Complete: %d, homes: %d, jobs: %d."),
		Summary.CompletedBuildingCount,
		Summary.ActiveHouseholdCapacity,
		Summary.ActiveJobCapacity));
}

void ACityFormPlayerController::SetToolStatus(const FString& Message, const bool bIsError)
{
	if (ToolPalette != nullptr)
	{
		ToolPalette->SetStatus(Message, bIsError);
	}
	if (bIsError)
	{
		UE_LOG(LogTemp, Warning, TEXT("City Form tool: %s"), *Message);
	}
}

bool ACityFormPlayerController::IsPointerOverToolPalette() const
{
	return ToolPalette != nullptr && ToolPalette->IsPointerOverPalette();
}

void ACityFormPlayerController::HandlePrimaryToolAction()
{
	if (IsPointerOverToolPalette())
	{
		return;
	}
	if (ActiveToolMode == ECityFormToolMode::Road && RoadPlacementTool != nullptr)
	{
		RoadPlacementTool->HandlePrimaryAction();
	}
	else if (ZoningTool != nullptr)
	{
		ZoningTool->HandlePrimaryAction();
	}
}

void ACityFormPlayerController::HandleCancelToolAction()
{
	if (RoadPlacementTool != nullptr && RoadPlacementTool->CancelPendingPlacement())
	{
		return;
	}
	if (ActiveToolMode != ECityFormToolMode::None)
	{
		SetToolMode(ECityFormToolMode::None);
		SetToolStatus(OpenCategory == ECityFormToolCategory::Roads
				? TEXT("Tool deselected. Choose a road type.")
				: TEXT("Tool deselected. Choose Residential, Commercial, or Clear Zone."));
		return;
	}
	if (OpenCategory != ECityFormToolCategory::None)
	{
		SetOpenCategory(ECityFormToolCategory::None);
	}
}

void ACityFormPlayerController::SetOpenCategory(const ECityFormToolCategory Category)
{
	if (OpenCategory == Category)
	{
		if (ToolPalette != nullptr)
		{
			ToolPalette->SetOpenCategory(Category);
		}
		return;
	}

	OpenCategory = Category;
	if (ActiveToolMode != ECityFormToolMode::None)
	{
		SetToolMode(ECityFormToolMode::None);
	}
	if (ToolPalette != nullptr)
	{
		ToolPalette->SetOpenCategory(OpenCategory);
	}
	if (DevelopmentVisualization == nullptr)
	{
		if (ACityFormGameMode* GameMode = GetWorld()->GetAuthGameMode<ACityFormGameMode>())
		{
			DevelopmentVisualization = GameMode->GetDevelopmentVisualization();
		}
	}
	if (DevelopmentVisualization != nullptr)
	{
		DevelopmentVisualization->SetParcelOverlayVisible(OpenCategory == ECityFormToolCategory::Zoning);
	}
	switch (OpenCategory)
	{
	case ECityFormToolCategory::Roads:
		SetToolStatus(TEXT("Choose Basic Two-Way Road to begin placing roads."));
		break;
	case ECityFormToolCategory::Zoning:
		SetToolStatus(TEXT("Choose Residential, Commercial, or Clear Zone. +5 Minutes advances development."));
		break;
	case ECityFormToolCategory::None:
	default:
		SetToolStatus(TEXT("Choose Roads or Zoning to open build tools."));
		break;
	}
}
