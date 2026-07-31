// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CityFormToolMode.h"
#include "GameFramework/PlayerController.h"
#include "CityFormPlayerController.generated.h"

/** Player-controller policy for a cursor-driven city builder. */
UCLASS()
class CITYFORM_API ACityFormPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACityFormPlayerController();

	virtual void SetupInputComponent() override;

	void SetToolMode(ECityFormToolMode ToolMode);
	void ToggleRoadCategory();
	void SetToolStatus(const FString& Message, bool bIsError = false);
	bool IsPointerOverToolPalette() const;

protected:
	virtual void BeginPlay() override;

private:
	void HandlePrimaryToolAction();
	void HandleCancelToolAction();
	void SetRoadCategoryOpen(bool bOpen);

	UPROPERTY(VisibleAnywhere, Category = "City Form|Tools")
	TObjectPtr<class UCityFormRoadPlacementComponent> RoadPlacementTool;

	UPROPERTY(Transient)
	TObjectPtr<class UCityFormToolPaletteWidget> ToolPalette;

	bool bRoadCategoryOpen = false;
};
