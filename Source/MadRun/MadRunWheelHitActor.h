#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MadRunWheelHitActor.generated.h"

class AMadRunPawn;
class USphereComponent;
class UMadRunWheelBase;

UCLASS(Blueprintable)
class AMadRunWheelHitActor : public AActor
{
	GENERATED_BODY()

public:
	AMadRunWheelHitActor();

	void InitializeWheel(AMadRunPawn* InVehicle, int32 InWheelIndex);

	UFUNCTION(BlueprintPure, Category="Wheel")
	UMadRunWheelBase* GetWheel() const;

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> Hitbox;

	UPROPERTY(Transient)
	TObjectPtr<AMadRunPawn> Vehicle;

	int32 WheelIndex = INDEX_NONE;
};
