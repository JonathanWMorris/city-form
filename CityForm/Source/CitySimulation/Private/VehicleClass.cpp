// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/VehicleClass.h"

#include <cmath>

namespace CityForm::Simulation
{
namespace
{

void ValidatePositiveFinite(
	const FVehicleClassDefinition& Definition, const double Value, const TCHAR* FieldName, FValidationReport& Report)
{
	if (!std::isfinite(Value))
	{
		Report.Add({EValidationSeverity::Error,
			EValidationIssueCode::NonFiniteVehicleValue,
			TEXT("VehicleClass"),
			Definition.Id.GetValue(),
			FString::Printf(TEXT("%s must be finite."), FieldName)});
	}
	else if (Value <= 0.0)
	{
		Report.Add({EValidationSeverity::Error,
			EValidationIssueCode::NonPositiveVehicleValue,
			TEXT("VehicleClass"),
			Definition.Id.GetValue(),
			FString::Printf(TEXT("%s must be greater than zero."), FieldName)});
	}
}

} // namespace

FVehicleClassDefinition FVehicleClassCatalog::MakeProvisionalPassengerCar()
{
	return {FVehicleClassId(1),
		4.5,
		1.8,
		1.5,
		7.5,
		1500.0,
		33.333,
		2.5,
		3.0,
		8.0,
		5.5,
		1.0,
		0,
		EVehiclePerformanceCategory::Standard};
}

FVehicleClassCatalog::FVehicleClassCatalog()
{
	Definitions.Add(MakeProvisionalPassengerCar());
}

FVehicleClassCatalog::FVehicleClassCatalog(TArray<FVehicleClassDefinition> InDefinitions)
	: Definitions(MoveTemp(InDefinitions))
{
}

const FVehicleClassDefinition* FVehicleClassCatalog::Find(const FVehicleClassId Id) const
{
	for (const FVehicleClassDefinition& Definition : Definitions)
	{
		if (Definition.Id == Id)
		{
			return &Definition;
		}
	}

	return nullptr;
}

const TArray<FVehicleClassDefinition>& FVehicleClassCatalog::GetDefinitions() const
{
	return Definitions;
}

FValidationReport FVehicleClassCatalog::Validate() const
{
	FValidationReport Report;

	for (int32 DefinitionIndex = 0; DefinitionIndex < Definitions.Num(); ++DefinitionIndex)
	{
		const FVehicleClassDefinition& Definition = Definitions[DefinitionIndex];

		if (!Definition.Id.IsValid())
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::InvalidVehicleClassId,
				TEXT("VehicleClass"),
				0,
				TEXT("A vehicle class must have a valid ID.")});
		}

		for (int32 EarlierIndex = 0; EarlierIndex < DefinitionIndex; ++EarlierIndex)
		{
			if (Definitions[EarlierIndex].Id == Definition.Id)
			{
				Report.Add({EValidationSeverity::Error,
					EValidationIssueCode::DuplicateVehicleClassId,
					TEXT("VehicleClass"),
					Definition.Id.GetValue(),
					TEXT("Vehicle class IDs must be unique within the catalog.")});
				break;
			}
		}

		ValidatePositiveFinite(Definition, Definition.LengthMeters, TEXT("LengthMeters"), Report);
		ValidatePositiveFinite(Definition, Definition.WidthMeters, TEXT("WidthMeters"), Report);
		ValidatePositiveFinite(Definition, Definition.HeightMeters, TEXT("HeightMeters"), Report);
		ValidatePositiveFinite(
			Definition, Definition.EffectiveQueueLengthMeters, TEXT("EffectiveQueueLengthMeters"), Report);
		ValidatePositiveFinite(Definition, Definition.MassKilograms, TEXT("MassKilograms"), Report);
		ValidatePositiveFinite(
			Definition, Definition.MaximumSpeedMetersPerSecond, TEXT("MaximumSpeedMetersPerSecond"), Report);
		ValidatePositiveFinite(Definition,
			Definition.MaximumAccelerationMetersPerSecondSquared,
			TEXT("MaximumAccelerationMetersPerSecondSquared"),
			Report);
		ValidatePositiveFinite(Definition,
			Definition.ComfortableDecelerationMetersPerSecondSquared,
			TEXT("ComfortableDecelerationMetersPerSecondSquared"),
			Report);
		ValidatePositiveFinite(Definition,
			Definition.EmergencyDecelerationMetersPerSecondSquared,
			TEXT("EmergencyDecelerationMetersPerSecondSquared"),
			Report);
		ValidatePositiveFinite(
			Definition, Definition.MinimumTurningRadiusMeters, TEXT("MinimumTurningRadiusMeters"), Report);
		ValidatePositiveFinite(Definition, Definition.PassengerCarEquivalent, TEXT("PassengerCarEquivalent"), Report);

		if (std::isfinite(Definition.EffectiveQueueLengthMeters) && std::isfinite(Definition.LengthMeters) &&
			Definition.EffectiveQueueLengthMeters < Definition.LengthMeters)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::QueueLengthShorterThanVehicle,
				TEXT("VehicleClass"),
				Definition.Id.GetValue(),
				TEXT("Effective queue length cannot be shorter than physical length.")});
		}

		if (std::isfinite(Definition.ComfortableDecelerationMetersPerSecondSquared) &&
			std::isfinite(Definition.EmergencyDecelerationMetersPerSecondSquared) &&
			Definition.ComfortableDecelerationMetersPerSecondSquared >
				Definition.EmergencyDecelerationMetersPerSecondSquared)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::ComfortableDecelerationExceedsEmergency,
				TEXT("VehicleClass"),
				Definition.Id.GetValue(),
				TEXT("Comfortable deceleration cannot exceed emergency deceleration.")});
		}
	}

	return Report;
}

} // namespace CityForm::Simulation
