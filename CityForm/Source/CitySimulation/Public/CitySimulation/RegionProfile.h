// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/Validation.h"
#include "Containers/UnrealString.h"

namespace CityForm::Simulation
{

struct FRegionProfile
{
	FString Identifier = TEXT("US-CA");
	double BasicTwoWayRoadDefaultSpeedLimitMetersPerSecond = 11.176;

	static FRegionProfile MakeCalifornia();
	FValidationReport Validate() const;
};

} // namespace CityForm::Simulation
