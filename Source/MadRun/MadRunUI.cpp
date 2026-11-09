// Copyright Epic Games, Inc. All Rights Reserved.


#include "MadRunUI.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "MadRun.h"

#define LOCTEXT_NAMESPACE "MadRunVehicleUI"

void UMadRunUI::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		UE_LOG(LogMadRun, Error, TEXT("Vehicle UI '%s' has no root widget for the fuel display."), *GetNameSafe(this));
		return;
	}

	UWidget* ExistingRoot = WidgetTree->RootWidget;
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("VehicleHUDOverlay"));
	WidgetTree->RootWidget = Overlay;
	Overlay->AddChildToOverlay(ExistingRoot);

	UBorder* FuelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FuelBackground"));
	FuelBackground->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
	FuelBackground->SetPadding(FMargin(12.0f, 6.0f));
	UOverlaySlot* FuelSlot = Overlay->AddChildToOverlay(FuelBackground);
	FuelSlot->SetHorizontalAlignment(HAlign_Left);
	FuelSlot->SetVerticalAlignment(VAlign_Top);
	FuelSlot->SetPadding(FMargin(24.0f));

	FuelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FuelText"));
	FuelBackground->AddChild(FuelText);
	FuelBackground->SetVisibility(ESlateVisibility::HitTestInvisible);
	ClearFuel();
}

void UMadRunUI::UpdateFuel(float Fuel, float Capacity)
{
	if (FuelText)
	{
		FNumberFormattingOptions Options;
		Options.SetMinimumFractionalDigits(1);
		Options.SetMaximumFractionalDigits(1);
		FuelText->SetText(FText::Format(LOCTEXT("FuelFormat", "Fuel: {0} / {1} L"),
			FText::AsNumber(Fuel, &Options), FText::AsNumber(Capacity, &Options)));
	}
}

void UMadRunUI::ClearFuel()
{
	if (FuelText)
	{
		FuelText->SetText(LOCTEXT("FuelUnavailable", "Fuel: --"));
	}
}

void UMadRunUI::UpdateSpeed(float NewSpeed)
{
	// format the speed to KPH or MPH
	float FormattedSpeed = FMath::Abs(NewSpeed) * (bIsMPH ? 0.022f : 0.036f);

	// call the Blueprint handler
	OnSpeedUpdate(FormattedSpeed);
}

void UMadRunUI::UpdateGear(int32 NewGear)
{
	// call the Blueprint handler
	OnGearUpdate(NewGear);
}

#undef LOCTEXT_NAMESPACE