// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "Math/Vector.h"

/** Shared spatial contract for the project-owned prototype map and its editing tools. */
struct CITYFORM_API FCityFormPrototypeMapContract
{
	static constexpr double BuildableMinimumCentimeters = -90000.0;
	static constexpr double BuildableMaximumCentimeters = 90000.0;
	static constexpr double GroundElevationCentimeters = 0.0;
	static constexpr double GroundHitToleranceCentimeters = 50.0;

	static bool ContainsBuildablePoint(const FVector& PositionCentimeters);
	static bool IsPrototypeGroundHit(const FVector& PositionCentimeters);
};
