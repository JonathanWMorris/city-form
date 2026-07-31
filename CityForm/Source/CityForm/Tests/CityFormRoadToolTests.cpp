// Copyright Jonathan Morris. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "../CityFormPrototypeMapContract.h"
#include "../CityFormRoadPlacementComponent.h"
#include "Misc/AutomationTest.h"

using namespace CityForm::Simulation;

namespace
{
constexpr EAutomationTestFlags RoadToolTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCityFormPrototypeMapContractTest,
	"CityForm.Presentation.RoadTool.PrototypeMapContract",
	RoadToolTestFlags)

bool FCityFormPrototypeMapContractTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("The origin is buildable ground."),
		FCityFormPrototypeMapContract::IsPrototypeGroundHit(FVector::ZeroVector));
	TestTrue(TEXT("The documented positive boundary is included."),
		FCityFormPrototypeMapContract::ContainsBuildablePoint(FVector(90000.0, 90000.0, 0.0)));
	TestFalse(TEXT("Points beyond the buildable area are rejected."),
		FCityFormPrototypeMapContract::ContainsBuildablePoint(FVector(90000.1, 0.0, 0.0)));
	TestFalse(TEXT("Hits above the ground tolerance are rejected."),
		FCityFormPrototypeMapContract::IsPrototypeGroundHit(FVector(0.0, 0.0, 50.1)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCityFormRoadPlacementStateTest,
	"CityForm.Presentation.RoadTool.StateAndLength",
	RoadToolTestFlags)

bool FCityFormRoadPlacementStateTest::RunTest(const FString& Parameters)
{
	FCityFormRoadPlacementState State;
	TestEqual(TEXT("Placement begins idle."), State.State, ERoadPlacementState::Idle);
	State.SelectStart(FCityFormRoadEndpointInput::New(FVector(100.0, 200.0, 0.0)), FVector(100.0, 200.0, 0.0));
	TestEqual(TEXT("Selecting a start enters preview state."), State.State, ERoadPlacementState::StartSelected);
	State.Clear();
	TestEqual(TEXT("Cancel returns placement to idle."), State.State, ERoadPlacementState::Idle);
	TestFalse(TEXT("Sub-meter roads are rejected by the tool."),
		UCityFormRoadPlacementComponent::IsLongEnough(FVector::ZeroVector, FVector(99.0, 0.0, 0.0)));
	TestTrue(TEXT("One-meter roads are accepted by the tool."),
		UCityFormRoadPlacementComponent::IsLongEnough(FVector::ZeroVector, FVector(100.0, 0.0, 0.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCityFormRoadEndpointSnapTest,
	"CityForm.Presentation.RoadTool.EndpointSnapping",
	RoadToolTestFlags)

bool FCityFormRoadEndpointSnapTest::RunTest(const FString& Parameters)
{
	TArray<FCityFormRoadNodeSnapshot> Nodes;
	Nodes.Add({FRoadNodeId(2), FVector(10.0, 10.0, 0.0)});
	Nodes.Add({FRoadNodeId(1), FVector(10.0, 10.0, 0.0)});
	Nodes.Add({FRoadNodeId(3), FVector(100.0, 100.0, 0.0)});
	const auto Project = [](const FVector& World, FVector2D& Screen)
	{
		Screen = FVector2D(World.X, World.Y);
		return true;
	};

	const TOptional<FCityFormRoadNodeSnapshot> Snap =
		UCityFormRoadPlacementComponent::FindSnapCandidate(Nodes, FVector2D(11.0, 10.0), Project);
	TestTrue(TEXT("A nearby projected endpoint is selected."), Snap.IsSet());
	if (Snap.IsSet())
	{
		TestEqual(TEXT("Equal-distance snapping resolves to the lowest stable ID."), Snap->Id.GetValue(), uint64(1));
	}
	const TOptional<FCityFormRoadNodeSnapshot> Miss =
		UCityFormRoadPlacementComponent::FindSnapCandidate(Nodes, FVector2D(50.0, 50.0), Project);
	TestFalse(TEXT("Endpoints outside the screen-space radius are ignored."), Miss.IsSet());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
