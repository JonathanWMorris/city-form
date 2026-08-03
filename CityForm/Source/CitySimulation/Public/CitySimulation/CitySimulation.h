// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/Building.h"
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
	FDevelopmentConfig Development;
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
	int32 BuildingTypeCount = 0;
	int32 BuildingCount = 0;
	int32 PlannedBuildingCount = 0;
	int32 UnderConstructionBuildingCount = 0;
	int32 CompletedBuildingCount = 0;
	int32 ActiveHouseholdCapacity = 0;
	int32 ActiveJobCapacity = 0;

	friend bool operator==(const FCitySummary& Left, const FCitySummary& Right)
	{
		return Left.Seed == Right.Seed && Left.CurrentTimeMilliseconds == Right.CurrentTimeMilliseconds &&
			Left.VehicleClassCount == Right.VehicleClassCount && Left.RoadTypeCount == Right.RoadTypeCount &&
			Left.RoadNodeCount == Right.RoadNodeCount && Left.RoadSegmentCount == Right.RoadSegmentCount &&
			Left.ParcelCount == Right.ParcelCount && Left.ResidentialParcelCount == Right.ResidentialParcelCount &&
			Left.CommercialParcelCount == Right.CommercialParcelCount &&
			Left.UnzonedParcelCount == Right.UnzonedParcelCount && Left.BuildingTypeCount == Right.BuildingTypeCount &&
			Left.BuildingCount == Right.BuildingCount && Left.PlannedBuildingCount == Right.PlannedBuildingCount &&
			Left.UnderConstructionBuildingCount == Right.UnderConstructionBuildingCount &&
			Left.CompletedBuildingCount == Right.CompletedBuildingCount &&
			Left.ActiveHouseholdCapacity == Right.ActiveHouseholdCapacity &&
			Left.ActiveJobCapacity == Right.ActiveJobCapacity;
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
	const FBuildingTypeCatalog& GetBuildingTypes() const;
	const FBuildingCollection& GetBuildings() const;

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
	 * Assigns Residential or Commercial and creates a Planned placeholder Building. Applying
	 * the existing zone is idempotent; changing it replaces any unoccupied placeholder with a
	 * new identity and timeline. All validation occurs before parcel or building mutation.
	 */
	FApplyZoneResult ApplyZone(FParcelId Id, EZoneCategory Zone);
	/** Returns a parcel to None and removes its unoccupied placeholder Building. */
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
	FBuildingTypeCatalog BuildingTypes;
	FRoadGraph RoadGraph;
	FParcelLayout Parcels;
	FBuildingCollection Buildings;
};

} // namespace CityForm::Simulation
