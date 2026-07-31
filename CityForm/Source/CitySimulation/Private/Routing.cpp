// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/Routing.h"

#include "Algo/Reverse.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/Optional.h"

#include <cmath>
#include <queue>
#include <vector>

namespace CityForm::Simulation
{
namespace
{

struct FNodeRouteState
{
	int64 ArrivalMilliseconds = 0;
	int64 HeuristicMilliseconds = 0;
	TOptional<FRoadTraversal> Predecessor;
	bool bReached = false;
};

struct FOpenEntry
{
	FRoadNodeId NodeId;
	int64 ArrivalMilliseconds = 0;
	int64 HeuristicMilliseconds = 0;
	int64 PredictedTotalMilliseconds = 0;
};

struct FOpenEntryWorse
{
	bool operator()(const FOpenEntry& Left, const FOpenEntry& Right) const
	{
		if (Left.PredictedTotalMilliseconds != Right.PredictedTotalMilliseconds)
		{
			return Left.PredictedTotalMilliseconds > Right.PredictedTotalMilliseconds;
		}
		if (Left.HeuristicMilliseconds != Right.HeuristicMilliseconds)
		{
			return Left.HeuristicMilliseconds > Right.HeuristicMilliseconds;
		}
		return Right.NodeId < Left.NodeId;
	}
};

FRouteResult Failure(const ERouteErrorCode Code, FString Message)
{
	FRouteResult Result;
	Result.Error = {Code, MoveTemp(Message)};
	return Result;
}

bool IsValidVehicleClass(const FVehicleClassDefinition& VehicleClass)
{
	return VehicleClass.Id.IsValid() && std::isfinite(VehicleClass.MaximumSpeedMetersPerSecond) &&
		VehicleClass.MaximumSpeedMetersPerSecond > 0.0;
}

FRouteError ComputeHeuristic(const FRoadGraph& Graph,
	const FRoadNodeId NodeId,
	const FRoadNodeId DestinationNodeId,
	const FVehicleClassDefinition& VehicleClass,
	const ERouteHeuristicMode Mode,
	int64& OutMilliseconds)
{
	OutMilliseconds = 0;
	if (Mode == ERouteHeuristicMode::Zero)
	{
		return {};
	}

	const FRoadNode* Node = Graph.FindNode(NodeId);
	const FRoadNode* Destination = Graph.FindNode(DestinationNodeId);
	if (Node == nullptr || Destination == nullptr)
	{
		return {ERouteErrorCode::InvalidGraph, TEXT("The routing heuristic could not resolve a graph node.")};
	}

	const double DistanceMeters = std::hypot(
		Destination->PositionMeters.X - Node->PositionMeters.X, Destination->PositionMeters.Y - Node->PositionMeters.Y);
	const double Milliseconds = DistanceMeters / VehicleClass.MaximumSpeedMetersPerSecond * 1000.0;
	if (!std::isfinite(Milliseconds) || Milliseconds < 0.0 || Milliseconds >= static_cast<double>(MAX_int64))
	{
		return {ERouteErrorCode::InvalidTraversalCost, TEXT("The routing heuristic produced an invalid duration.")};
	}

	OutMilliseconds = static_cast<int64>(std::floor(Milliseconds));
	return {};
}

bool IsPreferredPredecessor(const FRoadTraversal& Candidate, const TOptional<FRoadTraversal>& Existing)
{
	if (!Existing.IsSet())
	{
		return true;
	}
	if (Candidate.SegmentId != Existing->SegmentId)
	{
		return Candidate.SegmentId < Existing->SegmentId;
	}
	return Candidate.FromNodeId < Existing->FromNodeId;
}

bool WouldCreatePredecessorCycle(const FRoadTraversal& Candidate, const TMap<FRoadNodeId, FNodeRouteState>& States)
{
	FRoadNodeId CurrentNodeId = Candidate.FromNodeId;
	int32 RemainingNodes = States.Num() + 1;
	while (CurrentNodeId.IsValid() && RemainingNodes-- > 0)
	{
		if (CurrentNodeId == Candidate.ToNodeId)
		{
			return true;
		}

		const FNodeRouteState* State = States.Find(CurrentNodeId);
		if (State == nullptr || !State->Predecessor.IsSet())
		{
			return false;
		}
		CurrentNodeId = State->Predecessor->FromNodeId;
	}

	return RemainingNodes < 0;
}

bool HasTopologicalPath(const FRoadGraph& Graph, const FRoadNodeId OriginNodeId, const FRoadNodeId DestinationNodeId)
{
	TArray<FRoadNodeId> Pending;
	TSet<FRoadNodeId> Visited;
	Pending.Add(OriginNodeId);
	Visited.Add(OriginNodeId);

	while (!Pending.IsEmpty())
	{
		const FRoadNodeId CurrentNodeId = Pending.Pop(EAllowShrinking::No);
		if (CurrentNodeId == DestinationNodeId)
		{
			return true;
		}

		for (const FRoadTraversal& Traversal : Graph.GetOutgoingTraversals(CurrentNodeId))
		{
			if (!Visited.Contains(Traversal.ToNodeId))
			{
				Visited.Add(Traversal.ToNodeId);
				Pending.Add(Traversal.ToNodeId);
			}
		}
	}

	return false;
}

FRouteResult ReconstructRoute(
	const FRoadGraph& Graph, const FRouteQuery& Query, const TMap<FRoadNodeId, FNodeRouteState>& States)
{
	const FNodeRouteState* DestinationState = States.Find(Query.DestinationNodeId);
	if (DestinationState == nullptr || !DestinationState->bReached)
	{
		return Failure(ERouteErrorCode::Disconnected, TEXT("The destination is disconnected from the origin."));
	}

	FRoute Route;
	Route.ArrivalInstant = FSimulationInstant(DestinationState->ArrivalMilliseconds);
	Route.TravelDuration =
		FSimulationDuration(DestinationState->ArrivalMilliseconds - Query.DepartureInstant.GetMillisecondsSinceStart());

	FRoadNodeId CurrentNodeId = Query.DestinationNodeId;
	Route.NodeIds.Add(CurrentNodeId);
	while (CurrentNodeId != Query.OriginNodeId)
	{
		const FNodeRouteState* State = States.Find(CurrentNodeId);
		if (State == nullptr || !State->Predecessor.IsSet())
		{
			return Failure(ERouteErrorCode::InvalidGraph, TEXT("Route reconstruction found a missing predecessor."));
		}

		const FRoadTraversal Traversal = State->Predecessor.GetValue();
		const FRoadSegment* Segment = Graph.FindSegment(Traversal.SegmentId);
		if (Segment == nullptr)
		{
			return Failure(ERouteErrorCode::InvalidGraph, TEXT("Route reconstruction found a missing road segment."));
		}

		Route.Traversals.Add(Traversal);
		Route.TotalDistanceMeters += Segment->LengthMeters;
		CurrentNodeId = Traversal.FromNodeId;
		Route.NodeIds.Add(CurrentNodeId);

		if (Route.NodeIds.Num() > Graph.GetNodes().Num())
		{
			return Failure(ERouteErrorCode::InvalidGraph, TEXT("Route reconstruction detected a predecessor cycle."));
		}
	}

	Algo::Reverse(Route.Traversals);
	Algo::Reverse(Route.NodeIds);
	return {MoveTemp(Route), {}};
}

} // namespace

FTraversalCostResult FTraversalCostResult::Prohibited()
{
	FTraversalCostResult Result;
	Result.Status = ETraversalCostStatus::Prohibited;
	return Result;
}

FTraversalCostResult FTraversalCostResult::Failure(const ERouteErrorCode Code, FString Message)
{
	FTraversalCostResult Result;
	Result.Status = ETraversalCostStatus::Error;
	Result.Error = {Code, MoveTemp(Message)};
	return Result;
}

bool FFreeFlowTraversalCostProvider::GuaranteesFifo() const
{
	return true;
}

FTraversalCostResult FFreeFlowTraversalCostProvider::Evaluate(const FRoadGraph& Graph,
	const FRoadTypeCatalog& RoadTypes,
	const FRoadTraversal& Traversal,
	const FSimulationInstant,
	const FVehicleClassDefinition& VehicleClass) const
{
	const FRoadSegment* Segment = Graph.FindSegment(Traversal.SegmentId);
	if (Segment == nullptr ||
		(Traversal.FromNodeId != Segment->EndpointA && Traversal.FromNodeId != Segment->EndpointB) ||
		(Traversal.ToNodeId != Segment->EndpointA && Traversal.ToNodeId != Segment->EndpointB) ||
		Traversal.FromNodeId == Traversal.ToNodeId)
	{
		return FTraversalCostResult::Failure(
			ERouteErrorCode::InvalidGraph, TEXT("A traversal must reference a segment and its two endpoints."));
	}

	const FRoadSpeedLimitResult SpeedLimit = Graph.ResolveSpeedLimit(*Segment, RoadTypes);
	if (!SpeedLimit.IsSuccess())
	{
		return FTraversalCostResult::Failure(
			ERouteErrorCode::InvalidTraversalCost, TEXT("The traversal speed limit could not be resolved."));
	}

	const double EffectiveSpeedMetersPerSecond =
		FMath::Min(SpeedLimit.SpeedLimitMetersPerSecond, VehicleClass.MaximumSpeedMetersPerSecond);
	const double Milliseconds = Segment->LengthMeters / EffectiveSpeedMetersPerSecond * 1000.0;
	if (!std::isfinite(Milliseconds) || Milliseconds <= 0.0 || Milliseconds >= static_cast<double>(MAX_int64))
	{
		return FTraversalCostResult::Failure(ERouteErrorCode::InvalidTraversalCost,
			TEXT("Free-flow traversal time must be positive and representable."));
	}

	return {FSimulationDuration(FMath::Max<int64>(1, static_cast<int64>(std::ceil(Milliseconds)))),
		ETraversalCostStatus::Success,
		{}};
}

FRouteResult FTimeDependentRouter::FindRoute(const FRoadGraph& Graph,
	const FRoadTypeCatalog& RoadTypes,
	const FVehicleClassCatalog& VehicleClasses,
	const FRouteQuery& Query,
	const ITraversalCostProvider& CostProvider,
	const FRouteOptions Options)
{
	const FRoadNode* Origin = Graph.FindNode(Query.OriginNodeId);
	if (Origin == nullptr)
	{
		return Failure(ERouteErrorCode::InvalidOrigin, TEXT("The route origin must reference an existing road node."));
	}
	if (Graph.FindNode(Query.DestinationNodeId) == nullptr)
	{
		return Failure(
			ERouteErrorCode::InvalidDestination, TEXT("The route destination must reference an existing road node."));
	}
	if (Query.DepartureInstant.GetMillisecondsSinceStart() < 0)
	{
		return Failure(ERouteErrorCode::InvalidDepartureTime, TEXT("The route departure instant cannot be negative."));
	}

	const FVehicleClassDefinition* VehicleClass = VehicleClasses.Find(Query.VehicleClassId);
	if (VehicleClass == nullptr || !IsValidVehicleClass(*VehicleClass))
	{
		return Failure(ERouteErrorCode::InvalidVehicleClass, TEXT("The route must reference a valid VehicleClass."));
	}
	if (!CostProvider.GuaranteesFifo())
	{
		return Failure(ERouteErrorCode::ProviderDoesNotGuaranteeFifo,
			TEXT("Time-dependent A* requires a FIFO traversal-cost provider."));
	}

	if (Query.OriginNodeId == Query.DestinationNodeId)
	{
		FRoute Route;
		Route.NodeIds.Add(Query.OriginNodeId);
		Route.ArrivalInstant = Query.DepartureInstant;
		return {MoveTemp(Route), {}};
	}

	TMap<FRoadNodeId, FNodeRouteState> States;
	int64 OriginHeuristic = 0;
	const FRouteError OriginHeuristicError = ComputeHeuristic(
		Graph, Query.OriginNodeId, Query.DestinationNodeId, *VehicleClass, Options.HeuristicMode, OriginHeuristic);
	if (OriginHeuristicError.IsSet())
	{
		return {{}, OriginHeuristicError};
	}

	FNodeRouteState& OriginState = States.Add(Query.OriginNodeId);
	OriginState.ArrivalMilliseconds = Query.DepartureInstant.GetMillisecondsSinceStart();
	OriginState.HeuristicMilliseconds = OriginHeuristic;
	OriginState.bReached = true;
	if (OriginHeuristic > MAX_int64 - OriginState.ArrivalMilliseconds)
	{
		return Failure(ERouteErrorCode::TimeOverflow, TEXT("A* predicted total time exceeds the simulation range."));
	}

	std::priority_queue<FOpenEntry, std::vector<FOpenEntry>, FOpenEntryWorse> Open;
	Open.push({Query.OriginNodeId,
		OriginState.ArrivalMilliseconds,
		OriginHeuristic,
		OriginState.ArrivalMilliseconds + OriginHeuristic});

	while (!Open.empty())
	{
		const FOpenEntry Current = Open.top();
		Open.pop();

		const FNodeRouteState* CurrentState = States.Find(Current.NodeId);
		if (CurrentState == nullptr || Current.ArrivalMilliseconds != CurrentState->ArrivalMilliseconds)
		{
			continue;
		}

		const FNodeRouteState* DestinationState = States.Find(Query.DestinationNodeId);
		if (DestinationState != nullptr && DestinationState->bReached &&
			Current.PredictedTotalMilliseconds > DestinationState->ArrivalMilliseconds)
		{
			break;
		}

		const TArray<FRoadTraversal> Traversals = Graph.GetOutgoingTraversals(Current.NodeId);
		for (const FRoadTraversal& Traversal : Traversals)
		{
			const FTraversalCostResult Cost = CostProvider.Evaluate(
				Graph, RoadTypes, Traversal, FSimulationInstant(Current.ArrivalMilliseconds), *VehicleClass);
			if (Cost.IsProhibited())
			{
				continue;
			}
			if (!Cost.IsSuccess() || Cost.Duration.GetMilliseconds() < 0)
			{
				return Failure(Cost.Error.IsSet() ? Cost.Error.Code : ERouteErrorCode::InvalidTraversalCost,
					Cost.Error.IsSet() ? Cost.Error.Message : TEXT("A traversal cost must be non-negative."));
			}

			const FSimulationInstantResult Arrival =
				AddSimulationDuration(FSimulationInstant(Current.ArrivalMilliseconds), Cost.Duration);
			if (!Arrival.IsSuccess())
			{
				return Failure(ERouteErrorCode::TimeOverflow, TEXT("Route arrival time exceeds the simulation range."));
			}

			FNodeRouteState* ExistingState = States.Find(Traversal.ToNodeId);
			const int64 CandidateArrival = Arrival.Instant.GetMillisecondsSinceStart();
			const bool bBetterArrival = ExistingState == nullptr || !ExistingState->bReached ||
				CandidateArrival < ExistingState->ArrivalMilliseconds;
			const bool bPreferredEqualArrival = ExistingState != nullptr && ExistingState->bReached &&
				CandidateArrival == ExistingState->ArrivalMilliseconds &&
				IsPreferredPredecessor(Traversal, ExistingState->Predecessor);
			if (!bBetterArrival && !bPreferredEqualArrival)
			{
				continue;
			}
			if (WouldCreatePredecessorCycle(Traversal, States))
			{
				continue;
			}

			if (ExistingState == nullptr)
			{
				ExistingState = &States.Add(Traversal.ToNodeId);
			}

			int64 Heuristic = 0;
			const FRouteError HeuristicError = ComputeHeuristic(
				Graph, Traversal.ToNodeId, Query.DestinationNodeId, *VehicleClass, Options.HeuristicMode, Heuristic);
			if (HeuristicError.IsSet())
			{
				return {{}, HeuristicError};
			}
			if (Heuristic > MAX_int64 - CandidateArrival)
			{
				return Failure(
					ERouteErrorCode::TimeOverflow, TEXT("A* predicted total time exceeds the simulation range."));
			}

			ExistingState->ArrivalMilliseconds = CandidateArrival;
			ExistingState->HeuristicMilliseconds = Heuristic;
			ExistingState->Predecessor = Traversal;
			ExistingState->bReached = true;
			Open.push({Traversal.ToNodeId, CandidateArrival, Heuristic, CandidateArrival + Heuristic});
		}
	}

	const FNodeRouteState* DestinationState = States.Find(Query.DestinationNodeId);
	if (DestinationState == nullptr || !DestinationState->bReached)
	{
		const bool bTopologicallyConnected = HasTopologicalPath(Graph, Query.OriginNodeId, Query.DestinationNodeId);
		return Failure(bTopologicallyConnected ? ERouteErrorCode::NoPermittedRoute : ERouteErrorCode::Disconnected,
			bTopologicallyConnected ? TEXT("No permitted route connects the origin and destination.")
									: TEXT("The destination is disconnected from the origin."));
	}

	return ReconstructRoute(Graph, Query, States);
}

} // namespace CityForm::Simulation
