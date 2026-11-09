#include "MadRunWheelBase.h"
#include "MadRun.h"

void UMadRunWheelBase::Init(UChaosWheeledVehicleMovementComponent* InVehicleSim, int32 InWheelIndex)
{
	Super::Init(InVehicleSim, InWheelIndex);
	PartHealth.Initialize();
}

int32 UMadRunWheelBase::GetHealth_Implementation() const { return PartHealth.Health; }
int32 UMadRunWheelBase::GetMaxHealth_Implementation() const { return FMath::Max(PartHealth.MaxHealth, 1); }

void UMadRunWheelBase::SetHealth_Implementation(int32 NewHealth)
{
	const int32 OldHealth = PartHealth.Health;
	PartHealth.Set(NewHealth);
	if (OldHealth > 0 && PartHealth.Health == 0)
	{
		OnOutofWork.Broadcast();
	}
	else if (OldHealth == 0 && PartHealth.Health > 0)
	{
		OnRepaired.Broadcast();
	}
}

void UMadRunWheelBase::TakeDamage_Implementation(int32 DamageAmount)
{
	if (DamageAmount < 0)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Wheel damage cannot be negative."));
		return;
	}
	SetHealth_Implementation(PartHealth.Health - FMath::Min(DamageAmount, PartHealth.Health));
}

void UMadRunWheelBase::DoRepair_Implementation()
{
	SetHealth_Implementation(GetMaxHealth_Implementation());
}
