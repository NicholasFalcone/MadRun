#include "MadRunFuelTank.h"
#include "MadRunPawn.h"
#include "MadRun.h"
#include "MadRunCollision.h"
#include "MadRunPartHealthDisplay.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

#define LOCTEXT_NAMESPACE "MadRunFuelTank"

AMadRunFuelTank::AMadRunFuelTank()
{
	PrimaryActorTick.bCanEverTick = false;
	TankMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tank Mesh"));
	SetRootComponent(TankMesh);
	TankMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Hitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
	Hitbox->SetupAttachment(TankMesh);
	Hitbox->InitBoxExtent(FVector(20.0f));
	Hitbox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Hitbox->SetCollisionObjectType(ECC_WorldDynamic);
	Hitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	Hitbox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Hitbox->SetCollisionResponseToChannel(MadRunProjectileChannel, ECR_Block);

	HealthDisplay = CreateDefaultSubobject<UMadRunPartHealthDisplay>(TEXT("Health Display"));
	HealthDisplay->SetupAttachment(TankMesh);
	HealthDisplay->SetHeightAbovePart(80.0f);
	HealthDisplay->SetPartName(LOCTEXT("TankName", "Fuel tank"));
}

void AMadRunFuelTank::BeginPlay()
{
	Super::BeginPlay();
	PartHealth.Initialize();
	Fuel = FMath::Max(Capacity, 0.0f);
}

float AMadRunFuelTank::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (GetNetMode() == NM_Client)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Fuel tank damage must be applied on the server."));
		return 0.0f;
	}
	UE_LOG(LogMadRun, Log, TEXT("Fuel tank taking damage: %f"), DamageAmount);
	if (!FMath::IsFinite(DamageAmount) || DamageAmount < 0.0f)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Fuel tank damage must be finite and non-negative."));
		return 0.0f;
	}
	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (AppliedDamage > 0.0f)
	{
		IMadRunPartHealth::Execute_TakeDamage(this, FMath::CeilToInt(FMath::Min(AppliedDamage, static_cast<float>(GetMaxHealth_Implementation()))));
	}
	return AppliedDamage;
}

int32 AMadRunFuelTank::GetHealth_Implementation() const { return PartHealth.Health; }
int32 AMadRunFuelTank::GetMaxHealth_Implementation() const { return FMath::Max(PartHealth.MaxHealth, 1); }

UMadRunFuelTankEquipment* AMadRunFuelTank::GetEquipment() const
{
	const AMadRunPawn* Vehicle = Cast<AMadRunPawn>(GetParentActor());
	return IsValid(Vehicle) && Vehicle->GetFuelTank() == this ? Vehicle->GetFuelTankEquipment() : nullptr;
}

bool AMadRunFuelTank::ApplyEquipmentVisual(const UMadRunFuelTankEquipment* Equipment)
{
	if (!IsValid(Equipment) || !IsValid(Equipment->Mesh) || !IsValid(TankMesh))
	{
		UE_LOG(LogMadRun, Warning, TEXT("Fuel tank '%s' requires a valid equipment mesh."), *GetNameSafe(this));
		return false;
	}
	return TankMesh->GetStaticMesh() == Equipment->Mesh || TankMesh->SetStaticMesh(Equipment->Mesh);
}

void AMadRunFuelTank::SetHealth_Implementation(int32 NewHealth)
{
	if (GetNetMode() == NM_Client)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Fuel tank health must be changed on the server."));
		return;
	}
	SetLocalHealth(NewHealth);
}

void AMadRunFuelTank::SetLocalHealth(int32 NewHealth)
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
	if (OldHealth != PartHealth.Health && HasActorBegunPlay())
	{
		HealthDisplay->ShowHealth(PartHealth.Health, GetMaxHealth_Implementation());
	}
}

void AMadRunFuelTank::TakeDamage_Implementation(int32 DamageAmount)
{
	if (DamageAmount < 0)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Fuel tank damage cannot be negative."));
		return;
	}
	SetHealth_Implementation(PartHealth.Health - FMath::Min(DamageAmount, PartHealth.Health));
}

void AMadRunFuelTank::DoRepair_Implementation()
{
	SetHealth_Implementation(GetMaxHealth_Implementation());
}

void AMadRunFuelTank::AddFuel(float Amount)
{
	if (GetNetMode() == NM_Client)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Fuel must be changed on the server."));
		return;
	}
	if (!FMath::IsFinite(Amount) || Amount < 0.0f)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Fuel amount must be finite and non-negative."));
		return;
	}
	Fuel = FMath::Clamp(Fuel + Amount, 0.0f, FMath::Max(Capacity, 0.0f));
}

void AMadRunFuelTank::ConsumeFuel(float DeltaSeconds, float Throttle)
{
	if (GetNetMode() == NM_Client)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Fuel must be consumed on the server."));
		return;
	}
	if (!FMath::IsFinite(DeltaSeconds) || !FMath::IsFinite(Throttle) || DeltaSeconds < 0.0f)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Fuel consumption requires a non-negative finite delta and finite throttle."));
		return;
	}
	const float DamageFraction = 1.0f - static_cast<float>(PartHealth.Health) / GetMaxHealth_Implementation();
	const float Rate = FMath::Max(FullThrottleConsumptionPerSecond, 0.0f) * FMath::Clamp(Throttle, 0.0f, 1.0f)
		+ FMath::Max(MaxLeakPerSecond, 0.0f) * FMath::Clamp(DamageFraction, 0.0f, 1.0f);
	Fuel = FMath::Max(0.0f, Fuel - Rate * DeltaSeconds);
}

void AMadRunFuelTank::ApplyReplicatedState(float NewFuel, int32 NewHealth)
{
	Fuel = FMath::Clamp(NewFuel, 0.0f, FMath::Max(Capacity, 0.0f));
	SetLocalHealth(NewHealth);
}

#undef LOCTEXT_NAMESPACE
