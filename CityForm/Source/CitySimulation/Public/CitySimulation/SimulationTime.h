// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/SimulationResult.h"

namespace CityForm::Simulation
{

class FSimulationTime
{
public:
	int64 GetTick() const;
	FAdvanceTicksResult AdvanceTicks(int64 Count);

private:
	int64 CurrentTick = 0;
};

} // namespace CityForm::Simulation
