// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/SimulationResult.h"

#include <random>

namespace CityForm::Simulation
{

struct FBoundedRandomResult
{
	uint64 Value = 0;
	FSimulationError Error;

	bool IsSuccess() const
	{
		return !Error.IsSet();
	}
};

class FDeterministicRandom
{
public:
	explicit FDeterministicRandom(uint64 Seed);

	uint64 NextUInt64();
	FBoundedRandomResult NextBoundedUInt64(uint64 ExclusiveUpperBound);
	double NextUnitDouble();

private:
	std::mt19937_64 Engine;
};

} // namespace CityForm::Simulation
