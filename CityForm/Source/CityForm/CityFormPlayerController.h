// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CityFormPlayerController.generated.h"

/** Player-controller policy for a cursor-driven city builder. */
UCLASS()
class CITYFORM_API ACityFormPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
