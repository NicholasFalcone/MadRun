#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MadRunTurret.generated.h"

class AMadRunPawn;
class AMadRunProjectile;
class USceneComponent;
class UStaticMeshComponent;
class UMadRunWeaponEquipment;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurretFired);

UCLASS(Blueprintable)
class AMadRunTurret : public AActor
{
	GENERATED_BODY()

public:
	AMadRunTurret();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="Turret")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category="Turret")
	void StopFire();

	UFUNCTION(BlueprintPure, Category="Turret")
	AActor* GetTarget() const { return Target.Get(); }

	UFUNCTION(BlueprintPure, Category="Equipment")
	UMadRunWeaponEquipment* GetEquipment() const;

	bool ApplyEquipment(const UMadRunWeaponEquipment* Equipment);

	UFUNCTION(BlueprintPure, Category="Turret|Weapon")
	int32 GetAmmoInMagazine() const { return AmmoInMagazine; }

	UFUNCTION(BlueprintPure, Category="Turret|Weapon")
	int32 GetMagazineSize() const { return MagazineSize; }

	UFUNCTION(BlueprintPure, Category="Turret|Weapon")
	bool IsReloading() const { return bReloading; }

private:
	AActor* FindTarget() const;
	bool Fire();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> Base;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> Barrel;

	UPROPERTY(EditDefaultsOnly, Category="Turret", meta=(ClampMin="0.0", Units="cm"))
	float TargetRange = 10000.0f;

	UPROPERTY(EditDefaultsOnly, Category="Turret", meta=(ClampMin="0.01", Units="s"))
	float FireInterval = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category="Turret|Weapon", meta=(ClampMin="0.0"))
	float Damage = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category="Turret|Weapon", meta=(ClampMin="1"))
	int32 MagazineSize = 30;

	UPROPERTY(EditDefaultsOnly, Category="Turret|Weapon", meta=(ClampMin="0.0", Units="s"))
	float ReloadTime = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category="Turret")
	TSubclassOf<AMadRunProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category="Turret", meta=(ClampMin="0.0", Units="cm"))
	float MuzzleDistance = 100.0f;

	TWeakObjectPtr<AActor> Target;
	bool bFiring = false;
	int32 AmmoInMagazine = 0;
	bool bReloading = false;
	double ReloadEndTime = 0.0;
	double NextFireTime = 0.0;

public:
	UPROPERTY(BlueprintAssignable, Category="Turret")
	FOnTurretFired OnTurretFired;
};
