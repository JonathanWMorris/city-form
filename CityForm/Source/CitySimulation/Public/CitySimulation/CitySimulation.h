// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/DeterministicRandom.h"
#include "CitySimulation/Parcel.h"
#include "CitySimulation/RegionProfile.h"
#include "CitySimulation/RoadGraph.h"
#include "CitySimulation/RoadType.h"
#include "CitySimulation/Routing.h"
#include "CitySimulation/SimulationTime.h"
#include "CitySimulation/Validation.h"
#include "CitySimulation/VehicleClass.h"

namespace CityForm::Simulation
{

struct FSimulationConfig
{
	uint64 Seed = 0;
	FRegionProfile RegionProfile = FRegionProfile::MakeCalifornia();
};

struct FCitySummary
{
	uint64 Seed = 0;
	int64 CurrentTimeMilliseconds = 0;
	int32 VehicleClassCount = 0;
	int32 RoadTypeCount = 0;
	int32 RoadNodeCount = 0;
	int32 RoadSegmentCount = 0;
	int32 ParcelCount = 0;

	friend bool operator==(const FCitySummary& Left, const FCitySummary& Right)
	{
		return Left.Seed == Right.Seed && Left.CurrentTimeMilliseconds == Right.CurrentTimeMilliseconds &&
			Left.VehicleClassCount == Right.VehicleClassCount && Left.RoadTypeCount == Right.RoadTypeCount &&
			Left.RoadNodeCount == Right.RoadNodeCount && Left.RoadSegmentCount == Right.RoadSegmentCount &&
			Left.ParcelCount == Right.ParcelCount;
	}
};

class CITYSIMULATION_API FCitySimulation
{
public:
	explicit FCitySimulation(FSimulationConfig InConfig);

	const FSimulationConfig& GetConfig() const;
	const FSimulationClock& GetClock() const;
	FDeterministicRandom& GetRandom();
	const FVehicleClassCatalog& GetVehicleClasses() const;
	const FRoadTypeCatalog& GetRoadTypes() const;
	const FRoadGraph& GetRoadGraph() const;
	const FParcelLayout& GetParcelLayout() const;

	FAdvanceTimeResult Advance(FSimulationDuration Duration);
	FAddRoadNodeResult AddRoadNode(FSimPoint2D PositionMeters);
	FAddRoadSegmentResult AddRoadSegment(
		FRoadNodeId EndpointA, FRoadNodeId EndpointB, FRoadSegmentDefinition Definition);
	FCreateRoadSegmentResult CreateRoadSegment(
		FRoadEndpointInput EndpointA, FRoadEndpointInput EndpointB, FRoadSegmentDefinition Definition);
	/**
	 * Deterministically recomputes the full parcel set from the current road graph, replacing
	 * any previous set. Automatically invoked after a successful AddRoadSegment or
	 * CreateRoadSegment; also safe to call directly at any time by future callers.
	 */
	FRegenerateParcelsResult RegenerateParcels();
	FRouteResult FindRoute(const FRouteQuery& Query) const;
	FValidationReport Validate() const;
	FCitySummary GetSummary() const;

private:
	FSimulationConfig Config;
	FSimulationClock Clock;
	FDeterministicRandom Random;
	FVehicleClassCatalog VehicleClasses;
	FRoadTypeCatalog RoadTypes;
	FRoadGraph RoadGraph;
	FParcelLayout Parcels;
};

} // namespace CityForm::Simulation
