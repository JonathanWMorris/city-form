// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CityFormSimulationSubsystem.h"
#include "GameFramework/Actor.h"

#include "CityFormDevelopmentVisualizationActor.generated.h"

enum class ECityFormBuildingVisualStyle : uint8
{
	Planned,
	UnderConstruction,
	ResidentialComplete,
	CommercialComplete
};

struct FCityFormParcelBoundaryVisual
{
	CityForm::Simulation::FParcelId ParcelId;
	CityForm::Simulation::EZoneCategory Zone = CityForm::Simulation::EZoneCategory::None;
	FTransform Transform;
};

struct FCityFormBuildingVisual
{
	CityForm::Simulation::FBuildingId BuildingId;
	ECityFormBuildingVisualStyle Style = ECityFormBuildingVisualStyle::Planned;
	FTransform Transform;
};

/** Rebuildable parcel and Building presentation derived only from detached snapshots. */
UCLASS()
class CITYFORM_API ACityFormDevelopmentVisualizationActor final : public AActor
{
	GENERATED_BODY()

public:
	ACityFormDevelopmentVisualizationActor();

	static TArray<FCityFormParcelBoundaryVisual> BuildParcelBoundaryVisuals(
		const FCityFormDevelopmentSnapshot& Snapshot);
	static TArray<FCityFormBuildingVisual> BuildBuildingVisuals(const FCityFormDevelopmentSnapshot& Snapshot);

	void RebuildFromSimulation();
	void SetParcelOverlayVisible(bool bVisible);

	bool IsParcelOverlayVisible() const
	{
		return bParcelOverlayVisible;
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	class UInstancedStaticMeshComponent* CreateInstances(const TCHAR* Name, const FLinearColor& Color);
	void ApplyParcelOverlayVisibility();

	UPROPERTY(VisibleAnywhere, Category = "City Form|Development")
	TObjectPtr<class USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "City Form|Development")
	TObjectPtr<class UInstancedStaticMeshComponent> UnzonedParcelEdges;

	UPROPERTY(VisibleAnywhere, Category = "City Form|Development")
	TObjectPtr<class UInstancedStaticMeshComponent> ResidentialParcelEdges;

	UPROPERTY(VisibleAnywhere, Category = "City Form|Development")
	TObjectPtr<class UInstancedStaticMeshComponent> CommercialParcelEdges;

	UPROPERTY(VisibleAnywhere, Category = "City Form|Development")
	TObjectPtr<class UInstancedStaticMeshComponent> PlannedBuildings;

	UPROPERTY(VisibleAnywhere, Category = "City Form|Development")
	TObjectPtr<class UInstancedStaticMeshComponent> ConstructionBuildings;

	UPROPERTY(VisibleAnywhere, Category = "City Form|Development")
	TObjectPtr<class UInstancedStaticMeshComponent> ResidentialBuildings;

	UPROPERTY(VisibleAnywhere, Category = "City Form|Development")
	TObjectPtr<class UInstancedStaticMeshComponent> CommercialBuildings;

	UPROPERTY(Transient)
	TObjectPtr<UCityFormSimulationSubsystem> SimulationSubsystem;

	FDelegateHandle DevelopmentChangedHandle;
	bool bParcelOverlayVisible = false;

	static constexpr double EdgeThicknessCentimeters = 8.0;
	static constexpr double EdgeHeightCentimeters = 4.0;
	static constexpr double PlannedHeightCentimeters = 20.0;
	static constexpr double ConstructionHeightCentimeters = 300.0;
	static constexpr double ResidentialHeightCentimeters = 600.0;
	static constexpr double CommercialHeightCentimeters = 900.0;
	static constexpr double BuildingFootprintScale = 0.65;
	static constexpr double EngineCubeSizeCentimeters = 100.0;
};
