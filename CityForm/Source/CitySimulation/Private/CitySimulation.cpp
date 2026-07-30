// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/CitySimulation.h"

namespace CityForm::Simulation
{

FCitySimulation::FCitySimulation(const FSimulationConfig InConfig)
	: Config(InConfig)
	, Random(InConfig.Seed)
	, RoadTypes(InConfig.RegionProfile)
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

FAdvanceTimeResult FCitySimulation::Advance(const FSimulationDuration Duration)
{
	return Clock.Advance(Duration);
}

FAddRoadNodeResult FCitySimulation::AddRoadNode(const FSimPoint2D PositionMeters)
{
	return RoadGraph.AddRoadNode(PositionMeters);
}

FAddRoadSegmentResult FCitySimulation::AddRoadSegment(
	const FRoadNodeId EndpointA,
	const FRoadNodeId EndpointB,
	FRoadSegmentDefinition Definition)
{
	return RoadGraph.AddRoadSegment(
		EndpointA,
		EndpointB,
		MoveTemp(Definition),
		RoadTypes);
}

FRouteResult FCitySimulation::FindRoute(const FRouteQuery& Query) const
{
	const FFreeFlowTraversalCostProvider CostProvider;
	return FTimeDependentRouter::FindRoute(
		RoadGraph,
		RoadTypes,
		VehicleClasses,
		Query,
		CostProvider);
}

FValidationReport FCitySimulation::Validate() const
{
	FValidationReport Report;
	if (Clock.GetCurrentInstant().GetMillisecondsSinceStart() < 0)
	{
		Report.Add({
			EValidationSeverity::Error,
			EValidationIssueCode::NegativeSimulationTime,
			TEXT("SimulationTime"),
			0,
			TEXT("Simulation time cannot be negative.")});
	}

	Report.Append(Config.RegionProfile.Validate());
	Report.Append(VehicleClasses.Validate());
	Report.Append(RoadTypes.Validate());
	Report.Append(RoadGraph.Validate(RoadTypes));
	return Report;
}

FCitySummary FCitySimulation::GetSummary() const
{
	return {
		Config.Seed,
		Clock.GetCurrentInstant().GetMillisecondsSinceStart(),
		VehicleClasses.GetDefinitions().Num(),
		RoadTypes.GetDefinitions().Num(),
		RoadGraph.GetNodes().Num(),
		RoadGraph.GetSegments().Num()};
}

} // namespace CityForm::Simulation
