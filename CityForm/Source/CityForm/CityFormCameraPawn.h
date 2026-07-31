// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "CityFormCameraTuning.h"
#include "CityFormCameraPawn.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USceneComponent;
class USpringArmComponent;

/** Ground-focused presentation pawn for navigating and editing a city. */
UCLASS()
class CITYFORM_API ACityFormCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ACityFormCameraPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "City Form|Camera")
	void SetCameraInputSuppressed(bool bSuppressed);

	UFUNCTION(BlueprintPure, Category = "City Form|Camera")
	bool IsCameraInputSuppressed() const
	{
		return bInputSuppressed;
	}

	UFUNCTION(BlueprintCallable, Category = "City Form|Camera")
	void SetEdgeScrollingEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "City Form|Camera")
	bool IsEdgeScrollingEnabled() const
	{
		return bEdgeScrollingEnabled;
	}

	UFUNCTION(BlueprintPure, Category = "City Form|Camera")
	FVector GetFocusLocation() const
	{
		return GetActorLocation();
	}

	UFUNCTION(BlueprintPure, Category = "City Form|Camera")
	double GetZoomDistance() const;

	UFUNCTION(BlueprintPure, Category = "City Form|Camera")
	double GetCameraYaw() const;

	UFUNCTION(BlueprintPure, Category = "City Form|Camera")
	double GetCameraPitch() const;

	static double ComputeSmoothingAlpha(double Response, double DeltaSeconds);
	static double ComputePanSpeed(const FCityFormCameraTuning& InTuning, double ZoomDistance);
	static FVector2D CombinePanInput(const FVector2D& KeyboardInput, const FVector2D& EdgeInput);
	static FVector ClampFocusToBounds(const FCityFormCameraTuning& InTuning, const FVector& Focus);

protected:
	virtual void BeginPlay() override;

private:
	void ConfigureRuntimeMappings();
	FVector2D GetEdgePanInput() const;
	void HandlePan(const FInputActionValue& Value);
	void HandlePanCompleted(const FInputActionValue& Value);
	void HandleScrollZoom(const FInputActionValue& Value);
	void HandleKeyboardZoom(const FInputActionValue& Value);
	void HandleKeyboardZoomCompleted(const FInputActionValue& Value);
	void HandleRotate(const FInputActionValue& Value);
	void HandleRotateCompleted(const FInputActionValue& Value);
	void HandleTilt(const FInputActionValue& Value);
	void HandleTiltCompleted(const FInputActionValue& Value);
	void HandleOrbitHeld(const FInputActionValue& Value);
	void HandleOrbitReleased(const FInputActionValue& Value);
	void HandleOrbit(const FInputActionValue& Value);
	void ClearContinuousInput();

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USceneComponent> FocusRoot;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, Category = "Camera")
	FCityFormCameraTuning Tuning;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> MappingContextAsset;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RuntimeMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> PanAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ScrollZoomAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> KeyboardZoomAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> RotateAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> TiltAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> OrbitHeldAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> OrbitAction;

	FVector TargetFocus = FVector::ZeroVector;
	double TargetZoom = 75000.0;
	double TargetYaw = -45.0;
	double TargetPitch = -55.0;
	FVector2D PanInput = FVector2D::ZeroVector;
	double KeyboardZoomInput = 0.0;
	double RotateInput = 0.0;
	double TiltInput = 0.0;
	bool bOrbitHeld = false;
	bool bInputSuppressed = false;
	bool bEdgeScrollingEnabled = true;
};
