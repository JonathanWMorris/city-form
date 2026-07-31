// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/RoadType.h"
#include "CitySimulation/SimulationResult.h"
#include "CitySimulation/StrongId.h"
#include "CitySimulation/Validation.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Misc/Optional.h"

namespace CityForm::Simulation
{

struct CITYSIMULATION_API FSimPoint2D
{
	double X = 0.0;
	double Y = 0.0;

	bool IsFinite() const;
};

struct FRoadNode
{
	FRoadNodeId Id;
	FSimPoint2D PositionMeters;
};

struct FRoadSegmentDefinition
{
	FRoadTypeId RoadTypeId;
	TOptional<double> SpeedLimitOverrideMetersPerSecond;
};

struct FRoadSegment
{
	FRoadSegmentId Id;
	FRoadNodeId EndpointA;
	FRoadNodeId EndpointB;
	FRoadSegmentDefinition Definition;
	double LengthMeters = 0.0;
};

struct FRoadTraversal
{
	FRoadSegmentId SegmentId;
	FRoadNodeId FromNodeId;
	FRoadNodeId ToNodeId;

	friend bool operator==(const FRoadTraversal& Left, const FRoadTraversal& Right)
	{
		return Left.SegmentId == Right.SegmentId && Left.FromNodeId == Right.FromNodeId &&
			Left.ToNodeId == Right.ToNodeId;
	}
};

struct FAddRoadNodeResult
{
	FRoadNodeId NodeId;
	FSimulationError Error;

	bool IsSuccess() const
	{
		return NodeId.IsValid() && !Error.IsSet();
	}
};

struct FAddRoadSegmentResult
{
	FRoadSegmentId SegmentId;
	FSimulationError Error;

	bool IsSuccess() const
	{
		return SegmentId.IsValid() && !Error.IsSet();
	}
};

struct FRoadEndpointInput
{
	TOptional<FRoadNodeId> ExistingNodeId;
	FSimPoint2D PositionMeters;

	static FRoadEndpointInput Existing(const FRoadNodeId NodeId)
	{
		return {NodeId, {}};
	}

	static FRoadEndpointInput New(const FSimPoint2D Position)
	{
		return {{}, Position};
	}
};

struct FCreateRoadSegmentResult
{
	FRoadNodeId EndpointA;
	FRoadNodeId EndpointB;
	FRoadSegmentId SegmentId;
	FSimulationError Error;

	bool IsSuccess() const
	{
		return EndpointA.IsValid() && EndpointB.IsValid() && SegmentId.IsValid() && !Error.IsSet();
	}
};

class CITYSIMULATION_API FRoadGraph
{
public:
	static constexpr double LengthValidationToleranceMeters = 1.0e-9;

	FAddRoadNodeResult AddRoadNode(FSimPoint2D PositionMeters);
	FAddRoadSegmentResult AddRoadSegment(FRoadNodeId EndpointA,
		FRoadNodeId EndpointB,
		FRoadSegmentDefinition Definition,
		const FRoadTypeCatalog& RoadTypes);
	/**
	 * Validates both endpoints and the segment before allocating any records.
	 * A failed result leaves nodes, segments, indexes, adjacency, and ID allocators unchanged.
	 */
	FCreateRoadSegmentResult CreateRoadSegment(FRoadEndpointInput EndpointA,
		FRoadEndpointInput EndpointB,
		FRoadSegmentDefinition Definition,
		const FRoadTypeCatalog& RoadTypes);

	const FRoadNode* FindNode(FRoadNodeId Id) const;
	const FRoadSegment* FindSegment(FRoadSegmentId Id) const;
	const TArray<FRoadNode>& GetNodes() const;
	const TArray<FRoadSegment>& GetSegments() const;
	TArray<FRoadTraversal> GetOutgoingTraversals(FRoadNodeId NodeId) const;

	FRoadSpeedLimitResult ResolveSpeedLimit(const FRoadSegment& Segment, const FRoadTypeCatalog& RoadTypes) const;
	FValidationReport Validate(const FRoadTypeCatalog& RoadTypes) const;
	static FValidationReport ValidateRecords(
		const TArray<FRoadNode>& Nodes, const TArray<FRoadSegment>& Segments, const FRoadTypeCatalog& RoadTypes);

private:
	TStrongIdAllocator<FRoadNodeId> NodeIdAllocator;
	TStrongIdAllocator<FRoadSegmentId> SegmentIdAllocator;
	TArray<FRoadNode> Nodes;
	TArray<FRoadSegment> Segments;
	TMap<FRoadNodeId, int32> NodeIndexes;
	TMap<FRoadSegmentId, int32> SegmentIndexes;
	TMap<FRoadNodeId, TArray<FRoadSegmentId>> Adjacency;
	TMap<FRoadNodeId, TMap<FRoadNodeId, FRoadSegmentId>> Connections;
};

} // namespace CityForm::Simulation
