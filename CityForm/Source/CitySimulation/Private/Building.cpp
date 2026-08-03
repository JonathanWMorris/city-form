// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/Building.h"

#include "Misc/AssertionMacros.h"

namespace CityForm::Simulation
{

FValidationReport FDevelopmentConfig::Validate() const
{
	FValidationReport Report;
	if (PlanningDuration.GetMilliseconds() <= 0 || ConstructionDuration.GetMilliseconds() <= 0)
	{
		Report.Add({EValidationSeverity::Error,
			EValidationIssueCode::NonPositiveDevelopmentDuration,
			TEXT("DevelopmentConfig"),
			0,
			TEXT("Planning and construction durations must both be greater than zero.")});
	}
	if (DetachedHouseHouseholdCapacity < 0 || SmallCommercialJobCapacity < 0)
	{
		Report.Add({EValidationSeverity::Error,
			EValidationIssueCode::NegativeBuildingCapacity,
			TEXT("DevelopmentConfig"),
			0,
			TEXT("Configured building capacities must not be negative.")});
	}
	return Report;
}

FBuildingTypeCatalog::FBuildingTypeCatalog(const FDevelopmentConfig& Config)
{
	Definitions.Add({GetDetachedHouseTypeId(),
		TEXT("DetachedHouse"),
		EZoneCategory::Residential,
		Config.DetachedHouseHouseholdCapacity,
		0});
	Definitions.Add({GetSmallCommercialTypeId(),
		TEXT("SmallCommercial"),
		EZoneCategory::Commercial,
		0,
		Config.SmallCommercialJobCapacity});
}

const FBuildingTypeDefinition* FBuildingTypeCatalog::Find(const FBuildingTypeId Id) const
{
	return Definitions.FindByPredicate(
		[Id](const FBuildingTypeDefinition& Definition)
		{
			return Definition.Id == Id;
		});
}

const FBuildingTypeDefinition* FBuildingTypeCatalog::FindForZone(const EZoneCategory Zone) const
{
	return Definitions.FindByPredicate(
		[Zone](const FBuildingTypeDefinition& Definition)
		{
			return Definition.Zone == Zone;
		});
}

const TArray<FBuildingTypeDefinition>& FBuildingTypeCatalog::GetDefinitions() const
{
	return Definitions;
}

FValidationReport FBuildingTypeCatalog::Validate() const
{
	FValidationReport Report;
	TMap<FBuildingTypeId, bool> SeenIds;
	TMap<FString, bool> SeenKeys;
	for (const FBuildingTypeDefinition& Definition : Definitions)
	{
		if (!Definition.Id.IsValid())
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::InvalidBuildingTypeId,
				TEXT("BuildingType"),
				0,
				TEXT("A building type must have a valid ID.")});
		}
		else if (SeenIds.Contains(Definition.Id))
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::DuplicateBuildingTypeId,
				TEXT("BuildingType"),
				Definition.Id.GetValue(),
				TEXT("Building type IDs must be unique.")});
		}
		else
		{
			SeenIds.Add(Definition.Id, true);
		}

		if (Definition.Key.IsEmpty())
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::EmptyBuildingTypeKey,
				TEXT("BuildingType"),
				Definition.Id.GetValue(),
				TEXT("A building type must have a stable key.")});
		}
		else if (SeenKeys.Contains(Definition.Key))
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::DuplicateBuildingTypeKey,
				TEXT("BuildingType"),
				Definition.Id.GetValue(),
				TEXT("Building type keys must be unique.")});
		}
		else
		{
			SeenKeys.Add(Definition.Key, true);
		}

		if (Definition.Zone != EZoneCategory::Residential && Definition.Zone != EZoneCategory::Commercial)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::InvalidBuildingTypeZone,
				TEXT("BuildingType"),
				Definition.Id.GetValue(),
				TEXT("A building type must support Residential or Commercial zoning.")});
		}
		if (Definition.HouseholdCapacity < 0 || Definition.JobCapacity < 0)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::NegativeBuildingCapacity,
				TEXT("BuildingType"),
				Definition.Id.GetValue(),
				TEXT("Building type capacities must not be negative.")});
		}
		if ((Definition.Zone == EZoneCategory::Residential && Definition.JobCapacity != 0) ||
			(Definition.Zone == EZoneCategory::Commercial && Definition.HouseholdCapacity != 0))
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::InvalidActiveBuildingCapacity,
				TEXT("BuildingType"),
				Definition.Id.GetValue(),
				TEXT("Building type capacity must match its zone category.")});
		}
	}
	return Report;
}

FPrepareBuildingResult FBuildingCollection::PrepareReplacement(const EZoneCategory Zone,
	const FSimulationInstant CurrentInstant,
	const FBuildingTypeCatalog& BuildingTypes,
	const FDevelopmentConfig& Config) const
{
	const FBuildingTypeDefinition* Type = BuildingTypes.FindForZone(Zone);
	if (Type == nullptr)
	{
		return {{}, {ESimulationErrorCode::InvalidBuildingType, TEXT("No building type supports the requested zone.")}};
	}
	if (!BuildingIdAllocator.CanAllocate(1))
	{
		return {{}, {ESimulationErrorCode::IdExhausted, TEXT("The building ID allocator is exhausted.")}};
	}

	const FSimulationInstantResult ConstructionStartsAt =
		AddSimulationDuration(CurrentInstant, Config.PlanningDuration);
	if (!ConstructionStartsAt.IsSuccess())
	{
		return {{}, ConstructionStartsAt.Error};
	}
	const FSimulationInstantResult CompletesAt =
		AddSimulationDuration(ConstructionStartsAt.Instant, Config.ConstructionDuration);
	if (!CompletesAt.IsSuccess())
	{
		return {{}, CompletesAt.Error};
	}

	return {{Type->Id, CurrentInstant, CurrentInstant, ConstructionStartsAt.Instant, CompletesAt.Instant}, {}};
}

FBuildingId FBuildingCollection::CommitReplacement(const FParcelId ParcelId, const FBuildingPlan& Plan)
{
	RemoveForParcel(ParcelId);
	const TStrongIdAllocationResult<FBuildingId> Allocation = BuildingIdAllocator.Allocate();
	check(Allocation.IsSuccess());
	Buildings.Add({Allocation.Id,
		ParcelId,
		Plan.BuildingTypeId,
		EDevelopmentStage::Planned,
		Plan.CreatedAt,
		Plan.PlannedAt,
		Plan.ConstructionStartsAt,
		Plan.CompletesAt,
		0,
		0});
	RebuildIndexes();
	return Allocation.Id;
}

void FBuildingCollection::RemoveForParcel(const FParcelId ParcelId)
{
	const int32* Index = BuildingIndexesByParcel.Find(ParcelId);
	if (Index != nullptr && Buildings.IsValidIndex(*Index))
	{
		Buildings.RemoveAt(*Index);
		RebuildIndexes();
	}
}

void FBuildingCollection::AdvanceTo(const FSimulationInstant CurrentInstant, const FBuildingTypeCatalog& BuildingTypes)
{
	const int64 CurrentMilliseconds = CurrentInstant.GetMillisecondsSinceStart();
	for (FBuilding& Building : Buildings)
	{
		const FBuildingTypeDefinition* Type = BuildingTypes.Find(Building.BuildingTypeId);
		if (CurrentMilliseconds >= Building.CompletesAt.GetMillisecondsSinceStart())
		{
			Building.Stage = EDevelopmentStage::Complete;
			Building.HouseholdCapacity = Type != nullptr ? Type->HouseholdCapacity : 0;
			Building.JobCapacity = Type != nullptr ? Type->JobCapacity : 0;
		}
		else if (CurrentMilliseconds >= Building.ConstructionStartsAt.GetMillisecondsSinceStart())
		{
			Building.Stage = EDevelopmentStage::UnderConstruction;
			Building.HouseholdCapacity = 0;
			Building.JobCapacity = 0;
		}
		else
		{
			Building.Stage = EDevelopmentStage::Planned;
			Building.HouseholdCapacity = 0;
			Building.JobCapacity = 0;
		}
	}
}

const FBuilding* FBuildingCollection::FindBuilding(const FBuildingId Id) const
{
	const int32* Index = BuildingIndexes.Find(Id);
	return Index != nullptr && Buildings.IsValidIndex(*Index) ? &Buildings[*Index] : nullptr;
}

const FBuilding* FBuildingCollection::FindBuildingForParcel(const FParcelId ParcelId) const
{
	const int32* Index = BuildingIndexesByParcel.Find(ParcelId);
	return Index != nullptr && Buildings.IsValidIndex(*Index) ? &Buildings[*Index] : nullptr;
}

const TArray<FBuilding>& FBuildingCollection::GetBuildings() const
{
	return Buildings;
}

FValidationReport FBuildingCollection::Validate(
	const FParcelLayout& Parcels, const FBuildingTypeCatalog& BuildingTypes) const
{
	FValidationReport Report;
	TMap<FBuildingId, bool> SeenIds;
	TMap<FParcelId, bool> SeenParcels;
	for (const FBuilding& Building : Buildings)
	{
		if (!Building.Id.IsValid())
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::InvalidBuildingId,
				TEXT("Building"),
				0,
				TEXT("A building must have a valid ID.")});
		}
		else if (SeenIds.Contains(Building.Id))
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::DuplicateBuildingId,
				TEXT("Building"),
				Building.Id.GetValue(),
				TEXT("Building IDs must be unique.")});
		}
		else
		{
			SeenIds.Add(Building.Id, true);
		}

		const FParcel* Parcel = Parcels.FindParcel(Building.ParcelId);
		if (Parcel == nullptr)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::InvalidBuildingParcel,
				TEXT("Building"),
				Building.Id.GetValue(),
				TEXT("A building must reference an existing parcel.")});
		}
		else if (SeenParcels.Contains(Building.ParcelId))
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::DuplicateBuildingParcel,
				TEXT("Building"),
				Building.Id.GetValue(),
				TEXT("A parcel may contain at most one building.")});
		}
		else
		{
			SeenParcels.Add(Building.ParcelId, true);
		}

		const FBuildingTypeDefinition* Type = BuildingTypes.Find(Building.BuildingTypeId);
		if (Type == nullptr)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::MissingBuildingType,
				TEXT("Building"),
				Building.Id.GetValue(),
				TEXT("A building must reference an existing building type.")});
		}
		else if (Parcel != nullptr && Parcel->Zone != Type->Zone)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::IncompatibleBuildingZone,
				TEXT("Building"),
				Building.Id.GetValue(),
				TEXT("A building type must match its parcel's zone.")});
		}

		if (Building.Stage != EDevelopmentStage::Planned && Building.Stage != EDevelopmentStage::UnderConstruction &&
			Building.Stage != EDevelopmentStage::Complete)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::InvalidDevelopmentStage,
				TEXT("Building"),
				Building.Id.GetValue(),
				TEXT("A building must have a recognized development stage.")});
		}

		const int64 Created = Building.CreatedAt.GetMillisecondsSinceStart();
		const int64 Planned = Building.PlannedAt.GetMillisecondsSinceStart();
		const int64 Construction = Building.ConstructionStartsAt.GetMillisecondsSinceStart();
		const int64 Complete = Building.CompletesAt.GetMillisecondsSinceStart();
		if (Created < 0 || Created != Planned || Planned >= Construction || Construction >= Complete)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::InvalidBuildingTimeline,
				TEXT("Building"),
				Building.Id.GetValue(),
				TEXT("A building timeline must be ordered Created/Planned, Construction, Complete.")});
		}

		if (Building.HouseholdCapacity < 0 || Building.JobCapacity < 0)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::NegativeBuildingCapacity,
				TEXT("Building"),
				Building.Id.GetValue(),
				TEXT("Building capacities must not be negative.")});
		}
		else if ((Building.Stage != EDevelopmentStage::Complete &&
					 (Building.HouseholdCapacity != 0 || Building.JobCapacity != 0)) ||
			(Type != nullptr && Building.Stage == EDevelopmentStage::Complete &&
				(Building.HouseholdCapacity != Type->HouseholdCapacity || Building.JobCapacity != Type->JobCapacity)))
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::InvalidActiveBuildingCapacity,
				TEXT("Building"),
				Building.Id.GetValue(),
				TEXT("Only complete buildings expose their type's configured capacity.")});
		}
	}
	return Report;
}

void FBuildingCollection::RebuildIndexes()
{
	BuildingIndexes.Empty(Buildings.Num());
	BuildingIndexesByParcel.Empty(Buildings.Num());
	for (int32 Index = 0; Index < Buildings.Num(); ++Index)
	{
		BuildingIndexes.Add(Buildings[Index].Id, Index);
		BuildingIndexesByParcel.Add(Buildings[Index].ParcelId, Index);
	}
}

} // namespace CityForm::Simulation
