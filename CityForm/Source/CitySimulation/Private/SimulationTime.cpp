// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/SimulationTime.h"

namespace CityForm::Simulation
{

FSimulationInstantResult AddSimulationDuration(
	const FSimulationInstant Instant,
	const FSimulationDuration Duration)
{
	if (Duration.GetMilliseconds() < 0)
	{
		return {
			{},
			{
				ESimulationErrorCode::NegativeDuration,
				TEXT("Simulation time cannot advance by a negative duration.")
			}};
	}

	if (Duration.GetMilliseconds() >
		MAX_int64 - Instant.GetMillisecondsSinceStart())
	{
		return {
			{},
			{
				ESimulationErrorCode::TimeOverflow,
				TEXT("Simulation time advancement would overflow its millisecond range.")
			}};
	}

	return {
		FSimulationInstant(
			Instant.GetMillisecondsSinceStart() + Duration.GetMilliseconds()),
		{}};
}

FSimulationInstant FSimulationClock::GetCurrentInstant() const
{
	return CurrentInstant;
}

FAdvanceTimeResult FSimulationClock::Advance(const FSimulationDuration Duration)
{
	const FSimulationInstantResult Result =
		AddSimulationDuration(CurrentInstant, Duration);
	if (!Result.IsSuccess())
	{
		return {Result.Error};
	}

	CurrentInstant = Result.Instant;
	return {};
}

} // namespace CityForm::Simulation
