// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/CitySimulation.h"

namespace CityForm::Simulation
{

FCitySimulation::FCitySimulation(const FSimulationConfig InConfig)
	: Config(InConfig)
	, Random(InConfig.Seed)
{
}

const FSimulationConfig& FCitySimulation::GetConfig() const
{
	return Config;
}

const FSimulationTime& FCitySimulation::GetTime() const
{
	return Time;
}

FDeterministicRandom& FCitySimulation::GetRandom()
{
	return Random;
}

const FVehicleClassCatalog& FCitySimulation::GetVehicleClasses() const
{
	return VehicleClasses;
}

FAdvanceTicksResult FCitySimulation::AdvanceTicks(const int64 Count)
{
	return Time.AdvanceTicks(Count);
}

FValidationReport FCitySimulation::Validate() const
{
	FValidationReport Report;
	if (Time.GetTick() < 0)
	{
		Report.Add({
			EValidationSeverity::Error,
			EValidationIssueCode::NegativeSimulationTick,
			TEXT("SimulationTime"),
			0,
			TEXT("Simulation time cannot be negative.")});
	}

	Report.Append(VehicleClasses.Validate());
	return Report;
}

FCitySummary FCitySimulation::GetSummary() const
{
	return {
		Config.Seed,
		Time.GetTick(),
		VehicleClasses.GetDefinitions().Num()};
}

} // namespace CityForm::Simulation
