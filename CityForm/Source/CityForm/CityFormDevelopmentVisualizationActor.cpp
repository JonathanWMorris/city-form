// Copyright Jonathan Morris. All Rights Reserved.

#include "CityFormDevelopmentVisualizationActor.h"

#include "CityFormPrototypeMapContract.h"
#include "CitySimulation/Building.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "UObject/ConstructorHelpers.h"

using namespace CityForm::Simulation;

namespace
{
UStaticMesh* CubeMesh = nullptr;

UInstancedStaticMeshComponent* SelectParcelComponent(const EZoneCategory Zone,
	UInstancedStaticMeshComponent* Unzoned,
	UInstancedStaticMeshComponent* Residential,
	UInstancedStaticMeshComponent* Commercial)
{
	switch (Zone)
	{
	case EZoneCategory::Residential:
		return Residential;
	case EZoneCategory::Commercial:
		return Commercial;
	case EZoneCategory::None:
	default:
		return Unzoned;
	}
}
} // namespace

ACityFormDevelopmentVisualizationActor::ACityFormDevelopmentVisualizationActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
	UnzonedParcelEdges = CreateInstances(TEXT("UnzonedParcelEdges"), FLinearColor(0.55f, 0.58f, 0.62f));
	ResidentialParcelEdges = CreateInstances(TEXT("ResidentialParcelEdges"), FLinearColor(0.1f, 0.75f, 0.3f));
	CommercialParcelEdges = CreateInstances(TEXT("CommercialParcelEdges"), FLinearColor(0.12f, 0.45f, 0.95f));
	PlannedBuildings = CreateInstances(TEXT("PlannedBuildings"), FLinearColor(0.55f, 0.55f, 0.55f));
	ConstructionBuildings = CreateInstances(TEXT("ConstructionBuildings"), FLinearColor(0.95f, 0.5f, 0.08f));
	ResidentialBuildings = CreateInstances(TEXT("ResidentialBuildings"), FLinearColor(0.12f, 0.62f, 0.28f));
	CommercialBuildings = CreateInstances(TEXT("CommercialBuildings"), FLinearColor(0.1f, 0.35f, 0.85f));
}

UInstancedStaticMeshComponent* ACityFormDevelopmentVisualizationActor::CreateInstances(
	const TCHAR* Name, const FLinearColor& Color)
{
	UInstancedStaticMeshComponent* Instances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
	Instances->SetupAttachment(SceneRoot);
	Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Instances->SetCastShadow(false);
	Instances->SetMobility(EComponentMobility::Movable);
	Instances->SetStaticMesh(CubeMesh);
	Instances->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color.R, Color.G, Color.B));
	return Instances;
}

TArray<FCityFormParcelBoundaryVisual> ACityFormDevelopmentVisualizationActor::BuildParcelBoundaryVisuals(
	const FCityFormDevelopmentSnapshot& Snapshot)
{
	TArray<FCityFormParcelBoundaryVisual> Visuals;
	Visuals.Reserve(Snapshot.Parcels.Num() * 4);
	for (const FCityFormParcelSnapshot& Parcel : Snapshot.Parcels)
	{
		if (Parcel.WidthCentimeters <= 0.0 || Parcel.DepthCentimeters <= 0.0)
		{
			continue;
		}
		const double HeadingDegrees = FMath::RadiansToDegrees(Parcel.HeadingRadians);
		const FVector Along(FMath::Cos(Parcel.HeadingRadians), FMath::Sin(Parcel.HeadingRadians), 0.0);
		const FVector Across(-Along.Y, Along.X, 0.0);
		const double Z = FCityFormPrototypeMapContract::GroundElevationCentimeters + EdgeHeightCentimeters * 0.5;
		const FVector AlongScale(Parcel.WidthCentimeters / EngineCubeSizeCentimeters,
			EdgeThicknessCentimeters / EngineCubeSizeCentimeters,
			EdgeHeightCentimeters / EngineCubeSizeCentimeters);
		const FVector AcrossScale(Parcel.DepthCentimeters / EngineCubeSizeCentimeters,
			EdgeThicknessCentimeters / EngineCubeSizeCentimeters,
			EdgeHeightCentimeters / EngineCubeSizeCentimeters);
		for (const double Sign : {-1.0, 1.0})
		{
			FVector AlongCenter = Parcel.CenterCentimeters + Across * (Sign * Parcel.DepthCentimeters * 0.5);
			AlongCenter.Z = Z;
			Visuals.Add(
				{Parcel.Id, Parcel.Zone, FTransform(FRotator(0.0, HeadingDegrees, 0.0), AlongCenter, AlongScale)});

			FVector AcrossCenter = Parcel.CenterCentimeters + Along * (Sign * Parcel.WidthCentimeters * 0.5);
			AcrossCenter.Z = Z;
			Visuals.Add({Parcel.Id,
				Parcel.Zone,
				FTransform(FRotator(0.0, HeadingDegrees + 90.0, 0.0), AcrossCenter, AcrossScale)});
		}
	}
	return Visuals;
}

TArray<FCityFormBuildingVisual> ACityFormDevelopmentVisualizationActor::BuildBuildingVisuals(
	const FCityFormDevelopmentSnapshot& Snapshot)
{
	TMap<FParcelId, const FCityFormParcelSnapshot*> ParcelsById;
	for (const FCityFormParcelSnapshot& Parcel : Snapshot.Parcels)
	{
		ParcelsById.Add(Parcel.Id, &Parcel);
	}

	TArray<FCityFormBuildingVisual> Visuals;
	Visuals.Reserve(Snapshot.Buildings.Num());
	for (const FCityFormBuildingSnapshot& Building : Snapshot.Buildings)
	{
		const FCityFormParcelSnapshot* const* ParcelPointer = ParcelsById.Find(Building.ParcelId);
		if (ParcelPointer == nullptr)
		{
			continue;
		}
		const FCityFormParcelSnapshot& Parcel = **ParcelPointer;
		ECityFormBuildingVisualStyle Style = ECityFormBuildingVisualStyle::Planned;
		double Height = PlannedHeightCentimeters;
		if (Building.Stage == EDevelopmentStage::UnderConstruction)
		{
			Style = ECityFormBuildingVisualStyle::UnderConstruction;
			Height = ConstructionHeightCentimeters;
		}
		else if (Building.Stage == EDevelopmentStage::Complete &&
			Building.BuildingTypeId == FBuildingTypeCatalog::GetDetachedHouseTypeId())
		{
			Style = ECityFormBuildingVisualStyle::ResidentialComplete;
			Height = ResidentialHeightCentimeters;
		}
		else if (Building.Stage == EDevelopmentStage::Complete &&
			Building.BuildingTypeId == FBuildingTypeCatalog::GetSmallCommercialTypeId())
		{
			Style = ECityFormBuildingVisualStyle::CommercialComplete;
			Height = CommercialHeightCentimeters;
		}
		else if (Building.Stage == EDevelopmentStage::Complete)
		{
			continue;
		}
		FVector Center = Parcel.CenterCentimeters;
		Center.Z = FCityFormPrototypeMapContract::GroundElevationCentimeters + Height * 0.5;
		const FVector Scale(Parcel.WidthCentimeters * BuildingFootprintScale / EngineCubeSizeCentimeters,
			Parcel.DepthCentimeters * BuildingFootprintScale / EngineCubeSizeCentimeters,
			Height / EngineCubeSizeCentimeters);
		Visuals.Add({Building.Id,
			Style,
			FTransform(FRotator(0.0, FMath::RadiansToDegrees(Parcel.HeadingRadians), 0.0), Center, Scale)});
	}
	return Visuals;
}

void ACityFormDevelopmentVisualizationActor::BeginPlay()
{
	Super::BeginPlay();
	UGameInstance* GameInstance = GetGameInstance();
	SimulationSubsystem =
		GameInstance != nullptr ? GameInstance->GetSubsystem<UCityFormSimulationSubsystem>() : nullptr;
	if (SimulationSubsystem != nullptr)
	{
		DevelopmentChangedHandle = SimulationSubsystem->OnDevelopmentChanged().AddUObject(
			this, &ACityFormDevelopmentVisualizationActor::RebuildFromSimulation);
		RebuildFromSimulation();
	}
	ApplyParcelOverlayVisibility();
}

void ACityFormDevelopmentVisualizationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SimulationSubsystem != nullptr && DevelopmentChangedHandle.IsValid())
	{
		SimulationSubsystem->OnDevelopmentChanged().Remove(DevelopmentChangedHandle);
	}
	DevelopmentChangedHandle.Reset();
	SimulationSubsystem = nullptr;
	Super::EndPlay(EndPlayReason);
}

void ACityFormDevelopmentVisualizationActor::RebuildFromSimulation()
{
	for (UInstancedStaticMeshComponent* Instances : {UnzonedParcelEdges.Get(),
			 ResidentialParcelEdges.Get(),
			 CommercialParcelEdges.Get(),
			 PlannedBuildings.Get(),
			 ConstructionBuildings.Get(),
			 ResidentialBuildings.Get(),
			 CommercialBuildings.Get()})
	{
		Instances->ClearInstances();
	}
	if (SimulationSubsystem == nullptr)
	{
		return;
	}

	const FCityFormDevelopmentSnapshot Snapshot = SimulationSubsystem->CreateDevelopmentSnapshot();
	for (const FCityFormParcelBoundaryVisual& Visual : BuildParcelBoundaryVisuals(Snapshot))
	{
		SelectParcelComponent(Visual.Zone, UnzonedParcelEdges, ResidentialParcelEdges, CommercialParcelEdges)
			->AddInstance(Visual.Transform);
	}
	for (const FCityFormBuildingVisual& Visual : BuildBuildingVisuals(Snapshot))
	{
		UInstancedStaticMeshComponent* Instances = PlannedBuildings;
		switch (Visual.Style)
		{
		case ECityFormBuildingVisualStyle::UnderConstruction:
			Instances = ConstructionBuildings;
			break;
		case ECityFormBuildingVisualStyle::ResidentialComplete:
			Instances = ResidentialBuildings;
			break;
		case ECityFormBuildingVisualStyle::CommercialComplete:
			Instances = CommercialBuildings;
			break;
		case ECityFormBuildingVisualStyle::Planned:
		default:
			break;
		}
		Instances->AddInstance(Visual.Transform);
	}
	ApplyParcelOverlayVisibility();
}

void ACityFormDevelopmentVisualizationActor::SetParcelOverlayVisible(const bool bVisible)
{
	bParcelOverlayVisible = bVisible;
	ApplyParcelOverlayVisibility();
}

void ACityFormDevelopmentVisualizationActor::ApplyParcelOverlayVisibility()
{
	UnzonedParcelEdges->SetVisibility(bParcelOverlayVisible, true);
	ResidentialParcelEdges->SetVisibility(bParcelOverlayVisible, true);
	CommercialParcelEdges->SetVisibility(bParcelOverlayVisible, true);
}
