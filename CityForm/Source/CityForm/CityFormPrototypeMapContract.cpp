// Copyright Jonathan Morris. All Rights Reserved.

#include "CityFormPrototypeMapContract.h"

bool FCityFormPrototypeMapContract::ContainsBuildablePoint(const FVector& PositionCentimeters)
{
	return FMath::IsFinite(PositionCentimeters.X) && FMath::IsFinite(PositionCentimeters.Y) &&
		FMath::IsFinite(PositionCentimeters.Z) && PositionCentimeters.X >= BuildableMinimumCentimeters &&
		PositionCentimeters.X <= BuildableMaximumCentimeters && PositionCentimeters.Y >= BuildableMinimumCentimeters &&
		PositionCentimeters.Y <= BuildableMaximumCentimeters;
}

bool FCityFormPrototypeMapContract::IsPrototypeGroundHit(const FVector& PositionCentimeters)
{
	return ContainsBuildablePoint(PositionCentimeters) &&
		FMath::Abs(PositionCentimeters.Z - GroundElevationCentimeters) <= GroundHitToleranceCentimeters;
}
