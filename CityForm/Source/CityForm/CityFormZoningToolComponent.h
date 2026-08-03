// Copyright Jonathan Morris. All Rights Reserved.

#pragma once

#include "CityFormSimulationSubsystem.h"
#include "CityFormToolMode.h"
#include "Components/ActorComponent.h"

#include "CityFormZoningToolComponent.generated.h"

/** Single-click parcel zoning that submits commands through the simulation bridge. */
UCLASS(ClassGroup = "City Form")
class CITYFORM_API UCityFormZoningToolComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	void SetToolMode(ECityFormToolMode InToolMode);
	void HandlePrimaryAction();

	ECityFormToolMode GetToolMode() const
	{
		return ToolMode;
	}

	static TOptional<FCityFormParcelSnapshot> FindParcelAtPoint(
		const TArray<FCityFormParcelSnapshot>& Parcels, const FVector& WorldPoint);

private:
	void SetStatus(const FString& Message, bool bIsError = false) const;

	ECityFormToolMode ToolMode = ECityFormToolMode::None;
};
