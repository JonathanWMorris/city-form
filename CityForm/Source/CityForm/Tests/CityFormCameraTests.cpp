// Copyright Jonathan Morris. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "../CityFormCameraPawn.h"
#include "../CityFormCameraTuning.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"

namespace
{
constexpr EAutomationTestFlags CameraTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCityFormCameraTuningTest,
	"CityForm.Presentation.Camera.Tuning",
	CameraTestFlags)

bool FCityFormCameraTuningTest::RunTest(const FString& Parameters)
{
	FCityFormCameraTuning Tuning;
	FString Error;
	TestTrue(TEXT("Default camera tuning is valid."), Tuning.IsValid(&Error));
	TestTrue(TEXT("Valid tuning has no error message."), Error.IsEmpty());

	Tuning.InitialZoom = Tuning.MaximumZoom + 1.0;
	TestFalse(TEXT("Initial zoom outside the supported range is rejected."), Tuning.IsValid(&Error));
	TestFalse(TEXT("Invalid tuning produces an actionable message."), Error.IsEmpty());

	Tuning = FCityFormCameraTuning();
	Tuning.BuildableMinimum.X = Tuning.BuildableMaximum.X;
	TestFalse(TEXT("Empty buildable bounds are rejected."), Tuning.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCityFormCameraBoundsAndSpeedTest,
	"CityForm.Presentation.Camera.BoundsAndSpeed",
	CameraTestFlags)

bool FCityFormCameraBoundsAndSpeedTest::RunTest(const FString& Parameters)
{
	const FCityFormCameraTuning Tuning;
	const FVector Clamped = ACityFormCameraPawn::ClampFocusToBounds(
		Tuning,
		FVector(Tuning.BuildableMaximum.X + 500.0, Tuning.BuildableMinimum.Y - 500.0, 200.0));
	TestEqual(TEXT("Focus clamps at the maximum X bound."), Clamped.X, Tuning.BuildableMaximum.X);
	TestEqual(TEXT("Focus clamps at the minimum Y bound."), Clamped.Y, Tuning.BuildableMinimum.Y);
	TestEqual(TEXT("Ground focus remains on the ground plane."), Clamped.Z, 0.0);

	TestEqual(
		TEXT("Minimum zoom uses minimum pan speed."),
		ACityFormCameraPawn::ComputePanSpeed(Tuning, Tuning.MinimumZoom),
		Tuning.MinimumPanSpeed);
	TestEqual(
		TEXT("Maximum zoom uses maximum pan speed."),
		ACityFormCameraPawn::ComputePanSpeed(Tuning, Tuning.MaximumZoom),
		Tuning.MaximumPanSpeed);
	TestTrue(
		TEXT("Pan speed increases between the zoom limits."),
		ACityFormCameraPawn::ComputePanSpeed(Tuning, Tuning.InitialZoom) > Tuning.MinimumPanSpeed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCityFormCameraInputMathTest,
	"CityForm.Presentation.Camera.InputMath",
	CameraTestFlags)

bool FCityFormCameraInputMathTest::RunTest(const FString& Parameters)
{
	const FVector2D Diagonal = ACityFormCameraPawn::CombinePanInput(FVector2D(1.0, 1.0), FVector2D::ZeroVector);
	TestTrue(TEXT("Diagonal movement is normalized."), FMath::IsNearlyEqual(Diagonal.Size(), 1.0, 1.e-6));

	const double Response = 12.0;
	double AtThirtyHertz = 0.0;
	double AtSixtyHertz = 0.0;
	for (int32 Step = 0; Step < 30; ++Step)
	{
		AtThirtyHertz = FMath::Lerp(AtThirtyHertz, 1.0, ACityFormCameraPawn::ComputeSmoothingAlpha(Response, 1.0 / 30.0));
	}
	for (int32 Step = 0; Step < 60; ++Step)
	{
		AtSixtyHertz = FMath::Lerp(AtSixtyHertz, 1.0, ACityFormCameraPawn::ComputeSmoothingAlpha(Response, 1.0 / 60.0));
	}
	TestTrue(
		TEXT("Exponential smoothing is stable across frame cadences."),
		FMath::IsNearlyEqual(AtThirtyHertz, AtSixtyHertz, 1.e-9));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCityFormCameraInputAssetsTest,
	"CityForm.Presentation.Camera.InputAssets",
	CameraTestFlags)

bool FCityFormCameraInputAssetsTest::RunTest(const FString& Parameters)
{
	const UInputAction* Pan = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_CameraPan.IA_CameraPan"));
	const UInputAction* Zoom = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_CameraZoom.IA_CameraZoom"));
	const UInputMappingContext* Context = LoadObject<UInputMappingContext>(
		nullptr,
		TEXT("/Game/Input/IMC_CityBuilderCamera.IMC_CityBuilderCamera"));
	TestNotNull(TEXT("The camera pan action is available."), Pan);
	TestNotNull(TEXT("The wheel and trackpad zoom action is available."), Zoom);
	TestNotNull(TEXT("The city-builder mapping context is available."), Context);
	if (Pan != nullptr)
	{
		TestTrue(TEXT("Pan produces a two-dimensional value."), Pan->ValueType == EInputActionValueType::Axis2D);
		TestTrue(
			TEXT("Simultaneous pan keys accumulate for diagonal movement."),
			Pan->AccumulationBehavior == EInputActionAccumulationBehavior::Cumulative);
	}
	if (Zoom != nullptr)
	{
		TestTrue(TEXT("Scroll zoom produces a one-dimensional value."), Zoom->ValueType == EInputActionValueType::Axis1D);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
