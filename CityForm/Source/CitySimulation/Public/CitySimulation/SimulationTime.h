// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/SimulationResult.h"

namespace CityForm::Simulation
{

class FSimulationDuration
{
public:
	constexpr FSimulationDuration() = default;

	explicit constexpr FSimulationDuration(const int64 InMilliseconds)
		: Milliseconds(InMilliseconds)
	{
	}

	constexpr int64 GetMilliseconds() const
	{
		return Milliseconds;
	}

	friend constexpr bool operator==(
		const FSimulationDuration Left,
		const FSimulationDuration Right)
	{
		return Left.Milliseconds == Right.Milliseconds;
	}

	friend constexpr bool operator<(
		const FSimulationDuration Left,
		const FSimulationDuration Right)
	{
		return Left.Milliseconds < Right.Milliseconds;
	}

private:
	int64 Milliseconds = 0;
};

class FSimulationInstant
{
public:
	constexpr FSimulationInstant() = default;

	explicit constexpr FSimulationInstant(const int64 InMillisecondsSinceStart)
		: MillisecondsSinceStart(InMillisecondsSinceStart)
	{
	}

	constexpr int64 GetMillisecondsSinceStart() const
	{
		return MillisecondsSinceStart;
	}

	friend constexpr bool operator==(
		const FSimulationInstant Left,
		const FSimulationInstant Right)
	{
		return Left.MillisecondsSinceStart == Right.MillisecondsSinceStart;
	}

	friend constexpr bool operator<(
		const FSimulationInstant Left,
		const FSimulationInstant Right)
	{
		return Left.MillisecondsSinceStart < Right.MillisecondsSinceStart;
	}

private:
	int64 MillisecondsSinceStart = 0;
};

struct FSimulationInstantResult
{
	FSimulationInstant Instant;
	FSimulationError Error;

	bool IsSuccess() const
	{
		return !Error.IsSet();
	}
};

FSimulationInstantResult AddSimulationDuration(
	FSimulationInstant Instant,
	FSimulationDuration Duration);

class FSimulationClock
{
public:
	FSimulationInstant GetCurrentInstant() const;
	FAdvanceTimeResult Advance(FSimulationDuration Duration);

private:
	FSimulationInstant CurrentInstant;
};

} // namespace CityForm::Simulation
