// Copyright Epic Games, Inc. All Rights Reserved.

#include "CityFormCameraPawn.h"

#include "CityFormPlayerController.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
template <typename T>
T* LoadInputAsset(const TCHAR* Path)
{
	ConstructorHelpers::FObjectFinder<T> Finder(Path);
	return Finder.Succeeded() ? Finder.Object : nullptr;
}

void AddNegateModifier(UInputMappingContext* Context, FEnhancedActionKeyMapping& Mapping)
{
	Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
}

void AddYAxisSwizzle(UInputMappingContext* Context, FEnhancedActionKeyMapping& Mapping)
{
	UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Context);
	Swizzle->Order = EInputAxisSwizzle::YXZ;
	Mapping.Modifiers.Add(Swizzle);
}
}

ACityFormCameraPawn::ACityFormCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;

	FocusRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FocusRoot"));
	SetRootComponent(FocusRoot);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(FocusRoot);
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bUsePawnControlRotation = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	MappingContextAsset = LoadInputAsset<UInputMappingContext>(TEXT("/Game/Input/IMC_CityBuilderCamera.IMC_CityBuilderCamera"));
	PanAction = LoadInputAsset<UInputAction>(TEXT("/Game/Input/IA_CameraPan.IA_CameraPan"));
	ScrollZoomAction = LoadInputAsset<UInputAction>(TEXT("/Game/Input/IA_CameraZoom.IA_CameraZoom"));
	KeyboardZoomAction = LoadInputAsset<UInputAction>(TEXT("/Game/Input/IA_CameraZoomKeyboard.IA_CameraZoomKeyboard"));
	RotateAction = LoadInputAsset<UInputAction>(TEXT("/Game/Input/IA_CameraRotate.IA_CameraRotate"));
	TiltAction = LoadInputAsset<UInputAction>(TEXT("/Game/Input/IA_CameraTilt.IA_CameraTilt"));
	OrbitHeldAction = LoadInputAsset<UInputAction>(TEXT("/Game/Input/IA_CameraOrbitHeld.IA_CameraOrbitHeld"));
	OrbitAction = LoadInputAsset<UInputAction>(TEXT("/Game/Input/IA_CameraOrbit.IA_CameraOrbit"));
}

void ACityFormCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	FString ValidationError;
	if (!Tuning.IsValid(&ValidationError))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid city-builder camera tuning: %s. Using defaults."), *ValidationError);
		Tuning = FCityFormCameraTuning();
	}

	TargetFocus = ClampFocusToBounds(Tuning, GetActorLocation());
	SetActorLocation(TargetFocus);
	TargetZoom = Tuning.InitialZoom;
	TargetYaw = Tuning.InitialYaw;
	TargetPitch = Tuning.InitialPitch;
	SpringArm->TargetArmLength = TargetZoom;
	SpringArm->SetRelativeRotation(FRotator(TargetPitch, TargetYaw, 0.0));
}

void ACityFormCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	ULocalPlayer* LocalPlayer = PlayerController != nullptr ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer != nullptr
		? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()
		: nullptr;
	if (EnhancedInput == nullptr || InputSubsystem == nullptr || MappingContextAsset == nullptr ||
		PanAction == nullptr || ScrollZoomAction == nullptr || KeyboardZoomAction == nullptr ||
		RotateAction == nullptr || TiltAction == nullptr || OrbitHeldAction == nullptr || OrbitAction == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("City-builder camera Enhanced Input assets are unavailable."));
		return;
	}

	ConfigureRuntimeMappings();
	InputSubsystem->AddMappingContext(RuntimeMappingContext, 0);

	EnhancedInput->BindAction(PanAction, ETriggerEvent::Triggered, this, &ACityFormCameraPawn::HandlePan);
	EnhancedInput->BindAction(PanAction, ETriggerEvent::Completed, this, &ACityFormCameraPawn::HandlePanCompleted);
	EnhancedInput->BindAction(PanAction, ETriggerEvent::Canceled, this, &ACityFormCameraPawn::HandlePanCompleted);
	EnhancedInput->BindAction(ScrollZoomAction, ETriggerEvent::Triggered, this, &ACityFormCameraPawn::HandleScrollZoom);
	EnhancedInput->BindAction(KeyboardZoomAction, ETriggerEvent::Triggered, this, &ACityFormCameraPawn::HandleKeyboardZoom);
	EnhancedInput->BindAction(KeyboardZoomAction, ETriggerEvent::Completed, this, &ACityFormCameraPawn::HandleKeyboardZoomCompleted);
	EnhancedInput->BindAction(KeyboardZoomAction, ETriggerEvent::Canceled, this, &ACityFormCameraPawn::HandleKeyboardZoomCompleted);
	EnhancedInput->BindAction(RotateAction, ETriggerEvent::Triggered, this, &ACityFormCameraPawn::HandleRotate);
	EnhancedInput->BindAction(RotateAction, ETriggerEvent::Completed, this, &ACityFormCameraPawn::HandleRotateCompleted);
	EnhancedInput->BindAction(RotateAction, ETriggerEvent::Canceled, this, &ACityFormCameraPawn::HandleRotateCompleted);
	EnhancedInput->BindAction(TiltAction, ETriggerEvent::Triggered, this, &ACityFormCameraPawn::HandleTilt);
	EnhancedInput->BindAction(TiltAction, ETriggerEvent::Completed, this, &ACityFormCameraPawn::HandleTiltCompleted);
	EnhancedInput->BindAction(TiltAction, ETriggerEvent::Canceled, this, &ACityFormCameraPawn::HandleTiltCompleted);
	EnhancedInput->BindAction(OrbitHeldAction, ETriggerEvent::Started, this, &ACityFormCameraPawn::HandleOrbitHeld);
	EnhancedInput->BindAction(OrbitHeldAction, ETriggerEvent::Completed, this, &ACityFormCameraPawn::HandleOrbitReleased);
	EnhancedInput->BindAction(OrbitHeldAction, ETriggerEvent::Canceled, this, &ACityFormCameraPawn::HandleOrbitReleased);
	EnhancedInput->BindAction(OrbitAction, ETriggerEvent::Triggered, this, &ACityFormCameraPawn::HandleOrbit);
}

void ACityFormCameraPawn::ConfigureRuntimeMappings()
{
	RuntimeMappingContext = DuplicateObject<UInputMappingContext>(MappingContextAsset, this);
	RuntimeMappingContext->UnmapAll();

	FEnhancedActionKeyMapping& W = RuntimeMappingContext->MapKey(PanAction, EKeys::W);
	AddYAxisSwizzle(RuntimeMappingContext, W);
	FEnhancedActionKeyMapping& S = RuntimeMappingContext->MapKey(PanAction, EKeys::S);
	AddNegateModifier(RuntimeMappingContext, S);
	AddYAxisSwizzle(RuntimeMappingContext, S);
	FEnhancedActionKeyMapping& A = RuntimeMappingContext->MapKey(PanAction, EKeys::A);
	AddNegateModifier(RuntimeMappingContext, A);
	RuntimeMappingContext->MapKey(PanAction, EKeys::D);

	RuntimeMappingContext->MapKey(ScrollZoomAction, EKeys::MouseWheelAxis);
	RuntimeMappingContext->MapKey(KeyboardZoomAction, EKeys::Z);
	FEnhancedActionKeyMapping& X = RuntimeMappingContext->MapKey(KeyboardZoomAction, EKeys::X);
	AddNegateModifier(RuntimeMappingContext, X);

	FEnhancedActionKeyMapping& Q = RuntimeMappingContext->MapKey(RotateAction, EKeys::Q);
	AddNegateModifier(RuntimeMappingContext, Q);
	RuntimeMappingContext->MapKey(RotateAction, EKeys::E);

	RuntimeMappingContext->MapKey(TiltAction, EKeys::R);
	FEnhancedActionKeyMapping& F = RuntimeMappingContext->MapKey(TiltAction, EKeys::F);
	AddNegateModifier(RuntimeMappingContext, F);
	RuntimeMappingContext->MapKey(TiltAction, EKeys::Home);
	FEnhancedActionKeyMapping& End = RuntimeMappingContext->MapKey(TiltAction, EKeys::End);
	AddNegateModifier(RuntimeMappingContext, End);

	RuntimeMappingContext->MapKey(OrbitHeldAction, EKeys::MiddleMouseButton);
	RuntimeMappingContext->MapKey(OrbitAction, EKeys::Mouse2D);
}

void ACityFormCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DeltaSeconds <= 0.0f)
	{
		return;
	}

	const FVector2D CombinedPan = bInputSuppressed
		? FVector2D::ZeroVector
		: CombinePanInput(PanInput, GetEdgePanInput());
	if (!CombinedPan.IsNearlyZero())
	{
		const double YawRadians = FMath::DegreesToRadians(TargetYaw);
		const FVector Forward(FMath::Cos(YawRadians), FMath::Sin(YawRadians), 0.0);
		const FVector Right(-Forward.Y, Forward.X, 0.0);
		const double Distance = ComputePanSpeed(Tuning, TargetZoom) * DeltaSeconds;
		TargetFocus += (Forward * CombinedPan.Y + Right * CombinedPan.X) * Distance;
		TargetFocus = ClampFocusToBounds(Tuning, TargetFocus);
	}

	if (!bInputSuppressed)
	{
		TargetZoom = FMath::Clamp(
			TargetZoom - KeyboardZoomInput * Tuning.KeyboardZoomSpeed * DeltaSeconds,
			Tuning.MinimumZoom,
			Tuning.MaximumZoom);
		TargetYaw = FMath::UnwindDegrees(TargetYaw + RotateInput * Tuning.KeyboardYawSpeed * DeltaSeconds);
		TargetPitch = FMath::Clamp(
			TargetPitch + TiltInput * Tuning.KeyboardPitchSpeed * DeltaSeconds,
			Tuning.MinimumPitch,
			Tuning.MaximumPitch);
	}

	const double Alpha = ComputeSmoothingAlpha(Tuning.SmoothingResponse, DeltaSeconds);
	SetActorLocation(ClampFocusToBounds(Tuning, FMath::Lerp(GetActorLocation(), TargetFocus, Alpha)));
	SpringArm->TargetArmLength = FMath::Lerp(static_cast<double>(SpringArm->TargetArmLength), TargetZoom, Alpha);

	const FRotator CurrentRotation = SpringArm->GetRelativeRotation();
	const double SmoothedYaw = CurrentRotation.Yaw + FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TargetYaw) * Alpha;
	const double SmoothedPitch = FMath::Lerp(static_cast<double>(CurrentRotation.Pitch), TargetPitch, Alpha);
	SpringArm->SetRelativeRotation(FRotator(SmoothedPitch, SmoothedYaw, 0.0));
}

FVector2D ACityFormCameraPawn::GetEdgePanInput() const
{
	if (!bEdgeScrollingEnabled || bInputSuppressed || bOrbitHeld || Tuning.EdgeScrollThreshold <= 0)
	{
		return FVector2D::ZeroVector;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	const ACityFormPlayerController* CityFormController = Cast<ACityFormPlayerController>(PlayerController);
	if (CityFormController != nullptr && CityFormController->IsPointerOverToolPalette())
	{
		return FVector2D::ZeroVector;
	}
	const ULocalPlayer* LocalPlayer = PlayerController != nullptr ? PlayerController->GetLocalPlayer() : nullptr;
	const UGameViewportClient* ViewportClient = LocalPlayer != nullptr ? LocalPlayer->ViewportClient : nullptr;
	if (PlayerController == nullptr || ViewportClient == nullptr || ViewportClient->Viewport == nullptr ||
		!ViewportClient->Viewport->HasFocus())
	{
		return FVector2D::ZeroVector;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	int32 ViewportX = 0;
	int32 ViewportY = 0;
	PlayerController->GetViewportSize(ViewportX, ViewportY);
	if (!PlayerController->GetMousePosition(MouseX, MouseY) || ViewportX <= 0 || ViewportY <= 0 ||
		MouseX < 0.0f || MouseY < 0.0f || MouseX > ViewportX || MouseY > ViewportY)
	{
		return FVector2D::ZeroVector;
	}

	FVector2D Result = FVector2D::ZeroVector;
	if (MouseX <= Tuning.EdgeScrollThreshold)
	{
		Result.X = -1.0;
	}
	else if (MouseX >= ViewportX - Tuning.EdgeScrollThreshold)
	{
		Result.X = 1.0;
	}
	if (MouseY <= Tuning.EdgeScrollThreshold)
	{
		Result.Y = 1.0;
	}
	else if (MouseY >= ViewportY - Tuning.EdgeScrollThreshold)
	{
		Result.Y = -1.0;
	}
	return Result;
}

void ACityFormCameraPawn::HandlePan(const FInputActionValue& Value)
{
	if (!bInputSuppressed)
	{
		PanInput = Value.Get<FVector2D>();
	}
}

void ACityFormCameraPawn::HandlePanCompleted(const FInputActionValue& Value)
{
	PanInput = FVector2D::ZeroVector;
}

void ACityFormCameraPawn::HandleScrollZoom(const FInputActionValue& Value)
{
	if (!bInputSuppressed)
	{
		TargetZoom = FMath::Clamp(
			TargetZoom - Value.Get<float>() * Tuning.ScrollZoomStep,
			Tuning.MinimumZoom,
			Tuning.MaximumZoom);
	}
}

void ACityFormCameraPawn::HandleKeyboardZoom(const FInputActionValue& Value)
{
	if (!bInputSuppressed)
	{
		KeyboardZoomInput = Value.Get<float>();
	}
}

void ACityFormCameraPawn::HandleKeyboardZoomCompleted(const FInputActionValue& Value)
{
	KeyboardZoomInput = 0.0;
}

void ACityFormCameraPawn::HandleRotate(const FInputActionValue& Value)
{
	if (!bInputSuppressed)
	{
		RotateInput = Value.Get<float>();
	}
}

void ACityFormCameraPawn::HandleRotateCompleted(const FInputActionValue& Value)
{
	RotateInput = 0.0;
}

void ACityFormCameraPawn::HandleTilt(const FInputActionValue& Value)
{
	if (!bInputSuppressed)
	{
		TiltInput = Value.Get<float>();
	}
}

void ACityFormCameraPawn::HandleTiltCompleted(const FInputActionValue& Value)
{
	TiltInput = 0.0;
}

void ACityFormCameraPawn::HandleOrbitHeld(const FInputActionValue& Value)
{
	bOrbitHeld = !bInputSuppressed;
}

void ACityFormCameraPawn::HandleOrbitReleased(const FInputActionValue& Value)
{
	bOrbitHeld = false;
}

void ACityFormCameraPawn::HandleOrbit(const FInputActionValue& Value)
{
	if (!bInputSuppressed && bOrbitHeld)
	{
		const FVector2D Delta = Value.Get<FVector2D>();
		TargetYaw = FMath::UnwindDegrees(TargetYaw + Delta.X * Tuning.PointerOrbitSensitivity);
		TargetPitch = FMath::Clamp(
			TargetPitch - Delta.Y * Tuning.PointerOrbitSensitivity,
			Tuning.MinimumPitch,
			Tuning.MaximumPitch);
	}
}

void ACityFormCameraPawn::SetCameraInputSuppressed(bool bSuppressed)
{
	bInputSuppressed = bSuppressed;
	if (bInputSuppressed)
	{
		ClearContinuousInput();
	}
}

void ACityFormCameraPawn::SetEdgeScrollingEnabled(bool bEnabled)
{
	bEdgeScrollingEnabled = bEnabled;
}

void ACityFormCameraPawn::ClearContinuousInput()
{
	PanInput = FVector2D::ZeroVector;
	KeyboardZoomInput = 0.0;
	RotateInput = 0.0;
	TiltInput = 0.0;
	bOrbitHeld = false;
}

double ACityFormCameraPawn::GetZoomDistance() const
{
	return SpringArm != nullptr ? SpringArm->TargetArmLength : 0.0;
}

double ACityFormCameraPawn::GetCameraYaw() const
{
	return SpringArm != nullptr ? SpringArm->GetRelativeRotation().Yaw : 0.0;
}

double ACityFormCameraPawn::GetCameraPitch() const
{
	return SpringArm != nullptr ? SpringArm->GetRelativeRotation().Pitch : 0.0;
}

double ACityFormCameraPawn::ComputeSmoothingAlpha(double Response, double DeltaSeconds)
{
	if (Response <= 0.0 || DeltaSeconds <= 0.0)
	{
		return Response > 0.0 ? 0.0 : 1.0;
	}
	return 1.0 - FMath::Exp(-Response * DeltaSeconds);
}

double ACityFormCameraPawn::ComputePanSpeed(const FCityFormCameraTuning& InTuning, double ZoomDistance)
{
	const double Range = InTuning.MaximumZoom - InTuning.MinimumZoom;
	const double ZoomFraction = Range > UE_DOUBLE_SMALL_NUMBER
		? FMath::Clamp((ZoomDistance - InTuning.MinimumZoom) / Range, 0.0, 1.0)
		: 0.0;
	return FMath::Lerp(InTuning.MinimumPanSpeed, InTuning.MaximumPanSpeed, ZoomFraction);
}

FVector2D ACityFormCameraPawn::CombinePanInput(const FVector2D& KeyboardInput, const FVector2D& EdgeInput)
{
	return (KeyboardInput + EdgeInput).GetClampedToMaxSize(1.0);
}

FVector ACityFormCameraPawn::ClampFocusToBounds(const FCityFormCameraTuning& InTuning, const FVector& Focus)
{
	return FVector(
		FMath::Clamp(Focus.X, InTuning.BuildableMinimum.X, InTuning.BuildableMaximum.X),
		FMath::Clamp(Focus.Y, InTuning.BuildableMinimum.Y, InTuning.BuildableMaximum.Y),
		0.0);
}
