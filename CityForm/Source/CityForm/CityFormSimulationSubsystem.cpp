// Copyright Jonathan Morris. All Rights Reserved.

#include "CityFormSimulationSubsystem.h"

#include "CityFormCoordinateConversion.h"

using namespace CityForm::Simulation;

void UCityFormSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	checkf(!Simulation.IsValid(), TEXT("The City Form simulation subsystem was initialized twice."));

	FSimulationConfig Config;
	Config.Seed = PrototypeSeed;
	Config.RegionProfile = FRegionProfile::MakeCalifornia();
	Simulation = MakeUnique<FCitySimulation>(MoveTemp(Config));
}

void UCityFormSimulationSubsystem::Deinitialize()
{
	Simulation.Reset();
	Super::Deinitialize();
}

FAddRoadNodeResult UCityFormSimulationSubsystem::AddRoadNode(
	const FVector& UnrealPositionCentimeters)
{
	return GetSimulation().AddRoadNode(
		FCityFormCoordinateConversion::ToSimulationMeters(UnrealPositionCentimeters));
}

FAddRoadSegmentResult UCityFormSimulationSubsystem::AddRoadSegment(
	const FRoadNodeId EndpointA,
	const FRoadNodeId EndpointB,
	FRoadSegmentDefinition Definition)
{
	return GetSimulation().AddRoadSegment(EndpointA, EndpointB, MoveTemp(Definition));
}

FCityFormRoadGraphSnapshot UCityFormSimulationSubsystem::CreateRoadGraphSnapshot() const
{
	const FRoadGraph& RoadGraph = GetSimulation().GetRoadGraph();
	FCityFormRoadGraphSnapshot Snapshot;
	Snapshot.Nodes.Reserve(RoadGraph.GetNodes().Num());
	Snapshot.Segments.Reserve(RoadGraph.GetSegments().Num());

	for (const FRoadNode& Node : RoadGraph.GetNodes())
	{
		Snapshot.Nodes.Add({
			Node.Id,
			FCityFormCoordinateConversion::ToUnrealCentimeters(Node.PositionMeters)});
	}

	for (const FRoadSegment& Segment : RoadGraph.GetSegments())
	{
		Snapshot.Segments.Add({
			Segment.Id,
			Segment.EndpointA,
			Segment.EndpointB,
			Segment.Definition.RoadTypeId,
			FCityFormCoordinateConversion::ToUnrealCentimeters(Segment.LengthMeters)});
	}

	return Snapshot;
}

FCitySummary UCityFormSimulationSubsystem::GetCitySummary() const
{
	return GetSimulation().GetSummary();
}

FValidationReport UCityFormSimulationSubsystem::ValidateCity() const
{
	return GetSimulation().Validate();
}

FCitySimulation& UCityFormSimulationSubsystem::GetSimulation()
{
	checkf(Simulation.IsValid(), TEXT("The City Form simulation subsystem is not initialized."));
	return *Simulation;
}

const FCitySimulation& UCityFormSimulationSubsystem::GetSimulation() const
{
	checkf(Simulation.IsValid(), TEXT("The City Form simulation subsystem is not initialized."));
	return *Simulation;
}
