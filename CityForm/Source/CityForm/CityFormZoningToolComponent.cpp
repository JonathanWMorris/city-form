// Copyright Jonathan Morris. All Rights Reserved.

#include "CityFormZoningToolComponent.h"

#include "CityFormPlayerController.h"
#include "CityFormPrototypeMapContract.h"
#include "Engine/GameInstance.h"

using namespace CityForm::Simulation;

void UCityFormZoningToolComponent::SetToolMode(const ECityFormToolMode InToolMode)
{
	ToolMode = InToolMode;
	switch (ToolMode)
	{
	case ECityFormToolMode::ResidentialZone:
		SetStatus(TEXT("Select a highlighted parcel to apply Residential zoning."));
		break;
	case ECityFormToolMode::CommercialZone:
		SetStatus(TEXT("Select a highlighted parcel to apply Commercial zoning."));
		break;
	case ECityFormToolMode::ClearZone:
		SetStatus(TEXT("Select a highlighted parcel to clear its zoning."));
		break;
	default:
		break;
	}
}

void UCityFormZoningToolComponent::HandlePrimaryAction()
{
	if (ToolMode != ECityFormToolMode::ResidentialZone && ToolMode != ECityFormToolMode::CommercialZone &&
		ToolMode != ECityFormToolMode::ClearZone)
	{
		return;
	}

	ACityFormPlayerController* Controller = Cast<ACityFormPlayerController>(GetOwner());
	if (Controller == nullptr || Controller->IsPointerOverToolPalette())
	{
		return;
	}

	FHitResult Hit;
	if (!Controller->GetHitResultUnderCursor(ECC_Visibility, true, Hit) ||
		!FCityFormPrototypeMapContract::IsPrototypeGroundHit(Hit.ImpactPoint))
	{
		SetStatus(TEXT("Zoning must target a highlighted parcel on the prototype ground."), true);
		return;
	}

	UGameInstance* GameInstance = Controller->GetGameInstance();
	UCityFormSimulationSubsystem* Simulation =
		GameInstance != nullptr ? GameInstance->GetSubsystem<UCityFormSimulationSubsystem>() : nullptr;
	if (Simulation == nullptr)
	{
		SetStatus(TEXT("The city simulation is unavailable."), true);
		return;
	}

	const TOptional<FCityFormParcelSnapshot> Parcel =
		FindParcelAtPoint(Simulation->CreateDevelopmentSnapshot().Parcels, Hit.ImpactPoint);
	if (!Parcel.IsSet())
	{
		SetStatus(TEXT("Choose a point inside a highlighted parcel boundary."), true);
		return;
	}

	if (ToolMode == ECityFormToolMode::ClearZone)
	{
		const FClearZoneResult Result = Simulation->ClearZone(Parcel->Id);
		if (!Result.IsSuccess())
		{
			SetStatus(Result.Error.Message, true);
			return;
		}
		SetStatus(FString::Printf(TEXT("Cleared zoning from parcel %llu."), Parcel->Id.GetValue()));
		return;
	}

	const EZoneCategory Zone =
		ToolMode == ECityFormToolMode::ResidentialZone ? EZoneCategory::Residential : EZoneCategory::Commercial;
	const FApplyZoneResult Result = Simulation->ApplyZone(Parcel->Id, Zone);
	if (!Result.IsSuccess())
	{
		SetStatus(Result.Error.Message, true);
		return;
	}
	SetStatus(FString::Printf(TEXT("Applied %s zoning to parcel %llu."),
		Zone == EZoneCategory::Residential ? TEXT("Residential") : TEXT("Commercial"),
		Parcel->Id.GetValue()));
}

TOptional<FCityFormParcelSnapshot> UCityFormZoningToolComponent::FindParcelAtPoint(
	const TArray<FCityFormParcelSnapshot>& Parcels, const FVector& WorldPoint)
{
	TOptional<FCityFormParcelSnapshot> Best;
	for (const FCityFormParcelSnapshot& Parcel : Parcels)
	{
		if (!FMath::IsFinite(Parcel.HeadingRadians) || !FMath::IsFinite(Parcel.WidthCentimeters) ||
			!FMath::IsFinite(Parcel.DepthCentimeters) || Parcel.WidthCentimeters <= 0.0 ||
			Parcel.DepthCentimeters <= 0.0)
		{
			continue;
		}
		const FVector2D Delta(WorldPoint.X - Parcel.CenterCentimeters.X, WorldPoint.Y - Parcel.CenterCentimeters.Y);
		const double CosHeading = FMath::Cos(Parcel.HeadingRadians);
		const double SinHeading = FMath::Sin(Parcel.HeadingRadians);
		const double Along = Delta.X * CosHeading + Delta.Y * SinHeading;
		const double Across = -Delta.X * SinHeading + Delta.Y * CosHeading;
		if (FMath::Abs(Along) <= Parcel.WidthCentimeters * 0.5 && FMath::Abs(Across) <= Parcel.DepthCentimeters * 0.5 &&
			(!Best.IsSet() || Parcel.Id < Best->Id))
		{
			Best = Parcel;
		}
	}
	return Best;
}

void UCityFormZoningToolComponent::SetStatus(const FString& Message, const bool bIsError) const
{
	if (ACityFormPlayerController* Controller = Cast<ACityFormPlayerController>(GetOwner()))
	{
		Controller->SetToolStatus(Message, bIsError);
	}
}
