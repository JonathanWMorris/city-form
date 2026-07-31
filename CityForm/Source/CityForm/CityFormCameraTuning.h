// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CityFormCameraTuning.generated.h"

/** Tunable presentation values for the prototype city-builder camera. */
USTRUCT(BlueprintType)
struct CITYFORM_API FCityFormCameraTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Bounds", meta = (Units = "cm"))
	FVector2D BuildableMinimum = FVector2D(-90000.0, -90000.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Bounds", meta = (Units = "cm"))
	FVector2D BuildableMaximum = FVector2D(90000.0, 90000.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	double MinimumPanSpeed = 3000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	double MaximumPanSpeed = 60000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "1.0", Units = "cm"))
	double MinimumZoom = 2500.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "1.0", Units = "cm"))
	double MaximumZoom = 150000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "1.0", Units = "cm"))
	double InitialZoom = 75000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "0.0", Units = "cm"))
	double ScrollZoomStep = 8000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "0.0", Units = "cm/s"))
	double KeyboardZoomSpeed = 60000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMin = "-89.0", ClampMax = "-1.0", Units = "deg"))
	double MinimumPitch = -80.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMin = "-89.0", ClampMax = "-1.0", Units = "deg"))
	double MaximumPitch = -25.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMin = "-89.0", ClampMax = "-1.0", Units = "deg"))
	double InitialPitch = -55.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (Units = "deg"))
	double InitialYaw = -45.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMin = "0.0", Units = "deg/s"))
	double KeyboardYawSpeed = 90.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMin = "0.0", Units = "deg/s"))
	double KeyboardPitchSpeed = 60.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit", meta = (ClampMin = "0.0"))
	double PointerOrbitSensitivity = 0.2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Smoothing", meta = (ClampMin = "0.0"))
	double SmoothingResponse = 12.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Edge Scrolling", meta = (ClampMin = "0"))
	int32 EdgeScrollThreshold = 24;

	/** Returns false and an actionable message when the ranges cannot produce a valid camera. */
	bool IsValid(FString* OutError = nullptr) const;
};
