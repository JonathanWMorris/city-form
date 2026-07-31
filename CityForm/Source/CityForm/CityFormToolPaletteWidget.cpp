// Copyright Jonathan Morris. All Rights Reserved.

#include "CityFormToolPaletteWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "CityFormPlayerController.h"

namespace
{
constexpr float DockWidth = 720.0f;
constexpr float ClosedDockHeight = 118.0f;
constexpr float OpenDockHeight = 190.0f;
const FLinearColor DockColor(0.025f, 0.035f, 0.05f, 0.94f);
const FLinearColor TrayColor(0.06f, 0.075f, 0.095f, 0.98f);
const FLinearColor ActiveColor(0.12f, 0.58f, 0.34f, 1.0f);
const FLinearColor InactiveColor(0.42f, 0.45f, 0.49f, 1.0f);

void SetFontSize(UTextBlock* Text, const int32 Size)
{
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = Size;
	Text->SetFont(Font);
}

UWidget* MakeRoadSymbol(UWidgetTree* WidgetTree)
{
	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>();
	IconSize->SetWidthOverride(34.0f);
	IconSize->SetHeightOverride(20.0f);
	UBorder* Asphalt = WidgetTree->ConstructWidget<UBorder>();
	Asphalt->SetBrushColor(FLinearColor(0.06f, 0.07f, 0.08f, 1.0f));
	Asphalt->SetPadding(FMargin(3.0f, 8.0f));
	IconSize->AddChild(Asphalt);
	UBorder* CenterLine = WidgetTree->ConstructWidget<UBorder>();
	CenterLine->SetBrushColor(FLinearColor(0.92f, 0.78f, 0.22f, 1.0f));
	Asphalt->AddChild(CenterLine);
	return IconSize;
}

UButton* AddRoadButton(
	UWidgetTree* WidgetTree,
	UHorizontalBox* Row,
	const TCHAR* ButtonName,
	const TCHAR* Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
	Button->SetBackgroundColor(InactiveColor);
	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>();
	Button->AddChild(Content);
	UHorizontalBoxSlot* IconSlot = Content->AddChildToHorizontalBox(MakeRoadSymbol(WidgetTree));
	IconSlot->SetPadding(FMargin(10.0f, 9.0f, 8.0f, 9.0f));
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	SetFontSize(Text, 18);
	UHorizontalBoxSlot* TextSlot = Content->AddChildToHorizontalBox(Text);
	TextSlot->SetVerticalAlignment(VAlign_Center);
	TextSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
	Row->AddChildToHorizontalBox(Button);
	return Button;
}
}

void UCityFormToolPaletteWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	DockSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DockSize"));
	DockSize->SetWidthOverride(DockWidth);
	DockSize->SetHeightOverride(ClosedDockHeight);
	WidgetTree->RootWidget = DockSize;

	PaletteBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PaletteBorder"));
	PaletteBorder->SetPadding(FMargin(10.0f));
	PaletteBorder->SetBrushColor(DockColor);
	DockSize->AddChild(PaletteBorder);

	UVerticalBox* DockColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	PaletteBorder->AddChild(DockColumn);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetAutoWrapText(true);
	StatusText->SetWrapTextAt(DockWidth - 32.0f);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	SetFontSize(StatusText, 16);
	UVerticalBoxSlot* StatusSlot = DockColumn->AddChildToVerticalBox(StatusText);
	StatusSlot->SetPadding(FMargin(4.0f, 2.0f, 4.0f, 8.0f));

	RoadTray = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RoadTray"));
	RoadTray->SetBrushColor(TrayColor);
	RoadTray->SetPadding(FMargin(8.0f));
	UHorizontalBox* RoadToolRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	RoadTray->AddChild(RoadToolRow);
	BasicRoadButton = AddRoadButton(
		WidgetTree,
		RoadToolRow,
		TEXT("BasicRoadButton"),
		TEXT("Basic Two-Way Road"));
	BasicRoadButton->OnClicked.AddDynamic(this, &UCityFormToolPaletteWidget::HandleBasicRoadClicked);
	UVerticalBoxSlot* TraySlot = DockColumn->AddChildToVerticalBox(RoadTray);
	TraySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	UHorizontalBox* CategoryRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	DockColumn->AddChildToVerticalBox(CategoryRow);
	RoadCategoryButton = AddRoadButton(
		WidgetTree,
		CategoryRow,
		TEXT("RoadCategoryButton"),
		TEXT("Roads"));
	RoadCategoryButton->OnClicked.AddDynamic(this, &UCityFormToolPaletteWidget::HandleRoadCategoryClicked);

	SetRoadCategoryOpen(false);
	SetSelectedTool(ECityFormToolMode::None);
	SetStatus(TEXT("Choose Roads to open the road-building tools."));
}

void UCityFormToolPaletteWidget::InitializeForController(ACityFormPlayerController* InController)
{
	Controller = InController;
}

void UCityFormToolPaletteWidget::SetSelectedTool(const ECityFormToolMode ToolMode)
{
	if (BasicRoadButton != nullptr)
	{
		BasicRoadButton->SetBackgroundColor(
			ToolMode == ECityFormToolMode::Road ? ActiveColor : InactiveColor);
	}
}

void UCityFormToolPaletteWidget::SetRoadCategoryOpen(const bool bOpen)
{
	if (DockSize != nullptr)
	{
		DockSize->SetHeightOverride(bOpen ? OpenDockHeight : ClosedDockHeight);
	}
	if (RoadTray != nullptr)
	{
		RoadTray->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (RoadCategoryButton != nullptr)
	{
		RoadCategoryButton->SetBackgroundColor(bOpen ? ActiveColor : InactiveColor);
	}
}

void UCityFormToolPaletteWidget::SetStatus(const FString& Message, const bool bIsError)
{
	if (StatusText != nullptr)
	{
		StatusText->SetText(FText::FromString(Message));
		StatusText->SetColorAndOpacity(
			bIsError
				? FSlateColor(FLinearColor(1.0f, 0.3f, 0.25f))
				: FSlateColor(FLinearColor::White));
	}
}

bool UCityFormToolPaletteWidget::IsPointerOverPalette() const
{
	return PaletteBorder != nullptr && PaletteBorder->IsHovered();
}

void UCityFormToolPaletteWidget::HandleRoadCategoryClicked()
{
	if (Controller != nullptr)
	{
		Controller->ToggleRoadCategory();
	}
}

void UCityFormToolPaletteWidget::HandleBasicRoadClicked()
{
	if (Controller != nullptr)
	{
		Controller->SetToolMode(ECityFormToolMode::Road);
	}
}
