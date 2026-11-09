#include "MadRunTurret.h"
#include "MadRunPawn.h"
#include "MadRunProjectile.h"
#include "MadRunCollision.h"
#include "MadRun.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

AMadRunTurret::AMadRunTurret()
{
	PrimaryActorTick.bCanEverTick = true;
	Base = CreateDefaultSubobject<USceneComponent>(TEXT("Base"));
	SetRootComponent(Base);
	Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel"));
	Barrel->SetupAttachment(Base);
	Barrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Barrel->SetRelativeScale3D(FVector(1.0f, 0.18f, 0.18f));
	Barrel->SetRelativeLocation(FVector(50.0f, 0.0f, 0.0f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Barrel->SetStaticMesh(Cube.Object);
	}
	ProjectileClass = AMadRunProjectile::StaticClass();
}

AMadRunPawn* AMadRunTurret::FindTarget() const
{
	const AMadRunPawn* Vehicle = Cast<AMadRunPawn>(GetParentActor());
	if (!IsValid(Vehicle))
	{
		return nullptr;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TurretLineOfSight), false);
	Params.AddIgnoredActor(Vehicle);
	TArray<AActor*> Attached;
	Vehicle->GetAttachedActors(Attached, true, true);
	Params.AddIgnoredActors(Attached);
	AMadRunPawn* Best = nullptr;
	float BestDistanceSquared = FMath::Square(FMath::Max(0.0f, TargetRange));
	const FVector Origin = GetActorLocation();
	for (TActorIterator<AMadRunPawn> It(GetWorld()); It; ++It)
	{
		AMadRunPawn* Candidate = *It;
		if (!IsValid(Candidate) || Candidate == Vehicle || Candidate->IsActorBeingDestroyed())
		{
			continue;
		}
		const FVector AimPoint = Candidate->GetMesh()->Bounds.Origin;
		const float DistanceSquared = FVector::DistSquared(Origin, AimPoint);
		if (DistanceSquared >= BestDistanceSquared)
		{
			continue;
		}
		FHitResult Hit;
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Origin, AimPoint, MadRunProjectileChannel, Params);
		AActor* HitActor = Hit.GetActor();
		if (bBlocked && HitActor != Candidate && (!HitActor || !HitActor->IsAttachedTo(Candidate)))
		{
			continue;
		}
		Best = Candidate;
		BestDistanceSquared = DistanceSquared;
	}
	return Best;
}

void AMadRunTurret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Target = FindTarget();
	if (AMadRunPawn* CurrentTarget = Target.Get())
	{
		SetActorRotation((CurrentTarget->GetMesh()->Bounds.Origin - GetActorLocation()).Rotation());
	}
	else if (const AActor* Parent = GetParentActor())
	{
		SetActorRotation(Parent->GetActorRotation());
	}
	if (bFiring && GetWorld()->GetTimeSeconds() >= NextFireTime)
	{
		Fire();
		NextFireTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.01f, FireInterval);
	}
}

void AMadRunTurret::StartFire()
{
	bFiring = true;
}

void AMadRunTurret::StopFire()
{
	bFiring = false;
}

void AMadRunTurret::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	StopFire();
	Super::EndPlay(EndPlayReason);
}

void AMadRunTurret::Fire()
{
	AMadRunPawn* Vehicle = Cast<AMadRunPawn>(GetParentActor());
	if (!IsValid(Vehicle) || !ProjectileClass)
	{
		UE_LOG(LogMadRun, Error, TEXT("Turret '%s' cannot fire without a vehicle and projectile class."), *GetNameSafe(this));
		return;
	}
	FActorSpawnParameters Params;
	Params.Owner = Vehicle;
	Params.Instigator = Vehicle;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Origin = GetActorLocation();
	const FVector Muzzle = Origin + GetActorForwardVector() * FMath::Max(0.0f, MuzzleDistance);
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(TurretMuzzle), false);
	TraceParams.AddIgnoredActor(Vehicle);
	TArray<AActor*> Attached;
	Vehicle->GetAttachedActors(Attached, true, true);
	TraceParams.AddIgnoredActors(Attached);
	FHitResult Hit;
	const FVector SpawnLocation = GetWorld()->LineTraceSingleByChannel(Hit, Origin, Muzzle, MadRunProjectileChannel, TraceParams)
		? Origin : Muzzle;
	if (!GetWorld()->SpawnActor<AMadRunProjectile>(ProjectileClass, SpawnLocation, GetActorRotation(), Params))
	{
		UE_LOG(LogMadRun, Error, TEXT("Turret '%s' failed to spawn a projectile."), *GetNameSafe(this));
	}
	OnTurretFired.Broadcast();
}
