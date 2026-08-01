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

} // namespace

FGenerateParcelRecordsResult FParcelLayout::GenerateRecords(
	const TArray<FRoadNode>& Nodes, const TArray<FRoadSegment>& Segments, const FRegionProfile& RegionProfile)
{
	FGenerateParcelRecordsResult Result;

	const double CellSizeMeters = RegionProfile.ParcelCellSizeMeters;
	const double SetbackMeters = RegionProfile.ParcelSetbackMeters;
	const int32 MaxDepthRows = RegionProfile.ParcelMaxDepthRows;
	if (!std::isfinite(CellSizeMeters) || CellSizeMeters <= 0.0 || !std::isfinite(SetbackMeters) || MaxDepthRows <= 0)
	{
		return Result;
	}

	TMap<FRoadNodeId, const FRoadNode*> NodesById;
	for (const FRoadNode& Node : Nodes)
	{
		NodesById.Add(Node.Id, &Node);
	}

	TStrongIdAllocator<FParcelId> ParcelIdAllocator;

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

			for (int32 Column = 0; Column < ColumnCount; ++Column)
			{
				const double AlongMeters = (Column + 0.5) * CellSizeMeters;
				const double ColumnCenterX = NodeA.PositionMeters.X + Direction.Ux * AlongMeters;
				const double ColumnCenterY = NodeA.PositionMeters.Y + Direction.Uy * AlongMeters;

				for (int32 Row = 0; Row < MaxDepthRows; ++Row)
				{
					const double AcrossMeters = SetbackMeters + (Row + 0.5) * CellSizeMeters;
					const FSimPoint2D Center{
						ColumnCenterX + PerpX * AcrossMeters, ColumnCenterY + PerpY * AcrossMeters};

					const TStrongIdAllocationResult<FParcelId> Allocation = ParcelIdAllocator.Allocate();
					if (!Allocation.IsSuccess())
					{
						return {{}, Allocation.Error};
					}

					Result.Parcels.Add({Allocation.Id,
						Segment.Id,
						Side,
						Column,
						Row,
						1,
						1,
						CellSizeMeters,
						CellSizeMeters,
						Center,
						Direction.HeadingRadians});
				}
			}
		}
	}

	return Result;
}

FRegenerateParcelsResult FParcelLayout::RegenerateParcels(
	const FRoadGraph& RoadGraph, const FRegionProfile& RegionProfile)
{
	FGenerateParcelRecordsResult Generated =
		GenerateRecords(RoadGraph.GetNodes(), RoadGraph.GetSegments(), RegionProfile);
	if (!Generated.IsSuccess())
	{
		return {0, Generated.Error};
	}

	Parcels = MoveTemp(Generated.Parcels);
	RebuildIndex();
	return {Parcels.Num(), {}};
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
