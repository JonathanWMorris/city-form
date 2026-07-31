// Copyright Epic Games, Inc. All Rights Reserved.

#include "CityFormGameMode.h"

ACityFormGameMode::ACityFormGameMode()
{
	// Stage 3 adds a project-owned city-builder pawn. Until then, do not fall
	// back to Unreal's freely flying DefaultPawn.
	DefaultPawnClass = nullptr;
}
