// Copyright Epic Games, Inc. All Rights Reserved.

#include "CityFormCameraTuning.h"

bool FCityFormCameraTuning::IsValid(FString* OutError) const
{
	auto Fail = [OutError](const TCHAR* Message)
	{
		if (OutError != nullptr)
		{
			*OutError = Message;
		}
		return false;
	};

	if (!BuildableMinimum.ContainsNaN() && !BuildableMaximum.ContainsNaN())
	{
		if (BuildableMinimum.X >= BuildableMaximum.X || BuildableMinimum.Y >= BuildableMaximum.Y)
		{
			return Fail(TEXT("Buildable minimum must be less than buildable maximum on both axes."));
		}
	}
	else
	{
		return Fail(TEXT("Buildable bounds must be finite."));
	}

	if (!FMath::IsFinite(MinimumPanSpeed) || !FMath::IsFinite(MaximumPanSpeed) || MinimumPanSpeed < 0.0 ||
		MaximumPanSpeed < MinimumPanSpeed)
	{
		return Fail(TEXT("Pan speeds must be finite, non-negative, and ordered."));
	}

	if (!FMath::IsFinite(MinimumZoom) || !FMath::IsFinite(MaximumZoom) || !FMath::IsFinite(InitialZoom) ||
		MinimumZoom <= 0.0 || MaximumZoom < MinimumZoom || InitialZoom < MinimumZoom || InitialZoom > MaximumZoom)
	{
		return Fail(TEXT("Zoom distances must be finite, positive, ordered, and contain the initial zoom."));
	}

	if (!FMath::IsFinite(MinimumPitch) || !FMath::IsFinite(MaximumPitch) || !FMath::IsFinite(InitialPitch) ||
		MinimumPitch < -89.0 || MaximumPitch > -1.0 || MinimumPitch > MaximumPitch || InitialPitch < MinimumPitch ||
		InitialPitch > MaximumPitch)
	{
		return Fail(TEXT("Pitch limits must be ordered within -89 to -1 degrees and contain the initial pitch."));
	}

	if (!FMath::IsFinite(InitialYaw) || !FMath::IsFinite(ScrollZoomStep) || !FMath::IsFinite(KeyboardZoomSpeed) ||
		!FMath::IsFinite(KeyboardYawSpeed) || !FMath::IsFinite(KeyboardPitchSpeed) ||
		!FMath::IsFinite(PointerOrbitSensitivity) || !FMath::IsFinite(SmoothingResponse) || ScrollZoomStep < 0.0 ||
		KeyboardZoomSpeed < 0.0 || KeyboardYawSpeed < 0.0 || KeyboardPitchSpeed < 0.0 ||
		PointerOrbitSensitivity < 0.0 || SmoothingResponse < 0.0 || EdgeScrollThreshold < 0)
	{
		return Fail(TEXT("Camera rates and thresholds must be finite and non-negative."));
	}

	if (OutError != nullptr)
	{
		OutError->Reset();
	}
	return true;
}
