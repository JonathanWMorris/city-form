// Copyright Jonathan Morris. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "CitySimulation/CitySimulation.h"
#include "CitySimulation/Routing.h"
#include "Misc/AutomationTest.h"

using namespace CityForm::Simulation;

namespace
{

constexpr EAutomationTestFlags RoutingTestFlags =
	EAutomationTestFlags_ApplicationContextMask |
	EAutomationTestFlags::EngineFilter;

FRoadNodeId AddNode(
	FAutomationTestBase& Test,
	FCitySimulation& City,
	const double X,
	const double Y)
{
	const FAddRoadNodeResult Result = City.AddRoadNode({X, Y});
	Test.TestTrue(TEXT("Test setup adds a road node."), Result.IsSuccess());
	return Result.NodeId;
}

FRoadSegmentId AddSegment(
	FAutomationTestBase& Test,
	FCitySimulation& City,
	const FRoadNodeId From,
	const FRoadNodeId To,
	const TOptional<double>& SpeedOverride = {})
{
	const FAddRoadSegmentResult Result = City.AddRoadSegment(
		From,
		To,
		{
			FRoadTypeCatalog::GetBasicTwoWayRoadTypeId(),
			SpeedOverride
		});
	Test.TestTrue(TEXT("Test setup adds a road segment."), Result.IsSuccess());
	return Result.SegmentId;
}

class FTimeSwitchingCostProvider final : public ITraversalCostProvider
{
public:
	FTimeSwitchingCostProvider(
		const FRoadSegmentId InEarlyPathFirst,
		const FRoadSegmentId InEarlyPathSecond,
		const FRoadSegmentId InStablePathFirst,
		const FRoadSegmentId InStablePathSecond)
		: EarlyPathFirst(InEarlyPathFirst)
		, EarlyPathSecond(InEarlyPathSecond)
		, StablePathFirst(InStablePathFirst)
		, StablePathSecond(InStablePathSecond)
	{
	}

	virtual bool GuaranteesFifo() const override
	{
		return true;
	}

	virtual FTraversalCostResult Evaluate(
		const FRoadGraph&,
		const FRoadTypeCatalog&,
		const FRoadTraversal& Traversal,
		const FSimulationInstant EntryInstant,
		const FVehicleClassDefinition&) const override
	{
		int64 DurationMilliseconds = 0;
		if (Traversal.SegmentId == EarlyPathFirst)
		{
			DurationMilliseconds =
				EntryInstant.GetMillisecondsSinceStart() < 50000
					? 10000
					: 100000;
		}
		else if (Traversal.SegmentId == EarlyPathSecond)
		{
			DurationMilliseconds = 10000;
		}
		else if (
			Traversal.SegmentId == StablePathFirst ||
			Traversal.SegmentId == StablePathSecond)
		{
			DurationMilliseconds = 30000;
		}
		else
		{
			return FTraversalCostResult::Prohibited();
		}

		return {
			FSimulationDuration(DurationMilliseconds),
			ETraversalCostStatus::Success,
			{}};
	}

private:
	FRoadSegmentId EarlyPathFirst;
	FRoadSegmentId EarlyPathSecond;
	FRoadSegmentId StablePathFirst;
	FRoadSegmentId StablePathSecond;
};

enum class EContractProviderBehavior : uint8
{
	NonFifo,
	Negative,
	Zero,
	Prohibited,
	Overflow
};

class FContractCostProvider final : public ITraversalCostProvider
{
public:
	explicit FContractCostProvider(const EContractProviderBehavior InBehavior)
		: Behavior(InBehavior)
	{
	}

	virtual bool GuaranteesFifo() const override
	{
		return Behavior != EContractProviderBehavior::NonFifo;
	}

	virtual FTraversalCostResult Evaluate(
		const FRoadGraph&,
		const FRoadTypeCatalog&,
		const FRoadTraversal&,
		const FSimulationInstant,
		const FVehicleClassDefinition&) const override
	{
		switch (Behavior)
		{
		case EContractProviderBehavior::Negative:
			return {
				FSimulationDuration(-1),
				ETraversalCostStatus::Success,
				{}};
		case EContractProviderBehavior::Zero:
			return {
				FSimulationDuration(),
				ETraversalCostStatus::Success,
				{}};
		case EContractProviderBehavior::Prohibited:
			return FTraversalCostResult::Prohibited();
		case EContractProviderBehavior::Overflow:
			return {
				FSimulationDuration(10),
				ETraversalCostStatus::Success,
				{}};
		case EContractProviderBehavior::NonFifo:
		default:
			return {
				FSimulationDuration(1),
				ETraversalCostStatus::Success,
				{}};
		}
	}

private:
	EContractProviderBehavior Behavior;
};

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRouteQueryTest,
	"CityForm.Simulation.Routing.RouteQueries",
	RoutingTestFlags)

bool FRouteQueryTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({101});
	const FRoadNodeId A = AddNode(*this, City, 0.0, 0.0);
	const FRoadNodeId B = AddNode(*this, City, 100.0, 0.0);
	const FRoadNodeId Disconnected = AddNode(*this, City, 200.0, 0.0);
	const FRoadSegmentId Segment =
		AddSegment(*this, City, A, B, TOptional<double>(10.0));

	const FRouteResult Direct = City.FindRoute(
		{A, B, FSimulationInstant(1000), FVehicleClassId(1)});
	TestTrue(TEXT("A direct route succeeds."), Direct.IsSuccess());
	TestEqual(TEXT("A direct route has one traversal."), Direct.Route.Traversals.Num(), 1);
	TestEqual(TEXT("The direct route uses the segment."), Direct.Route.Traversals[0].SegmentId, Segment);
	TestEqual(TEXT("A direct route has both endpoint nodes."), Direct.Route.NodeIds.Num(), 2);
	TestEqual(TEXT("The route distance is measured in meters."), Direct.Route.TotalDistanceMeters, 100.0);
	TestEqual(
		TEXT("The route duration uses milliseconds."),
		Direct.Route.TravelDuration.GetMilliseconds(),
		int64(10000));
	TestEqual(
		TEXT("Arrival includes the departure instant."),
		Direct.Route.ArrivalInstant.GetMillisecondsSinceStart(),
		int64(11000));

	const FRouteResult SameNode = City.FindRoute(
		{A, A, FSimulationInstant(2000), FVehicleClassId(1)});
	TestTrue(TEXT("A same-node query succeeds."), SameNode.IsSuccess());
	TestEqual(TEXT("A same-node route has no traversals."), SameNode.Route.Traversals.Num(), 0);
	TestEqual(TEXT("A same-node route retains its node."), SameNode.Route.NodeIds.Num(), 1);
	TestEqual(
		TEXT("A same-node route has zero duration."),
		SameNode.Route.TravelDuration.GetMilliseconds(),
		int64(0));

	const FRouteResult DisconnectedResult = City.FindRoute(
		{A, Disconnected, FSimulationInstant(), FVehicleClassId(1)});
	TestTrue(
		TEXT("A disconnected query has a stable code."),
		DisconnectedResult.Error.Code == ERouteErrorCode::Disconnected);

	const FRouteResult InvalidOrigin = City.FindRoute(
		{FRoadNodeId(999), B, FSimulationInstant(), FVehicleClassId(1)});
	TestTrue(
		TEXT("An invalid origin has a stable code."),
		InvalidOrigin.Error.Code == ERouteErrorCode::InvalidOrigin);

	const FRouteResult InvalidDestination = City.FindRoute(
		{A, FRoadNodeId(999), FSimulationInstant(), FVehicleClassId(1)});
	TestTrue(
		TEXT("An invalid destination has a stable code."),
		InvalidDestination.Error.Code == ERouteErrorCode::InvalidDestination);

	const FRouteResult InvalidVehicle = City.FindRoute(
		{A, B, FSimulationInstant(), FVehicleClassId(999)});
	TestTrue(
		TEXT("An invalid VehicleClass has a stable code."),
		InvalidVehicle.Error.Code == ERouteErrorCode::InvalidVehicleClass);

	const FRouteResult InvalidDeparture = City.FindRoute(
		{A, B, FSimulationInstant(-1), FVehicleClassId(1)});
	TestTrue(
		TEXT("A negative departure has a stable code."),
		InvalidDeparture.Error.Code == ERouteErrorCode::InvalidDepartureTime);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRouteOptimalityTest,
	"CityForm.Simulation.Routing.OptimalityAndHeuristics",
	RoutingTestFlags)

bool FRouteOptimalityTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({102});
	const FRoadNodeId A = AddNode(*this, City, 0.0, 0.0);
	const FRoadNodeId B = AddNode(*this, City, 100.0, 0.0);
	const FRoadNodeId C = AddNode(*this, City, 0.0, 100.0);
	const FRoadNodeId D = AddNode(*this, City, 200.0, 0.0);
	const FRoadSegmentId AB = AddSegment(*this, City, A, B);
	const FRoadSegmentId BD = AddSegment(*this, City, B, D);
	AddSegment(*this, City, A, C);
	AddSegment(*this, City, C, D);

	const FFreeFlowTraversalCostProvider Provider;
	const FRouteQuery Query{A, D, FSimulationInstant(), FVehicleClassId(1)};
	const FRouteResult AStar = FTimeDependentRouter::FindRoute(
		City.GetRoadGraph(),
		City.GetRoadTypes(),
		City.GetVehicleClasses(),
		Query,
		Provider);
	const FRouteResult Dijkstra = FTimeDependentRouter::FindRoute(
		City.GetRoadGraph(),
		City.GetRoadTypes(),
		City.GetVehicleClasses(),
		Query,
		Provider,
		{ERouteHeuristicMode::Zero});

	TestTrue(TEXT("The known-optimal A* route succeeds."), AStar.IsSuccess());
	TestTrue(TEXT("The zero-heuristic route succeeds."), Dijkstra.IsSuccess());
	TestEqual(TEXT("The optimal route has two segments."), AStar.Route.Traversals.Num(), 2);
	TestEqual(TEXT("The optimal route begins on AB."), AStar.Route.Traversals[0].SegmentId, AB);
	TestEqual(TEXT("The optimal route finishes on BD."), AStar.Route.Traversals[1].SegmentId, BD);
	TestEqual(
		TEXT("A* and zero-heuristic duration agree."),
		AStar.Route.TravelDuration.GetMilliseconds(),
		Dijkstra.Route.TravelDuration.GetMilliseconds());
	TestTrue(
		TEXT("A* and zero-heuristic traversals agree."),
		AStar.Route.Traversals == Dijkstra.Route.Traversals);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRouteTieBreakingTest,
	"CityForm.Simulation.Routing.DeterministicTieBreaking",
	RoutingTestFlags)

bool FRouteTieBreakingTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({103});
	const FRoadNodeId A = AddNode(*this, City, 0.0, 0.0);
	const FRoadNodeId B = AddNode(*this, City, 1.0, 1.0);
	const FRoadNodeId C = AddNode(*this, City, 1.0, -1.0);
	const FRoadNodeId D = AddNode(*this, City, 2.0, 0.0);
	const FRoadSegmentId AB = AddSegment(*this, City, A, B);
	const FRoadSegmentId BD = AddSegment(*this, City, B, D);
	AddSegment(*this, City, A, C);
	AddSegment(*this, City, C, D);

	for (int32 Attempt = 0; Attempt < 3; ++Attempt)
	{
		const FRouteResult Route = City.FindRoute(
			{A, D, FSimulationInstant(), FVehicleClassId(1)});
		TestTrue(TEXT("An equal-cost route succeeds."), Route.IsSuccess());
		TestEqual(TEXT("The stable route begins on the lowest segment."), Route.Route.Traversals[0].SegmentId, AB);
		TestEqual(TEXT("The stable destination predecessor wins."), Route.Route.Traversals[1].SegmentId, BD);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVehicleAwareFreeFlowTest,
	"CityForm.Simulation.Routing.VehicleAwareFreeFlow",
	RoutingTestFlags)

bool FVehicleAwareFreeFlowTest::RunTest(const FString& Parameters)
{
	FCitySimulation DefaultRoadCity({104});
	const FRoadNodeId DefaultA = AddNode(*this, DefaultRoadCity, 0.0, 0.0);
	const FRoadNodeId DefaultB = AddNode(*this, DefaultRoadCity, 100.0, 0.0);
	AddSegment(*this, DefaultRoadCity, DefaultA, DefaultB);
	const FRouteResult RegionalDefault = DefaultRoadCity.FindRoute(
		{DefaultA, DefaultB, FSimulationInstant(), FVehicleClassId(1)});
	TestEqual(
		TEXT("The California road default resolves to 8,948 milliseconds."),
		RegionalDefault.Route.TravelDuration.GetMilliseconds(),
		int64(8948));

	FCitySimulation OverrideCity({105});
	const FRoadNodeId OverrideA = AddNode(*this, OverrideCity, 0.0, 0.0);
	const FRoadNodeId OverrideB = AddNode(*this, OverrideCity, 100.0, 0.0);
	AddSegment(*this, OverrideCity, OverrideA, OverrideB, TOptional<double>(20.0));

	const FRouteResult PassengerCar = OverrideCity.FindRoute(
		{OverrideA, OverrideB, FSimulationInstant(), FVehicleClassId(1)});
	TestEqual(
		TEXT("The segment speed override affects free-flow cost."),
		PassengerCar.Route.TravelDuration.GetMilliseconds(),
		int64(5000));

	FVehicleClassDefinition SlowDefinition =
		FVehicleClassCatalog::MakeProvisionalPassengerCar();
	SlowDefinition.Id = FVehicleClassId(2);
	SlowDefinition.MaximumSpeedMetersPerSecond = 5.0;
	TArray<FVehicleClassDefinition> SlowDefinitions;
	SlowDefinitions.Add(SlowDefinition);
	const FVehicleClassCatalog SlowCatalog(MoveTemp(SlowDefinitions));
	const FFreeFlowTraversalCostProvider Provider;
	const FRouteResult SlowVehicle = FTimeDependentRouter::FindRoute(
		OverrideCity.GetRoadGraph(),
		OverrideCity.GetRoadTypes(),
		SlowCatalog,
		{OverrideA, OverrideB, FSimulationInstant(), FVehicleClassId(2)},
		Provider);
	TestEqual(
		TEXT("Vehicle maximum speed constrains free-flow cost."),
		SlowVehicle.Route.TravelDuration.GetMilliseconds(),
		int64(20000));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTimeDependentRouteTest,
	"CityForm.Simulation.Routing.DepartureTimeChangesRoute",
	RoutingTestFlags)

bool FTimeDependentRouteTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({106});
	const FRoadNodeId A = AddNode(*this, City, 0.0, 0.0);
	const FRoadNodeId B = AddNode(*this, City, 1.0, 1.0);
	const FRoadNodeId C = AddNode(*this, City, 1.0, -1.0);
	const FRoadNodeId D = AddNode(*this, City, 2.0, 0.0);
	const FRoadSegmentId AB = AddSegment(*this, City, A, B);
	const FRoadSegmentId BD = AddSegment(*this, City, B, D);
	const FRoadSegmentId AC = AddSegment(*this, City, A, C);
	const FRoadSegmentId CD = AddSegment(*this, City, C, D);

	const FTimeSwitchingCostProvider Provider(AB, BD, AC, CD);
	const FRouteOptions ZeroHeuristic{ERouteHeuristicMode::Zero};
	const FRouteResult Early = FTimeDependentRouter::FindRoute(
		City.GetRoadGraph(),
		City.GetRoadTypes(),
		City.GetVehicleClasses(),
		{A, D, FSimulationInstant(0), FVehicleClassId(1)},
		Provider,
		ZeroHeuristic);
	const FRouteResult Late = FTimeDependentRouter::FindRoute(
		City.GetRoadGraph(),
		City.GetRoadTypes(),
		City.GetVehicleClasses(),
		{A, D, FSimulationInstant(60000), FVehicleClassId(1)},
		Provider,
		ZeroHeuristic);

	TestEqual(TEXT("The early route uses its fast path."), Early.Route.Traversals[0].SegmentId, AB);
	TestEqual(TEXT("The late route changes to the stable path."), Late.Route.Traversals[0].SegmentId, AC);
	TestEqual(
		TEXT("The early route takes twenty seconds."),
		Early.Route.TravelDuration.GetMilliseconds(),
		int64(20000));
	TestEqual(
		TEXT("The late route takes sixty seconds."),
		Late.Route.TravelDuration.GetMilliseconds(),
		int64(60000));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRouteProviderContractTest,
	"CityForm.Simulation.Routing.ProviderContracts",
	RoutingTestFlags)

bool FRouteProviderContractTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({107});
	const FRoadNodeId A = AddNode(*this, City, 0.0, 0.0);
	const FRoadNodeId B = AddNode(*this, City, 1.0, 0.0);
	AddSegment(*this, City, A, B);
	const FRouteOptions ZeroHeuristic{ERouteHeuristicMode::Zero};

	const auto RunWith = [&](const EContractProviderBehavior Behavior, const int64 Departure = 0)
	{
		const FContractCostProvider Provider(Behavior);
		return FTimeDependentRouter::FindRoute(
			City.GetRoadGraph(),
			City.GetRoadTypes(),
			City.GetVehicleClasses(),
			{A, B, FSimulationInstant(Departure), FVehicleClassId(1)},
			Provider,
			ZeroHeuristic);
	};

	TestTrue(
		TEXT("A non-FIFO provider is rejected."),
		RunWith(EContractProviderBehavior::NonFifo).Error.Code ==
			ERouteErrorCode::ProviderDoesNotGuaranteeFifo);
	TestTrue(
		TEXT("A negative traversal cost is rejected."),
		RunWith(EContractProviderBehavior::Negative).Error.Code ==
			ERouteErrorCode::InvalidTraversalCost);
	const FRouteResult ZeroCost =
		RunWith(EContractProviderBehavior::Zero);
	TestTrue(TEXT("A zero traversal cost succeeds."), ZeroCost.IsSuccess());
	TestEqual(
		TEXT("A zero traversal cost is accepted."),
		ZeroCost.Route.TravelDuration.GetMilliseconds(),
		int64(0));
	TestTrue(
		TEXT("A fully prohibited path is distinct from disconnection."),
		RunWith(EContractProviderBehavior::Prohibited).Error.Code ==
			ERouteErrorCode::NoPermittedRoute);
	TestTrue(
		TEXT("Arrival overflow has a stable route code."),
		RunWith(EContractProviderBehavior::Overflow, MAX_int64 - 5).Error.Code ==
			ERouteErrorCode::TimeOverflow);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
