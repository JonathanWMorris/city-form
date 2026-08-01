// Copyright Jonathan Morris. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "CitySimulation/CitySimulation.h"
#include "CitySimulation/Parcel.h"
#include "CitySimulation/RoadGraph.h"
#include "CitySimulation/RoadType.h"
#include "Misc/AutomationTest.h"

#include <cmath>
#include <limits>

using namespace CityForm::Simulation;

namespace
{

constexpr EAutomationTestFlags ParcelTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

int32 CountParcelIssues(const FValidationReport& Report, const EValidationIssueCode Code)
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

FRoadSegmentDefinition MakeBasicRoad()
{
	return {FRoadTypeCatalog::GetBasicTwoWayRoadTypeId(), TOptional<double>()};
}

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParcelDeterminismTest, "CityForm.Simulation.Parcel.Determinism", ParcelTestFlags)

bool FParcelDeterminismTest::RunTest(const FString& Parameters)
{
	FCitySimulation FirstCity({601});
	FCitySimulation SecondCity({601});

	const FRoadNodeId FirstA = FirstCity.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId FirstB = FirstCity.AddRoadNode({100.0, 0.0}).NodeId;
	FirstCity.AddRoadSegment(FirstA, FirstB, MakeBasicRoad());

	const FRoadNodeId SecondA = SecondCity.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId SecondB = SecondCity.AddRoadNode({100.0, 0.0}).NodeId;
	SecondCity.AddRoadSegment(SecondA, SecondB, MakeBasicRoad());

	const TArray<FParcel>& FirstParcels = FirstCity.GetParcelLayout().GetParcels();
	const TArray<FParcel>& SecondParcels = SecondCity.GetParcelLayout().GetParcels();
	TestTrue(TEXT("Identical graphs produce a non-empty parcel set."), FirstParcels.Num() > 0);
	TestEqual(TEXT("Identical graphs produce the same parcel count."), FirstParcels.Num(), SecondParcels.Num());

	bool bAllMatch = FirstParcels.Num() == SecondParcels.Num();
	for (int32 Index = 0; bAllMatch && Index < FirstParcels.Num(); ++Index)
	{
		const FParcel& A = FirstParcels[Index];
		const FParcel& B = SecondParcels[Index];
		bAllMatch = A.Id == B.Id && A.RoadSegmentId == B.RoadSegmentId && A.Side == B.Side &&
			A.ColumnIndex == B.ColumnIndex && A.RowIndex == B.RowIndex &&
			A.CenterPositionMeters.X == B.CenterPositionMeters.X &&
			A.CenterPositionMeters.Y == B.CenterPositionMeters.Y && A.HeadingRadians == B.HeadingRadians;
	}
	TestTrue(TEXT("The same road graph produces the same ordered parcel set."), bAllMatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParcelBothSidesGeneratedTest, "CityForm.Simulation.Parcel.BothSidesGenerated", ParcelTestFlags)

bool FParcelBothSidesGeneratedTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({602});
	const FRoadNodeId NodeA = City.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId NodeB = City.AddRoadNode({100.0, 0.0}).NodeId;
	City.AddRoadSegment(NodeA, NodeB, MakeBasicRoad());

	const TArray<FParcel>& Parcels = City.GetParcelLayout().GetParcels();
	int32 LeftCount = 0;
	int32 RightCount = 0;
	for (const FParcel& Parcel : Parcels)
	{
		if (Parcel.Side == ERoadSide::Left)
		{
			++LeftCount;
		}
		else
		{
			++RightCount;
		}
	}
	TestTrue(TEXT("Both sides produce parcels."), LeftCount > 0 && RightCount > 0);
	TestEqual(TEXT("Both sides produce an equal number of parcels."), LeftCount, RightCount);

	const FParcel* LeftOrigin = Parcels.FindByPredicate(
		[](const FParcel& Candidate)
		{
			return Candidate.Side == ERoadSide::Left && Candidate.ColumnIndex == 0 && Candidate.RowIndex == 0;
		});
	const FParcel* RightOrigin = Parcels.FindByPredicate(
		[](const FParcel& Candidate)
		{
			return Candidate.Side == ERoadSide::Right && Candidate.ColumnIndex == 0 && Candidate.RowIndex == 0;
		});
	TestNotNull(TEXT("A column zero, row zero Left parcel exists."), LeftOrigin);
	TestNotNull(TEXT("A column zero, row zero Right parcel exists."), RightOrigin);
	if (LeftOrigin != nullptr && RightOrigin != nullptr)
	{
		const double MidpointX = (LeftOrigin->CenterPositionMeters.X + RightOrigin->CenterPositionMeters.X) * 0.5;
		const double MidpointY = (LeftOrigin->CenterPositionMeters.Y + RightOrigin->CenterPositionMeters.Y) * 0.5;
		TestEqual(TEXT("The Left/Right midpoint lands on the segment centerline (X)."), MidpointX, 4.0);
		TestEqual(TEXT("The Left/Right midpoint lands on the segment centerline (Y)."), MidpointY, 0.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParcelDiagonalSegmentAlignedTilingTest,
	"CityForm.Simulation.Parcel.DiagonalSegmentAlignedTiling",
	ParcelTestFlags)

bool FParcelDiagonalSegmentAlignedTilingTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({603});
	const FRoadNodeId NodeA = City.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId NodeB = City.AddRoadNode({30.0, 40.0}).NodeId;
	City.AddRoadSegment(NodeA, NodeB, MakeBasicRoad());

	const FParcel* FirstCell = City.GetParcelLayout().GetParcels().FindByPredicate(
		[](const FParcel& Candidate)
		{
			return Candidate.Side == ERoadSide::Left && Candidate.ColumnIndex == 0 && Candidate.RowIndex == 0;
		});
	TestNotNull(TEXT("Column zero, row zero, Left exists for the diagonal segment."), FirstCell);
	if (FirstCell != nullptr)
	{
		TestTrue(TEXT("The cell center matches the worked segment-aligned example (X)."),
			std::fabs(FirstCell->CenterPositionMeters.X - (-4.0)) < 1e-9);
		TestTrue(TEXT("The cell center matches the worked segment-aligned example (Y)."),
			std::fabs(FirstCell->CenterPositionMeters.Y - 8.0) < 1e-9);
		TestTrue(TEXT("The cell heading matches the segment's own direction, not a world-axis grid."),
			std::fabs(FirstCell->HeadingRadians - std::atan2(40.0, 30.0)) < 1e-9);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParcelLeftoverRemainderSkippedTest, "CityForm.Simulation.Parcel.LeftoverRemainderSkipped", ParcelTestFlags)

bool FParcelLeftoverRemainderSkippedTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({604});
	const FRoadNodeId NodeA = City.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId NodeB = City.AddRoadNode({20.0, 0.0}).NodeId;
	City.AddRoadSegment(NodeA, NodeB, MakeBasicRoad());

	const TArray<FParcel>& Parcels = City.GetParcelLayout().GetParcels();
	int32 MaxColumnIndex = -1;
	for (const FParcel& Parcel : Parcels)
	{
		MaxColumnIndex = Parcel.ColumnIndex > MaxColumnIndex ? Parcel.ColumnIndex : MaxColumnIndex;
	}
	TestEqual(TEXT("A 20m segment with 8m cells produces exactly two columns (16m used, 4m leftover skipped)."),
		MaxColumnIndex,
		1);
	TestEqual(TEXT("Two columns, four rows, two sides produce sixteen parcels total."), Parcels.Num(), 16);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParcelDepthCappedAtMaxRowsTest, "CityForm.Simulation.Parcel.DepthCappedAtMaxRows", ParcelTestFlags)

bool FParcelDepthCappedAtMaxRowsTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({605});
	const FRoadNodeId NodeA = City.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId NodeB = City.AddRoadNode({100.0, 0.0}).NodeId;
	City.AddRoadSegment(NodeA, NodeB, MakeBasicRoad());

	const int32 MaxDepthRows = FRegionProfile::MakeCalifornia().ParcelMaxDepthRows;
	bool bAnyRowAtLimit = false;
	bool bAnyRowBeyondLimit = false;
	for (const FParcel& Parcel : City.GetParcelLayout().GetParcels())
	{
		if (Parcel.RowIndex >= MaxDepthRows)
		{
			bAnyRowBeyondLimit = true;
		}
		if (Parcel.RowIndex == MaxDepthRows - 1)
		{
			bAnyRowAtLimit = true;
		}
	}
	TestFalse(TEXT("No parcel exceeds the configured max depth rows."), bAnyRowBeyondLimit);
	TestTrue(TEXT("The deepest configured row is actually generated."), bAnyRowAtLimit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParcelSegmentTooShortProducesNoParcelsWithoutErrorTest,
	"CityForm.Simulation.Parcel.SegmentTooShortProducesNoParcelsWithoutError",
	ParcelTestFlags)

bool FParcelSegmentTooShortProducesNoParcelsWithoutErrorTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({606});
	const FRoadNodeId NodeA = City.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId NodeB = City.AddRoadNode({5.0, 0.0}).NodeId;
	const FAddRoadSegmentResult Segment = City.AddRoadSegment(NodeA, NodeB, MakeBasicRoad());
	TestTrue(TEXT("The short road segment is still accepted by the road graph."), Segment.IsSuccess());

	const FRegenerateParcelsResult Regenerated = City.RegenerateParcels();
	TestTrue(TEXT("Regenerating parcels for a too-short segment succeeds."), Regenerated.IsSuccess());
	TestEqual(
		TEXT("A segment too short for one cell produces zero parcels, not an error."), Regenerated.ParcelCount, 0);
	TestEqual(TEXT("The stored parcel set is empty."), City.GetParcelLayout().GetParcels().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParcelRegenerateReplacesRatherThanAccumulatesTest,
	"CityForm.Simulation.Parcel.RegenerateReplacesRatherThanAccumulates",
	ParcelTestFlags)

bool FParcelRegenerateReplacesRatherThanAccumulatesTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({607});
	const FRoadNodeId NodeA = City.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId NodeB = City.AddRoadNode({100.0, 0.0}).NodeId;
	City.AddRoadSegment(NodeA, NodeB, MakeBasicRoad());

	const int32 FirstCount = City.GetParcelLayout().GetParcels().Num();
	const FRegenerateParcelsResult Second = City.RegenerateParcels();
	TestTrue(TEXT("A repeated regenerate call succeeds."), Second.IsSuccess());
	TestEqual(TEXT("Calling RegenerateParcels again does not double the set."), Second.ParcelCount, FirstCount);
	TestEqual(TEXT("The stored parcel array size matches, not accumulates."),
		City.GetParcelLayout().GetParcels().Num(),
		FirstCount);

	const FRoadNodeId NodeC = City.AddRoadNode({0.0, 100.0}).NodeId;
	const FRoadNodeId NodeD = City.AddRoadNode({100.0, 100.0}).NodeId;
	City.AddRoadSegment(NodeC, NodeD, MakeBasicRoad());

	const TArray<FParcel>& AllParcels = City.GetParcelLayout().GetParcels();
	TestTrue(
		TEXT("Adding a second segment produces more parcels than the first alone."), AllParcels.Num() > FirstCount);

	TMap<FParcelId, bool> SeenIds;
	bool bAllUnique = true;
	for (const FParcel& Parcel : AllParcels)
	{
		if (SeenIds.Contains(Parcel.Id))
		{
			bAllUnique = false;
		}
		SeenIds.Add(Parcel.Id, true);
	}
	TestTrue(TEXT("No duplicate parcel IDs exist after regenerating with two segments."), bAllUnique);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParcelAutoRegenerateAfterRoadCommandsTest,
	"CityForm.Simulation.Parcel.AutoRegenerateAfterRoadCommands",
	ParcelTestFlags)

bool FParcelAutoRegenerateAfterRoadCommandsTest::RunTest(const FString& Parameters)
{
	FCitySimulation City({608});
	TestEqual(TEXT("A fresh city has no parcels."), City.GetParcelLayout().GetParcels().Num(), 0);

	const FRoadNodeId NodeA = City.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId NodeB = City.AddRoadNode({100.0, 0.0}).NodeId;
	const FAddRoadSegmentResult Segment = City.AddRoadSegment(NodeA, NodeB, MakeBasicRoad());
	TestTrue(TEXT("The road segment is accepted."), Segment.IsSuccess());

	TestTrue(TEXT("A successful AddRoadSegment alone populates the parcel set, without an explicit "
				  "RegenerateParcels call."),
		City.GetParcelLayout().GetParcels().Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParcelRecordValidationCatchesEachNewIssueCodeTest,
	"CityForm.Simulation.Parcel.RecordValidationCatchesEachNewIssueCode",
	ParcelTestFlags)

bool FParcelRecordValidationCatchesEachNewIssueCodeTest::RunTest(const FString& Parameters)
{
	FRoadGraph Graph;
	const FRoadTypeCatalog RoadTypes;
	const FRoadNodeId NodeA = Graph.AddRoadNode({0.0, 0.0}).NodeId;
	const FRoadNodeId NodeB = Graph.AddRoadNode({100.0, 0.0}).NodeId;
	const FRoadSegmentId SegmentId = Graph.AddRoadSegment(NodeA, NodeB, MakeBasicRoad(), RoadTypes).SegmentId;

	const FRegionProfile RegionProfile = FRegionProfile::MakeCalifornia();
	TArray<FParcel> Parcels;

	// Invalid ID: otherwise fully correct (Column 0, Row 0, Left on the real segment).
	Parcels.Add({FParcelId(), SegmentId, ERoadSide::Left, 0, 0, 1, 1, 8.0, 8.0, {4.0, 8.0}, 0.0});

	// Duplicate ID: two entries sharing the same valid, otherwise-correct ID.
	Parcels.Add({FParcelId(2), SegmentId, ERoadSide::Left, 0, 0, 1, 1, 8.0, 8.0, {4.0, 8.0}, 0.0});
	Parcels.Add({FParcelId(2), SegmentId, ERoadSide::Left, 0, 0, 1, 1, 8.0, 8.0, {4.0, 8.0}, 0.0});

	// Invalid road-segment reference (skips the geometry check entirely, so any geometry is fine here).
	Parcels.Add({FParcelId(3), FRoadSegmentId(999), ERoadSide::Left, 0, 0, 1, 1, 8.0, 8.0, {4.0, 8.0}, 0.0});

	// Invalid side (also skips the geometry check).
	Parcels.Add({FParcelId(4), SegmentId, static_cast<ERoadSide>(99), 0, 0, 1, 1, 8.0, 8.0, {4.0, 8.0}, 0.0});

	// Negative column index, with geometry recomputed to stay self-consistent so this only trips one code.
	Parcels.Add({FParcelId(5), SegmentId, ERoadSide::Left, -1, 0, 1, 1, 8.0, 8.0, {-4.0, 8.0}, 0.0});

	// Zero cell width, with geometry recomputed for CellsWide=0 to stay self-consistent.
	Parcels.Add({FParcelId(6), SegmentId, ERoadSide::Left, 0, 0, 0, 1, 8.0, 8.0, {0.0, 8.0}, 0.0});

	// Non-finite center position (geometry check is skipped for non-finite self geometry).
	Parcels.Add({FParcelId(7),
		SegmentId,
		ERoadSide::Left,
		0,
		0,
		1,
		1,
		8.0,
		8.0,
		{std::numeric_limits<double>::quiet_NaN(), 8.0},
		0.0});

	// Non-finite width, otherwise self-consistent geometry.
	Parcels.Add({FParcelId(8),
		SegmentId,
		ERoadSide::Left,
		0,
		0,
		1,
		1,
		std::numeric_limits<double>::quiet_NaN(),
		8.0,
		{4.0, 8.0},
		0.0});

	// Non-positive (but finite) depth, otherwise self-consistent geometry.
	Parcels.Add({FParcelId(9), SegmentId, ERoadSide::Left, 0, 0, 1, 1, 8.0, 0.0, {4.0, 8.0}, 0.0});

	// Deliberately wrong cached center position for an otherwise fully valid parcel.
	Parcels.Add({FParcelId(10), SegmentId, ERoadSide::Left, 0, 0, 1, 1, 8.0, 8.0, {999.0, 999.0}, 0.0});

	const FValidationReport Report = FParcelLayout::ValidateRecords(Parcels, Graph, RegionProfile);
	TestFalse(TEXT("Malformed parcel records fail validation."), Report.IsValid());
	TestEqual(
		TEXT("An invalid parcel ID is reported."), CountParcelIssues(Report, EValidationIssueCode::InvalidParcelId), 1);
	TestEqual(TEXT("A duplicate parcel ID is reported."),
		CountParcelIssues(Report, EValidationIssueCode::DuplicateParcelId),
		1);
	TestEqual(TEXT("An invalid parcel road-segment reference is reported."),
		CountParcelIssues(Report, EValidationIssueCode::InvalidParcelRoadSegment),
		1);
	TestEqual(TEXT("An invalid parcel side is reported."),
		CountParcelIssues(Report, EValidationIssueCode::InvalidParcelSide),
		1);
	TestEqual(TEXT("A negative parcel index is reported."),
		CountParcelIssues(Report, EValidationIssueCode::NegativeParcelIndex),
		1);
	TestEqual(TEXT("A non-positive parcel cell count is reported."),
		CountParcelIssues(Report, EValidationIssueCode::NonPositiveParcelCellCount),
		1);
	TestEqual(TEXT("A non-finite parcel position is reported."),
		CountParcelIssues(Report, EValidationIssueCode::NonFiniteParcelPosition),
		1);
	TestEqual(TEXT("A non-finite parcel dimension is reported."),
		CountParcelIssues(Report, EValidationIssueCode::NonFiniteParcelDimensions),
		1);
	TestEqual(TEXT("A non-positive parcel dimension is reported."),
		CountParcelIssues(Report, EValidationIssueCode::NonPositiveParcelDimensions),
		1);
	TestEqual(TEXT("A parcel geometry mismatch is reported."),
		CountParcelIssues(Report, EValidationIssueCode::ParcelGeometryMismatch),
		1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParcelRegionProfileDefaultsValidateTest,
	"CityForm.Simulation.Parcel.RegionProfileParcelDefaultsValidate",
	ParcelTestFlags)

bool FParcelRegionProfileDefaultsValidateTest::RunTest(const FString& Parameters)
{
	const FRegionProfile California = FRegionProfile::MakeCalifornia();
	TestEqual(TEXT("California defaults to 8m parcel cells."), California.ParcelCellSizeMeters, 8.0);
	TestEqual(TEXT("California defaults to four parcel depth rows."), California.ParcelMaxDepthRows, 4);
	TestEqual(TEXT("California defaults to a 4m parcel setback."), California.ParcelSetbackMeters, 4.0);
	TestTrue(TEXT("The California profile with parcel defaults validates."), California.Validate().IsValid());

	FRegionProfile NonFiniteCellSize = California;
	NonFiniteCellSize.ParcelCellSizeMeters = std::numeric_limits<double>::quiet_NaN();
	TestEqual(TEXT("A non-finite parcel cell size is reported."),
		CountParcelIssues(NonFiniteCellSize.Validate(), EValidationIssueCode::NonFiniteRegionalValue),
		1);

	FRegionProfile NonPositiveCellSize = California;
	NonPositiveCellSize.ParcelCellSizeMeters = 0.0;
	TestEqual(TEXT("A non-positive parcel cell size is reported."),
		CountParcelIssues(NonPositiveCellSize.Validate(), EValidationIssueCode::NonPositiveRegionalValue),
		1);

	FRegionProfile NonPositiveDepthRows = California;
	NonPositiveDepthRows.ParcelMaxDepthRows = 0;
	TestEqual(TEXT("A non-positive parcel depth-row count is reported."),
		CountParcelIssues(NonPositiveDepthRows.Validate(), EValidationIssueCode::NonPositiveRegionalValue),
		1);

	FRegionProfile NonFiniteSetback = California;
	NonFiniteSetback.ParcelSetbackMeters = std::numeric_limits<double>::infinity();
	TestEqual(TEXT("A non-finite parcel setback is reported."),
		CountParcelIssues(NonFiniteSetback.Validate(), EValidationIssueCode::NonFiniteRegionalValue),
		1);

	FRegionProfile NonPositiveSetback = California;
	NonPositiveSetback.ParcelSetbackMeters = -1.0;
	TestEqual(TEXT("A non-positive parcel setback is reported."),
		CountParcelIssues(NonPositiveSetback.Validate(), EValidationIssueCode::NonPositiveRegionalValue),
		1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
