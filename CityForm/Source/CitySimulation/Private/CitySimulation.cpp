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

const FSimulationTime& FCitySimulation::GetTime() const
{
	return Time;
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

FAdvanceTicksResult FCitySimulation::AdvanceTicks(const int64 Count)
{
	return Time.AdvanceTicks(Count);
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

FValidationReport FCitySimulation::Validate() const
{
	FValidationReport Report;
	if (Time.GetTick() < 0)
	{
		Report.Add({
			EValidationSeverity::Error,
			EValidationIssueCode::NegativeSimulationTick,
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
		Time.GetTick(),
		VehicleClasses.GetDefinitions().Num(),
		RoadTypes.GetDefinitions().Num(),
		RoadGraph.GetNodes().Num(),
		RoadGraph.GetSegments().Num()};
}

} // namespace CityForm::Simulation
