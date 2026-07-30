// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/SimulationTime.h"

namespace CityForm::Simulation
{

int64 FSimulationTime::GetTick() const
{
	return CurrentTick;
}

FAdvanceTicksResult FSimulationTime::AdvanceTicks(const int64 Count)
{
	if (Count < 0)
	{
		return {
			{ESimulationErrorCode::NegativeTickCount, TEXT("Simulation time cannot advance by a negative count.")}};
	}

	if (Count > MAX_int64 - CurrentTick)
	{
		return {
			{ESimulationErrorCode::TickOverflow, TEXT("Simulation time advancement would overflow its tick range.")}};
	}

	CurrentTick += Count;
	return {};
}

} // namespace CityForm::Simulation
