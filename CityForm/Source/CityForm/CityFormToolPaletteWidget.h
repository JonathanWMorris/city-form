// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CityFormToolMode.h"

#include "CityFormToolPaletteWidget.generated.h"

class ACityFormPlayerController;
class UBorder;
class UButton;
class USizeBox;
class UTextBlock;

/** Minimal source-defined build toolbar that can grow with the walking skeleton. */
UCLASS()
class CITYFORM_API UCityFormToolPaletteWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForController(ACityFormPlayerController* InController);
	void SetSelectedTool(ECityFormToolMode ToolMode);
	void SetOpenCategory(ECityFormToolCategory Category);
	void SetStatus(const FString& Message, bool bIsError = false);
	bool IsPointerOverPalette() const;

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void HandleRoadCategoryClicked();

	UFUNCTION()
	void HandleBasicRoadClicked();

	UFUNCTION()
	void HandleZoningCategoryClicked();

	UFUNCTION()
	void HandleResidentialClicked();

	UFUNCTION()
	void HandleCommercialClicked();

	UFUNCTION()
	void HandleClearZoneClicked();

	UFUNCTION()
	void HandleAdvanceFiveMinutesClicked();

	TObjectPtr<ACityFormPlayerController> Controller;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PaletteBorder;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> DockSize;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RoadTray;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ZoningTray;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RoadCategoryButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> BasicRoadButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ZoningCategoryButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResidentialButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CommercialButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ClearZoneButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;
};
