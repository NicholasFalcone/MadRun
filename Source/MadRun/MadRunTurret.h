#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MadRunTurret.generated.h"

class AMadRunPawn;
class AMadRunProjectile;
class USceneComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurretFired);

UCLASS(Blueprintable)
class AMadRunTurret : public AActor
{
	GENERATED_BODY()

public:
	AMadRunTurret();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="Turret")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category="Turret")
	void StopFire();

	UFUNCTION(BlueprintPure, Category="Turret")
	AMadRunPawn* GetTarget() const { return Target.Get(); }

private:
	AMadRunPawn* FindTarget() const;
	void Fire();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> Base;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> Barrel;

	UPROPERTY(EditDefaultsOnly, Category="Turret", meta=(ClampMin="0.0", Units="cm"))
	float TargetRange = 10000.0f;

	UPROPERTY(EditDefaultsOnly, Category="Turret", meta=(ClampMin="0.01", Units="s"))
	float FireInterval = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category="Turret")
	TSubclassOf<AMadRunProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category="Turret", meta=(ClampMin="0.0", Units="cm"))
	float MuzzleDistance = 100.0f;

	TWeakObjectPtr<AMadRunPawn> Target;
	bool bFiring = false;
	double NextFireTime = 0.0;

public:
	UPROPERTY(BlueprintAssignable, Category="Turret")
	FOnTurretFired OnTurretFired;
};
