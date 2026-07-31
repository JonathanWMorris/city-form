// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/RoadGraph.h"
#include "Math/Vector.h"

/** Explicit conversion at the boundary between Unreal presentation and simulation space. */
struct CITYFORM_API FCityFormCoordinateConversion
{
	static constexpr double UnrealCentimetersPerSimulationMeter = 100.0;

	static CityForm::Simulation::FSimPoint2D ToSimulationMeters(const FVector& UnrealPositionCentimeters);
	static FVector ToUnrealCentimeters(CityForm::Simulation::FSimPoint2D SimulationPositionMeters);
	static double ToUnrealCentimeters(double SimulationDistanceMeters);
};
