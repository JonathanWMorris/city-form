// Copyright Jonathan Morris. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "../CityFormDevelopmentVisualizationActor.h"
#include "../CityFormZoningToolComponent.h"
#include "CitySimulation/Building.h"
#include "Misc/AutomationTest.h"

using namespace CityForm::Simulation;

namespace
{
constexpr EAutomationTestFlags ZoningTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCityFormParcelPickingTest, "CityForm.Presentation.Zoning.ParcelPicking", ZoningTestFlags)

bool FCityFormParcelPickingTest::RunTest(const FString& Parameters)
{
	TArray<FCityFormParcelSnapshot> Parcels;
	Parcels.Add({FParcelId(9), FVector::ZeroVector, 0.0, 1600.0, 3200.0, EZoneCategory::None});
	TestTrue(TEXT("A point inside an axis-aligned parcel is selected."),
		UCityFormZoningToolComponent::FindParcelAtPoint(Parcels, FVector(700.0, 1500.0, 0.0)).IsSet());
	TestFalse(TEXT("A point beyond a parcel edge is rejected."),
		UCityFormZoningToolComponent::FindParcelAtPoint(Parcels, FVector(801.0, 0.0, 0.0)).IsSet());

	Parcels[0].HeadingRadians = UE_DOUBLE_PI * 0.5;
	TestTrue(TEXT("Picking rotates with the parcel footprint."),
		UCityFormZoningToolComponent::FindParcelAtPoint(Parcels, FVector(1500.0, 700.0, 0.0)).IsSet());
	TestFalse(TEXT("A world-space point outside the rotated width is rejected."),
		UCityFormZoningToolComponent::FindParcelAtPoint(Parcels, FVector(0.0, 801.0, 0.0)).IsSet());

	Parcels.Add({FParcelId(2), FVector::ZeroVector, UE_DOUBLE_PI * 0.5, 1600.0, 3200.0, EZoneCategory::None});
	const TOptional<FCityFormParcelSnapshot> Overlap =
		UCityFormZoningToolComponent::FindParcelAtPoint(Parcels, FVector::ZeroVector);
	TestTrue(TEXT("An overlapping point selects a parcel."), Overlap.IsSet());
	if (Overlap.IsSet())
	{
		TestEqual(TEXT("Overlap tie-breaking chooses the lowest stable parcel ID."), Overlap->Id, FParcelId(2));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCityFormDevelopmentVisualTransformTest, "CityForm.Presentation.Zoning.DerivedVisuals", ZoningTestFlags)

bool FCityFormDevelopmentVisualTransformTest::RunTest(const FString& Parameters)
{
	FCityFormDevelopmentSnapshot Snapshot;
	Snapshot.Parcels.Add({FParcelId(1), FVector(1000.0, 2000.0, 0.0), 0.0, 1600.0, 3200.0, EZoneCategory::Residential});
	Snapshot.Buildings.Add({FBuildingId(1),
		FParcelId(1),
		FBuildingTypeCatalog::GetDetachedHouseTypeId(),
		EDevelopmentStage::Planned,
		0,
		0});

	const TArray<FCityFormParcelBoundaryVisual> Boundaries =
		ACityFormDevelopmentVisualizationActor::BuildParcelBoundaryVisuals(Snapshot);
	TestEqual(TEXT("One rectangular parcel produces four boundary strips."), Boundaries.Num(), 4);
	bool bAllBoundaryIdsAndZonesMatch = true;
	for (const FCityFormParcelBoundaryVisual& Boundary : Boundaries)
	{
		bAllBoundaryIdsAndZonesMatch = bAllBoundaryIdsAndZonesMatch && Boundary.ParcelId == FParcelId(1) &&
			Boundary.Zone == EZoneCategory::Residential;
	}
	TestTrue(TEXT("Every boundary keeps the parcel ID and zone."), bAllBoundaryIdsAndZonesMatch);

	TArray<FCityFormBuildingVisual> Buildings = ACityFormDevelopmentVisualizationActor::BuildBuildingVisuals(Snapshot);
	TestEqual(TEXT("One Building record produces one visual."), Buildings.Num(), 1);
	TestTrue(TEXT("A Planned record selects the foundation style."),
		Buildings[0].Style == ECityFormBuildingVisualStyle::Planned);
	TestEqual(TEXT("The visual remains centered on its Parcel (X)."), Buildings[0].Transform.GetLocation().X, 1000.0);
	TestEqual(TEXT("The visual remains centered on its Parcel (Y)."), Buildings[0].Transform.GetLocation().Y, 2000.0);

	Snapshot.Buildings[0].Stage = EDevelopmentStage::UnderConstruction;
	Buildings = ACityFormDevelopmentVisualizationActor::BuildBuildingVisuals(Snapshot);
	TestTrue(TEXT("Construction selects the orange intermediate style."),
		Buildings[0].Style == ECityFormBuildingVisualStyle::UnderConstruction);
	Snapshot.Buildings[0].Stage = EDevelopmentStage::Complete;
	Buildings = ACityFormDevelopmentVisualizationActor::BuildBuildingVisuals(Snapshot);
	TestTrue(TEXT("A complete DetachedHouse selects the residential style."),
		Buildings[0].Style == ECityFormBuildingVisualStyle::ResidentialComplete);
	Snapshot.Buildings[0].BuildingTypeId = FBuildingTypeCatalog::GetSmallCommercialTypeId();
	Buildings = ACityFormDevelopmentVisualizationActor::BuildBuildingVisuals(Snapshot);
	TestTrue(TEXT("A complete SmallCommercial selects the commercial style."),
		Buildings[0].Style == ECityFormBuildingVisualStyle::CommercialComplete);

	Snapshot.Buildings[0].ParcelId = FParcelId(999);
	TestEqual(TEXT("A dangling presentation record produces no visual."),
		ACityFormDevelopmentVisualizationActor::BuildBuildingVisuals(Snapshot).Num(),
		0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
