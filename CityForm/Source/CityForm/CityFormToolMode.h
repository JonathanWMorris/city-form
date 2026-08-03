// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CityFormToolMode.generated.h"

UENUM(BlueprintType)
enum class ECityFormToolMode : uint8
{
	None,
	Road,
	ResidentialZone,
	CommercialZone,
	ClearZone
};

enum class ECityFormToolCategory : uint8
{
	None,
	Roads,
	Zoning
};
