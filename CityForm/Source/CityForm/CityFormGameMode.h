// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CityFormGameMode.generated.h"

/**
 * Project-owned gameplay configuration for City Form's Unreal presentation.
 * Authoritative city state belongs to CitySimulation, never to this class.
 */
UCLASS()
class CITYFORM_API ACityFormGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACityFormGameMode();
};
