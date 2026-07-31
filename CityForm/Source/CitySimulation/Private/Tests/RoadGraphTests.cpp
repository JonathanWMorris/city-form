// Copyright Jonathan Morris. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "CitySimulation/CitySimulation.h"
#include "CitySimulation/RoadGraph.h"
#include "CitySimulation/RoadType.h"
#include "Misc/AutomationTest.h"

#include <limits>
#include <type_traits>

using namespace CityForm::Simulation;

namespace
{

constexpr EAutomationTestFlags RoadGraphTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

int32 CountRoadIssues(const FValidationReport& Report, const EValidationIssueCode Code)
{
	int32 Count = 0;
	for (const FValidationIssue& Issue : Report.GetIssues())
	{
		if (Issue.Code == Code)
		{
			++Count;
		}
	}
	return Count;
}

FRoadSegmentDefinition MakeBasicRoad(const TOptional<double>& Override = TOptional<double>())
{
	return {FRoadTypeCatalog::GetBasicTwoWayRoadTypeId(), Override};
}

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionalRoadDefaultsTest,
	"CityForm.Simulation.RoadGraph.RegionalRoadDefaults",
	RoadGraphTestFlags)

bool FRegionalRoadDefaultsTest::RunTest(const FString& Parameters)
{
	const FRegionProfile California = FRegionProfile::MakeCalifornia();
	TestEqual(TEXT("The default region is California."), California.Identifier, FString(TEXT("US-CA")));
	TestEqual(TEXT("California's Basic Two-Way Road default is exactly 25 mph in m/s."),
		California.BasicTwoWayRoadDefaultSpeedLimitMetersPerSecond,
		11.176);
	TestTrue(TEXT("The California profile validates."), California.Validate().IsValid());

	FCitySimulation CaliforniaCity({101, California});
	const FRoadTypeDefinition* BasicRoad =
		CaliforniaCity.GetRoadTypes().Find(FRoadTypeCatalog::GetBasicTwoWayRoadTypeId());
	TestNotNull(TEXT("The Basic Two-Way Road has stable ID one."), BasicRoad);
	if (BasicRoad != nullptr)
	{
		TestEqual(TEXT("The road type has a stable key."), BasicRoad->Key, FString(TEXT("BasicTwoWayRoad")));
		TestEqual(
			TEXT("The road type receives the regional default."), BasicRoad->DefaultSpeedLimitMetersPerSecond, 11.176);
	}

	FRegionProfile CustomRegion = California;
	CustomRegion.Identifier = TEXT("TEST-REGION");
	CustomRegion.BasicTwoWayRoadDefaultSpeedLimitMetersPerSecond = 20.0;
	FCitySimulation CustomCity({102, CustomRegion});
	const FRoadSpeedLimitResult CustomDefault =
		CustomCity.GetRoadTypes().ResolveSpeedLimit(FRoadTypeCatalog::GetBasicTwoWayRoadTypeId(), TOptional<double>());
	TestTrue(TEXT("A custom regional default resolves."), CustomDefault.IsSuccess());
	TestEqual(TEXT("Changing city configuration changes the default without graph-code changes."),
		CustomDefault.SpeedLimitMetersPerSecond,
		20.0);

	const FRoadSpeedLimitResult Override = CustomCity.GetRoadTypes().ResolveSpeedLimit(
		FRoadTypeCatalog::GetBasicTwoWayRoadTypeId(), TOptional<double>(8.5));
	TestEqual(TEXT("A segment override takes precedence."), Override.SpeedLimitMetersPerSecond, 8.5);

	FRegionProfile InvalidRegion = California;
	InvalidRegion.Identifier.Reset();
	InvalidRegion.BasicTwoWayRoadDefaultSpeedLimitMetersPerSecond = std::numeric_limits<double>::quiet_NaN();
	const FValidationReport InvalidRegionReport = InvalidRegion.Validate();
	TestFalse(TEXT("An invalid regional profile fails validation."), InvalidRegionReport.IsValid());
	TestEqual(TEXT("An empty regional identifier is reported."),
		CountRoadIssues(InvalidRegionReport, EValidationIssueCode::EmptyRegionIdentifier),
		1);
	TestEqual(TEXT("A non-finite regional value is reported."),
		CountRoadIssues(InvalidRegionReport, EValidationIssueCode::NonFiniteRegionalValue),
		1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRoadTypeCatalogValidationTest,
	"CityForm.Simulation.RoadGraph.RoadTypeCatalogValidation",
	RoadGraphTestFlags)

bool FRoadTypeCatalogValidationTest::RunTest(const FString& Parameters)
{
	TArray<FRoadTypeDefinition> InvalidDefinitions;
	InvalidDefinitions.Add({FRoadTypeId(), TEXT(""), 0.0});
	InvalidDefinitions.Add({FRoadTypeId(2), TEXT("Duplicate"), 10.0});
	InvalidDefinitions.Add({FRoadTypeId(2), TEXT("Duplicate"), 12.0});
	const FValidationReport Report = FRoadTypeCatalog(MoveTemp(InvalidDefinitions)).Validate();

	TestFalse(TEXT("An invalid road-type catalog fails validation."), Report.IsValid());
	TestEqual(TEXT("An invalid road-type ID is reported."),
		CountRoadIssues(Report, EValidationIssueCode::InvalidRoadTypeId),
		1);
	TestEqual(TEXT("An empty road-type key is reported."),
		CountRoadIssues(Report, EValidationIssueCode::EmptyRoadTypeKey),
		1);
	TestEqual(TEXT("A non-positive road-type speed is reported."),
		CountRoadIssues(Report, EValidationIssueCode::NonPositiveRoadTypeValue),
		1);
	TestEqual(TEXT("A duplicate road-type ID is reported."),
		CountRoadIssues(Report, EValidationIssueCode::DuplicateRoadTypeId),
		1);
	TestEqual(TEXT("A duplicate road-type key is reported."),
		CountRoadIssues(Report, EValidationIssueCode::DuplicateRoadTypeKey),
		1);

	FRoadTypeCatalog CaliforniaCatalog;
	const FRoadSpeedLimitResult InvalidType =
		CaliforniaCatalog.ResolveSpeedLimit(FRoadTypeId(999), TOptional<double>());
	TestFalse(TEXT("An unknown road type cannot resolve a speed."), InvalidType.IsSuccess());
	TestTrue(TEXT("An unknown road type has a stable error code."),
		InvalidType.Error.Code == ESimulationErrorCode::InvalidRoadType);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRoadNodeCommandTest,
	"CityForm.Simulation.RoadGraph.RoadNodeCommands",
	RoadGraphTestFlags)

bool FRoadNodeCommandTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({201});

	const FAddRoadNodeResult InvalidNode = City.AddRoadNode({std::numeric_limits<double>::quiet_NaN(), 0.0});
	TestFalse(TEXT("A non-finite node is rejected."), InvalidNode.IsSuccess());
	TestTrue(TEXT("A non-finite node has a stable error code."),
		InvalidNode.Error.Code == ESimulationErrorCode::NonFiniteRoadPosition);

	const FAddRoadNodeResult FirstNode = City.AddRoadNode({10.0, 20.0});
	const FAddRoadNodeResult SecondNode = City.AddRoadNode({30.0, 40.0});
	TestTrue(TEXT("A finite node is accepted."), FirstNode.IsSuccess());
	TestEqual(TEXT("A rejected node does not consume an ID."), FirstNode.NodeId.GetValue(), uint64(1));
	TestEqual(TEXT("Node IDs are monotonic."), SecondNode.NodeId.GetValue(), uint64(2));

	const FRoadNode* StoredNode = City.GetRoadGraph().FindNode(FirstNode.NodeId);
	TestNotNull(TEXT("A created node can be found by ID."), StoredNode);
	if (StoredNode != nullptr)
	{
		TestEqual(TEXT("The node stores X in meters."), StoredNode->PositionMeters.X, 10.0);
		TestEqual(TEXT("The node stores Y in meters."), StoredNode->PositionMeters.Y, 20.0);
	}

	TestTrue(TEXT("Isolated nodes are valid."), City.Validate().IsValid());
	TestEqual(TEXT("The city summary counts road nodes."), City.GetSummary().RoadNodeCount, 2);
	TestEqual(TEXT("The city summary has no road segments yet."), City.GetSummary().RoadSegmentCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRoadSegmentCommandTest,
	"CityForm.Simulation.RoadGraph.RoadSegmentCommands",
	RoadGraphTestFlags)

bool FRoadSegmentCommandTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({301});
	const FRoadNodeId NodeA = City.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId NodeB = City.AddRoadNode({3.0, 4.0}).NodeId;
	const FRoadNodeId NodeC = City.AddRoadNode({10.0, 0.0}).NodeId;

	const FAddRoadSegmentResult SegmentAB = City.AddRoadSegment(NodeA, NodeB, MakeBasicRoad());
	const FAddRoadSegmentResult SegmentBC = City.AddRoadSegment(NodeB, NodeC, MakeBasicRoad(TOptional<double>(8.5)));
	const FAddRoadSegmentResult SegmentAC = City.AddRoadSegment(NodeA, NodeC, MakeBasicRoad());
	TestTrue(TEXT("The first road segment is accepted."), SegmentAB.IsSuccess());
	TestEqual(TEXT("Road-segment IDs begin at one."), SegmentAB.SegmentId.GetValue(), uint64(1));
	TestEqual(TEXT("Road-segment IDs are monotonic."), SegmentBC.SegmentId.GetValue(), uint64(2));
	TestEqual(TEXT("A third connection remains valid."), SegmentAC.SegmentId.GetValue(), uint64(3));

	const FRoadSegment* StoredAB = City.GetRoadGraph().FindSegment(SegmentAB.SegmentId);
	TestNotNull(TEXT("A created segment can be found by ID."), StoredAB);
	if (StoredAB != nullptr)
	{
		TestEqual(TEXT("Segment length is derived in meters."), StoredAB->LengthMeters, 5.0);
		const FRoadSpeedLimitResult Speed = City.GetRoadGraph().ResolveSpeedLimit(*StoredAB, City.GetRoadTypes());
		TestEqual(TEXT("An unmodified segment uses its road-type default."), Speed.SpeedLimitMetersPerSecond, 11.176);
	}

	const FRoadSegment* StoredBC = City.GetRoadGraph().FindSegment(SegmentBC.SegmentId);
	TestNotNull(TEXT("The overridden segment can be found."), StoredBC);
	if (StoredBC != nullptr)
	{
		const FRoadSpeedLimitResult Speed = City.GetRoadGraph().ResolveSpeedLimit(*StoredBC, City.GetRoadTypes());
		TestEqual(TEXT("A segment speed override is retained."), Speed.SpeedLimitMetersPerSecond, 8.5);
	}

	const TArray<FRoadTraversal> FromA = City.GetRoadGraph().GetOutgoingTraversals(NodeA);
	TestEqual(TEXT("Node A exposes two outgoing traversals."), FromA.Num(), 2);
	if (FromA.Num() == 2)
	{
		TestEqual(
			TEXT("Traversal ordering begins with the lowest segment ID."), FromA[0].SegmentId.GetValue(), uint64(1));
		TestTrue(TEXT("The first traversal points from A to B."),
			FromA[0] == (FRoadTraversal{SegmentAB.SegmentId, NodeA, NodeB}));
		TestEqual(TEXT("Traversal ordering remains deterministic."), FromA[1].SegmentId.GetValue(), uint64(3));
	}

	const TArray<FRoadTraversal> FromB = City.GetRoadGraph().GetOutgoingTraversals(NodeB);
	TestEqual(TEXT("Two-way segments expose reverse traversals."), FromB.Num(), 2);
	if (FromB.Num() > 0)
	{
		TestTrue(TEXT("The reverse traversal points from B to A."),
			FromB[0] == (FRoadTraversal{SegmentAB.SegmentId, NodeB, NodeA}));
	}

	TestTrue(TEXT("The populated graph validates."), City.Validate().IsValid());
	TestEqual(TEXT("The summary counts one road type."), City.GetSummary().RoadTypeCount, 1);
	TestEqual(TEXT("The summary counts three road segments."), City.GetSummary().RoadSegmentCount, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCompoundRoadCommandTest,
	"CityForm.Simulation.RoadGraph.CompoundRoadCommands",
	RoadGraphTestFlags)

bool FCompoundRoadCommandTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({351});
	const FCreateRoadSegmentResult First = City.CreateRoadSegment(
		FRoadEndpointInput::New({0.0, 0.0}), FRoadEndpointInput::New({100.0, 0.0}), MakeBasicRoad());
	TestTrue(TEXT("A compound command creates two new endpoints."), First.IsSuccess());
	TestEqual(TEXT("The first endpoint receives ID one."), First.EndpointA.GetValue(), uint64(1));
	TestEqual(TEXT("The second endpoint receives ID two."), First.EndpointB.GetValue(), uint64(2));

	const FCreateRoadSegmentResult Second = City.CreateRoadSegment(
		FRoadEndpointInput::Existing(First.EndpointB), FRoadEndpointInput::New({100.0, 100.0}), MakeBasicRoad());
	TestTrue(TEXT("A compound command can reuse one endpoint."), Second.IsSuccess());
	TestEqual(TEXT("The existing endpoint identity is retained."), Second.EndpointA, First.EndpointB);
	TestEqual(TEXT("Only one new node ID is allocated."), Second.EndpointB.GetValue(), uint64(3));

	const FCreateRoadSegmentResult Third = City.CreateRoadSegment(
		FRoadEndpointInput::Existing(Second.EndpointB), FRoadEndpointInput::Existing(First.EndpointA), MakeBasicRoad());
	TestTrue(TEXT("A compound command can connect two existing endpoints."), Third.IsSuccess());
	TestEqual(TEXT("Three connected roads use only three nodes."), City.GetSummary().RoadNodeCount, 3);
	TestEqual(TEXT("Three connected roads create three segments."), City.GetSummary().RoadSegmentCount, 3);
	TestTrue(TEXT("The connected graph validates."), City.Validate().IsValid());

	const FCitySummary BeforeFailures = City.GetSummary();
	FRoadSegmentDefinition InvalidType = MakeBasicRoad();
	InvalidType.RoadTypeId = FRoadTypeId(999);
	const FCreateRoadSegmentResult Invalid =
		City.CreateRoadSegment(FRoadEndpointInput::New({std::numeric_limits<double>::quiet_NaN(), 0.0}),
			FRoadEndpointInput::New({200.0, 0.0}),
			InvalidType);
	TestEqual(TEXT("Endpoint validation runs before mutation."),
		Invalid.Error.Code,
		ESimulationErrorCode::NonFiniteRoadPosition);
	TestTrue(TEXT("A rejected compound command leaves summary counts unchanged."), City.GetSummary() == BeforeFailures);

	const FCreateRoadSegmentResult Duplicate = City.CreateRoadSegment(
		FRoadEndpointInput::Existing(First.EndpointB), FRoadEndpointInput::Existing(First.EndpointA), MakeBasicRoad());
	TestEqual(TEXT("Duplicate connections return a typed error."),
		Duplicate.Error.Code,
		ESimulationErrorCode::DuplicateRoadConnection);
	TestTrue(TEXT("A duplicate compound command is atomic."), City.GetSummary() == BeforeFailures);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvalidRoadCommandAtomicityTest,
	"CityForm.Simulation.RoadGraph.InvalidCommandAtomicity",
	RoadGraphTestFlags)

bool FInvalidRoadCommandAtomicityTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({401});
	const FRoadNodeId NodeA = City.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId ColocatedNode = City.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId NodeC = City.AddRoadNode({10.0, 0.0}).NodeId;

	const FAddRoadSegmentResult MissingEndpoint = City.AddRoadSegment(NodeA, FRoadNodeId(999), MakeBasicRoad());
	TestTrue(TEXT("A missing endpoint has a stable code."),
		MissingEndpoint.Error.Code == ESimulationErrorCode::InvalidRoadNode);

	const FAddRoadSegmentResult SameEndpoint = City.AddRoadSegment(NodeA, NodeA, MakeBasicRoad());
	TestTrue(TEXT("An identical endpoint has a stable code."),
		SameEndpoint.Error.Code == ESimulationErrorCode::SameRoadEndpoint);

	const FAddRoadSegmentResult ZeroLength = City.AddRoadSegment(NodeA, ColocatedNode, MakeBasicRoad());
	TestTrue(TEXT("Colocated endpoints have a stable code."),
		ZeroLength.Error.Code == ESimulationErrorCode::ZeroLengthRoadSegment);

	FRoadSegmentDefinition InvalidRoadType = MakeBasicRoad();
	InvalidRoadType.RoadTypeId = FRoadTypeId(999);
	const FAddRoadSegmentResult UnknownType = City.AddRoadSegment(NodeA, NodeC, InvalidRoadType);
	TestTrue(TEXT("An unknown road type has a stable code."),
		UnknownType.Error.Code == ESimulationErrorCode::InvalidRoadType);

	const FAddRoadSegmentResult NonFiniteOverride =
		City.AddRoadSegment(NodeA, NodeC, MakeBasicRoad(TOptional<double>(std::numeric_limits<double>::quiet_NaN())));
	TestTrue(TEXT("A non-finite override has a stable code."),
		NonFiniteOverride.Error.Code == ESimulationErrorCode::InvalidSpeedLimit);

	const FAddRoadSegmentResult InvalidOverride =
		City.AddRoadSegment(NodeA, NodeC, MakeBasicRoad(TOptional<double>(0.0)));
	TestTrue(TEXT("A non-positive override has a stable code."),
		InvalidOverride.Error.Code == ESimulationErrorCode::InvalidSpeedLimit);

	const FAddRoadSegmentResult ValidSegment = City.AddRoadSegment(NodeA, NodeC, MakeBasicRoad());
	TestEqual(TEXT("Rejected commands do not consume segment IDs."), ValidSegment.SegmentId.GetValue(), uint64(1));

	const FAddRoadSegmentResult ReverseDuplicate = City.AddRoadSegment(NodeC, NodeA, MakeBasicRoad());
	TestTrue(TEXT("A reversed duplicate has a stable code."),
		ReverseDuplicate.Error.Code == ESimulationErrorCode::DuplicateRoadConnection);
	TestEqual(TEXT("Rejected commands do not mutate segment storage."), City.GetRoadGraph().GetSegments().Num(), 1);
	TestTrue(TEXT("The graph remains valid after rejected commands."), City.Validate().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRoadGraphRecordValidationTest,
	"CityForm.Simulation.RoadGraph.RecordValidation",
	RoadGraphTestFlags)

bool FRoadGraphRecordValidationTest::RunTest(const FString& Parameters)
{
	TArray<FRoadNode> Nodes;
	Nodes.Add({FRoadNodeId(1), {0.0, 0.0}});
	Nodes.Add({FRoadNodeId(2), {3.0, 4.0}});
	Nodes.Add({FRoadNodeId(2), {5.0, 5.0}});
	Nodes.Add({FRoadNodeId(), {std::numeric_limits<double>::infinity(), 0.0}});

	TArray<FRoadSegment> Segments;
	Segments.Add({FRoadSegmentId(1), FRoadNodeId(1), FRoadNodeId(2), MakeBasicRoad(), 5.0});
	Segments.Add({FRoadSegmentId(2), FRoadNodeId(2), FRoadNodeId(1), MakeBasicRoad(), 5.0});
	Segments.Add({FRoadSegmentId(3),
		FRoadNodeId(1),
		FRoadNodeId(999),
		{FRoadTypeId(999), TOptional<double>(std::numeric_limits<double>::quiet_NaN())},
		4.0});
	Segments.Add({FRoadSegmentId(4), FRoadNodeId(1), FRoadNodeId(1), MakeBasicRoad(), 0.0});
	Segments.Add(
		{FRoadSegmentId(4), FRoadNodeId(1), FRoadNodeId(2), MakeBasicRoad(), std::numeric_limits<double>::infinity()});
	Segments.Add({FRoadSegmentId(), FRoadNodeId(2), FRoadNodeId(2), MakeBasicRoad(), 1.0});

	const FValidationReport Report = FRoadGraph::ValidateRecords(Nodes, Segments, FRoadTypeCatalog());
	TestFalse(TEXT("Malformed road records fail validation."), Report.IsValid());
	TestEqual(TEXT("A duplicate node ID is reported."),
		CountRoadIssues(Report, EValidationIssueCode::DuplicateRoadNodeId),
		1);
	TestEqual(TEXT("A non-finite position is reported."),
		CountRoadIssues(Report, EValidationIssueCode::NonFiniteRoadPosition),
		1);
	TestTrue(TEXT("Duplicate connections are reported."),
		CountRoadIssues(Report, EValidationIssueCode::DuplicateRoadConnection) >= 1);
	TestEqual(TEXT("A duplicate segment ID is reported."),
		CountRoadIssues(Report, EValidationIssueCode::DuplicateRoadSegmentId),
		1);
	TestEqual(TEXT("An invalid segment ID is reported."),
		CountRoadIssues(Report, EValidationIssueCode::InvalidRoadSegmentId),
		1);
	TestEqual(TEXT("A dangling endpoint is reported."),
		CountRoadIssues(Report, EValidationIssueCode::MissingRoadSegmentEndpoint),
		1);
	TestEqual(TEXT("An invalid road-type reference is reported."),
		CountRoadIssues(Report, EValidationIssueCode::MissingRoadType),
		1);
	TestEqual(TEXT("A non-finite override is reported."),
		CountRoadIssues(Report, EValidationIssueCode::NonFiniteSpeedLimitOverride),
		1);
	TestTrue(TEXT("Identical endpoints are reported."),
		CountRoadIssues(Report, EValidationIssueCode::IdenticalRoadSegmentEndpoints) >= 1);
	TestEqual(TEXT("A non-positive cached length is reported."),
		CountRoadIssues(Report, EValidationIssueCode::NonPositiveRoadSegmentLength),
		1);
	TestEqual(TEXT("A non-finite cached length is reported."),
		CountRoadIssues(Report, EValidationIssueCode::NonFiniteRoadSegmentLength),
		1);
	TestTrue(TEXT("Invalid cached geometry is reported."),
		CountRoadIssues(Report, EValidationIssueCode::RoadSegmentLengthMismatch) >= 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRoadGraphRepeatabilityTest,
	"CityForm.Simulation.RoadGraph.Repeatability",
	RoadGraphTestFlags)

bool FRoadGraphRepeatabilityTest::RunTest(const FString& Parameters)
{
	FCitySimulation FirstCity({501});
	FCitySimulation SecondCity({501});

	const FRoadNodeId FirstA = FirstCity.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId FirstB = FirstCity.AddRoadNode({100.0, 0.0}).NodeId;
	const FRoadNodeId SecondA = SecondCity.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId SecondB = SecondCity.AddRoadNode({100.0, 0.0}).NodeId;
	const FRoadSegmentId FirstSegment = FirstCity.AddRoadSegment(FirstA, FirstB, MakeBasicRoad()).SegmentId;
	const FRoadSegmentId SecondSegment = SecondCity.AddRoadSegment(SecondA, SecondB, MakeBasicRoad()).SegmentId;

	TestTrue(TEXT("Equivalent node commands produce equal IDs."), FirstA == SecondA && FirstB == SecondB);
	TestTrue(TEXT("Equivalent segment commands produce equal IDs."), FirstSegment == SecondSegment);
	TestTrue(TEXT("Equivalent graphs produce equal summaries."), FirstCity.GetSummary() == SecondCity.GetSummary());
	TestTrue(TEXT("Equivalent graphs produce equal traversals."),
		FirstCity.GetRoadGraph().GetOutgoingTraversals(FirstA) ==
			SecondCity.GetRoadGraph().GetOutgoingTraversals(SecondA));
	TestTrue(TEXT("The first graph validates headlessly."), FirstCity.Validate().IsValid());
	TestTrue(TEXT("The second graph validates headlessly."), SecondCity.Validate().IsValid());

	static_assert(!std::is_convertible_v<FRoadTypeId, FRoadSegmentId>);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
