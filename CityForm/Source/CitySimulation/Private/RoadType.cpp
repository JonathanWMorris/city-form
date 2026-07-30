// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/RoadType.h"

#include <cmath>

namespace CityForm::Simulation
{

FRoadTypeId FRoadTypeCatalog::GetBasicTwoWayRoadTypeId()
{
	return FRoadTypeId(1);
}

FRoadTypeDefinition FRoadTypeCatalog::MakeBasicTwoWayRoad(const FRegionProfile& RegionProfile)
{
	return {
		GetBasicTwoWayRoadTypeId(),
		TEXT("BasicTwoWayRoad"),
		RegionProfile.BasicTwoWayRoadDefaultSpeedLimitMetersPerSecond};
}

FRoadTypeCatalog::FRoadTypeCatalog()
	: FRoadTypeCatalog(FRegionProfile::MakeCalifornia())
{
}

FRoadTypeCatalog::FRoadTypeCatalog(const FRegionProfile& RegionProfile)
{
	Definitions.Add(MakeBasicTwoWayRoad(RegionProfile));
}

FRoadTypeCatalog::FRoadTypeCatalog(TArray<FRoadTypeDefinition> InDefinitions)
	: Definitions(MoveTemp(InDefinitions))
{
}

const FRoadTypeDefinition* FRoadTypeCatalog::Find(const FRoadTypeId Id) const
{
	for (const FRoadTypeDefinition& Definition : Definitions)
	{
		if (Definition.Id == Id)
		{
			return &Definition;
		}
	}

	return nullptr;
}

const TArray<FRoadTypeDefinition>& FRoadTypeCatalog::GetDefinitions() const
{
	return Definitions;
}

FRoadSpeedLimitResult FRoadTypeCatalog::ResolveSpeedLimit(
	const FRoadTypeId RoadTypeId,
	const TOptional<double>& OverrideMetersPerSecond) const
{
	const FRoadTypeDefinition* Definition = Find(RoadTypeId);
	if (Definition == nullptr)
	{
		return {
			0.0,
			{ESimulationErrorCode::InvalidRoadType, TEXT("The road type does not exist in this city.")}};
	}

	const double SpeedLimit = OverrideMetersPerSecond.IsSet()
		? OverrideMetersPerSecond.GetValue()
		: Definition->DefaultSpeedLimitMetersPerSecond;

	if (!std::isfinite(SpeedLimit) || SpeedLimit <= 0.0)
	{
		return {
			0.0,
			{ESimulationErrorCode::InvalidSpeedLimit, TEXT("A road speed limit must be finite and greater than zero.")}};
	}

	return {SpeedLimit, {}};
}

FValidationReport FRoadTypeCatalog::Validate() const
{
	FValidationReport Report;

	for (int32 DefinitionIndex = 0; DefinitionIndex < Definitions.Num(); ++DefinitionIndex)
	{
		const FRoadTypeDefinition& Definition = Definitions[DefinitionIndex];

		if (!Definition.Id.IsValid())
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::InvalidRoadTypeId,
				TEXT("RoadType"),
				0,
				TEXT("A road type must have a valid ID.")});
		}

		if (Definition.Key.IsEmpty())
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::EmptyRoadTypeKey,
				TEXT("RoadType"),
				Definition.Id.GetValue(),
				TEXT("A road type must have a stable key.")});
		}

		if (!std::isfinite(Definition.DefaultSpeedLimitMetersPerSecond))
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::NonFiniteRoadTypeValue,
				TEXT("RoadType"),
				Definition.Id.GetValue(),
				TEXT("A road type default speed limit must be finite.")});
		}
		else if (Definition.DefaultSpeedLimitMetersPerSecond <= 0.0)
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::NonPositiveRoadTypeValue,
				TEXT("RoadType"),
				Definition.Id.GetValue(),
				TEXT("A road type default speed limit must be greater than zero.")});
		}

		for (int32 EarlierIndex = 0; EarlierIndex < DefinitionIndex; ++EarlierIndex)
		{
			const FRoadTypeDefinition& EarlierDefinition = Definitions[EarlierIndex];
			if (EarlierDefinition.Id == Definition.Id)
			{
				Report.Add({
					EValidationSeverity::Error,
					EValidationIssueCode::DuplicateRoadTypeId,
					TEXT("RoadType"),
					Definition.Id.GetValue(),
					TEXT("Road type IDs must be unique within the catalog.")});
				break;
			}
		}

		for (int32 EarlierIndex = 0; EarlierIndex < DefinitionIndex; ++EarlierIndex)
		{
			if (Definitions[EarlierIndex].Key == Definition.Key)
			{
				Report.Add({
					EValidationSeverity::Error,
					EValidationIssueCode::DuplicateRoadTypeKey,
					TEXT("RoadType"),
					Definition.Id.GetValue(),
					TEXT("Road type keys must be unique within the catalog.")});
				break;
			}
		}
	}

	return Report;
}

} // namespace CityForm::Simulation
