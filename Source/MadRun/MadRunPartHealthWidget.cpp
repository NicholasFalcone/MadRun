#include "MadRunPartHealthWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "MadRun.h"

#define LOCTEXT_NAMESPACE "MadRunPartHealthWidget"

bool UMadRunPartHealthWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (!WidgetTree)
	{
		UE_LOG(LogMadRun, Error, TEXT("Part health widget has no widget tree."));
		return false;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));
	Background->SetPadding(FMargin(6.0f));
	WidgetTree->RootWidget = Background;

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
	Background->AddChild(Content);
	Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthLabel"));
	Content->AddChildToVerticalBox(Label);
	Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("HealthSlider"));
	Slider->SetLocked(true);
	Slider->SetSliderBarColor(FLinearColor(0.2f, 0.85f, 0.2f));
	Slider->SetVisibility(ESlateVisibility::HitTestInvisible);
	UVerticalBoxSlot* BarSlot = Content->AddChildToVerticalBox(Slider);
	BarSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
	return true;
}

void UMadRunPartHealthWidget::UpdateHealth(const FText& PartName, int32 Health, int32 MaxHealth)
{
	if (!Label || !Slider)
	{
		UE_LOG(LogMadRun, Error, TEXT("Part health widget '%s' was not initialized."), *GetNameSafe(this));
		return;
	}
	Label->SetText(FText::Format(LOCTEXT("HealthFormat", "{0}: {1} / {2}"),
		PartName, FText::AsNumber(Health), FText::AsNumber(MaxHealth)));
	Slider->SetValue(MaxHealth > 0 ? FMath::Clamp(static_cast<float>(Health) / MaxHealth, 0.0f, 1.0f) : 0.0f);
}

#undef LOCTEXT_NAMESPACE
