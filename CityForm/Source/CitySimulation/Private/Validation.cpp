// Copyright Jonathan Morris. All Rights Reserved.

#include "CitySimulation/Validation.h"

namespace CityForm::Simulation
{

void FValidationReport::Add(FValidationIssue Issue)
{
	Issues.Add(MoveTemp(Issue));
}

void FValidationReport::Append(const FValidationReport& Other)
{
	Issues.Append(Other.GetIssues());
}

bool FValidationReport::IsValid() const
{
	for (const FValidationIssue& Issue : Issues)
	{
		if (Issue.Severity == EValidationSeverity::Error)
		{
			return false;
		}
	}

	return true;
}

const TArray<FValidationIssue>& FValidationReport::GetIssues() const
{
	return Issues;
}

} // namespace CityForm::Simulation
