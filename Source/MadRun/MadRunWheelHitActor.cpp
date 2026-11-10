#include "MadRunWheelHitActor.h"
#include "MadRun.h"
#include "MadRunCollision.h"
#include "MadRunPawn.h"
#include "MadRunWheelBase.h"
#include "MadRunPartHealthDisplay.h"
#include "Components/SphereComponent.h"

#define LOCTEXT_NAMESPACE "MadRunWheelHitActor"

AMadRunWheelHitActor::AMadRunWheelHitActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Hitbox = CreateDefaultSubobject<USphereComponent>(TEXT("Hitbox"));
	SetRootComponent(Hitbox);
	Hitbox->InitSphereRadius(50.0f);
	Hitbox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Hitbox->SetCollisionObjectType(ECC_WorldDynamic);
	Hitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	Hitbox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Hitbox->SetCollisionResponseToChannel(MadRunProjectileChannel, ECR_Block);

	HealthDisplay = CreateDefaultSubobject<UMadRunPartHealthDisplay>(TEXT("Health Display"));
	HealthDisplay->SetupAttachment(Hitbox);
	HealthDisplay->SetPartName(LOCTEXT("WheelName", "Wheel"));
}

void AMadRunWheelHitActor::InitializeWheel(AMadRunPawn* InVehicle, int32 InWheelIndex)
{
	if (IsValid(ObservedWheel))
	{
		ObservedWheel->OnHealthChanged.RemoveDynamic(this, &AMadRunWheelHitActor::HandleHealthChanged);
	}
	Vehicle = InVehicle;
	WheelIndex = InWheelIndex;
	ObservedWheel = GetWheel();
	if (IsValid(ObservedWheel))
	{
		ObservedWheel->OnHealthChanged.AddUniqueDynamic(this, &AMadRunWheelHitActor::HandleHealthChanged);
	}
	else
	{
		UE_LOG(LogMadRun, Error, TEXT("Cannot display health for wheel %d on '%s': wheel is unavailable."), WheelIndex, *GetNameSafe(Vehicle));
	}
}

void AMadRunWheelHitActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(ObservedWheel))
	{
		ObservedWheel->OnHealthChanged.RemoveDynamic(this, &AMadRunWheelHitActor::HandleHealthChanged);
	}
	ObservedWheel = nullptr;
	Super::EndPlay(EndPlayReason);
}

void AMadRunWheelHitActor::HandleHealthChanged(int32 Health, int32 MaxHealth)
{
	HealthDisplay->ShowHealth(Health, MaxHealth);
}

UMadRunWheelBase* AMadRunWheelHitActor::GetWheel() const
{
	return IsValid(Vehicle) ? Vehicle->GetWheelByIndex(WheelIndex) : nullptr;
}

float AMadRunWheelHitActor::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (GetNetMode() == NM_Client)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Wheel damage must be applied on the server."));
		return 0.0f;
	}
	if (!FMath::IsFinite(DamageAmount) || DamageAmount < 0.0f)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Wheel hit damage must be finite and non-negative."));
		return 0.0f;
	}

	UMadRunWheelBase* Wheel = GetWheel();
	if (!Wheel)
	{
		UE_LOG(LogMadRun, Error, TEXT("Wheel hit actor '%s' has no valid wheel."), *GetNameSafe(this));
		return 0.0f;
	}

	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (AppliedDamage > 0.0f)
	{
		const int32 Maximum = IMadRunPartHealth::Execute_GetMaxHealth(Wheel);
		IMadRunPartHealth::Execute_TakeDamage(Wheel, FMath::CeilToInt(FMath::Min(AppliedDamage, static_cast<float>(Maximum))));
	}
	return AppliedDamage;
}

#undef LOCTEXT_NAMESPACE
