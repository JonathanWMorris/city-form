// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/StrongId.h"
#include "CitySimulation/Validation.h"
#include "Containers/Array.h"

namespace CityForm::Simulation
{

enum class EVehiclePerformanceCategory : uint8
{
	Standard
};

struct FVehicleClassDefinition
{
	FVehicleClassId Id;
	double LengthMeters = 0.0;
	double WidthMeters = 0.0;
	double HeightMeters = 0.0;
	double EffectiveQueueLengthMeters = 0.0;
	double MassKilograms = 0.0;
	double MaximumSpeedMetersPerSecond = 0.0;
	double MaximumAccelerationMetersPerSecondSquared = 0.0;
	double ComfortableDecelerationMetersPerSecondSquared = 0.0;
	double EmergencyDecelerationMetersPerSecondSquared = 0.0;
	double MinimumTurningRadiusMeters = 0.0;
	double PassengerCarEquivalent = 0.0;
	uint32 RestrictionMask = 0;
	EVehiclePerformanceCategory PerformanceCategory = EVehiclePerformanceCategory::Standard;
};

class FVehicleClassCatalog
{
public:
	static FVehicleClassDefinition MakeProvisionalPassengerCar();

	FVehicleClassCatalog();
	explicit FVehicleClassCatalog(TArray<FVehicleClassDefinition> InDefinitions);

	const FVehicleClassDefinition* Find(FVehicleClassId Id) const;
	const TArray<FVehicleClassDefinition>& GetDefinitions() const;
	FValidationReport Validate() const;

private:
	TArray<FVehicleClassDefinition> Definitions;
};

} // namespace CityForm::Simulation
