// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MadRunCopilotStats.h"
#include "MadRunCopilotCombatComponent.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFireStarted, AActor*, Target, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFireCompleted, bool, bHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAmmoCounted);

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	Idle,
	Aiming,
	Firing,
	Reloading
};

UCLASS(ClassGroup = (Copilot), meta = (BlueprintSpawnableComponent))
class MADRUN_API UCopilotCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCopilotCombatComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Initialize the component with copilot stats
	UFUNCTION(BlueprintCallable, Category = "Copilot|Combat")
	void Initialize(const FCopilotStats& InStats);

	// Set the weapon and ammo configuration
	UFUNCTION(BlueprintCallable, Category = "Copilot|Combat")
	void SetWeapon(AActor* InWeapon, int32 MaxAmmo);

	// Start aiming at a target
	UFUNCTION(BlueprintCallable, Category = "Copilot|Combat")
	void AimAtTarget(AActor* InTarget);

	// Fire at the current target
	UFUNCTION(BlueprintCallable, Category = "Copilot|Combat")
	void FireAtTarget();

	// Reload weapon
	UFUNCTION(BlueprintCallable, Category = "Copilot|Combat")
	void Reload(float ReloadTime);

	// Add ammo
	UFUNCTION(BlueprintCallable, Category = "Copilot|Combat")
	void AddAmmo(int32 Amount);

	// Get current combat state
	UFUNCTION(BlueprintCallable, Category = "Copilot|Combat")
	ECombatState GetCombatState() const { return CurrentState; }

	// Get current ammo count
	UFUNCTION(BlueprintCallable, Category = "Copilot|Combat")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	// Get max ammo
	UFUNCTION(BlueprintCallable, Category = "Copilot|Combat")
	int32 GetMaxAmmo() const { return MaxAmmo; }

	// Get reload progress (0.0 - 1.0)
	UFUNCTION(BlueprintCallable, Category = "Copilot|Combat")
	float GetReloadProgress() const;

	// Get the accuracy deviation in degrees based on copilot accuracy skill
	UFUNCTION(BlueprintCallable, Category = "Copilot|Combat")
	float GetCurrentAccuracyDeviation() const;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Copilot|Combat")
	FOnFireStarted OnFireStarted;

	UPROPERTY(BlueprintAssignable, Category = "Copilot|Combat")
	FOnFireCompleted OnFireCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Copilot|Combat")
	FOnAmmoCounted OnAmmoCounted;

private:
	void CompleteReload();

	FCopilotStats CopilotStats;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentWeapon;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentTarget;

	ECombatState CurrentState = ECombatState::Idle;

	int32 CurrentAmmo = 0;
	int32 MaxAmmo = 0;

	float ElapsedReloadTime = 0.0f;
	float TotalReloadTime = 0.0f;
};
