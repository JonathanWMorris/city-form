// Copyright Jonathan Morris. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "CitySimulation/Building.h"
#include "CitySimulation/CitySimulation.h"
#include "CitySimulation/RoadType.h"
#include "Misc/AutomationTest.h"

using namespace CityForm::Simulation;

namespace
{

constexpr EAutomationTestFlags BuildingTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

FParcelId AddParcelFixture(FCitySimulation& City)
{
	const FRoadNodeId NodeA = City.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId NodeB = City.AddRoadNode({32.0, 0.0}).NodeId;
	City.AddRoadSegment(NodeA, NodeB, {FRoadTypeCatalog::GetBasicTwoWayRoadTypeId(), TOptional<double>()});
	return City.GetParcelLayout().GetParcels()[0].Id;
}

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuildingTypeCatalogTest, "CityForm.Simulation.Building.TypeCatalog", BuildingTestFlags)

bool FBuildingTypeCatalogTest::RunTest(const FString& Parameters)
{
	const FDevelopmentConfig Config;
	const FBuildingTypeCatalog Catalog(Config);
	TestEqual(TEXT("The starter catalog contains two building types."), Catalog.GetDefinitions().Num(), 2);
	const FBuildingTypeDefinition* House = Catalog.Find(FBuildingTypeCatalog::GetDetachedHouseTypeId());
	const FBuildingTypeDefinition* Commercial = Catalog.Find(FBuildingTypeCatalog::GetSmallCommercialTypeId());
	TestNotNull(TEXT("DetachedHouse exists."), House);
	TestNotNull(TEXT("SmallCommercial exists."), Commercial);
	if (House != nullptr && Commercial != nullptr)
	{
		TestTrue(TEXT("DetachedHouse is residential."), House->Zone == EZoneCategory::Residential);
		TestEqual(TEXT("DetachedHouse supports one household."), House->HouseholdCapacity, 1);
		TestEqual(TEXT("DetachedHouse exposes no jobs."), House->JobCapacity, 0);
		TestTrue(TEXT("SmallCommercial is commercial."), Commercial->Zone == EZoneCategory::Commercial);
		TestEqual(TEXT("SmallCommercial exposes no households."), Commercial->HouseholdCapacity, 0);
		TestEqual(TEXT("SmallCommercial supports eight jobs."), Commercial->JobCapacity, 8);
	}
	TestTrue(TEXT("The default development config validates."), Config.Validate().IsValid());
	TestTrue(TEXT("The starter building catalog validates."), Catalog.Validate().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuildingCreatedFromZoningTest, "CityForm.Simulation.Building.CreatedFromZoning", BuildingTestFlags)

bool FBuildingCreatedFromZoningTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({701});
	const FParcelId ParcelId = AddParcelFixture(City);
	TestTrue(TEXT("Residential zoning succeeds."), City.ApplyZone(ParcelId, EZoneCategory::Residential).IsSuccess());
	const FBuilding* Building = City.GetBuildings().FindBuildingForParcel(ParcelId);
	TestNotNull(TEXT("Zoning creates one building for the parcel."), Building);
	if (Building != nullptr)
	{
		TestTrue(TEXT("The new building has a stable ID."), Building->Id.IsValid());
		TestTrue(TEXT("The new building is a DetachedHouse."),
			Building->BuildingTypeId == FBuildingTypeCatalog::GetDetachedHouseTypeId());
		TestTrue(TEXT("The new building starts Planned."), Building->Stage == EDevelopmentStage::Planned);
		TestEqual(TEXT("The building is created at the current instant."),
			Building->CreatedAt.GetMillisecondsSinceStart(),
			0ll);
		TestEqual(
			TEXT("The timeline starts at the current instant."), Building->PlannedAt.GetMillisecondsSinceStart(), 0ll);
		TestEqual(TEXT("Construction starts after two simulated minutes."),
			Building->ConstructionStartsAt.GetMillisecondsSinceStart(),
			120000ll);
		TestEqual(TEXT("Completion occurs after five simulated minutes."),
			Building->CompletesAt.GetMillisecondsSinceStart(),
			300000ll);
		TestEqual(TEXT("Planned buildings expose no household capacity."), Building->HouseholdCapacity, 0);
	}
	TestTrue(TEXT("The city remains valid."), City.Validate().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuildingDevelopmentProgressTest, "CityForm.Simulation.Building.DevelopmentProgress", BuildingTestFlags)

bool FBuildingDevelopmentProgressTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({702});
	const FParcelId ParcelId = AddParcelFixture(City);
	City.ApplyZone(ParcelId, EZoneCategory::Residential);
	City.Advance(FSimulationDuration(119999));
	TestTrue(TEXT("The building remains Planned before the boundary."),
		City.GetBuildings().FindBuildingForParcel(ParcelId)->Stage == EDevelopmentStage::Planned);
	City.Advance(FSimulationDuration(1));
	TestTrue(TEXT("The building enters construction at two minutes."),
		City.GetBuildings().FindBuildingForParcel(ParcelId)->Stage == EDevelopmentStage::UnderConstruction);
	City.Advance(FSimulationDuration(180000));
	const FBuilding* Building = City.GetBuildings().FindBuildingForParcel(ParcelId);
	TestTrue(TEXT("The building completes at five minutes."), Building->Stage == EDevelopmentStage::Complete);
	TestEqual(TEXT("A complete DetachedHouse activates one household slot."), Building->HouseholdCapacity, 1);
	TestEqual(TEXT("A DetachedHouse activates no jobs."), Building->JobCapacity, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuildingLargeAdvanceTest, "CityForm.Simulation.Building.LargeAdvanceCrossesStages", BuildingTestFlags)

bool FBuildingLargeAdvanceTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({703});
	const FParcelId ParcelId = AddParcelFixture(City);
	City.ApplyZone(ParcelId, EZoneCategory::Commercial);
	City.Advance(FSimulationDuration(600000));
	const FBuilding* Building = City.GetBuildings().FindBuildingForParcel(ParcelId);
	TestTrue(
		TEXT("A large time jump crosses both development boundaries."), Building->Stage == EDevelopmentStage::Complete);
	TestEqual(TEXT("Complete SmallCommercial activates eight jobs."), Building->JobCapacity, 8);
	TestEqual(TEXT("SmallCommercial activates no household capacity."), Building->HouseholdCapacity, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuildingSameZoneIdempotentTest, "CityForm.Simulation.Building.SameZoneIsIdempotent", BuildingTestFlags)

bool FBuildingSameZoneIdempotentTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({704});
	const FParcelId ParcelId = AddParcelFixture(City);
	City.ApplyZone(ParcelId, EZoneCategory::Residential);
	City.Advance(FSimulationDuration(150000));
	const FBuilding Before = *City.GetBuildings().FindBuildingForParcel(ParcelId);
	TestTrue(
		TEXT("Applying the existing zone succeeds."), City.ApplyZone(ParcelId, EZoneCategory::Residential).IsSuccess());
	const FBuilding* After = City.GetBuildings().FindBuildingForParcel(ParcelId);
	TestTrue(TEXT("Same-zone application preserves building identity."), After->Id == Before.Id);
	TestTrue(TEXT("Same-zone application preserves progress."), After->Stage == Before.Stage);
	TestEqual(TEXT("Same-zone application preserves completion time."),
		After->CompletesAt.GetMillisecondsSinceStart(),
		Before.CompletesAt.GetMillisecondsSinceStart());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuildingRezoneRestartsTest, "CityForm.Simulation.Building.RezoneRestartsDevelopment", BuildingTestFlags)

bool FBuildingRezoneRestartsTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({705});
	const FParcelId ParcelId = AddParcelFixture(City);
	City.ApplyZone(ParcelId, EZoneCategory::Residential);
	City.Advance(FSimulationDuration(200000));
	const FBuildingId OriginalId = City.GetBuildings().FindBuildingForParcel(ParcelId)->Id;
	TestTrue(TEXT("Rezoning succeeds."), City.ApplyZone(ParcelId, EZoneCategory::Commercial).IsSuccess());
	const FBuilding* Replacement = City.GetBuildings().FindBuildingForParcel(ParcelId);
	TestTrue(TEXT("Rezoning creates a new building identity."), Replacement->Id != OriginalId);
	TestTrue(TEXT("Rezoning selects SmallCommercial."),
		Replacement->BuildingTypeId == FBuildingTypeCatalog::GetSmallCommercialTypeId());
	TestTrue(TEXT("Rezoning restarts at Planned."), Replacement->Stage == EDevelopmentStage::Planned);
	TestEqual(TEXT("The replacement timeline starts at the rezone instant."),
		Replacement->PlannedAt.GetMillisecondsSinceStart(),
		200000ll);
	TestEqual(TEXT("The replacement completes five minutes after rezoning."),
		Replacement->CompletesAt.GetMillisecondsSinceStart(),
		500000ll);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuildingClearZoneRemovesTest, "CityForm.Simulation.Building.ClearZoneRemovesPlaceholder", BuildingTestFlags)

bool FBuildingClearZoneRemovesTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({706});
	const FParcelId ParcelId = AddParcelFixture(City);
	City.ApplyZone(ParcelId, EZoneCategory::Residential);
	TestTrue(TEXT("ClearZone succeeds."), City.ClearZone(ParcelId).IsSuccess());
	TestNull(TEXT("ClearZone removes the placeholder building."), City.GetBuildings().FindBuildingForParcel(ParcelId));
	TestTrue(TEXT("The parcel is unzoned."), City.GetParcelLayout().FindParcel(ParcelId)->Zone == EZoneCategory::None);
	TestTrue(TEXT("The city remains valid."), City.Validate().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuildingRejectedZoneAtomicTest, "CityForm.Simulation.Building.RejectedZoneIsAtomic", BuildingTestFlags)

bool FBuildingRejectedZoneAtomicTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({707});
	const FParcelId ParcelId = AddParcelFixture(City);
	TestFalse(TEXT("None is rejected by ApplyZone."), City.ApplyZone(ParcelId, EZoneCategory::None).IsSuccess());
	TestTrue(TEXT("Rejected zoning leaves the parcel unzoned."),
		City.GetParcelLayout().FindParcel(ParcelId)->Zone == EZoneCategory::None);
	TestEqual(TEXT("Rejected zoning creates no building."), City.GetBuildings().GetBuildings().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuildingTimelineOverflowAtomicTest, "CityForm.Simulation.Building.TimelineOverflowIsAtomic", BuildingTestFlags)

bool FBuildingTimelineOverflowAtomicTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({709});
	const FParcelId ParcelId = AddParcelFixture(City);
	TestTrue(
		TEXT("The clock can approach its maximum."), City.Advance(FSimulationDuration(MAX_int64 - 100000)).IsSuccess());
	const FApplyZoneResult Result = City.ApplyZone(ParcelId, EZoneCategory::Residential);
	TestFalse(TEXT("A development timeline that would overflow is rejected."), Result.IsSuccess());
	TestTrue(
		TEXT("The overflow reports the stable time error."), Result.Error.Code == ESimulationErrorCode::TimeOverflow);
	TestTrue(TEXT("The rejected command leaves the parcel unzoned."),
		City.GetParcelLayout().FindParcel(ParcelId)->Zone == EZoneCategory::None);
	TestEqual(TEXT("The rejected command creates no Building."), City.GetBuildings().GetBuildings().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBuildingSummaryTest, "CityForm.Simulation.Building.Summary", BuildingTestFlags)

bool FBuildingSummaryTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({708});
	const FParcelId ParcelId = AddParcelFixture(City);
	City.ApplyZone(ParcelId, EZoneCategory::Commercial);
	FCitySummary Summary = City.GetSummary();
	TestEqual(TEXT("The starter catalog has two types."), Summary.BuildingTypeCount, 2);
	TestEqual(TEXT("One building exists."), Summary.BuildingCount, 1);
	TestEqual(TEXT("The building begins Planned."), Summary.PlannedBuildingCount, 1);
	TestEqual(TEXT("No active jobs exist before completion."), Summary.ActiveJobCapacity, 0);
	City.Advance(FSimulationDuration(300000));
	Summary = City.GetSummary();
	TestEqual(TEXT("The building is complete."), Summary.CompletedBuildingCount, 1);
	TestEqual(TEXT("Eight active jobs exist after completion."), Summary.ActiveJobCapacity, 8);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
