// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/CitySimulation.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "CityFormSimulationSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnCityFormRoadGraphChanged);

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

	CityForm::Simulation::FCreateRoadSegmentResult CreateRoadSegment(
		FCityFormRoadEndpointInput EndpointA,
		FCityFormRoadEndpointInput EndpointB,
		CityForm::Simulation::FRoadSegmentDefinition Definition);

	FCityFormRoadGraphSnapshot CreateRoadGraphSnapshot() const;
	FOnCityFormRoadGraphChanged& OnRoadGraphChanged() { return RoadGraphChanged; }
	CityForm::Simulation::FCitySummary GetCitySummary() const;
	CityForm::Simulation::FValidationReport ValidateCity() const;

private:
	CityForm::Simulation::FCitySimulation& GetSimulation();
	const CityForm::Simulation::FCitySimulation& GetSimulation() const;

	TUniquePtr<CityForm::Simulation::FCitySimulation> Simulation;
	FOnCityFormRoadGraphChanged RoadGraphChanged;
};
