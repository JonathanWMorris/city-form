// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/RegionProfile.h"

#include <cmath>

namespace CityForm::Simulation
{

FRegionProfile FRegionProfile::MakeCalifornia()
{
	return {};
}

FValidationReport FRegionProfile::Validate() const
{
	FValidationReport Report;

	if (Identifier.IsEmpty())
	{
		Report.Add({
			EValidationSeverity::Error,
			EValidationIssueCode::EmptyRegionIdentifier,
			TEXT("RegionProfile"),
			0,
			TEXT("A region profile must have a stable identifier.")});
	}

	if (!std::isfinite(BasicTwoWayRoadDefaultSpeedLimitMetersPerSecond))
	{
		Report.Add({
			EValidationSeverity::Error,
			EValidationIssueCode::NonFiniteRegionalValue,
			TEXT("RegionProfile"),
			0,
			TEXT("The Basic Two-Way Road speed limit must be finite.")});
	}
	else if (BasicTwoWayRoadDefaultSpeedLimitMetersPerSecond <= 0.0)
	{
		Report.Add({
			EValidationSeverity::Error,
			EValidationIssueCode::NonPositiveRegionalValue,
			TEXT("RegionProfile"),
			0,
			TEXT("The Basic Two-Way Road speed limit must be greater than zero.")});
	}

	return Report;
}

} // namespace CityForm::Simulation
