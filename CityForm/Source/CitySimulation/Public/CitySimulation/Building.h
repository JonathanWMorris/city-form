// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/Parcel.h"
#include "CitySimulation/SimulationTime.h"
#include "CitySimulation/StrongId.h"
#include "CitySimulation/Validation.h"
#include "Containers/Array.h"
#include "Containers/Map.h"

namespace CityForm::Simulation
{

class FCitySimulation;

enum class EDevelopmentStage : uint8
{
	Planned,
	UnderConstruction,
	Complete
};

struct FDevelopmentConfig
{
	FSimulationDuration PlanningDuration = FSimulationDuration(120000);
	FSimulationDuration ConstructionDuration = FSimulationDuration(180000);
	int32 DetachedHouseHouseholdCapacity = 1;
	int32 SmallCommercialJobCapacity = 8;

	FValidationReport Validate() const;
};

struct FBuildingTypeDefinition
{
	FBuildingTypeId Id;
	FString Key;
	EZoneCategory Zone = EZoneCategory::None;
	int32 HouseholdCapacity = 0;
	int32 JobCapacity = 0;
};

class CITYSIMULATION_API FBuildingTypeCatalog
{
public:
	explicit FBuildingTypeCatalog(const FDevelopmentConfig& Config);

	static constexpr FBuildingTypeId GetDetachedHouseTypeId()
	{
		return FBuildingTypeId(1);
	}

	static constexpr FBuildingTypeId GetSmallCommercialTypeId()
	{
		return FBuildingTypeId(2);
	}

	const FBuildingTypeDefinition* Find(FBuildingTypeId Id) const;
	const FBuildingTypeDefinition* FindForZone(EZoneCategory Zone) const;
	const TArray<FBuildingTypeDefinition>& GetDefinitions() const;
	FValidationReport Validate() const;

private:
	TArray<FBuildingTypeDefinition> Definitions;
};

struct FBuilding
{
	FBuildingId Id;
	FParcelId ParcelId;
	FBuildingTypeId BuildingTypeId;
	EDevelopmentStage Stage = EDevelopmentStage::Planned;
	FSimulationInstant CreatedAt;
	FSimulationInstant PlannedAt;
	FSimulationInstant ConstructionStartsAt;
	FSimulationInstant CompletesAt;
	int32 HouseholdCapacity = 0;
	int32 JobCapacity = 0;
};

struct FBuildingPlan
{
	FBuildingTypeId BuildingTypeId;
	FSimulationInstant CreatedAt;
	FSimulationInstant PlannedAt;
	FSimulationInstant ConstructionStartsAt;
	FSimulationInstant CompletesAt;
};

struct FPrepareBuildingResult
{
	FBuildingPlan Plan;
	FSimulationError Error;

	bool IsSuccess() const
	{
		return !Error.IsSet();
	}
};

/** Owns authoritative placeholder Buildings and their deterministic development timelines. */
class CITYSIMULATION_API FBuildingCollection
{
public:
	const FBuilding* FindBuilding(FBuildingId Id) const;
	const FBuilding* FindBuildingForParcel(FParcelId ParcelId) const;
	const TArray<FBuilding>& GetBuildings() const;
	FValidationReport Validate(const FParcelLayout& Parcels, const FBuildingTypeCatalog& BuildingTypes) const;

private:
	friend class FCitySimulation;

	FPrepareBuildingResult PrepareReplacement(EZoneCategory Zone,
		FSimulationInstant CurrentInstant,
		const FBuildingTypeCatalog& BuildingTypes,
		const FDevelopmentConfig& Config) const;
	FBuildingId CommitReplacement(FParcelId ParcelId, const FBuildingPlan& Plan);
	void RemoveForParcel(FParcelId ParcelId);
	void AdvanceTo(FSimulationInstant CurrentInstant, const FBuildingTypeCatalog& BuildingTypes);
	void RebuildIndexes();

	TStrongIdAllocator<FBuildingId> BuildingIdAllocator;
	TArray<FBuilding> Buildings;
	TMap<FBuildingId, int32> BuildingIndexes;
	TMap<FParcelId, int32> BuildingIndexesByParcel;
};

} // namespace CityForm::Simulation
