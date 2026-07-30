// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CitySimulation/SimulationResult.h"
#include "Templates/TypeHash.h"

namespace CityForm::Simulation
{

template <typename Tag>
class TStrongId
{
public:
	constexpr TStrongId() = default;

	explicit constexpr TStrongId(const uint64 InValue)
		: Value(InValue)
	{
	}

	constexpr bool IsValid() const
	{
		return Value != 0;
	}

	constexpr uint64 GetValue() const
	{
		return Value;
	}

	friend constexpr bool operator==(const TStrongId Left, const TStrongId Right)
	{
		return Left.Value == Right.Value;
	}

	friend constexpr bool operator!=(const TStrongId Left, const TStrongId Right)
	{
		return !(Left == Right);
	}

	friend constexpr bool operator<(const TStrongId Left, const TStrongId Right)
	{
		return Left.Value < Right.Value;
	}

	friend uint32 GetTypeHash(const TStrongId Id)
	{
		return ::GetTypeHash(Id.Value);
	}

private:
	uint64 Value = 0;
};

template <typename IdType>
struct TStrongIdAllocationResult
{
	IdType Id;
	FSimulationError Error;

	bool IsSuccess() const
	{
		return Id.IsValid() && !Error.IsSet();
	}
};

template <typename IdType>
class TStrongIdAllocator
{
public:
	explicit constexpr TStrongIdAllocator(const uint64 InNextValue = 1)
		: NextValue(InNextValue)
	{
	}

	TStrongIdAllocationResult<IdType> Allocate()
	{
		if (NextValue == 0)
		{
			return {
				IdType(),
				{ESimulationErrorCode::IdExhausted, TEXT("The strong-ID allocator is exhausted.")}};
		}

		const IdType AllocatedId(NextValue);
		NextValue = NextValue == MAX_uint64 ? 0 : NextValue + 1;
		return {AllocatedId, {}};
	}

	uint64 GetNextValueForDiagnostics() const
	{
		return NextValue;
	}

private:
	uint64 NextValue;
};

struct FRoadNodeIdTag;
struct FRoadSegmentIdTag;
struct FRoadTypeIdTag;
struct FVehicleClassIdTag;

using FRoadNodeId = TStrongId<FRoadNodeIdTag>;
using FRoadSegmentId = TStrongId<FRoadSegmentIdTag>;
using FRoadTypeId = TStrongId<FRoadTypeIdTag>;
using FVehicleClassId = TStrongId<FVehicleClassIdTag>;

} // namespace CityForm::Simulation
