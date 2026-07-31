// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CityFormSimulationSubsystem.h"
#include "GameFramework/Actor.h"

#include "CityFormRoadVisualizationActor.generated.h"

struct FCityFormRoadVisualInstance
{
	CityForm::Simulation::FRoadSegmentId SegmentId;
	FTransform Transform;
};

/** Rebuildable Unreal presentation derived exclusively from detached road snapshots. */
UCLASS()
class CITYFORM_API ACityFormRoadVisualizationActor final : public AActor
{
	GENERATED_BODY()

public:
	ACityFormRoadVisualizationActor();

	static TArray<FCityFormRoadVisualInstance> BuildVisualInstances(
		const FCityFormRoadGraphSnapshot& Snapshot);
	void RebuildFromSimulation();

	const TArray<CityForm::Simulation::FRoadSegmentId>& GetSegmentIdsByInstance() const
	{
		return SegmentIdsByInstance;
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "City Form|Roads")
	TObjectPtr<class UInstancedStaticMeshComponent> RoadInstances;

	UPROPERTY(Transient)
	TObjectPtr<UCityFormSimulationSubsystem> SimulationSubsystem;

	FDelegateHandle RoadGraphChangedHandle;
	TArray<CityForm::Simulation::FRoadSegmentId> SegmentIdsByInstance;

	static constexpr double RoadWidthCentimeters = 800.0;
	static constexpr double RoadThicknessCentimeters = 20.0;
	static constexpr double EngineCubeSizeCentimeters = 100.0;
};
