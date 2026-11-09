#include "MadRunWheelHitActor.h"
#include "MadRun.h"
#include "MadRunCollision.h"
#include "MadRunPawn.h"
#include "MadRunWheelBase.h"
#include "Components/SphereComponent.h"

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
}

void AMadRunWheelHitActor::InitializeWheel(AMadRunPawn* InVehicle, int32 InWheelIndex)
{
	Vehicle = InVehicle;
	WheelIndex = InWheelIndex;
}

UMadRunWheelBase* AMadRunWheelHitActor::GetWheel() const
{
	return IsValid(Vehicle) ? Vehicle->GetWheelByIndex(WheelIndex) : nullptr;
}

float AMadRunWheelHitActor::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
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
