// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/CitySimulation.h"

namespace CityForm::Simulation
{

FCitySimulation::FCitySimulation(const FSimulationConfig InConfig)
	: Config(InConfig), Random(InConfig.Seed), RoadTypes(InConfig.RegionProfile), BuildingTypes(InConfig.Development)
{
}

const FSimulationConfig& FCitySimulation::GetConfig() const
{
	return Config;
}

const FSimulationClock& FCitySimulation::GetClock() const
{
	return Clock;
}

FDeterministicRandom& FCitySimulation::GetRandom()
{
	return Random;
}

const FVehicleClassCatalog& FCitySimulation::GetVehicleClasses() const
{
	return VehicleClasses;
}

const FRoadTypeCatalog& FCitySimulation::GetRoadTypes() const
{
	return RoadTypes;
}

const FRoadGraph& FCitySimulation::GetRoadGraph() const
{
	return RoadGraph;
}

const FParcelLayout& FCitySimulation::GetParcelLayout() const
{
	return Parcels;
}

const FBuildingTypeCatalog& FCitySimulation::GetBuildingTypes() const
{
	return BuildingTypes;
}

const FBuildingCollection& FCitySimulation::GetBuildings() const
{
	return Buildings;
}

FAdvanceTimeResult FCitySimulation::Advance(const FSimulationDuration Duration)
{
	const FAdvanceTimeResult Result = Clock.Advance(Duration);
	if (Result.IsSuccess())
	{
		Buildings.AdvanceTo(Clock.GetCurrentInstant(), BuildingTypes);
	}
	return Result;
}

FAddRoadNodeResult FCitySimulation::AddRoadNode(const FSimPoint2D PositionMeters)
{
	return RoadGraph.AddRoadNode(PositionMeters);
}

FAddRoadSegmentResult FCitySimulation::AddRoadSegment(
	const FRoadNodeId EndpointA, const FRoadNodeId EndpointB, FRoadSegmentDefinition Definition)
{
	const FAddRoadSegmentResult Result =
		RoadGraph.AddRoadSegment(EndpointA, EndpointB, MoveTemp(Definition), RoadTypes);
	if (Result.IsSuccess())
	{
		Parcels.RegenerateParcels(RoadGraph, Config.RegionProfile);
	}
	return Result;
}

FCreateRoadSegmentResult FCitySimulation::CreateRoadSegment(
	FRoadEndpointInput EndpointA, FRoadEndpointInput EndpointB, FRoadSegmentDefinition Definition)
{
	const FCreateRoadSegmentResult Result =
		RoadGraph.CreateRoadSegment(MoveTemp(EndpointA), MoveTemp(EndpointB), MoveTemp(Definition), RoadTypes);
	if (Result.IsSuccess())
	{
		Parcels.RegenerateParcels(RoadGraph, Config.RegionProfile);
	}
	return Result;
}

FRegenerateParcelsResult FCitySimulation::RegenerateParcels()
{
	return Parcels.RegenerateParcels(RoadGraph, Config.RegionProfile);
}

FApplyZoneResult FCitySimulation::ApplyZone(const FParcelId Id, const EZoneCategory Zone)
{
	if (Zone != EZoneCategory::Residential && Zone != EZoneCategory::Commercial)
	{
		return {{ESimulationErrorCode::InvalidZoneCategory,
			TEXT("ApplyZone requires Residential or Commercial; use ClearZone to unassign a parcel.")}};
	}

	const FParcel* Parcel = Parcels.FindParcel(Id);
	if (Parcel == nullptr)
	{
		return {{ESimulationErrorCode::InvalidParcel, TEXT("ApplyZone requires an existing parcel ID.")}};
	}
	if (Parcel->Zone == Zone && Buildings.FindBuildingForParcel(Id) != nullptr)
	{
		return {};
	}

	const FPrepareBuildingResult Prepared =
		Buildings.PrepareReplacement(Zone, Clock.GetCurrentInstant(), BuildingTypes, Config.Development);
	if (!Prepared.IsSuccess())
	{
		return {Prepared.Error};
	}

	const FApplyZoneResult ZoneResult = Parcels.ApplyZone(Id, Zone);
	check(ZoneResult.IsSuccess());
	Buildings.CommitReplacement(Id, Prepared.Plan);
	return {};
}

FClearZoneResult FCitySimulation::ClearZone(const FParcelId Id)
{
	const FClearZoneResult Result = Parcels.ClearZone(Id);
	if (Result.IsSuccess())
	{
		Buildings.RemoveForParcel(Id);
	}
	return Result;
}

FRouteResult FCitySimulation::FindRoute(const FRouteQuery& Query) const
{
	const FFreeFlowTraversalCostProvider CostProvider;
	return FTimeDependentRouter::FindRoute(RoadGraph, RoadTypes, VehicleClasses, Query, CostProvider);
}

FValidationReport FCitySimulation::Validate() const
{
	FValidationReport Report;
	if (Clock.GetCurrentInstant().GetMillisecondsSinceStart() < 0)
	{
		Report.Add({EValidationSeverity::Error,
			EValidationIssueCode::NegativeSimulationTime,
			TEXT("SimulationTime"),
			0,
			TEXT("Simulation time cannot be negative.")});
	}

	Report.Append(Config.RegionProfile.Validate());
	Report.Append(Config.Development.Validate());
	Report.Append(VehicleClasses.Validate());
	Report.Append(RoadTypes.Validate());
	Report.Append(BuildingTypes.Validate());
	Report.Append(RoadGraph.Validate(RoadTypes));
	Report.Append(Parcels.Validate(RoadGraph, Config.RegionProfile));
	Report.Append(Buildings.Validate(Parcels, BuildingTypes));
	return Report;
}

FCitySummary FCitySimulation::GetSummary() const
{
	int32 ResidentialCount = 0;
	int32 CommercialCount = 0;
	int32 UnzonedCount = 0;
	for (const FParcel& Parcel : Parcels.GetParcels())
	{
		switch (Parcel.Zone)
		{
		case EZoneCategory::Residential:
			++ResidentialCount;
			break;
		case EZoneCategory::Commercial:
			++CommercialCount;
			break;
		case EZoneCategory::None:
		default:
			++UnzonedCount;
			break;
		}
	}

	int32 PlannedCount = 0;
	int32 UnderConstructionCount = 0;
	int32 CompletedCount = 0;
	int32 ActiveHouseholdCapacity = 0;
	int32 ActiveJobCapacity = 0;
	for (const FBuilding& Building : Buildings.GetBuildings())
	{
		switch (Building.Stage)
		{
		case EDevelopmentStage::Planned:
			++PlannedCount;
			break;
		case EDevelopmentStage::UnderConstruction:
			++UnderConstructionCount;
			break;
		case EDevelopmentStage::Complete:
			++CompletedCount;
			break;
		default:
			break;
		}
		ActiveHouseholdCapacity += Building.HouseholdCapacity;
		ActiveJobCapacity += Building.JobCapacity;
	}

	return {Config.Seed,
		Clock.GetCurrentInstant().GetMillisecondsSinceStart(),
		VehicleClasses.GetDefinitions().Num(),
		RoadTypes.GetDefinitions().Num(),
		RoadGraph.GetNodes().Num(),
		RoadGraph.GetSegments().Num(),
		Parcels.GetParcels().Num(),
		ResidentialCount,
		CommercialCount,
		UnzonedCount,
		BuildingTypes.GetDefinitions().Num(),
		Buildings.GetBuildings().Num(),
		PlannedCount,
		UnderConstructionCount,
		CompletedCount,
		ActiveHouseholdCapacity,
		ActiveJobCapacity};
}

} // namespace CityForm::Simulation
