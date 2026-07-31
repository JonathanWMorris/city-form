// Copyright Jonathan Morris. All Rights Reserved.

#include "CityFormToolPaletteWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CityFormPlayerController.h"

namespace
{
UButton* AddToolButton(
	UWidgetTree* WidgetTree,
	UHorizontalBox* Row,
	const TCHAR* ButtonName,
	const TCHAR* Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetJustification(ETextJustify::Center);
	Button->AddChild(Text);
	Row->AddChildToHorizontalBox(Button);
	return Button;
}
}

void UCityFormToolPaletteWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PaletteSize"));
	SizeBox->SetMinDesiredWidth(620.0f);
	SizeBox->SetMinDesiredHeight(100.0f);
	WidgetTree->RootWidget = SizeBox;

	PaletteBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PaletteBorder"));
	PaletteBorder->SetPadding(FMargin(12.0f));
	PaletteBorder->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.05f, 0.92f));
	SizeBox->AddChild(PaletteBorder);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	PaletteBorder->AddChild(Column);
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	Column->AddChildToVerticalBox(ButtonRow);

	RoadButton = AddToolButton(WidgetTree, ButtonRow, TEXT("RoadButton"), TEXT("Road"));
	ResidentialButton = AddToolButton(WidgetTree, ButtonRow, TEXT("ResidentialButton"), TEXT("Residential"));
	CommercialButton = AddToolButton(WidgetTree, ButtonRow, TEXT("CommercialButton"), TEXT("Commercial"));
	RoadButton->OnClicked.AddDynamic(this, &UCityFormToolPaletteWidget::HandleRoadClicked);
	ResidentialButton->OnClicked.AddDynamic(this, &UCityFormToolPaletteWidget::HandleResidentialClicked);
	CommercialButton->OnClicked.AddDynamic(this, &UCityFormToolPaletteWidget::HandleCommercialClicked);
	ResidentialButton->SetIsEnabled(false);
	CommercialButton->SetIsEnabled(false);

	ToolText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ToolText"));
	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	Column->AddChildToVerticalBox(ToolText);
	Column->AddChildToVerticalBox(StatusText);
	SetSelectedTool(ECityFormToolMode::None);
	SetStatus(TEXT("Choose the Road tool to begin. Residential and Commercial arrive in Stage 5."));
}

void UCityFormToolPaletteWidget::InitializeForController(ACityFormPlayerController* InController)
{
	Controller = InController;
}

void UCityFormToolPaletteWidget::SetSelectedTool(const ECityFormToolMode ToolMode)
{
	if (ToolText != nullptr)
	{
		ToolText->SetText(FText::FromString(
			ToolMode == ECityFormToolMode::Road ? TEXT("Selected tool: Road") : TEXT("Selected tool: None")));
	}
	if (RoadButton != nullptr)
	{
		RoadButton->SetBackgroundColor(
			ToolMode == ECityFormToolMode::Road
				? FLinearColor(0.15f, 0.65f, 0.3f, 1.0f)
				: FLinearColor::White);
	}
}

void UCityFormToolPaletteWidget::SetStatus(const FString& Message, const bool bIsError)
{
	if (StatusText != nullptr)
	{
		StatusText->SetText(FText::FromString(Message));
		StatusText->SetColorAndOpacity(
			bIsError ? FSlateColor(FLinearColor(1.0f, 0.25f, 0.2f)) : FSlateColor(FLinearColor::White));
	}
}

bool UCityFormToolPaletteWidget::IsPointerOverPalette() const
{
	return PaletteBorder != nullptr && PaletteBorder->IsHovered();
}

void UCityFormToolPaletteWidget::HandleRoadClicked()
{
	if (Controller != nullptr)
	{
		Controller->SetToolMode(ECityFormToolMode::Road);
	}
}

void UCityFormToolPaletteWidget::HandleResidentialClicked()
{
}

void UCityFormToolPaletteWidget::HandleCommercialClicked()
{
}
