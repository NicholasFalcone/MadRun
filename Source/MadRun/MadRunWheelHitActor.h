#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MadRunWheelHitActor.generated.h"

class AMadRunPawn;
class USphereComponent;
class UMadRunWheelBase;
class UMadRunPartHealthDisplay;

UCLASS(Blueprintable)
class AMadRunWheelHitActor : public AActor
{
	GENERATED_BODY()

public:
	AMadRunWheelHitActor();

	void InitializeWheel(AMadRunPawn* InVehicle, int32 InWheelIndex);
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category="Wheel")
	UMadRunWheelBase* GetWheel() const;

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

private:
	UFUNCTION()
	void HandleHealthChanged(int32 Health, int32 MaxHealth);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMadRunPartHealthDisplay> HealthDisplay;

	UPROPERTY(Transient)
	TObjectPtr<UMadRunWheelBase> ObservedWheel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> Hitbox;

	UPROPERTY(Transient)
	TObjectPtr<AMadRunPawn> Vehicle;

	int32 WheelIndex = INDEX_NONE;
};
