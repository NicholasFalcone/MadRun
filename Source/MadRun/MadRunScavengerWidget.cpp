#include "MadRunScavengerWidget.h"
#include "MadRun.h"
#include "MadRunEquipment.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "MadRunScavenger"

bool UMadRunScavengerWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (!WidgetTree)
	{
		UE_LOG(LogMadRun, Error, TEXT("Scavenger widget has no widget tree."));
		return false;
	}
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	Panel->SetPadding(FMargin(20.0f));
	UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel);
	PanelSlot->SetHorizontalAlignment(HAlign_Center);
	PanelSlot->SetVerticalAlignment(VAlign_Center);
	Message = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Message"));
	Message->SetWrapTextAt(500.0f);
	Panel->AddChild(Message);
	SetVisibility(ESlateVisibility::Collapsed);
	return true;
}

void UMadRunScavengerWidget::ShowHint(const FText& KeyName)
{
	Message->SetText(FText::Format(LOCTEXT("Hint", "Press {0} to search for equipment"), KeyName));
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMadRunScavengerWidget::ShowOffer(const UMadRunEquipment* Equipment, const FText& AcceptKey, const FText& RejectKey)
{
	if (!IsValid(Equipment))
	{
		UE_LOG(LogMadRun, Error, TEXT("Cannot show an invalid scavenger offer."));
		return;
	}
	Message->SetText(FText::Format(LOCTEXT("Offer", "Found: {0}\n{1}\n\nEquip this item?\n{2}: Equip    {3}: Cancel"),
		Equipment->Name, Equipment->Description, AcceptKey, RejectKey));
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMadRunScavengerWidget::ShowFailure(const FText& RejectKey)
{
	Message->SetText(FText::Format(LOCTEXT("Failure", "Cannot equip this item. Check its configuration.\n{0}: Close"), RejectKey));
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMadRunScavengerWidget::Clear()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE
