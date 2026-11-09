// Copyright Epic Games, Inc. All Rights Reserved.


#include "MadRunUI.h"
#include "MadRunFuelTank.h"
#include "MadRunPawn.h"
#include "MadRunWheelBase.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "MadRun.h"

#define LOCTEXT_NAMESPACE "MadRunVehicleUI"

void UMadRunUI::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		UE_LOG(LogMadRun, Error, TEXT("Vehicle UI '%s' has no root widget for the status display."), *GetNameSafe(this));
		return;
	}

	UWidget* ExistingRoot = WidgetTree->RootWidget;
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("VehicleHUDOverlay"));
	WidgetTree->RootWidget = Overlay;
	UOverlaySlot* OriginalSlot = Overlay->AddChildToOverlay(ExistingRoot);
	OriginalSlot->SetHorizontalAlignment(HAlign_Right);
	OriginalSlot->SetVerticalAlignment(VAlign_Bottom);
	OriginalSlot->SetPadding(FMargin(24.0f));

	UBorder* FuelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FuelBackground"));
	FuelBackground->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
	FuelBackground->SetPadding(FMargin(12.0f, 6.0f));
	UOverlaySlot* FuelSlot = Overlay->AddChildToOverlay(FuelBackground);
	FuelSlot->SetHorizontalAlignment(HAlign_Left);
	FuelSlot->SetVerticalAlignment(VAlign_Top);
	FuelSlot->SetPadding(FMargin(24.0f));

	UVerticalBox* StatusBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VehicleStatusBox"));
	FuelBackground->AddChild(StatusBox);
	FuelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FuelText"));
	StatusBox->AddChildToVerticalBox(FuelText);
	TankHealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TankHealthText"));
	StatusBox->AddChildToVerticalBox(TankHealthText)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
	WheelHealthBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WheelHealthBox"));
	StatusBox->AddChildToVerticalBox(WheelHealthBox)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
	FuelBackground->SetVisibility(ESlateVisibility::HitTestInvisible);
	UpdateVehicleStatus(nullptr);
}

void UMadRunUI::UpdateVehicleStatus(const AMadRunPawn* Vehicle)
{
	if (!FuelText || !TankHealthText || !WheelHealthBox)
	{
		UE_LOG(LogMadRun, Error, TEXT("Vehicle UI '%s' has no status widgets."), *GetNameSafe(this));
		return;
	}

	const AMadRunFuelTank* Tank = IsValid(Vehicle) && !Vehicle->IsActorBeingDestroyed() ? Vehicle->GetFuelTank() : nullptr;
	if (IsValid(Tank) && !Tank->IsActorBeingDestroyed())
	{
		FNumberFormattingOptions Options;
		Options.SetMinimumFractionalDigits(1);
		Options.SetMaximumFractionalDigits(1);
		FuelText->SetText(FText::Format(LOCTEXT("FuelFormat", "Fuel: {0} / {1} L"),
			FText::AsNumber(Tank->GetFuel(), &Options), FText::AsNumber(Tank->GetCapacity(), &Options)));
		TankHealthText->SetText(FText::Format(LOCTEXT("TankHealthFormat", "Fuel tank: {0} / {1}"),
			FText::AsNumber(IMadRunPartHealth::Execute_GetHealth(Tank)),
			FText::AsNumber(IMadRunPartHealth::Execute_GetMaxHealth(Tank))));
	}
	else
	{
		FuelText->SetText(LOCTEXT("FuelUnavailable", "Fuel: --"));
		TankHealthText->SetText(LOCTEXT("TankHealthUnavailable", "Fuel tank: --"));
	}

	const UChaosWheeledVehicleMovementComponent* Movement = IsValid(Vehicle) && !Vehicle->IsActorBeingDestroyed()
		? Vehicle->GetChaosVehicleMovement().Get() : nullptr;
	const int32 WheelCount = Movement ? Movement->WheelSetups.Num() : 0;
	while (WheelHealthTexts.Num() < WheelCount)
	{
		const FName Name(*FString::Printf(TEXT("WheelHealthText_%d"), WheelHealthTexts.Num()));
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		WheelHealthBox->AddChildToVerticalBox(Text)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
		WheelHealthTexts.Add(Text);
	}
	for (int32 Index = 0; Index < WheelHealthTexts.Num(); ++Index)
	{
		WheelHealthTexts[Index]->SetVisibility(Index < WheelCount ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Index >= WheelCount)
		{
			continue;
		}
		const UMadRunWheelBase* Wheel = Movement->Wheels.IsValidIndex(Index) ? Cast<UMadRunWheelBase>(Movement->Wheels[Index]) : nullptr;
		WheelHealthTexts[Index]->SetText(IsValid(Wheel)
			? FText::Format(LOCTEXT("WheelHealthFormat", "Wheel {0}: {1} / {2}"), FText::AsNumber(Index),
				FText::AsNumber(IMadRunPartHealth::Execute_GetHealth(Wheel)),
				FText::AsNumber(IMadRunPartHealth::Execute_GetMaxHealth(Wheel)))
			: FText::Format(LOCTEXT("WheelHealthUnavailable", "Wheel {0}: --"), FText::AsNumber(Index)));
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