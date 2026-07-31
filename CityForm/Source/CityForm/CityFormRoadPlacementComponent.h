// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CityFormSimulationSubsystem.h"
#include "CityFormToolMode.h"

#include "CityFormRoadPlacementComponent.generated.h"

enum class ERoadPlacementState : uint8
{
	Idle,
	StartSelected
};

struct FCityFormRoadPlacementState
{
	ERoadPlacementState State = ERoadPlacementState::Idle;
	FCityFormRoadEndpointInput StartEndpoint;
	FVector StartPosition = FVector::ZeroVector;

	void SelectStart(FCityFormRoadEndpointInput Endpoint, const FVector& Position)
	{
		StartEndpoint = MoveTemp(Endpoint);
		StartPosition = Position;
		State = ERoadPlacementState::StartSelected;
	}

	void Clear()
	{
		State = ERoadPlacementState::Idle;
		StartEndpoint = {};
		StartPosition = FVector::ZeroVector;
	}
};

/** Cursor-driven road placement state that submits only atomic simulation commands. */
UCLASS(ClassGroup = "City Form")
class CITYFORM_API UCityFormRoadPlacementComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	UCityFormRoadPlacementComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	void SetToolMode(ECityFormToolMode InToolMode);
	void HandlePrimaryAction();
	bool CancelPendingPlacement();

	ECityFormToolMode GetToolMode() const { return ToolMode; }
	ERoadPlacementState GetPlacementState() const { return Placement.State; }

	static bool IsLongEnough(const FVector& Start, const FVector& End);
	static TOptional<FCityFormRoadNodeSnapshot> FindSnapCandidate(
		const TArray<FCityFormRoadNodeSnapshot>& Nodes,
		const FVector2D& CursorPosition,
		TFunctionRef<bool(const FVector&, FVector2D&)> ProjectToScreen);

private:
	bool TryGetCursorEndpoint(FCityFormRoadEndpointInput& OutEndpoint, FVector& OutPosition) const;
	void DrawPreview() const;
	void ClearPendingPlacement();
	void SetStatus(const FString& Message, bool bIsError = false) const;

	ECityFormToolMode ToolMode = ECityFormToolMode::None;
	FCityFormRoadPlacementState Placement;

	static constexpr double MinimumRoadLengthCentimeters = 100.0;
	static constexpr double EndpointSnapRadiusPixels = 12.0;
	static constexpr float PreviewHeightCentimeters = 25.0f;
};
