#include "MadRunWheelBase.h"
#include "MadRun.h"
#include "ChaosWheeledVehicleMovementComponent.h"

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
		SetWheelOperational(false);
		OnOutofWork.Broadcast();
	}
	else if (OldHealth == 0 && PartHealth.Health > 0)
	{
		SetWheelOperational(true);
		OnRepaired.Broadcast();
	}
	if (OldHealth != PartHealth.Health)
	{
		OnHealthChanged.Broadcast(PartHealth.Health, GetMaxHealth_Implementation());
	}
}

void UMadRunWheelBase::SetWheelOperational(bool bOperational)
{
	if (!IsValid(VehicleComponent))
	{
		return;
	}
	if (!VehicleComponent->Wheels.IsValidIndex(WheelIndex) || VehicleComponent->Wheels[WheelIndex] != this)
	{
		UE_LOG(LogMadRun, Error, TEXT("Cannot update wheel '%s': invalid vehicle wheel index %d."), *GetNameSafe(this), WheelIndex);
		return;
	}

	bool bEngineEnabled = bAffectedByEngine;
	const EVehicleDifferential Differential = VehicleComponent->DifferentialSetup.DifferentialType;
	if (AxleType != EAxleType::Undefined)
	{
		bEngineEnabled = Differential == EVehicleDifferential::AllWheelDrive
			|| (AxleType == EAxleType::Front && Differential == EVehicleDifferential::FrontWheelDrive)
			|| (AxleType == EAxleType::Rear && Differential == EVehicleDifferential::RearWheelDrive);
	}

	VehicleComponent->SetAffectedByEngine(WheelIndex, bOperational && bEngineEnabled);
	VehicleComponent->SetAffectedBySteering(WheelIndex, bOperational && bAffectedBySteering);
	VehicleComponent->SetAffectedByBrake(WheelIndex, bOperational && bAffectedByBrake);
	VehicleComponent->SetAffectedByHandbrake(WheelIndex, bOperational && bAffectedByHandbrake);

	if (!bOperational)
	{
		// Chaos can apply engine and parking torque despite the affected-by flags.
		VehicleComponent->SetDriveTorque(0.0f, WheelIndex);
		VehicleComponent->SetBrakeTorque(0.0f, WheelIndex);
		VehicleComponent->SetTorqueCombineMethod(ETorqueCombineMethod::Override, WheelIndex);
	}
	else
	{
		VehicleComponent->SetTorqueCombineMethod(ExternalTorqueCombineMethod, WheelIndex);
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
