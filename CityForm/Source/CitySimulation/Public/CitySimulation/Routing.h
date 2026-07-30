// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/RoadGraph.h"
#include "CitySimulation/RoadType.h"
#include "CitySimulation/SimulationTime.h"
#include "CitySimulation/VehicleClass.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"

namespace CityForm::Simulation
{

enum class ERouteErrorCode : uint8
{
	None,
	InvalidOrigin,
	InvalidDestination,
	InvalidDepartureTime,
	InvalidVehicleClass,
	InvalidGraph,
	ProviderDoesNotGuaranteeFifo,
	InvalidTraversalCost,
	TimeOverflow,
	NoPermittedRoute,
	Disconnected
};

struct FRouteError
{
	ERouteErrorCode Code = ERouteErrorCode::None;
	FString Message;

	bool IsSet() const
	{
		return Code != ERouteErrorCode::None;
	}
};

enum class ETraversalCostStatus : uint8
{
	Success,
	Prohibited,
	Error
};

struct FTraversalCostResult
{
	FSimulationDuration Duration;
	ETraversalCostStatus Status = ETraversalCostStatus::Success;
	FRouteError Error;

	bool IsSuccess() const
	{
		return Status == ETraversalCostStatus::Success && !Error.IsSet();
	}

	bool IsProhibited() const
	{
		return Status == ETraversalCostStatus::Prohibited;
	}

	static FTraversalCostResult Prohibited();
	static FTraversalCostResult Failure(
		ERouteErrorCode Code,
		FString Message);
};

class ITraversalCostProvider
{
public:
	virtual ~ITraversalCostProvider() = default;

	virtual bool GuaranteesFifo() const = 0;
	virtual FTraversalCostResult Evaluate(
		const FRoadGraph& Graph,
		const FRoadTypeCatalog& RoadTypes,
		const FRoadTraversal& Traversal,
		FSimulationInstant EntryInstant,
		const FVehicleClassDefinition& VehicleClass) const = 0;
};

class FFreeFlowTraversalCostProvider final : public ITraversalCostProvider
{
public:
	virtual bool GuaranteesFifo() const override;
	virtual FTraversalCostResult Evaluate(
		const FRoadGraph& Graph,
		const FRoadTypeCatalog& RoadTypes,
		const FRoadTraversal& Traversal,
		FSimulationInstant EntryInstant,
		const FVehicleClassDefinition& VehicleClass) const override;
};

enum class ERouteHeuristicMode : uint8
{
	VehicleSpeedLowerBound,
	Zero
};

struct FRouteOptions
{
	ERouteHeuristicMode HeuristicMode =
		ERouteHeuristicMode::VehicleSpeedLowerBound;
};

struct FRouteQuery
{
	FRoadNodeId OriginNodeId;
	FRoadNodeId DestinationNodeId;
	FSimulationInstant DepartureInstant;
	FVehicleClassId VehicleClassId;
};

struct FRoute
{
	TArray<FRoadTraversal> Traversals;
	TArray<FRoadNodeId> NodeIds;
	double TotalDistanceMeters = 0.0;
	FSimulationInstant ArrivalInstant;
	FSimulationDuration TravelDuration;
};

struct FRouteResult
{
	FRoute Route;
	FRouteError Error;

	bool IsSuccess() const
	{
		return !Error.IsSet();
	}
};

class FTimeDependentRouter
{
public:
	static FRouteResult FindRoute(
		const FRoadGraph& Graph,
		const FRoadTypeCatalog& RoadTypes,
		const FVehicleClassCatalog& VehicleClasses,
		const FRouteQuery& Query,
		const ITraversalCostProvider& CostProvider,
		FRouteOptions Options = {});
};

} // namespace CityForm::Simulation
