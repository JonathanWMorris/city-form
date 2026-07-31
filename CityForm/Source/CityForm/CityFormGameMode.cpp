// Copyright Epic Games, Inc. All Rights Reserved.

#include "CityFormGameMode.h"
#include "CityFormCameraPawn.h"
#include "CityFormPlayerController.h"

ACityFormGameMode::ACityFormGameMode()
{
	DefaultPawnClass = ACityFormCameraPawn::StaticClass();
	PlayerControllerClass = ACityFormPlayerController::StaticClass();
}
