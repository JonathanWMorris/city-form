// Copyright Jonathan Morris. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "../CityFormCoordinateConversion.h"
#include "../CityFormSimulationSubsystem.h"
#include "CitySimulation/RoadType.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Subsystems/SubsystemCollection.h"
#include "UObject/StrongObjectPtr.h"

#include <limits>

using namespace CityForm::Simulation;

namespace
{
constexpr EAutomationTestFlags SimulationBridgeTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

class FScopedTestGameInstance
{
public:
	FScopedTestGameInstance()
	{
		GameInstance.Reset(NewObject<UGameInstance>());
		Subsystems.Initialize(GameInstance.Get());
	}

	~FScopedTestGameInstance()
	{
		Subsystems.Deinitialize();
	}

	UCityFormSimulationSubsystem* GetSubsystem() const
	{
		return Subsystems.GetSubsystem<UCityFormSimulationSubsystem>(
			UCityFormSimulationSubsystem::StaticClass());
	}

private:
	TStrongObjectPtr<UGameInstance> GameInstance;
	FSubsystemCollection<UGameInstanceSubsystem> Subsystems;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCityFormCoordinateConversionTest,
	"CityForm.Presentation.SimulationBridge.CoordinateConversion",
	SimulationBridgeTestFlags)

bool FCityFormCoordinateConversionTest::RunTest(const FString& Parameters)
{
	const FVector UnrealPosition(12345.0, -6789.0, 500.0);
	const FSimPoint2D SimulationPosition =
		FCityFormCoordinateConversion::ToSimulationMeters(UnrealPosition);
	TestEqual(TEXT("Unreal X centimeters convert to simulation meters."), SimulationPosition.X, 123.45);
	TestEqual(TEXT("Unreal Y centimeters convert to simulation meters."), SimulationPosition.Y, -67.89);

	const FVector RoundTrip = FCityFormCoordinateConversion::ToUnrealCentimeters(SimulationPosition);
	TestEqual(TEXT("Simulation X meters round-trip to Unreal centimeters."), RoundTrip.X, UnrealPosition.X);
	TestEqual(TEXT("Simulation Y meters round-trip to Unreal centimeters."), RoundTrip.Y, UnrealPosition.Y);
	TestEqual(TEXT("Simulation positions are presented on the ground plane."), RoundTrip.Z, 0.0);
	TestEqual(
		TEXT("One simulation meter equals one hundred Unreal centimeters."),
		FCityFormCoordinateConversion::ToUnrealCentimeters(1.0),
		100.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCityFormSimulationBridgeCommandsTest,
	"CityForm.Presentation.SimulationBridge.CommandsAndSnapshot",
	SimulationBridgeTestFlags)

bool FCityFormSimulationBridgeCommandsTest::RunTest(const FString& Parameters)
{
	FScopedTestGameInstance TestGameInstance;
	UCityFormSimulationSubsystem* Subsystem = TestGameInstance.GetSubsystem();
	TestNotNull(TEXT("The game instance owns a simulation subsystem."), Subsystem);
	if (Subsystem == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("The prototype city uses the documented seed."), Subsystem->GetCitySummary().Seed, uint64(0));
	int32 GraphChangeCount = 0;
	const FDelegateHandle ChangeHandle = Subsystem->OnRoadGraphChanged().AddLambda(
		[&GraphChangeCount]() { ++GraphChangeCount; });
	FRoadSegmentDefinition Definition;
	Definition.RoadTypeId = FRoadTypeCatalog::GetBasicTwoWayRoadTypeId();
	const FCreateRoadSegmentResult Segment = Subsystem->CreateRoadSegment(
		FCityFormRoadEndpointInput::New(FVector(100.0, 200.0, 30.0)),
		FCityFormRoadEndpointInput::New(FVector(400.0, 600.0, 90.0)),
		Definition);
	TestTrue(TEXT("The segment command succeeds."), Segment.IsSuccess());
	TestEqual(TEXT("A successful graph command publishes one presentation notification."), GraphChangeCount, 1);

	FCityFormRoadGraphSnapshot Snapshot = Subsystem->CreateRoadGraphSnapshot();
	TestEqual(TEXT("The snapshot contains both nodes."), Snapshot.Nodes.Num(), 2);
	TestEqual(TEXT("The snapshot contains the segment."), Snapshot.Segments.Num(), 1);
	TestEqual(TEXT("The first node ID is preserved."), Snapshot.Nodes[0].Id, Segment.EndpointA);
	TestEqual(TEXT("The first node is returned in Unreal centimeters."), Snapshot.Nodes[0].PositionCentimeters, FVector(100.0, 200.0, 0.0));
	TestEqual(TEXT("The segment ID is preserved."), Snapshot.Segments[0].Id, Segment.SegmentId);
	TestEqual(TEXT("The segment length is returned in Unreal centimeters."), Snapshot.Segments[0].LengthCentimeters, 500.0);

	Snapshot.Nodes.Reset();
	Snapshot.Segments.Reset();
	const FCityFormRoadGraphSnapshot FreshSnapshot = Subsystem->CreateRoadGraphSnapshot();
	TestEqual(TEXT("Changing a snapshot does not remove authoritative nodes."), FreshSnapshot.Nodes.Num(), 2);
	TestEqual(TEXT("Changing a snapshot does not remove authoritative segments."), FreshSnapshot.Segments.Num(), 1);
	TestTrue(TEXT("The resulting city passes authoritative validation."), Subsystem->ValidateCity().IsValid());
	Subsystem->OnRoadGraphChanged().Remove(ChangeHandle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCityFormSimulationBridgeErrorTest,
	"CityForm.Presentation.SimulationBridge.TypedErrors",
	SimulationBridgeTestFlags)

bool FCityFormSimulationBridgeErrorTest::RunTest(const FString& Parameters)
{
	FScopedTestGameInstance TestGameInstance;
	UCityFormSimulationSubsystem* Subsystem = TestGameInstance.GetSubsystem();
	if (!TestNotNull(TEXT("The game instance owns a simulation subsystem."), Subsystem))
	{
		return false;
	}

	FRoadSegmentDefinition Definition;
	int32 GraphChangeCount = 0;
	const FDelegateHandle ChangeHandle = Subsystem->OnRoadGraphChanged().AddLambda(
		[&GraphChangeCount]() { ++GraphChangeCount; });
	Definition.RoadTypeId = FRoadTypeCatalog::GetBasicTwoWayRoadTypeId();
	const FCreateRoadSegmentResult InvalidNode = Subsystem->CreateRoadSegment(
		FCityFormRoadEndpointInput::New(
			FVector(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0)),
		FCityFormRoadEndpointInput::New(FVector(100.0, 0.0, 0.0)),
		Definition);
	TestFalse(TEXT("A non-finite position is rejected."), InvalidNode.IsSuccess());
	TestEqual(
		TEXT("The simulation error code crosses the presentation boundary."),
		InvalidNode.Error.Code,
		ESimulationErrorCode::NonFiniteRoadPosition);

	const FCreateRoadSegmentResult InvalidSegment = Subsystem->CreateRoadSegment(
		FCityFormRoadEndpointInput::Existing(FRoadNodeId(1)),
		FCityFormRoadEndpointInput::Existing(FRoadNodeId(2)),
		Definition);
	TestFalse(TEXT("Unknown segment endpoints are rejected."), InvalidSegment.IsSuccess());
	TestEqual(
		TEXT("Invalid endpoint errors remain typed."),
		InvalidSegment.Error.Code,
		ESimulationErrorCode::InvalidRoadNode);

	const FCitySummary Summary = Subsystem->GetCitySummary();
	TestEqual(TEXT("Rejected commands do not create nodes."), Summary.RoadNodeCount, 0);
	TestEqual(TEXT("Rejected commands do not create segments."), Summary.RoadSegmentCount, 0);
	TestEqual(TEXT("Rejected commands publish no graph-change notification."), GraphChangeCount, 0);
	Subsystem->OnRoadGraphChanged().Remove(ChangeHandle);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
