// Copyright Epic Games, Inc. All Rights Reserved.

#include "MadRunCopilotCombatComponent.h"
#include "MadRunCopilotStats.h"

UCopilotCombatComponent::UCopilotCombatComponent()
{
	PrimaryComponentTick.TickInterval = 0.05f;
	PrimaryComponentTick.bCanEverTick = true;
}

void UCopilotCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// Disable tick initially
	SetComponentTickEnabled(false);
}

void UCopilotCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Handle reload timing
	if (CurrentState == ECombatState::Reloading)
	{
		ElapsedReloadTime += DeltaTime;

		if (ElapsedReloadTime >= TotalReloadTime)
		{
			CompleteReload();
		}
	}
}

void UCopilotCombatComponent::Initialize(const FCopilotStats& InStats)
{
	CopilotStats = InStats;
}

void UCopilotCombatComponent::SetWeapon(AActor* InWeapon, int32 InMaxAmmo)
{
	CurrentWeapon = InWeapon;
	MaxAmmo = InMaxAmmo;
	CurrentAmmo = InMaxAmmo;

	UE_LOG(LogTemp, Log, TEXT("UCopilotCombatComponent::SetWeapon - Weapon: %s, Max Ammo: %d"),
		InWeapon ? *InWeapon->GetName() : TEXT("None"), InMaxAmmo);
}

void UCopilotCombatComponent::AimAtTarget(AActor* InTarget)
{
	if (!InTarget)
	{
		CurrentState = ECombatState::Idle;
		CurrentTarget = nullptr;
		return;
	}

	CurrentTarget = InTarget;
	CurrentState = ECombatState::Aiming;

	UE_LOG(LogTemp, Log, TEXT("UCopilotCombatComponent::AimAtTarget - Target: %s, Accuracy Deviation: %.2f degrees"),
		*InTarget->GetName(), GetCurrentAccuracyDeviation());
}

void UCopilotCombatComponent::FireAtTarget()
{
	if (!CurrentWeapon || !CurrentTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCopilotCombatComponent::FireAtTarget - Missing Weapon or Target"));
		return;
	}

	if (CurrentAmmo <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCopilotCombatComponent::FireAtTarget - No ammo available"));
		return;
	}

	CurrentState = ECombatState::Firing;
	CurrentAmmo--;

	const float BaseAccuracyDeviation = GetCurrentAccuracyDeviation();
	const float DamageBonus = (CopilotStats.AccuracySkill / 100.0f) * 0.3f;  // Up to 30% more damage with high accuracy
	const float BaseDamage = 25.0f;
	const float TotalDamage = BaseDamage * (1.0f + DamageBonus);

	// Determine hit based on accuracy deviation
	bool bHit = (BaseAccuracyDeviation < 5.0f) || (FMath::Rand() % 100 < (100 - (BaseAccuracyDeviation * 10)));

	OnFireStarted.Broadcast(CurrentTarget, TotalDamage);
	OnFireCompleted.Broadcast(bHit);

	UE_LOG(LogTemp, Log,
		TEXT("UCopilotCombatComponent::FireAtTarget - Target: %s, Damage: %.2f, Hit: %d, Ammo: %d/%d"),
		*CurrentTarget->GetName(), TotalDamage, bHit, CurrentAmmo, MaxAmmo);

	CurrentState = ECombatState::Idle;
}

void UCopilotCombatComponent::Reload(float ReloadTime)
{
	if (CurrentAmmo >= MaxAmmo)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCopilotCombatComponent::Reload - Already fully loaded"));
		return;
	}

	CurrentState = ECombatState::Reloading;
	TotalReloadTime = ReloadTime;
	ElapsedReloadTime = 0.0f;

	SetComponentTickEnabled(true);

	UE_LOG(LogTemp, Log, TEXT("UCopilotCombatComponent::Reload - Started, duration: %.2f seconds"), ReloadTime);
}

void UCopilotCombatComponent::AddAmmo(int32 Amount)
{
	CurrentAmmo = FMath::Min(CurrentAmmo + Amount, MaxAmmo);
	OnAmmoCounted.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("UCopilotCombatComponent::AddAmmo - Added %d ammo, now have %d/%d"),
		Amount, CurrentAmmo, MaxAmmo);
}

float UCopilotCombatComponent::GetReloadProgress() const
{
	if (TotalReloadTime <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(ElapsedReloadTime / TotalReloadTime, 0.0f, 1.0f);
}

float UCopilotCombatComponent::GetCurrentAccuracyDeviation() const
{
	return CopilotStats.GetAccuracyDeviation();
}

void UCopilotCombatComponent::CompleteReload()
{
	CurrentAmmo = MaxAmmo;
	CurrentState = ECombatState::Idle;
	SetComponentTickEnabled(false);

	UE_LOG(LogTemp, Log, TEXT("UCopilotCombatComponent::CompleteReload - Reload complete, ammo: %d/%d"),
		CurrentAmmo, MaxAmmo);
}
