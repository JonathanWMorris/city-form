// Copyright Epic Games, Inc. All Rights Reserved.

#include "CityFormGameMode.h"
#include "CityFormCameraPawn.h"
#include "CityFormPlayerController.h"
#include "CityFormRoadVisualizationActor.h"
#include "Engine/World.h"

ACityFormGameMode::ACityFormGameMode()
{
	DefaultPawnClass = ACityFormCameraPawn::StaticClass();
	PlayerControllerClass = ACityFormPlayerController::StaticClass();
}

void ACityFormGameMode::BeginPlay()
{
	Super::BeginPlay();
	RoadVisualization = GetWorld()->SpawnActor<ACityFormRoadVisualizationActor>();
	ensureMsgf(RoadVisualization != nullptr, TEXT("City Form could not create its derived road presentation."));
}
