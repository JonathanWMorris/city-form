// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/Validation.h"
#include "Containers/UnrealString.h"

namespace CityForm::Simulation
{

struct CITYSIMULATION_API FRegionProfile
{
	FString Identifier = TEXT("US-CA");
	double BasicTwoWayRoadDefaultSpeedLimitMetersPerSecond = 11.176;
	double ParcelCellSizeMeters = 8.0;
	int32 ParcelDefaultWidthCells = 2;
	int32 ParcelDefaultDepthCells = 4;
	double ParcelSetbackMeters = 4.0;

	static FRegionProfile MakeCalifornia();
	FValidationReport Validate() const;
};

} // namespace CityForm::Simulation
