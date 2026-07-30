// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/DeterministicRandom.h"
#include "CitySimulation/SimulationTime.h"
#include "CitySimulation/Validation.h"
#include "CitySimulation/VehicleClass.h"

namespace CityForm::Simulation
{

struct FSimulationConfig
{
	uint64 Seed = 0;
};

struct FCitySummary
{
	uint64 Seed = 0;
	int64 CurrentTick = 0;
	int32 VehicleClassCount = 0;

	friend bool operator==(const FCitySummary& Left, const FCitySummary& Right)
	{
		return Left.Seed == Right.Seed &&
			Left.CurrentTick == Right.CurrentTick &&
			Left.VehicleClassCount == Right.VehicleClassCount;
	}
};

class FCitySimulation
{
public:
	explicit FCitySimulation(FSimulationConfig InConfig);

	const FSimulationConfig& GetConfig() const;
	const FSimulationTime& GetTime() const;
	FDeterministicRandom& GetRandom();
	const FVehicleClassCatalog& GetVehicleClasses() const;

	FAdvanceTicksResult AdvanceTicks(int64 Count);
	FValidationReport Validate() const;
	FCitySummary GetSummary() const;

private:
	FSimulationConfig Config;
	FSimulationTime Time;
	FDeterministicRandom Random;
	FVehicleClassCatalog VehicleClasses;
};

} // namespace CityForm::Simulation
