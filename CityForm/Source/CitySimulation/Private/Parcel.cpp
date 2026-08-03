// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/Parcel.h"

#include <cmath>

namespace CityForm::Simulation
{
namespace
{

struct FSegmentDirection
{
	double Ux = 0.0;
	double Uy = 0.0;
	double LengthMeters = 0.0;
	double HeadingRadians = 0.0;
};

FSegmentDirection ComputeSegmentDirection(const FSimPoint2D& From, const FSimPoint2D& To)
{
	const double Dx = To.X - From.X;
	const double Dy = To.Y - From.Y;
	const double Length = std::hypot(Dx, Dy);
	if (!std::isfinite(Length) || Length <= 0.0)
	{
		return {};
	}

	return {Dx / Length, Dy / Length, Length, std::atan2(Dy, Dx)};
}

void GetPerpendicular(const FSegmentDirection& Direction, const ERoadSide Side, double& OutPerpX, double& OutPerpY)
{
	if (Side == ERoadSide::Left)
	{
		OutPerpX = -Direction.Uy;
		OutPerpY = Direction.Ux;
	}
	else
	{
		OutPerpX = Direction.Uy;
		OutPerpY = -Direction.Ux;
	}
}

/** A parcel's geometric identity, independent of any allocated FParcelId. Zone is deliberately excluded: it is
 *  carried-forward state, not identity. */
struct FParcelFootprintKey
{
	FRoadSegmentId RoadSegmentId;
	ERoadSide Side = ERoadSide::Left;
	int32 ColumnIndex = 0;
	int32 RowIndex = 0;
	int32 CellsWide = 1;
	int32 CellsDeep = 1;

	friend bool operator==(const FParcelFootprintKey& Left, const FParcelFootprintKey& Right)
	{
		return Left.RoadSegmentId == Right.RoadSegmentId && Left.Side == Right.Side &&
			Left.ColumnIndex == Right.ColumnIndex && Left.RowIndex == Right.RowIndex &&
			Left.CellsWide == Right.CellsWide && Left.CellsDeep == Right.CellsDeep;
	}

	friend uint32 GetTypeHash(const FParcelFootprintKey& Key)
	{
		uint32 Hash = GetTypeHash(Key.RoadSegmentId);
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Key.Side)));
		Hash = HashCombine(Hash, GetTypeHash(Key.ColumnIndex));
		Hash = HashCombine(Hash, GetTypeHash(Key.RowIndex));
		Hash = HashCombine(Hash, GetTypeHash(Key.CellsWide));
		Hash = HashCombine(Hash, GetTypeHash(Key.CellsDeep));
		return Hash;
	}
};

FParcelFootprintKey MakeFootprintKey(const FParcel& Parcel)
{
	return {Parcel.RoadSegmentId, Parcel.Side, Parcel.ColumnIndex, Parcel.RowIndex, Parcel.CellsWide, Parcel.CellsDeep};
}

} // namespace

TArray<FParcel> FParcelLayout::GenerateCandidates(
	const TArray<FRoadNode>& Nodes, const TArray<FRoadSegment>& Segments, const FRegionProfile& RegionProfile)
{
	TArray<FParcel> Candidates;

	const double CellSizeMeters = RegionProfile.ParcelCellSizeMeters;
	const double SetbackMeters = RegionProfile.ParcelSetbackMeters;
	const int32 WidthCells = RegionProfile.ParcelDefaultWidthCells;
	const int32 DepthCells = RegionProfile.ParcelDefaultDepthCells;
	if (!std::isfinite(CellSizeMeters) || CellSizeMeters <= 0.0 || !std::isfinite(SetbackMeters) || WidthCells <= 0 ||
		DepthCells <= 0)
	{
		return Candidates;
	}

	TMap<FRoadNodeId, const FRoadNode*> NodesById;
	for (const FRoadNode& Node : Nodes)
	{
		NodesById.Add(Node.Id, &Node);
	}

	for (const FRoadSegment& Segment : Segments)
	{
		const FRoadNode* const* NodeAPointer = NodesById.Find(Segment.EndpointA);
		const FRoadNode* const* NodeBPointer = NodesById.Find(Segment.EndpointB);
		if (NodeAPointer == nullptr || NodeBPointer == nullptr)
		{
			continue;
		}

		const FRoadNode& NodeA = **NodeAPointer;
		const FRoadNode& NodeB = **NodeBPointer;
		if (!NodeA.PositionMeters.IsFinite() || !NodeB.PositionMeters.IsFinite())
		{
			continue;
		}

		const FSegmentDirection Direction = ComputeSegmentDirection(NodeA.PositionMeters, NodeB.PositionMeters);
		if (Direction.LengthMeters <= 0.0)
		{
			continue;
		}

		const int32 ColumnCount = static_cast<int32>(std::floor(Direction.LengthMeters / CellSizeMeters));
		if (ColumnCount <= 0)
		{
			continue;
		}

		for (const ERoadSide Side : {ERoadSide::Left, ERoadSide::Right})
		{
			double PerpX = 0.0;
			double PerpY = 0.0;
			GetPerpendicular(Direction, Side, PerpX, PerpY);

			for (int32 Column = 0; Column + WidthCells <= ColumnCount; Column += WidthCells)
			{
				const double AlongMeters = (Column + WidthCells * 0.5) * CellSizeMeters;
				const double ColumnCenterX = NodeA.PositionMeters.X + Direction.Ux * AlongMeters;
				const double ColumnCenterY = NodeA.PositionMeters.Y + Direction.Uy * AlongMeters;
				const double AcrossMeters = SetbackMeters + DepthCells * CellSizeMeters * 0.5;
				const FSimPoint2D Center{ColumnCenterX + PerpX * AcrossMeters, ColumnCenterY + PerpY * AcrossMeters};

				Candidates.Add({FParcelId(),
					Segment.Id,
					Side,
					Column,
					0,
					WidthCells,
					DepthCells,
					WidthCells * CellSizeMeters,
					DepthCells * CellSizeMeters,
					Center,
					Direction.HeadingRadians});
			}
		}
	}

	return Candidates;
}

FRegenerateParcelsResult FParcelLayout::RegenerateParcels(
	const FRoadGraph& RoadGraph, const FRegionProfile& RegionProfile)
{
	TArray<FParcel> Candidates = GenerateCandidates(RoadGraph.GetNodes(), RoadGraph.GetSegments(), RegionProfile);

	TMap<FParcelFootprintKey, int32> PreviousIndexByFootprint;
	PreviousIndexByFootprint.Reserve(Parcels.Num());
	for (int32 Index = 0; Index < Parcels.Num(); ++Index)
	{
		PreviousIndexByFootprint.Add(MakeFootprintKey(Parcels[Index]), Index);
	}

	// First pass: resolve matches and count how many brand-new IDs this call would need, without
	// mutating anything yet, so a would-be exhaustion failure is fully atomic.
	TArray<int32> MatchedPreviousIndex;
	MatchedPreviousIndex.Reserve(Candidates.Num());
	uint64 NewCandidateCount = 0;
	for (const FParcel& Candidate : Candidates)
	{
		const int32* Found = PreviousIndexByFootprint.Find(MakeFootprintKey(Candidate));
		MatchedPreviousIndex.Add(Found != nullptr ? *Found : INDEX_NONE);
		if (Found == nullptr)
		{
			++NewCandidateCount;
		}
	}

	if (!ParcelIdAllocator.CanAllocate(NewCandidateCount))
	{
		return {0, {ESimulationErrorCode::IdExhausted, TEXT("The strong-ID allocator is exhausted.")}};
	}

	// Second pass: commit. Every element either reuses a previous Id/Zone or allocates a new Id,
	// which is guaranteed to succeed since CanAllocate already confirmed capacity for all of them.
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		if (MatchedPreviousIndex[Index] != INDEX_NONE)
		{
			Candidates[Index].Id = Parcels[MatchedPreviousIndex[Index]].Id;
			Candidates[Index].Zone = Parcels[MatchedPreviousIndex[Index]].Zone;
		}
		else
		{
			Candidates[Index].Id = ParcelIdAllocator.Allocate().Id;
		}
	}

	Parcels = MoveTemp(Candidates);
	RebuildIndex();
	return {Parcels.Num(), {}};
}

FApplyZoneResult FParcelLayout::ApplyZone(const FParcelId Id, const EZoneCategory Zone)
{
	if (Zone != EZoneCategory::Residential && Zone != EZoneCategory::Commercial)
	{
		return {{ESimulationErrorCode::InvalidZoneCategory,
			TEXT("ApplyZone requires Residential or Commercial; use ClearZone to unassign a parcel.")}};
	}

	const int32* Index = ParcelIndexes.Find(Id);
	if (Index == nullptr || !Parcels.IsValidIndex(*Index))
	{
		return {{ESimulationErrorCode::InvalidParcel, TEXT("ApplyZone requires an existing parcel ID.")}};
	}

	Parcels[*Index].Zone = Zone;
	return {};
}

FClearZoneResult FParcelLayout::ClearZone(const FParcelId Id)
{
	const int32* Index = ParcelIndexes.Find(Id);
	if (Index == nullptr || !Parcels.IsValidIndex(*Index))
	{
		return {{ESimulationErrorCode::InvalidParcel, TEXT("ClearZone requires an existing parcel ID.")}};
	}

	Parcels[*Index].Zone = EZoneCategory::None;
	return {};
}

const FParcel* FParcelLayout::FindParcel(const FParcelId Id) const
{
	const int32* Index = ParcelIndexes.Find(Id);
	return Index != nullptr && Parcels.IsValidIndex(*Index) ? &Parcels[*Index] : nullptr;
}

const TArray<FParcel>& FParcelLayout::GetParcels() const
{
	return Parcels;
}

FValidationReport FParcelLayout::Validate(const FRoadGraph& RoadGraph, const FRegionProfile& RegionProfile) const
{
	return ValidateRecords(Parcels, RoadGraph, RegionProfile);
}

FValidationReport FParcelLayout::ValidateRecords(
	const TArray<FParcel>& ParcelsToValidate, const FRoadGraph& RoadGraph, const FRegionProfile& RegionProfile)
{
	FValidationReport Report;
	TMap<FParcelId, bool> SeenIds;

	for (const FParcel& Parcel : ParcelsToValidate)
	{
		if (!Parcel.Id.IsValid())
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::InvalidParcelId,
				TEXT("Parcel"),
				0,
				TEXT("A parcel must have a valid ID.")});
		}
		else if (SeenIds.Contains(Parcel.Id))
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::DuplicateParcelId,
				TEXT("Parcel"),
				Parcel.Id.GetValue(),
				TEXT("Parcel IDs must be unique.")});
		}
		else
		{
			SeenIds.Add(Parcel.Id, true);
		}

		const FRoadSegment* Segment = RoadGraph.FindSegment(Parcel.RoadSegmentId);
		if (Segment == nullptr)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::InvalidParcelRoadSegment,
				TEXT("Parcel"),
				Parcel.Id.GetValue(),
				TEXT("A parcel must reference an existing road segment.")});
		}

		if (Parcel.Side != ERoadSide::Left && Parcel.Side != ERoadSide::Right)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::InvalidParcelSide,
				TEXT("Parcel"),
				Parcel.Id.GetValue(),
				TEXT("A parcel side must be Left or Right.")});
		}

		if (Parcel.Zone != EZoneCategory::None && Parcel.Zone != EZoneCategory::Residential &&
			Parcel.Zone != EZoneCategory::Commercial)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::InvalidParcelZone,
				TEXT("Parcel"),
				Parcel.Id.GetValue(),
				TEXT("A parcel zone must be None, Residential, or Commercial.")});
		}

		if (Parcel.ColumnIndex < 0 || Parcel.RowIndex < 0)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::NegativeParcelIndex,
				TEXT("Parcel"),
				Parcel.Id.GetValue(),
				TEXT("A parcel's column and row index must not be negative.")});
		}

		if (Parcel.CellsWide < 1 || Parcel.CellsDeep < 1)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::NonPositiveParcelCellCount,
				TEXT("Parcel"),
				Parcel.Id.GetValue(),
				TEXT("A parcel must span at least one cell in width and depth.")});
		}

		if (!Parcel.CenterPositionMeters.IsFinite() || !std::isfinite(Parcel.HeadingRadians))
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::NonFiniteParcelPosition,
				TEXT("Parcel"),
				Parcel.Id.GetValue(),
				TEXT("A parcel's center position and heading must be finite.")});
		}

		if (!std::isfinite(Parcel.WidthMeters) || !std::isfinite(Parcel.DepthMeters))
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::NonFiniteParcelDimensions,
				TEXT("Parcel"),
				Parcel.Id.GetValue(),
				TEXT("A parcel's width and depth must be finite.")});
		}
		else if (Parcel.WidthMeters <= 0.0 || Parcel.DepthMeters <= 0.0)
		{
			Report.Add({EValidationSeverity::Error,
				EValidationIssueCode::NonPositiveParcelDimensions,
				TEXT("Parcel"),
				Parcel.Id.GetValue(),
				TEXT("A parcel's width and depth must be greater than zero.")});
		}

		const bool bHasFiniteSelfGeometry =
			Parcel.CenterPositionMeters.IsFinite() && std::isfinite(Parcel.HeadingRadians);
		if (Segment != nullptr && (Parcel.Side == ERoadSide::Left || Parcel.Side == ERoadSide::Right) &&
			bHasFiniteSelfGeometry)
		{
			const FRoadNode* NodeA = RoadGraph.FindNode(Segment->EndpointA);
			const FRoadNode* NodeB = RoadGraph.FindNode(Segment->EndpointB);
			if (NodeA != nullptr && NodeB != nullptr && NodeA->PositionMeters.IsFinite() &&
				NodeB->PositionMeters.IsFinite() && std::isfinite(RegionProfile.ParcelCellSizeMeters) &&
				RegionProfile.ParcelCellSizeMeters > 0.0)
			{
				const FSegmentDirection Direction =
					ComputeSegmentDirection(NodeA->PositionMeters, NodeB->PositionMeters);
				if (Direction.LengthMeters > 0.0)
				{
					double PerpX = 0.0;
					double PerpY = 0.0;
					GetPerpendicular(Direction, Parcel.Side, PerpX, PerpY);

					const double CellSizeMeters = RegionProfile.ParcelCellSizeMeters;
					const double AlongMeters =
						Parcel.ColumnIndex * CellSizeMeters + Parcel.CellsWide * CellSizeMeters * 0.5;
					const double AcrossMeters = RegionProfile.ParcelSetbackMeters + Parcel.RowIndex * CellSizeMeters +
						Parcel.CellsDeep * CellSizeMeters * 0.5;
					const double ExpectedX =
						NodeA->PositionMeters.X + Direction.Ux * AlongMeters + PerpX * AcrossMeters;
					const double ExpectedY =
						NodeA->PositionMeters.Y + Direction.Uy * AlongMeters + PerpY * AcrossMeters;

					const double PositionErrorMeters = std::hypot(
						Parcel.CenterPositionMeters.X - ExpectedX, Parcel.CenterPositionMeters.Y - ExpectedY);
					const double HeadingErrorRadians = std::fabs(Parcel.HeadingRadians - Direction.HeadingRadians);
					if (PositionErrorMeters > GeometryValidationToleranceMeters ||
						HeadingErrorRadians > GeometryValidationToleranceMeters)
					{
						Report.Add({EValidationSeverity::Error,
							EValidationIssueCode::ParcelGeometryMismatch,
							TEXT("Parcel"),
							Parcel.Id.GetValue(),
							TEXT("A parcel's cached geometry does not match its segment, side, and grid indices.")});
					}
				}
			}
		}
	}

	return Report;
}

void FParcelLayout::RebuildIndex()
{
	ParcelIndexes.Empty(Parcels.Num());
	for (int32 Index = 0; Index < Parcels.Num(); ++Index)
	{
		ParcelIndexes.Add(Parcels[Index].Id, Index);
	}
}

} // namespace CityForm::Simulation
