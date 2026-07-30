// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/RoadGraph.h"

#include <cmath>

namespace CityForm::Simulation
{
namespace
{

double DistanceBetween(const FSimPoint2D& Left, const FSimPoint2D& Right)
{
	return std::hypot(Right.X - Left.X, Right.Y - Left.Y);
}

void OrderEndpoints(
	const FRoadNodeId First,
	const FRoadNodeId Second,
	FRoadNodeId& Lower,
	FRoadNodeId& Higher)
{
	if (Second < First)
	{
		Lower = Second;
		Higher = First;
	}
	else
	{
		Lower = First;
		Higher = Second;
	}
}

bool HasConnection(
	const TMap<FRoadNodeId, TMap<FRoadNodeId, FRoadSegmentId>>& Connections,
	const FRoadNodeId First,
	const FRoadNodeId Second)
{
	FRoadNodeId Lower;
	FRoadNodeId Higher;
	OrderEndpoints(First, Second, Lower, Higher);

	const TMap<FRoadNodeId, FRoadSegmentId>* ConnectionsFromLower = Connections.Find(Lower);
	return ConnectionsFromLower != nullptr && ConnectionsFromLower->Contains(Higher);
}

void AddConnection(
	TMap<FRoadNodeId, TMap<FRoadNodeId, FRoadSegmentId>>& Connections,
	const FRoadNodeId First,
	const FRoadNodeId Second,
	const FRoadSegmentId SegmentId)
{
	FRoadNodeId Lower;
	FRoadNodeId Higher;
	OrderEndpoints(First, Second, Lower, Higher);
	Connections.FindOrAdd(Lower).Add(Higher, SegmentId);
}

} // namespace

bool FSimPoint2D::IsFinite() const
{
	return std::isfinite(X) && std::isfinite(Y);
}

FAddRoadNodeResult FRoadGraph::AddRoadNode(const FSimPoint2D PositionMeters)
{
	if (!PositionMeters.IsFinite())
	{
		return {
			FRoadNodeId(),
			{ESimulationErrorCode::NonFiniteRoadPosition, TEXT("Road-node coordinates must be finite.")}};
	}

	const TStrongIdAllocationResult<FRoadNodeId> Allocation = NodeIdAllocator.Allocate();
	if (!Allocation.IsSuccess())
	{
		return {FRoadNodeId(), Allocation.Error};
	}

	const int32 NewIndex = Nodes.Num();
	Nodes.Add({Allocation.Id, PositionMeters});
	NodeIndexes.Add(Allocation.Id, NewIndex);
	Adjacency.Add(Allocation.Id);
	return {Allocation.Id, {}};
}

FAddRoadSegmentResult FRoadGraph::AddRoadSegment(
	const FRoadNodeId EndpointA,
	const FRoadNodeId EndpointB,
	FRoadSegmentDefinition Definition,
	const FRoadTypeCatalog& RoadTypes)
{
	const FRoadNode* NodeA = FindNode(EndpointA);
	const FRoadNode* NodeB = FindNode(EndpointB);
	if (NodeA == nullptr || NodeB == nullptr)
	{
		return {
			FRoadSegmentId(),
			{ESimulationErrorCode::InvalidRoadNode, TEXT("Both road-segment endpoints must reference existing nodes.")}};
	}

	if (EndpointA == EndpointB)
	{
		return {
			FRoadSegmentId(),
			{ESimulationErrorCode::SameRoadEndpoint, TEXT("A road segment must connect two distinct nodes.")}};
	}

	const FRoadSpeedLimitResult SpeedLimit = RoadTypes.ResolveSpeedLimit(
		Definition.RoadTypeId,
		Definition.SpeedLimitOverrideMetersPerSecond);
	if (!SpeedLimit.IsSuccess())
	{
		return {FRoadSegmentId(), SpeedLimit.Error};
	}

	const double LengthMeters = DistanceBetween(NodeA->PositionMeters, NodeB->PositionMeters);
	if (!std::isfinite(LengthMeters) || LengthMeters <= 0.0)
	{
		return {
			FRoadSegmentId(),
			{ESimulationErrorCode::ZeroLengthRoadSegment, TEXT("A road segment must have positive finite length.")}};
	}

	if (HasConnection(Connections, EndpointA, EndpointB))
	{
		return {
			FRoadSegmentId(),
			{ESimulationErrorCode::DuplicateRoadConnection, TEXT("A connection already exists between these road nodes.")}};
	}

	const TStrongIdAllocationResult<FRoadSegmentId> Allocation = SegmentIdAllocator.Allocate();
	if (!Allocation.IsSuccess())
	{
		return {FRoadSegmentId(), Allocation.Error};
	}

	const int32 NewIndex = Segments.Num();
	Segments.Add({
		Allocation.Id,
		EndpointA,
		EndpointB,
		MoveTemp(Definition),
		LengthMeters});
	SegmentIndexes.Add(Allocation.Id, NewIndex);
	Adjacency.FindChecked(EndpointA).Add(Allocation.Id);
	Adjacency.FindChecked(EndpointB).Add(Allocation.Id);
	AddConnection(Connections, EndpointA, EndpointB, Allocation.Id);
	return {Allocation.Id, {}};
}

const FRoadNode* FRoadGraph::FindNode(const FRoadNodeId Id) const
{
	const int32* Index = NodeIndexes.Find(Id);
	return Index != nullptr && Nodes.IsValidIndex(*Index) ? &Nodes[*Index] : nullptr;
}

const FRoadSegment* FRoadGraph::FindSegment(const FRoadSegmentId Id) const
{
	const int32* Index = SegmentIndexes.Find(Id);
	return Index != nullptr && Segments.IsValidIndex(*Index) ? &Segments[*Index] : nullptr;
}

const TArray<FRoadNode>& FRoadGraph::GetNodes() const
{
	return Nodes;
}

const TArray<FRoadSegment>& FRoadGraph::GetSegments() const
{
	return Segments;
}

TArray<FRoadTraversal> FRoadGraph::GetOutgoingTraversals(const FRoadNodeId NodeId) const
{
	TArray<FRoadTraversal> Traversals;
	const TArray<FRoadSegmentId>* IncidentSegments = Adjacency.Find(NodeId);
	if (IncidentSegments == nullptr)
	{
		return Traversals;
	}

	Traversals.Reserve(IncidentSegments->Num());
	for (const FRoadSegmentId SegmentId : *IncidentSegments)
	{
		const FRoadSegment* Segment = FindSegment(SegmentId);
		if (Segment == nullptr)
		{
			continue;
		}

		const FRoadNodeId ToNodeId = Segment->EndpointA == NodeId
			? Segment->EndpointB
			: Segment->EndpointA;
		Traversals.Add({SegmentId, NodeId, ToNodeId});
	}

	Traversals.Sort([](const FRoadTraversal& Left, const FRoadTraversal& Right)
	{
		if (Left.SegmentId != Right.SegmentId)
		{
			return Left.SegmentId < Right.SegmentId;
		}
		return Left.ToNodeId < Right.ToNodeId;
	});
	return Traversals;
}

FRoadSpeedLimitResult FRoadGraph::ResolveSpeedLimit(
	const FRoadSegment& Segment,
	const FRoadTypeCatalog& RoadTypes) const
{
	return RoadTypes.ResolveSpeedLimit(
		Segment.Definition.RoadTypeId,
		Segment.Definition.SpeedLimitOverrideMetersPerSecond);
}

FValidationReport FRoadGraph::Validate(const FRoadTypeCatalog& RoadTypes) const
{
	return ValidateRecords(Nodes, Segments, RoadTypes);
}

FValidationReport FRoadGraph::ValidateRecords(
	const TArray<FRoadNode>& NodesToValidate,
	const TArray<FRoadSegment>& SegmentsToValidate,
	const FRoadTypeCatalog& RoadTypes)
{
	FValidationReport Report;
	TMap<FRoadNodeId, const FRoadNode*> NodesById;

	for (const FRoadNode& Node : NodesToValidate)
	{
		if (!Node.Id.IsValid())
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::InvalidRoadNodeId,
				TEXT("RoadNode"),
				0,
				TEXT("A road node must have a valid ID.")});
		}
		else if (NodesById.Contains(Node.Id))
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::DuplicateRoadNodeId,
				TEXT("RoadNode"),
				Node.Id.GetValue(),
				TEXT("Road node IDs must be unique.")});
		}
		else
		{
			NodesById.Add(Node.Id, &Node);
		}

		if (!Node.PositionMeters.IsFinite())
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::NonFiniteRoadPosition,
				TEXT("RoadNode"),
				Node.Id.GetValue(),
				TEXT("Road-node coordinates must be finite.")});
		}
	}

	TMap<FRoadSegmentId, bool> SegmentIds;
	TMap<FRoadNodeId, TMap<FRoadNodeId, FRoadSegmentId>> RecordConnections;
	for (const FRoadSegment& Segment : SegmentsToValidate)
	{
		if (!Segment.Id.IsValid())
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::InvalidRoadSegmentId,
				TEXT("RoadSegment"),
				0,
				TEXT("A road segment must have a valid ID.")});
		}
		else if (SegmentIds.Contains(Segment.Id))
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::DuplicateRoadSegmentId,
				TEXT("RoadSegment"),
				Segment.Id.GetValue(),
				TEXT("Road segment IDs must be unique.")});
		}
		else
		{
			SegmentIds.Add(Segment.Id, true);
		}

		const FRoadNode* const* NodeAPointer = NodesById.Find(Segment.EndpointA);
		const FRoadNode* const* NodeBPointer = NodesById.Find(Segment.EndpointB);
		const FRoadNode* NodeA = NodeAPointer != nullptr ? *NodeAPointer : nullptr;
		const FRoadNode* NodeB = NodeBPointer != nullptr ? *NodeBPointer : nullptr;

		if (NodeA == nullptr || NodeB == nullptr)
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::MissingRoadSegmentEndpoint,
				TEXT("RoadSegment"),
				Segment.Id.GetValue(),
				TEXT("Road-segment endpoints must reference existing nodes.")});
		}

		if (Segment.EndpointA == Segment.EndpointB)
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::IdenticalRoadSegmentEndpoints,
				TEXT("RoadSegment"),
				Segment.Id.GetValue(),
				TEXT("A road segment must connect two distinct node IDs.")});
		}
		else if (NodeA != nullptr && NodeB != nullptr)
		{
			if (HasConnection(RecordConnections, Segment.EndpointA, Segment.EndpointB))
			{
				Report.Add({
					EValidationSeverity::Error,
					EValidationIssueCode::DuplicateRoadConnection,
					TEXT("RoadSegment"),
					Segment.Id.GetValue(),
					TEXT("Only one segment may connect the same node pair in v0.1.")});
			}
			else
			{
				AddConnection(RecordConnections, Segment.EndpointA, Segment.EndpointB, Segment.Id);
			}
		}

		if (RoadTypes.Find(Segment.Definition.RoadTypeId) == nullptr)
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::MissingRoadType,
				TEXT("RoadSegment"),
				Segment.Id.GetValue(),
				TEXT("A road segment must reference an existing road type.")});
		}

		if (Segment.Definition.SpeedLimitOverrideMetersPerSecond.IsSet())
		{
			const double Override = Segment.Definition.SpeedLimitOverrideMetersPerSecond.GetValue();
			if (!std::isfinite(Override))
			{
				Report.Add({
					EValidationSeverity::Error,
					EValidationIssueCode::NonFiniteSpeedLimitOverride,
					TEXT("RoadSegment"),
					Segment.Id.GetValue(),
					TEXT("A speed-limit override must be finite.")});
			}
			else if (Override <= 0.0)
			{
				Report.Add({
					EValidationSeverity::Error,
					EValidationIssueCode::NonPositiveSpeedLimitOverride,
					TEXT("RoadSegment"),
					Segment.Id.GetValue(),
					TEXT("A speed-limit override must be greater than zero.")});
			}
		}

		if (!std::isfinite(Segment.LengthMeters))
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::NonFiniteRoadSegmentLength,
				TEXT("RoadSegment"),
				Segment.Id.GetValue(),
				TEXT("A road-segment length must be finite.")});
		}
		else if (Segment.LengthMeters <= 0.0)
		{
			Report.Add({
				EValidationSeverity::Error,
				EValidationIssueCode::NonPositiveRoadSegmentLength,
				TEXT("RoadSegment"),
				Segment.Id.GetValue(),
				TEXT("A road-segment length must be greater than zero.")});
		}

		if (NodeA != nullptr && NodeB != nullptr &&
			NodeA->PositionMeters.IsFinite() && NodeB->PositionMeters.IsFinite())
		{
			const double ExpectedLength = DistanceBetween(NodeA->PositionMeters, NodeB->PositionMeters);
			if (ExpectedLength <= 0.0 ||
				std::fabs(Segment.LengthMeters - ExpectedLength) > LengthValidationToleranceMeters)
			{
				Report.Add({
					EValidationSeverity::Error,
					EValidationIssueCode::RoadSegmentLengthMismatch,
					TEXT("RoadSegment"),
					Segment.Id.GetValue(),
					TEXT("The cached road-segment length does not match its endpoint geometry.")});
			}
		}
	}

	return Report;
}

} // namespace CityForm::Simulation
