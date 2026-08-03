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

FCreateRoadSegmentResult UCityFormSimulationSubsystem::CreateRoadSegment(
	FCityFormRoadEndpointInput EndpointA, FCityFormRoadEndpointInput EndpointB, FRoadSegmentDefinition Definition)
{
	auto ConvertEndpoint = [](FCityFormRoadEndpointInput Input)
	{
		return Input.ExistingNodeId.IsSet()
			? FRoadEndpointInput::Existing(Input.ExistingNodeId.GetValue())
			: FRoadEndpointInput::New(FCityFormCoordinateConversion::ToSimulationMeters(Input.PositionCentimeters));
	};

	FCreateRoadSegmentResult Result = GetSimulation().CreateRoadSegment(
		ConvertEndpoint(MoveTemp(EndpointA)), ConvertEndpoint(MoveTemp(EndpointB)), MoveTemp(Definition));
	if (Result.IsSuccess())
	{
		RoadGraphChanged.Broadcast();
		DevelopmentChanged.Broadcast();
	}
	return Result;
}

FCityFormDevelopmentSnapshot UCityFormSimulationSubsystem::CreateDevelopmentSnapshot() const
{
	const FCitySimulation& City = GetSimulation();
	FCityFormDevelopmentSnapshot Snapshot;
	Snapshot.Parcels.Reserve(City.GetParcelLayout().GetParcels().Num());
	Snapshot.Buildings.Reserve(City.GetBuildings().GetBuildings().Num());
	for (const FParcel& Parcel : City.GetParcelLayout().GetParcels())
	{
		Snapshot.Parcels.Add({Parcel.Id,
			FCityFormCoordinateConversion::ToUnrealCentimeters(Parcel.CenterPositionMeters),
			Parcel.HeadingRadians,
			FCityFormCoordinateConversion::ToUnrealCentimeters(Parcel.WidthMeters),
			FCityFormCoordinateConversion::ToUnrealCentimeters(Parcel.DepthMeters),
			Parcel.Zone});
	}
	for (const FBuilding& Building : City.GetBuildings().GetBuildings())
	{
		Snapshot.Buildings.Add({Building.Id,
			Building.ParcelId,
			Building.BuildingTypeId,
			Building.Stage,
			Building.HouseholdCapacity,
			Building.JobCapacity});
	}
	return Snapshot;
}

FApplyZoneResult UCityFormSimulationSubsystem::ApplyZone(const FParcelId ParcelId, const EZoneCategory Zone)
{
	const FApplyZoneResult Result = GetSimulation().ApplyZone(ParcelId, Zone);
	if (Result.IsSuccess())
	{
		DevelopmentChanged.Broadcast();
	}
	return Result;
}

FClearZoneResult UCityFormSimulationSubsystem::ClearZone(const FParcelId ParcelId)
{
	const FClearZoneResult Result = GetSimulation().ClearZone(ParcelId);
	if (Result.IsSuccess())
	{
		DevelopmentChanged.Broadcast();
	}
	return Result;
}

FAdvanceTimeResult UCityFormSimulationSubsystem::AdvanceSimulation(const FSimulationDuration Duration)
{
	const FAdvanceTimeResult Result = GetSimulation().Advance(Duration);
	if (Result.IsSuccess())
	{
		DevelopmentChanged.Broadcast();
	}
	return Result;
}

FCityFormRoadGraphSnapshot UCityFormSimulationSubsystem::CreateRoadGraphSnapshot() const
{
	const FRoadGraph& RoadGraph = GetSimulation().GetRoadGraph();
	FCityFormRoadGraphSnapshot Snapshot;
	Snapshot.Nodes.Reserve(RoadGraph.GetNodes().Num());
	Snapshot.Segments.Reserve(RoadGraph.GetSegments().Num());

	for (const FRoadNode& Node : RoadGraph.GetNodes())
	{
		Snapshot.Nodes.Add({Node.Id, FCityFormCoordinateConversion::ToUnrealCentimeters(Node.PositionMeters)});
	}

	for (const FRoadSegment& Segment : RoadGraph.GetSegments())
	{
		Snapshot.Segments.Add({Segment.Id,
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
