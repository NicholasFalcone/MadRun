#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MadRunPartHealth.h"
#include "MadRunFuelTank.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMadRunPartHealthDisplay;
class UMadRunFuelTankEquipment;

UCLASS(Blueprintable)
class AMadRunFuelTank : public AActor, public IMadRunPartHealth
{
	GENERATED_BODY()

public:
	AMadRunFuelTank();
	virtual void BeginPlay() override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	virtual int32 GetHealth_Implementation() const override;
	virtual int32 GetMaxHealth_Implementation() const override;
	virtual void SetHealth_Implementation(int32 NewHealth) override;
	virtual void TakeDamage_Implementation(int32 DamageAmount) override;
	virtual void DoRepair_Implementation() override;

	UFUNCTION(BlueprintPure, Category="Fuel")
	float GetFuel() const { return Fuel; }

	UFUNCTION(BlueprintPure, Category="Fuel")
	float GetCapacity() const { return Capacity; }

	UFUNCTION(BlueprintPure, Category="Equipment")
	UMadRunFuelTankEquipment* GetEquipment() const;

	bool ApplyEquipmentVisual(const UMadRunFuelTankEquipment* Equipment);

	UFUNCTION(BlueprintCallable, Category="Fuel")
	void AddFuel(float Amount);

	/** Advances consumption by DeltaSeconds, with throttle in the range [0, 1]. */
	void ConsumeFuel(float DeltaSeconds, float Throttle);

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnOutofWork OnOutofWork;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnRepaired OnRepaired;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMadRunPartHealthDisplay> HealthDisplay;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> TankMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> Hitbox;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Health", meta=(AllowPrivateAccess="true", ShowOnlyInnerProperties))
	FMadRunPartHealthState PartHealth;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fuel", meta=(AllowPrivateAccess="true", ClampMin="0.0"))
	float Capacity = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Fuel", meta=(AllowPrivateAccess="true"))
	float Fuel = 0.0f;

	/** Fuel consumed per second at full throttle. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fuel", meta=(AllowPrivateAccess="true", ClampMin="0.0"))
	float FullThrottleConsumptionPerSecond = 0.2f;

	/** Fuel lost per second at zero health; scales linearly with missing health. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fuel", meta=(AllowPrivateAccess="true", ClampMin="0.0"))
	float MaxLeakPerSecond = 0.1f;
};
