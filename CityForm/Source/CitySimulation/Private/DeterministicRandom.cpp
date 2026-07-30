// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/DeterministicRandom.h"

namespace CityForm::Simulation
{

FDeterministicRandom::FDeterministicRandom(const uint64 Seed)
	: Engine(Seed)
{
}

uint64 FDeterministicRandom::NextUInt64()
{
	return Engine();
}

FBoundedRandomResult FDeterministicRandom::NextBoundedUInt64(const uint64 ExclusiveUpperBound)
{
	if (ExclusiveUpperBound == 0)
	{
		return {
			0,
			{ESimulationErrorCode::InvalidRandomBound, TEXT("A random bound must be greater than zero.")}};
	}

	const uint64 RejectionThreshold = (0 - ExclusiveUpperBound) % ExclusiveUpperBound;
	for (;;)
	{
		const uint64 Candidate = NextUInt64();
		if (Candidate >= RejectionThreshold)
		{
			return {Candidate % ExclusiveUpperBound, {}};
		}
	}
}

double FDeterministicRandom::NextUnitDouble()
{
	constexpr double InverseTwoToThe53 = 1.0 / 9007199254740992.0;
	return static_cast<double>(NextUInt64() >> 11) * InverseTwoToThe53;
}

} // namespace CityForm::Simulation
