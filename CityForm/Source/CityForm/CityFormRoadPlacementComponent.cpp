// Copyright Jonathan Morris. All Rights Reserved.

#include "CityFormRoadPlacementComponent.h"

#include "CityFormPlayerController.h"
#include "CityFormPrototypeMapContract.h"
#include "CitySimulation/RoadType.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"

using namespace CityForm::Simulation;

UCityFormRoadPlacementComponent::UCityFormRoadPlacementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCityFormRoadPlacementComponent::TickComponent(
	const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (ToolMode == ECityFormToolMode::Road && Placement.State == ERoadPlacementState::StartSelected)
	{
		DrawPreview();
	}
}

void UCityFormRoadPlacementComponent::SetToolMode(const ECityFormToolMode InToolMode)
{
	if (ToolMode != InToolMode)
	{
		ClearPendingPlacement();
		ToolMode = InToolMode;
	}
	if (ToolMode == ECityFormToolMode::Road)
	{
		SetStatus(TEXT("Click a ground point to start a road. Secondary-click or Escape cancels."));
	}
}

void UCityFormRoadPlacementComponent::HandlePrimaryAction()
{
	if (ToolMode != ECityFormToolMode::Road)
	{
		return;
	}

	FCityFormRoadEndpointInput Endpoint;
	FVector Position;
	if (!TryGetCursorEndpoint(Endpoint, Position))
	{
		SetStatus(TEXT("Road endpoints must be on the prototype buildable ground."), true);
		return;
	}

	if (Placement.State == ERoadPlacementState::Idle)
	{
		Placement.SelectStart(Endpoint, Position);
		SetStatus(TEXT("Choose the road endpoint. Secondary-click or Escape cancels."));
		return;
	}

	if (!IsLongEnough(Placement.StartPosition, Position))
	{
		SetStatus(TEXT("Road segments must be at least one meter long."), true);
		return;
	}
	if (Placement.StartEndpoint.ExistingNodeId.IsSet() && Endpoint.ExistingNodeId.IsSet() &&
		Placement.StartEndpoint.ExistingNodeId.GetValue() == Endpoint.ExistingNodeId.GetValue())
	{
		SetStatus(TEXT("A road cannot end on its starting node."), true);
		return;
	}

	ACityFormPlayerController* Controller = Cast<ACityFormPlayerController>(GetOwner());
	UGameInstance* GameInstance = Controller != nullptr ? Controller->GetGameInstance() : nullptr;
	UCityFormSimulationSubsystem* Simulation =
		GameInstance != nullptr ? GameInstance->GetSubsystem<UCityFormSimulationSubsystem>() : nullptr;
	if (Simulation == nullptr)
	{
		SetStatus(TEXT("The city simulation is unavailable."), true);
		ClearPendingPlacement();
		return;
	}

	FRoadSegmentDefinition Definition;
	Definition.RoadTypeId = FRoadTypeCatalog::GetBasicTwoWayRoadTypeId();
	const FCreateRoadSegmentResult Result =
		Simulation->CreateRoadSegment(Placement.StartEndpoint, Endpoint, Definition);
	ClearPendingPlacement();
	if (!Result.IsSuccess())
	{
		SetStatus(Result.Error.Message, true);
		return;
	}

	SetStatus(FString::Printf(TEXT("Created road segment %llu. Click to start another."), Result.SegmentId.GetValue()));
}

bool UCityFormRoadPlacementComponent::CancelPendingPlacement()
{
	if (ToolMode == ECityFormToolMode::Road && Placement.State == ERoadPlacementState::StartSelected)
	{
		ClearPendingPlacement();
		SetStatus(TEXT("Road placement canceled. Click to start another road."));
		return true;
	}
	return false;
}

bool UCityFormRoadPlacementComponent::IsLongEnough(const FVector& Start, const FVector& End)
{
	return FVector::Dist2D(Start, End) >= MinimumRoadLengthCentimeters;
}

TOptional<FCityFormRoadNodeSnapshot> UCityFormRoadPlacementComponent::FindSnapCandidate(
	const TArray<FCityFormRoadNodeSnapshot>& Nodes,
	const FVector2D& CursorPosition,
	TFunctionRef<bool(const FVector&, FVector2D&)> ProjectToScreen)
{
	const double RadiusSquared = EndpointSnapRadiusPixels * EndpointSnapRadiusPixels;
	double BestDistanceSquared = RadiusSquared;
	TOptional<FCityFormRoadNodeSnapshot> Best;
	for (const FCityFormRoadNodeSnapshot& Node : Nodes)
	{
		FVector2D ScreenPosition;
		if (!ProjectToScreen(Node.PositionCentimeters, ScreenPosition))
		{
			continue;
		}
		const double DistanceSquared = FVector2D::DistSquared(CursorPosition, ScreenPosition);
		if (DistanceSquared <= BestDistanceSquared &&
			(!Best.IsSet() || DistanceSquared < BestDistanceSquared || Node.Id < Best->Id))
		{
			BestDistanceSquared = DistanceSquared;
			Best = Node;
		}
	}
	return Best;
}

bool UCityFormRoadPlacementComponent::TryGetCursorEndpoint(
	FCityFormRoadEndpointInput& OutEndpoint, FVector& OutPosition) const
{
	const ACityFormPlayerController* Controller = Cast<ACityFormPlayerController>(GetOwner());
	if (Controller == nullptr || Controller->IsPointerOverToolPalette())
	{
		return false;
	}

	FHitResult Hit;
	if (!Controller->GetHitResultUnderCursor(ECC_Visibility, true, Hit) ||
		!FCityFormPrototypeMapContract::IsPrototypeGroundHit(Hit.ImpactPoint))
	{
		return false;
	}

	OutPosition = Hit.ImpactPoint;
	OutPosition.Z = FCityFormPrototypeMapContract::GroundElevationCentimeters;
	float CursorX = 0.0f;
	float CursorY = 0.0f;
	if (!Controller->GetMousePosition(CursorX, CursorY))
	{
		OutEndpoint = FCityFormRoadEndpointInput::New(OutPosition);
		return true;
	}

	const UGameInstance* GameInstance = Controller->GetGameInstance();
	const UCityFormSimulationSubsystem* Simulation =
		GameInstance != nullptr ? GameInstance->GetSubsystem<UCityFormSimulationSubsystem>() : nullptr;
	if (Simulation == nullptr)
	{
		return false;
	}

	const FCityFormRoadGraphSnapshot Snapshot = Simulation->CreateRoadGraphSnapshot();
	const TOptional<FCityFormRoadNodeSnapshot> Snap = FindSnapCandidate(Snapshot.Nodes,
		FVector2D(CursorX, CursorY),
		[Controller](const FVector& WorldPosition, FVector2D& ScreenPosition)
		{
			return Controller->ProjectWorldLocationToScreen(WorldPosition, ScreenPosition);
		});
	if (Snap.IsSet())
	{
		OutPosition = Snap->PositionCentimeters;
		OutEndpoint = FCityFormRoadEndpointInput::Existing(Snap->Id);
	}
	else
	{
		OutEndpoint = FCityFormRoadEndpointInput::New(OutPosition);
	}
	return true;
}

void UCityFormRoadPlacementComponent::DrawPreview() const
{
	FCityFormRoadEndpointInput Endpoint;
	FVector Position;
	if (!TryGetCursorEndpoint(Endpoint, Position))
	{
		return;
	}

	const bool bSameNode = Placement.StartEndpoint.ExistingNodeId.IsSet() && Endpoint.ExistingNodeId.IsSet() &&
		Placement.StartEndpoint.ExistingNodeId.GetValue() == Endpoint.ExistingNodeId.GetValue();
	const FColor Color = IsLongEnough(Placement.StartPosition, Position) && !bSameNode ? FColor::Green : FColor::Red;
	FVector RaisedStart = Placement.StartPosition;
	FVector RaisedEnd = Position;
	RaisedStart.Z += PreviewHeightCentimeters;
	RaisedEnd.Z += PreviewHeightCentimeters;
	DrawDebugLine(GetWorld(), RaisedStart, RaisedEnd, Color, false, 0.0f, 0, 8.0f);
	DrawDebugSphere(GetWorld(), RaisedStart, 40.0f, 12, Color, false, 0.0f, 0, 4.0f);
	DrawDebugSphere(GetWorld(), RaisedEnd, 40.0f, 12, Color, false, 0.0f, 0, 4.0f);
}

void UCityFormRoadPlacementComponent::ClearPendingPlacement()
{
	Placement.Clear();
}

void UCityFormRoadPlacementComponent::SetStatus(const FString& Message, const bool bIsError) const
{
	if (ACityFormPlayerController* Controller = Cast<ACityFormPlayerController>(GetOwner()))
	{
		Controller->SetToolStatus(Message, bIsError);
	}
}
