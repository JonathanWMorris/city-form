// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CityFormToolMode.h"

#include "CityFormToolPaletteWidget.generated.h"

class ACityFormPlayerController;
class UBorder;
class UButton;
class UTextBlock;

/** Minimal source-defined build toolbar that can grow with the walking skeleton. */
UCLASS()
class CITYFORM_API UCityFormToolPaletteWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForController(ACityFormPlayerController* InController);
	void SetSelectedTool(ECityFormToolMode ToolMode);
	void SetStatus(const FString& Message, bool bIsError = false);
	bool IsPointerOverPalette() const;

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void HandleRoadClicked();

	UFUNCTION()
	void HandleResidentialClicked();

	UFUNCTION()
	void HandleCommercialClicked();

	TObjectPtr<ACityFormPlayerController> Controller;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PaletteBorder;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RoadButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResidentialButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CommercialButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ToolText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;
};
