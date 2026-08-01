// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/RegionProfile.h"
#include "CitySimulation/RoadGraph.h"
#include "CitySimulation/SimulationResult.h"
#include "CitySimulation/StrongId.h"
#include "CitySimulation/Validation.h"
#include "Containers/Array.h"
#include "Containers/Map.h"

namespace CityForm::Simulation
{

/**
 * A fixed, documented determinism convention (counterclockwise rotation of the segment
 * direction in simulation XY), not a claim about real-world left or right.
 */
enum class ERoadSide : uint8
{
	Left,
	Right
};

/**
 * A bounded piece of developable land generated beside one road segment. v0.1 generation
 * always produces CellsWide == CellsDeep == 1; the footprint fields exist so a future
 * feature can combine contiguous 1x1 parcels into a larger footprint without a schema
 * change. A Parcel still may contain at most one Building regardless of footprint size.
 */
struct FParcel
{
	FParcelId Id;
	FRoadSegmentId RoadSegmentId;
	ERoadSide Side = ERoadSide::Left;

	/** 0-based index of this parcel's first (lowest-arclength) cell along the segment, measured from EndpointA. */
	int32 ColumnIndex = 0;

	/** 0-based index of this parcel's first (nearest-road) cell in depth. Row 0 touches the setback line. */
	int32 RowIndex = 0;

	/** Footprint size in whole RegionProfile::ParcelCellSizeMeters cells. Always 1x1 in v0.1 generation. */
	int32 CellsWide = 1;
	int32 CellsDeep = 1;

	double WidthMeters = 0.0;
	double DepthMeters = 0.0;

	FSimPoint2D CenterPositionMeters;

	/** The generating segment's direction heading in radians, shared by every cell of that segment. */
	double HeadingRadians = 0.0;
};

struct FGenerateParcelRecordsResult
{
	TArray<FParcel> Parcels;
	FSimulationError Error;

	bool IsSuccess() const
	{
		return !Error.IsSet();
	}
};

struct FRegenerateParcelsResult
{
	int32 ParcelCount = 0;
	FSimulationError Error;

	bool IsSuccess() const
	{
		return !Error.IsSet();
	}
};

/**
 * Owns the current derived parcel set. RegenerateParcels is a pure, idempotent recompute
 * from the current RoadGraph and RegionProfile: the same inputs always produce the same
 * ordered parcel set, including identical IDs, because ID allocation restarts at one on
 * every call. It is safe to call at any time, repeatedly, by any future caller; it does
 * not require the RoadGraph to have just changed.
 */
class CITYSIMULATION_API FParcelLayout
{
public:
	/**
	 * Looser than FRoadGraph::LengthValidationToleranceMeters because the geometry-mismatch
	 * recompute chains more floating-point operations (direction, perpendicular, two offsets)
	 * than a single hypot() call.
	 */
	static constexpr double GeometryValidationToleranceMeters = 1.0e-6;

	/**
	 * A failed result (ID exhaustion only; practically unreachable given map-bounded segment
	 * lengths) leaves the previously stored parcel set unchanged.
	 */
	FRegenerateParcelsResult RegenerateParcels(const FRoadGraph& RoadGraph, const FRegionProfile& RegionProfile);

	const FParcel* FindParcel(FParcelId Id) const;
	const TArray<FParcel>& GetParcels() const;

	FValidationReport Validate(const FRoadGraph& RoadGraph, const FRegionProfile& RegionProfile) const;

	/**
	 * Read-only, side-effect-free validation usable on hand-built (and possibly malformed)
	 * parcel arrays, mirroring FRoadGraph::ValidateRecords. Does not attempt cross-parcel
	 * overlap detection between parcels of different segments; it only diagnoses a single
	 * parcel record's own fields and its reference into RoadGraph.
	 */
	static FValidationReport ValidateRecords(
		const TArray<FParcel>& ParcelsToValidate, const FRoadGraph& RoadGraph, const FRegionProfile& RegionProfile);

	/**
	 * Pure generation usable on hand-built Nodes/Segments arrays, mirroring
	 * FRoadGraph::ValidateRecords's (Nodes, Segments, config) shape. Segments whose endpoints
	 * cannot be resolved in Nodes are skipped rather than reported; diagnosing a malformed
	 * graph is FRoadGraph's own validation's job, not this function's.
	 */
	static FGenerateParcelRecordsResult GenerateRecords(
		const TArray<FRoadNode>& Nodes, const TArray<FRoadSegment>& Segments, const FRegionProfile& RegionProfile);

private:
	void RebuildIndex();

	TArray<FParcel> Parcels;
	TMap<FParcelId, int32> ParcelIndexes;
};

} // namespace CityForm::Simulation
