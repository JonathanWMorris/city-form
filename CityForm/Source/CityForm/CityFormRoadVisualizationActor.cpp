// Copyright Jonathan Morris. All Rights Reserved.

#include "CityFormRoadVisualizationActor.h"

#include "CityFormPrototypeMapContract.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "UObject/ConstructorHelpers.h"

using namespace CityForm::Simulation;

ACityFormRoadVisualizationActor::ACityFormRoadVisualizationActor()
{
	PrimaryActorTick.bCanEverTick = false;
	RoadInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RoadInstances"));
	SetRootComponent(RoadInstances);
	RoadInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RoadInstances->SetCastShadow(false);
	RoadInstances->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		RoadInstances->SetStaticMesh(CubeMesh.Object);
	}
}

TArray<FCityFormRoadVisualInstance> ACityFormRoadVisualizationActor::BuildVisualInstances(
	const FCityFormRoadGraphSnapshot& Snapshot)
{
	TMap<FRoadNodeId, FVector> NodePositions;
	NodePositions.Reserve(Snapshot.Nodes.Num());
	for (const FCityFormRoadNodeSnapshot& Node : Snapshot.Nodes)
	{
		NodePositions.Add(Node.Id, Node.PositionCentimeters);
	}

	TArray<FCityFormRoadVisualInstance> Instances;
	Instances.Reserve(Snapshot.Segments.Num());
	for (const FCityFormRoadSegmentSnapshot& Segment : Snapshot.Segments)
	{
		const FVector* Start = NodePositions.Find(Segment.EndpointA);
		const FVector* End = NodePositions.Find(Segment.EndpointB);
		if (Start == nullptr || End == nullptr)
		{
			continue;
		}

		const FVector Delta = *End - *Start;
		const double Length = Delta.Size2D();
		if (!FMath::IsFinite(Length) || Length <= UE_DOUBLE_SMALL_NUMBER)
		{
			continue;
		}

		FVector Center = (*Start + *End) * 0.5;
		Center.Z = FCityFormPrototypeMapContract::GroundElevationCentimeters + RoadThicknessCentimeters * 0.5;
		const FRotator Rotation(0.0, FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), 0.0);
		const FVector Scale(Length / EngineCubeSizeCentimeters,
			RoadWidthCentimeters / EngineCubeSizeCentimeters,
			RoadThicknessCentimeters / EngineCubeSizeCentimeters);
		Instances.Add({Segment.Id, FTransform(Rotation, Center, Scale)});
	}
	return Instances;
}

void ACityFormRoadVisualizationActor::BeginPlay()
{
	Super::BeginPlay();
	UGameInstance* GameInstance = GetGameInstance();
	SimulationSubsystem =
		GameInstance != nullptr ? GameInstance->GetSubsystem<UCityFormSimulationSubsystem>() : nullptr;
	if (SimulationSubsystem != nullptr)
	{
		RoadGraphChangedHandle = SimulationSubsystem->OnRoadGraphChanged().AddUObject(
			this, &ACityFormRoadVisualizationActor::RebuildFromSimulation);
		RebuildFromSimulation();
	}
}

void ACityFormRoadVisualizationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SimulationSubsystem != nullptr && RoadGraphChangedHandle.IsValid())
	{
		SimulationSubsystem->OnRoadGraphChanged().Remove(RoadGraphChangedHandle);
	}
	RoadGraphChangedHandle.Reset();
	SimulationSubsystem = nullptr;
	Super::EndPlay(EndPlayReason);
}

void ACityFormRoadVisualizationActor::RebuildFromSimulation()
{
	RoadInstances->ClearInstances();
	SegmentIdsByInstance.Reset();
	if (SimulationSubsystem == nullptr)
	{
		return;
	}

	const TArray<FCityFormRoadVisualInstance> Visuals =
		BuildVisualInstances(SimulationSubsystem->CreateRoadGraphSnapshot());
	SegmentIdsByInstance.Reserve(Visuals.Num());
	for (const FCityFormRoadVisualInstance& Visual : Visuals)
	{
		RoadInstances->AddInstance(Visual.Transform);
		SegmentIdsByInstance.Add(Visual.SegmentId);
	}
}
