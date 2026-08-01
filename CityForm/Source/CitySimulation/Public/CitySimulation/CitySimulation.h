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
	int32 ResidentialParcelCount = 0;
	int32 CommercialParcelCount = 0;
	int32 UnzonedParcelCount = 0;

	friend bool operator==(const FCitySummary& Left, const FCitySummary& Right)
	{
		return Left.Seed == Right.Seed && Left.CurrentTimeMilliseconds == Right.CurrentTimeMilliseconds &&
			Left.VehicleClassCount == Right.VehicleClassCount && Left.RoadTypeCount == Right.RoadTypeCount &&
			Left.RoadNodeCount == Right.RoadNodeCount && Left.RoadSegmentCount == Right.RoadSegmentCount &&
			Left.ParcelCount == Right.ParcelCount && Left.ResidentialParcelCount == Right.ResidentialParcelCount &&
			Left.CommercialParcelCount == Right.CommercialParcelCount &&
			Left.UnzonedParcelCount == Right.UnzonedParcelCount;
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
	 * Deterministically recomputes parcel geometry from the current road graph and reconciles
	 * it against the previous parcel set, preserving each unchanged parcel's ID and Zone.
	 * Automatically invoked after a successful AddRoadSegment or CreateRoadSegment; also safe
	 * to call directly at any time by future callers.
	 */
	FRegenerateParcelsResult RegenerateParcels();
	/**
	 * Assigns Residential or Commercial to an existing parcel, overwriting any prior Zone.
	 * Rejects EZoneCategory::None and an unknown FParcelId, atomically.
	 */
	FApplyZoneResult ApplyZone(FParcelId Id, EZoneCategory Zone);
	/** Returns an existing parcel's Zone to None. Succeeds as a no-op if already unzoned. */
	FClearZoneResult ClearZone(FParcelId Id);
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
