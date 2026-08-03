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
	void ToggleZoningCategory();
	void AdvanceFiveMinutes();
	void SetToolStatus(const FString& Message, bool bIsError = false);
	bool IsPointerOverToolPalette() const;

protected:
	virtual void BeginPlay() override;

private:
	void HandlePrimaryToolAction();
	void HandleCancelToolAction();
	void SetOpenCategory(ECityFormToolCategory Category);

	UPROPERTY(VisibleAnywhere, Category = "City Form|Tools")
	TObjectPtr<class UCityFormRoadPlacementComponent> RoadPlacementTool;

	UPROPERTY(VisibleAnywhere, Category = "City Form|Tools")
	TObjectPtr<class UCityFormZoningToolComponent> ZoningTool;

	UPROPERTY(Transient)
	TObjectPtr<class UCityFormToolPaletteWidget> ToolPalette;

	UPROPERTY(Transient)
	TObjectPtr<class ACityFormDevelopmentVisualizationActor> DevelopmentVisualization;

	ECityFormToolCategory OpenCategory = ECityFormToolCategory::None;
	ECityFormToolMode ActiveToolMode = ECityFormToolMode::None;
};
