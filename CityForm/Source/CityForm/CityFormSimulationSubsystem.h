// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/CitySimulation.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "CityFormSimulationSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnCityFormRoadGraphChanged);
DECLARE_MULTICAST_DELEGATE(FOnCityFormDevelopmentChanged);

struct FCityFormRoadNodeSnapshot
{
	CityForm::Simulation::FRoadNodeId Id;
	FVector PositionCentimeters = FVector::ZeroVector;
};

struct FCityFormRoadSegmentSnapshot
{
	CityForm::Simulation::FRoadSegmentId Id;
	CityForm::Simulation::FRoadNodeId EndpointA;
	CityForm::Simulation::FRoadNodeId EndpointB;
	CityForm::Simulation::FRoadTypeId RoadTypeId;
	double LengthCentimeters = 0.0;
};

/** A detached presentation copy. Changing it cannot mutate the authoritative road graph. */
struct FCityFormRoadGraphSnapshot
{
	TArray<FCityFormRoadNodeSnapshot> Nodes;
	TArray<FCityFormRoadSegmentSnapshot> Segments;
};

struct FCityFormParcelSnapshot
{
	CityForm::Simulation::FParcelId Id;
	FVector CenterCentimeters = FVector::ZeroVector;
	double HeadingRadians = 0.0;
	double WidthCentimeters = 0.0;
	double DepthCentimeters = 0.0;
	CityForm::Simulation::EZoneCategory Zone = CityForm::Simulation::EZoneCategory::None;
};

struct FCityFormBuildingSnapshot
{
	CityForm::Simulation::FBuildingId Id;
	CityForm::Simulation::FParcelId ParcelId;
	CityForm::Simulation::FBuildingTypeId BuildingTypeId;
	CityForm::Simulation::EDevelopmentStage Stage = CityForm::Simulation::EDevelopmentStage::Planned;
	int32 HouseholdCapacity = 0;
	int32 JobCapacity = 0;
};

/** A detached presentation copy of authoritative parcels and buildings. */
struct FCityFormDevelopmentSnapshot
{
	TArray<FCityFormParcelSnapshot> Parcels;
	TArray<FCityFormBuildingSnapshot> Buildings;
};

struct FCityFormRoadEndpointInput
{
	TOptional<CityForm::Simulation::FRoadNodeId> ExistingNodeId;
	FVector PositionCentimeters = FVector::ZeroVector;

	static FCityFormRoadEndpointInput Existing(const CityForm::Simulation::FRoadNodeId NodeId)
	{
		return {NodeId, {}};
	}

	static FCityFormRoadEndpointInput New(const FVector& Position)
	{
		return {{}, Position};
	}
};

/**
 * Owns the active authoritative city for the lifetime of the game instance.
 * Actors submit commands and consume detached snapshots; they never own simulation state.
 */
UCLASS()
class CITYFORM_API UCityFormSimulationSubsystem final : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr uint64 PrototypeSeed = 0;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	CityForm::Simulation::FCreateRoadSegmentResult CreateRoadSegment(FCityFormRoadEndpointInput EndpointA,
		FCityFormRoadEndpointInput EndpointB,
		CityForm::Simulation::FRoadSegmentDefinition Definition);

	/** Returns a detached copy whose lifetime and mutations cannot affect authoritative state. */
	FCityFormRoadGraphSnapshot CreateRoadGraphSnapshot() const;
	FCityFormDevelopmentSnapshot CreateDevelopmentSnapshot() const;
	CityForm::Simulation::FApplyZoneResult ApplyZone(
		CityForm::Simulation::FParcelId ParcelId, CityForm::Simulation::EZoneCategory Zone);
	CityForm::Simulation::FClearZoneResult ClearZone(CityForm::Simulation::FParcelId ParcelId);
	CityForm::Simulation::FAdvanceTimeResult AdvanceSimulation(CityForm::Simulation::FSimulationDuration Duration);

	/** Broadcasts synchronously after a road command succeeds, never after a rejected command. */
	FOnCityFormRoadGraphChanged& OnRoadGraphChanged()
	{
		return RoadGraphChanged;
	}
	/** Broadcasts after successful commands that may change parcel or building presentation. */
	FOnCityFormDevelopmentChanged& OnDevelopmentChanged()
	{
		return DevelopmentChanged;
	}
	CityForm::Simulation::FCitySummary GetCitySummary() const;
	CityForm::Simulation::FValidationReport ValidateCity() const;

private:
	CityForm::Simulation::FCitySimulation& GetSimulation();
	const CityForm::Simulation::FCitySimulation& GetSimulation() const;

	TUniquePtr<CityForm::Simulation::FCitySimulation> Simulation;
	FOnCityFormRoadGraphChanged RoadGraphChanged;
	FOnCityFormDevelopmentChanged DevelopmentChanged;
};
