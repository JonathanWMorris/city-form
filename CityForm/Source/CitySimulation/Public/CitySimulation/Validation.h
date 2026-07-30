// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"

namespace CityForm::Simulation
{

enum class EValidationSeverity : uint8
{
	Warning,
	Error
};

enum class EValidationIssueCode : uint8
{
	NegativeSimulationTick,
	InvalidVehicleClassId,
	DuplicateVehicleClassId,
	NonFiniteVehicleValue,
	NonPositiveVehicleValue,
	QueueLengthShorterThanVehicle,
	ComfortableDecelerationExceedsEmergency,
	EmptyRegionIdentifier,
	NonFiniteRegionalValue,
	NonPositiveRegionalValue,
	InvalidRoadTypeId,
	DuplicateRoadTypeId,
	EmptyRoadTypeKey,
	DuplicateRoadTypeKey,
	NonFiniteRoadTypeValue,
	NonPositiveRoadTypeValue,
	InvalidRoadNodeId,
	DuplicateRoadNodeId,
	NonFiniteRoadPosition,
	InvalidRoadSegmentId,
	DuplicateRoadSegmentId,
	MissingRoadSegmentEndpoint,
	IdenticalRoadSegmentEndpoints,
	DuplicateRoadConnection,
	NonFiniteRoadSegmentLength,
	NonPositiveRoadSegmentLength,
	RoadSegmentLengthMismatch,
	MissingRoadType,
	NonFiniteSpeedLimitOverride,
	NonPositiveSpeedLimitOverride
};

struct FValidationIssue
{
	EValidationSeverity Severity = EValidationSeverity::Error;
	EValidationIssueCode Code = EValidationIssueCode::NonPositiveVehicleValue;
	FString EntityCategory;
	uint64 EntityId = 0;
	FString Message;
};

class FValidationReport
{
public:
	void Add(FValidationIssue Issue);
	void Append(const FValidationReport& Other);

	bool IsValid() const;
	const TArray<FValidationIssue>& GetIssues() const;

private:
	TArray<FValidationIssue> Issues;
};

} // namespace CityForm::Simulation
