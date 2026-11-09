// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChaosVehicleWheel.h"
#include "MadRunPartHealth.h"
#include "MadRunWheelBase.generated.h"

class UMadRunTiresEquipment;

UCLASS()
class UMadRunWheelBase : public UChaosVehicleWheel, public IMadRunPartHealth
{
	GENERATED_BODY()

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health", meta=(AllowPrivateAccess="true", ShowOnlyInnerProperties))
	FMadRunPartHealthState PartHealth;

	void SetWheelOperational(bool bOperational);

public:
	virtual void Init(UChaosWheeledVehicleMovementComponent* InVehicleSim, int32 InWheelIndex) override;
	virtual int32 GetHealth_Implementation() const override;
	virtual int32 GetMaxHealth_Implementation() const override;
	virtual void SetHealth_Implementation(int32 NewHealth) override;
	virtual void DoRepair_Implementation() override;
	virtual void TakeDamage_Implementation(int32 DamageAmount) override;

	UFUNCTION(BlueprintPure, Category="Equipment")
	UMadRunTiresEquipment* GetEquipment() const;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnOutofWork OnOutofWork;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnRepaired OnRepaired;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnPartHealthChanged OnHealthChanged;
};
