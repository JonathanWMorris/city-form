// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/UnrealString.h"

namespace CityForm::Simulation
{

enum class ESimulationErrorCode : uint8
{
	None,
	IdExhausted,
	InvalidRandomBound,
	NegativeDuration,
	TimeOverflow,
	NonFiniteRoadPosition,
	InvalidRoadNode,
	SameRoadEndpoint,
	ZeroLengthRoadSegment,
	DuplicateRoadConnection,
	InvalidRoadType,
	InvalidSpeedLimit
};

struct FSimulationError
{
	ESimulationErrorCode Code = ESimulationErrorCode::None;
	FString Message;

	bool IsSet() const
	{
		return Code != ESimulationErrorCode::None;
	}
};

struct FAdvanceTimeResult
{
	FSimulationError Error;

	bool IsSuccess() const
	{
		return !Error.IsSet();
	}
};

} // namespace CityForm::Simulation
