// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/RegionProfile.h"
#include "CitySimulation/SimulationResult.h"
#include "CitySimulation/StrongId.h"
#include "CitySimulation/Validation.h"
#include "Containers/Array.h"
#include "Misc/Optional.h"

namespace CityForm::Simulation
{

struct FRoadTypeDefinition
{
	FRoadTypeId Id;
	FString Key;
	double DefaultSpeedLimitMetersPerSecond = 0.0;
};

struct FRoadSpeedLimitResult
{
	double SpeedLimitMetersPerSecond = 0.0;
	FSimulationError Error;

	bool IsSuccess() const
	{
		return !Error.IsSet();
	}
};

class CITYSIMULATION_API FRoadTypeCatalog
{
public:
	static FRoadTypeId GetBasicTwoWayRoadTypeId();
	static FRoadTypeDefinition MakeBasicTwoWayRoad(const FRegionProfile& RegionProfile);

	FRoadTypeCatalog();
	explicit FRoadTypeCatalog(const FRegionProfile& RegionProfile);
	explicit FRoadTypeCatalog(TArray<FRoadTypeDefinition> InDefinitions);

	const FRoadTypeDefinition* Find(FRoadTypeId Id) const;
	const TArray<FRoadTypeDefinition>& GetDefinitions() const;
	FRoadSpeedLimitResult ResolveSpeedLimit(
		FRoadTypeId RoadTypeId,
		const TOptional<double>& OverrideMetersPerSecond) const;
	FValidationReport Validate() const;

private:
	TArray<FRoadTypeDefinition> Definitions;
};

} // namespace CityForm::Simulation
