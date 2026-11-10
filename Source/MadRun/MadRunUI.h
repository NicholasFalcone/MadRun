// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MadRunUI.generated.h"

class UTextBlock;
class UVerticalBox;
class AMadRunPawn;

/**
 *  Simple Vehicle HUD class
 *  Displays the current speed, gear, fuel and part health.
 *  Widget setup is handled in a Blueprint subclass.
 */
UCLASS(abstract)
class UMadRunUI : public UUserWidget
{
	GENERATED_BODY()
	
protected:

	/** Controls the display of speed in Km/h or MPH */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vehicle")
	bool bIsMPH = false;

public:

	virtual void NativeOnInitialized() override;

	/** Called to update the speed display */
	void UpdateSpeed(float NewSpeed);

	/** Called to update the gear display */
	void UpdateGear(int32 NewGear);

	/** Updates or clears the status of the controlled vehicle. */
	void UpdateVehicleStatus(const AMadRunPawn* Vehicle);

	/** Updates the race progress message. */
	void UpdateRaceStatus(const FText& Status);

protected:

	/** Implemented in Blueprint to display the new speed */
	UFUNCTION(BlueprintImplementableEvent, Category="Vehicle")
	void OnSpeedUpdate(float NewSpeed);

	/** Implemented in Blueprint to display the new gear */
	UFUNCTION(BlueprintImplementableEvent, Category="Vehicle")
	void OnGearUpdate(int32 NewGear);

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FuelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TankHealthText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RaceStatusText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> WheelHealthBox;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> WheelHealthTexts;
};
