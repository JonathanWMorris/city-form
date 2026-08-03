// Copyright Jonathan Morris. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "CitySimulation/CitySimulation.h"
#include "CitySimulation/StrongId.h"
#include "Misc/AutomationTest.h"

#include <limits>
#include <type_traits>

using namespace CityForm::Simulation;

namespace
{

constexpr EAutomationTestFlags FoundationTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

int32 CountIssues(const FValidationReport& Report, const EValidationIssueCode Code)
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

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCitySimulationLifecycleTest,
	"CityForm.Simulation.Foundation.CityLifecycle",
	FoundationTestFlags)

bool FCitySimulationLifecycleTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({1234});

	TestEqual(TEXT("The configured seed is retained."), City.GetConfig().Seed, uint64(1234));
	TestEqual(TEXT("A city starts at zero milliseconds."),
		City.GetClock().GetCurrentInstant().GetMillisecondsSinceStart(),
		int64(0));
	TestTrue(TEXT("A new city validates."), City.Validate().IsValid());

	const FCitySummary InitialSummary = City.GetSummary();
	TestEqual(TEXT("The summary contains the seed."), InitialSummary.Seed, uint64(1234));
	TestEqual(TEXT("The summary starts at zero milliseconds."), InitialSummary.CurrentTimeMilliseconds, int64(0));
	TestEqual(TEXT("The default vehicle catalog contains one class."), InitialSummary.VehicleClassCount, 1);

	TestTrue(TEXT("Positive advancement succeeds."), City.Advance(FSimulationDuration(60000)).IsSuccess());
	TestEqual(TEXT("The city advances by integer milliseconds."),
		City.GetClock().GetCurrentInstant().GetMillisecondsSinceStart(),
		int64(60000));

	const FCitySummary AdvancedSummary = City.GetSummary();
	TestEqual(
		TEXT("The advanced summary reports the current time."), AdvancedSummary.CurrentTimeMilliseconds, int64(60000));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSimulationClockBoundaryTest,
	"CityForm.Simulation.Foundation.SimulationTimeBoundaries",
	FoundationTestFlags)

bool FSimulationClockBoundaryTest::RunTest(const FString& Parameters)
{
	static_assert(!std::is_convertible_v<int64, FSimulationInstant>);
	static_assert(!std::is_convertible_v<int64, FSimulationDuration>);
	static_assert(!std::is_convertible_v<FSimulationInstant, FSimulationDuration>);

	FSimulationClock Clock;
	TestTrue(TEXT("Zero advancement succeeds."), Clock.Advance(FSimulationDuration()).IsSuccess());
	TestEqual(TEXT("Zero advancement does not mutate time."),
		Clock.GetCurrentInstant().GetMillisecondsSinceStart(),
		int64(0));

	const FAdvanceTimeResult NegativeResult = Clock.Advance(FSimulationDuration(-1));
	TestFalse(TEXT("Negative advancement fails."), NegativeResult.IsSuccess());
	TestTrue(TEXT("Negative advancement has a stable code."),
		NegativeResult.Error.Code == ESimulationErrorCode::NegativeDuration);
	TestEqual(TEXT("Negative advancement is atomic."), Clock.GetCurrentInstant().GetMillisecondsSinceStart(), int64(0));

	TestTrue(TEXT("Advancement to the maximum instant succeeds."),
		Clock.Advance(FSimulationDuration(MAX_int64)).IsSuccess());
	const FAdvanceTimeResult OverflowResult = Clock.Advance(FSimulationDuration(1));
	TestFalse(TEXT("Overflowing advancement fails."), OverflowResult.IsSuccess());
	TestTrue(TEXT("Overflow has a stable code."), OverflowResult.Error.Code == ESimulationErrorCode::TimeOverflow);
	TestEqual(TEXT("Overflow does not mutate time."),
		Clock.GetCurrentInstant().GetMillisecondsSinceStart(),
		int64(MAX_int64));

	const FSimulationInstantResult Added = AddSimulationDuration(FSimulationInstant(1000), FSimulationDuration(250));
	TestTrue(TEXT("Checked instant addition succeeds."), Added.IsSuccess());
	TestEqual(
		TEXT("Checked instant addition uses milliseconds."), Added.Instant.GetMillisecondsSinceStart(), int64(1250));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDeterministicRandomTest,
	"CityForm.Simulation.Foundation.DeterministicRandom",
	FoundationTestFlags)

bool FDeterministicRandomTest::RunTest(const FString& Parameters)
{
	FDeterministicRandom GoldenRandom(42);
	TestEqual(TEXT("The first raw value matches the locked mt19937_64 sequence."),
		GoldenRandom.NextUInt64(),
		uint64(13930160852258120406ULL));
	TestEqual(TEXT("The second raw value matches the locked mt19937_64 sequence."),
		GoldenRandom.NextUInt64(),
		uint64(11788048577503494824ULL));
	TestEqual(TEXT("The third raw value matches the locked mt19937_64 sequence."),
		GoldenRandom.NextUInt64(),
		uint64(13874630024467741450ULL));

	FDeterministicRandom BoundedRandom(42);
	const FBoundedRandomResult BoundedResult = BoundedRandom.NextBoundedUInt64(10);
	TestTrue(TEXT("A positive bound succeeds."), BoundedResult.IsSuccess());
	TestEqual(TEXT("Bounded output uses the documented rejection mapping."), BoundedResult.Value, uint64(6));

	FDeterministicRandom InvalidBoundRandom(42);
	const FBoundedRandomResult InvalidBoundResult = InvalidBoundRandom.NextBoundedUInt64(0);
	TestFalse(TEXT("A zero bound fails."), InvalidBoundResult.IsSuccess());
	TestTrue(TEXT("A zero bound has a stable code."),
		InvalidBoundResult.Error.Code == ESimulationErrorCode::InvalidRandomBound);
	TestEqual(TEXT("An invalid bound does not consume random state."),
		InvalidBoundRandom.NextUInt64(),
		uint64(13930160852258120406ULL));

	FDeterministicRandom UnitRandomA(9);
	FDeterministicRandom UnitRandomB(9);
	const double UnitValue = UnitRandomA.NextUnitDouble();
	TestTrue(TEXT("Unit output is at least zero."), UnitValue >= 0.0);
	TestTrue(TEXT("Unit output is below one."), UnitValue < 1.0);
	TestTrue(TEXT("The explicitly mapped unit output is repeatable."), UnitValue == UnitRandomB.NextUnitDouble());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStrongIdTest,
	"CityForm.Simulation.Foundation.StrongIds",
	FoundationTestFlags)

bool FStrongIdTest::RunTest(const FString& Parameters)
{
	static_assert(!std::is_convertible_v<FRoadNodeId, FRoadSegmentId>);
	static_assert(!std::is_constructible_v<FRoadSegmentId, FRoadNodeId>);
	static_assert(!std::is_convertible_v<FBuildingId, FParcelId>);
	static_assert(!std::is_constructible_v<FBuildingTypeId, FBuildingId>);

	TestFalse(TEXT("A default ID is invalid."), FRoadNodeId().IsValid());

	TStrongIdAllocator<FRoadNodeId> Allocator;
	const TStrongIdAllocationResult<FRoadNodeId> First = Allocator.Allocate();
	const TStrongIdAllocationResult<FRoadNodeId> Second = Allocator.Allocate();
	TestTrue(TEXT("The first allocation succeeds."), First.IsSuccess());
	TestEqual(TEXT("Allocation begins at one."), First.Id.GetValue(), uint64(1));
	TestEqual(TEXT("Allocation is monotonic."), Second.Id.GetValue(), uint64(2));
	TestTrue(TEXT("Distinct IDs compare in allocation order."), First.Id < Second.Id);
	TestTrue(TEXT("Equal IDs hash identically."), GetTypeHash(First.Id) == GetTypeHash(FRoadNodeId(1)));

	TStrongIdAllocator<FRoadNodeId> BoundaryAllocator(MAX_uint64);
	const TStrongIdAllocationResult<FRoadNodeId> Last = BoundaryAllocator.Allocate();
	TestTrue(TEXT("The final representable ID can be allocated."), Last.IsSuccess());
	TestEqual(TEXT("The final ID retains its value."), Last.Id.GetValue(), uint64(MAX_uint64));

	const TStrongIdAllocationResult<FRoadNodeId> Exhausted = BoundaryAllocator.Allocate();
	TestFalse(TEXT("Allocation after the maximum fails."), Exhausted.IsSuccess());
	TestTrue(TEXT("Exhaustion has a stable code."), Exhausted.Error.Code == ESimulationErrorCode::IdExhausted);
	TestEqual(
		TEXT("An exhausted allocator remains exhausted."), BoundaryAllocator.GetNextValueForDiagnostics(), uint64(0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVehicleClassCatalogTest,
	"CityForm.Simulation.Foundation.VehicleClassCatalog",
	FoundationTestFlags)

bool FVehicleClassCatalogTest::RunTest(const FString& Parameters)
{
	FVehicleClassCatalog Catalog;
	TestTrue(TEXT("The provisional catalog validates."), Catalog.Validate().IsValid());

	const FVehicleClassDefinition* PassengerCar = Catalog.Find(FVehicleClassId(1));
	TestNotNull(TEXT("The passenger-car class uses stable ID one."), PassengerCar);
	if (PassengerCar != nullptr)
	{
		TestEqual(TEXT("Passenger-car length is provisional baseline."), PassengerCar->LengthMeters, 4.5);
		TestEqual(TEXT("Passenger-car queue length includes spacing."), PassengerCar->EffectiveQueueLengthMeters, 7.5);
		TestEqual(TEXT("Passenger-car mass is in kilograms."), PassengerCar->MassKilograms, 1500.0);
		TestEqual(TEXT("Passenger-car maximum speed is in m/s."), PassengerCar->MaximumSpeedMetersPerSecond, 33.333);
		TestEqual(TEXT("Passenger-car maximum acceleration is in m/s^2."),
			PassengerCar->MaximumAccelerationMetersPerSecondSquared,
			2.5);
		TestEqual(TEXT("Passenger-car comfortable deceleration is in m/s^2."),
			PassengerCar->ComfortableDecelerationMetersPerSecondSquared,
			3.0);
		TestEqual(TEXT("Passenger-car emergency deceleration is in m/s^2."),
			PassengerCar->EmergencyDecelerationMetersPerSecondSquared,
			8.0);
		TestEqual(TEXT("Passenger-car turning radius is in meters."), PassengerCar->MinimumTurningRadiusMeters, 5.5);
		TestEqual(TEXT("Passenger-car PCE is one."), PassengerCar->PassengerCarEquivalent, 1.0);
		TestEqual(TEXT("Passenger-car restrictions are empty."), PassengerCar->RestrictionMask, uint32(0));
	}

	FVehicleClassDefinition InvalidDefinition = FVehicleClassCatalog::MakeProvisionalPassengerCar();
	InvalidDefinition.Id = FVehicleClassId();
	InvalidDefinition.LengthMeters = std::numeric_limits<double>::quiet_NaN();
	InvalidDefinition.EffectiveQueueLengthMeters = 1.0;
	InvalidDefinition.ComfortableDecelerationMetersPerSecondSquared = 9.0;

	TArray<FVehicleClassDefinition> InvalidDefinitions;
	InvalidDefinitions.Add(InvalidDefinition);
	const FValidationReport InvalidReport = FVehicleClassCatalog(MoveTemp(InvalidDefinitions)).Validate();
	TestFalse(TEXT("An invalid definition fails validation."), InvalidReport.IsValid());
	TestEqual(
		TEXT("An invalid ID is reported."), CountIssues(InvalidReport, EValidationIssueCode::InvalidVehicleClassId), 1);
	TestEqual(TEXT("A non-finite value is reported."),
		CountIssues(InvalidReport, EValidationIssueCode::NonFiniteVehicleValue),
		1);
	TestEqual(TEXT("Invalid deceleration ordering is reported."),
		CountIssues(InvalidReport, EValidationIssueCode::ComfortableDecelerationExceedsEmergency),
		1);

	TArray<FVehicleClassDefinition> DuplicateDefinitions;
	DuplicateDefinitions.Add(FVehicleClassCatalog::MakeProvisionalPassengerCar());
	DuplicateDefinitions.Add(FVehicleClassCatalog::MakeProvisionalPassengerCar());
	const FVehicleClassCatalog DuplicateCatalog(MoveTemp(DuplicateDefinitions));
	const FValidationReport DuplicateReport = DuplicateCatalog.Validate();
	TestFalse(TEXT("Duplicate IDs fail validation."), DuplicateReport.IsValid());
	TestEqual(TEXT("A duplicate ID is reported once."),
		CountIssues(DuplicateReport, EValidationIssueCode::DuplicateVehicleClassId),
		1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHeadlessRepeatabilityTest,
	"CityForm.Simulation.Foundation.HeadlessRepeatability",
	FoundationTestFlags)

bool FHeadlessRepeatabilityTest::RunTest(const FString& Parameters)
{
	FCitySimulation FirstCity({8675309});
	FCitySimulation SecondCity({8675309});

	for (int32 Step = 0; Step < 10; ++Step)
	{
		TestTrue(TEXT("The first headless city advances."), FirstCity.Advance(FSimulationDuration(144000)).IsSuccess());
		TestTrue(
			TEXT("The second headless city advances."), SecondCity.Advance(FSimulationDuration(144000)).IsSuccess());
		TestEqual(TEXT("Seeded random output remains repeatable."),
			FirstCity.GetRandom().NextUInt64(),
			SecondCity.GetRandom().NextUInt64());
	}

	TestTrue(TEXT("The first headless city validates."), FirstCity.Validate().IsValid());
	TestTrue(TEXT("The second headless city validates."), SecondCity.Validate().IsValid());
	TestTrue(TEXT("Equivalent command sequences produce equal summaries."),
		FirstCity.GetSummary() == SecondCity.GetSummary());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
