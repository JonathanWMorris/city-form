// Copyright Jonathan Morris. All Rights Reserved.

#include "CityFormCoordinateConversion.h"

using CityForm::Simulation::FSimPoint2D;

FSimPoint2D FCityFormCoordinateConversion::ToSimulationMeters(
	const FVector& UnrealPositionCentimeters)
{
	return {
		UnrealPositionCentimeters.X / UnrealCentimetersPerSimulationMeter,
		UnrealPositionCentimeters.Y / UnrealCentimetersPerSimulationMeter};
}

FVector FCityFormCoordinateConversion::ToUnrealCentimeters(
	const FSimPoint2D SimulationPositionMeters)
{
	return {
		SimulationPositionMeters.X * UnrealCentimetersPerSimulationMeter,
		SimulationPositionMeters.Y * UnrealCentimetersPerSimulationMeter,
		0.0};
}

double FCityFormCoordinateConversion::ToUnrealCentimeters(
	const double SimulationDistanceMeters)
{
	return SimulationDistanceMeters * UnrealCentimetersPerSimulationMeter;
}
