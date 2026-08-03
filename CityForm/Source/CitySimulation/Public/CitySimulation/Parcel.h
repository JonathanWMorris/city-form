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

/** The allowed development category assigned to a Parcel. None means unassigned. */
enum class EZoneCategory : uint8
{
	None,
	Residential,
	Commercial
};

/**
 * A bounded, road-fronting piece of developable land generated beside one road segment.
 * Its footprint uses whole internal grid cells, but the Parcel itself is the zoning and
 * development unit. A Parcel may contain at most one Building regardless of footprint size.
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

	/** Footprint size in whole RegionProfile::ParcelCellSizeMeters cells. */
	int32 CellsWide = 1;
	int32 CellsDeep = 1;

	double WidthMeters = 0.0;
	double DepthMeters = 0.0;

	FSimPoint2D CenterPositionMeters;

	/** The generating segment's direction heading in radians, shared by every cell of that segment. */
	double HeadingRadians = 0.0;

	/** Persists across RegenerateParcels via footprint-key reconciliation; see FParcelLayout::RegenerateParcels. */
	EZoneCategory Zone = EZoneCategory::None;
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

struct FApplyZoneResult
{
	FSimulationError Error;

	bool IsSuccess() const
	{
		return !Error.IsSet();
	}
};

struct FClearZoneResult
{
	FSimulationError Error;

	bool IsSuccess() const
	{
		return !Error.IsSet();
	}
};

/**
 * Owns the current derived parcel set. RegenerateParcels deterministically recomputes
 * candidate geometry from the current RoadGraph and RegionProfile, then reconciles each
 * candidate against the previously stored set by a stable footprint key: a match keeps its
 * prior FParcelId and Zone, and a new footprint allocates a fresh ID from this layout's own
 * persistent allocator. Calling it repeatedly on an unchanged RoadGraph therefore reproduces
 * the exact same parcel set, including identical IDs and Zones. It is safe to call at any
 * time, repeatedly, by any future caller; it does not require the RoadGraph to have just
 * changed.
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
	 * lengths) leaves the previously stored parcel set, and this layout's ID allocator,
	 * unchanged.
	 */
	FRegenerateParcelsResult RegenerateParcels(const FRoadGraph& RoadGraph, const FRegionProfile& RegionProfile);

	/**
	 * Assigns Residential or Commercial to an existing parcel, overwriting any prior Zone.
	 * Rejects EZoneCategory::None (use ClearZone) and any unrecognized category, and rejects
	 * an unknown FParcelId, atomically: on failure the target parcel's Zone is left untouched.
	 */
	FApplyZoneResult ApplyZone(FParcelId Id, EZoneCategory Zone);

	/**
	 * Returns an existing parcel's Zone to None. Succeeds as a no-op if the parcel is already
	 * unzoned. Rejects an unknown FParcelId.
	 */
	FClearZoneResult ClearZone(FParcelId Id);

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
	 * Pure geometry-only candidate generation usable on hand-built Nodes/Segments arrays,
	 * mirroring FRoadGraph::ValidateRecords's (Nodes, Segments, config) shape. Segments whose
	 * endpoints cannot be resolved in Nodes are skipped rather than reported. Every returned
	 * candidate has an invalid (zero) Id and Zone == EZoneCategory::None; RegenerateParcels
	 * assigns final identity and carries forward persistent Zone state via footprint-key
	 * reconciliation. This function cannot fail: ID allocation, the only historical failure
	 * source, no longer happens here.
	 */
	static TArray<FParcel> GenerateCandidates(
		const TArray<FRoadNode>& Nodes, const TArray<FRoadSegment>& Segments, const FRegionProfile& RegionProfile);

private:
	void RebuildIndex();

	/** Persistent across RegenerateParcels calls so that FParcelId remains stable once allocated. */
	TStrongIdAllocator<FParcelId> ParcelIdAllocator;
	TArray<FParcel> Parcels;
	TMap<FParcelId, int32> ParcelIndexes;
};

} // namespace CityForm::Simulation
