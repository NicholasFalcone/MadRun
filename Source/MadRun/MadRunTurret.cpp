#include "MadRunTurret.h"
#include "MadRunPawn.h"
#include "MadRunPartHealth.h"
#include "MadRunProjectile.h"
#include "MadRunWheelBase.h"
#include "MadRunWheelHitActor.h"
#include "MadRunCollision.h"
#include "MadRun.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
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

void AMadRunTurret::BeginPlay()
{
	if (!FMath::IsFinite(FireInterval) || FireInterval < 0.01f || !FMath::IsFinite(Damage) || Damage < 0.0f
		|| MagazineSize < 1 || !FMath::IsFinite(ReloadTime) || ReloadTime < 0.0f)
	{
		UE_LOG(LogMadRun, Error, TEXT("Turret '%s' has invalid default weapon statistics."), *GetNameSafe(this));
	}
	else
	{
		AmmoInMagazine = MagazineSize;
	}
	Super::BeginPlay();
}

UMadRunWeaponEquipment* AMadRunTurret::GetEquipment() const
{
	const AMadRunPawn* Vehicle = Cast<AMadRunPawn>(GetParentActor());
	return IsValid(Vehicle) && Vehicle->GetTurret() == this ? Vehicle->GetWeaponEquipment() : nullptr;
}

bool AMadRunTurret::ApplyEquipment(const UMadRunWeaponEquipment* Equipment)
{
	if (!IsValid(Equipment) || !IsValid(Equipment->Mesh) || !IsValid(Barrel))
	{
		UE_LOG(LogMadRun, Warning, TEXT("Turret '%s' requires a valid equipment mesh."), *GetNameSafe(this));
		return false;
	}
	if (!FMath::IsFinite(Equipment->FireInterval) || Equipment->FireInterval < 0.01f
		|| !FMath::IsFinite(Equipment->Damage) || Equipment->Damage < 0.0f
		|| Equipment->MagazineSize < 1 || !FMath::IsFinite(Equipment->ReloadTime) || Equipment->ReloadTime < 0.0f)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Turret '%s' cannot equip weapon '%s': invalid weapon statistics."),
			*GetNameSafe(this), *GetNameSafe(Equipment));
		return false;
	}
	if (Barrel->GetStaticMesh() != Equipment->Mesh && !Barrel->SetStaticMesh(Equipment->Mesh))
	{
		return false;
	}
	Barrel->SetRelativeTransform(Equipment->MeshTransform);
	FireInterval = Equipment->FireInterval;
	Damage = Equipment->Damage;
	MagazineSize = Equipment->MagazineSize;
	ReloadTime = Equipment->ReloadTime;
	AmmoInMagazine = MagazineSize;
	bReloading = false;
	return true;
}

AActor* AMadRunTurret::FindTarget() const
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
	AActor* Best = nullptr;
	float BestDistanceSquared = FMath::Square(FMath::Max(0.0f, TargetRange));
	const FVector Origin = GetActorLocation();
	for (TActorIterator<AMadRunPawn> It(GetWorld()); It; ++It)
	{
		AMadRunPawn* Candidate = *It;
		if (!IsValid(Candidate) || Candidate == Vehicle || Candidate->IsActorBeingDestroyed())
		{
			continue;
		}
		TArray<AActor*> Parts;
		Candidate->GetAttachedActors(Parts, true, true);
		for (AActor* Part : Parts)
		{
			if (!IsValid(Part) || Part->IsActorBeingDestroyed())
			{
				continue;
			}
			UObject* HealthObject = Part;
			if (AMadRunWheelHitActor* WheelHitActor = Cast<AMadRunWheelHitActor>(Part))
			{
				HealthObject = WheelHitActor->GetWheel();
			}
			if (!IsValid(HealthObject) || !HealthObject->GetClass()->ImplementsInterface(UMadRunPartHealth::StaticClass())
				|| IMadRunPartHealth::Execute_GetHealth(HealthObject) <= 0)
			{
				continue;
			}
			FVector AimPoint;
			FVector Extent;
			Part->GetActorBounds(true, AimPoint, Extent);
			const float DistanceSquared = FVector::DistSquared(Origin, AimPoint);
			if (DistanceSquared >= BestDistanceSquared)
			{
				continue;
			}
			FHitResult Hit;
			if (!GetWorld()->LineTraceSingleByChannel(Hit, Origin, AimPoint, MadRunProjectileChannel, Params)
				|| Hit.GetActor() != Part)
			{
				continue;
			}
			Best = Part;
			BestDistanceSquared = DistanceSquared;
		}
	}
	return Best;
}

void AMadRunTurret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Target = FindTarget();
	if (AActor* CurrentTarget = Target.Get())
	{
		FVector AimPoint;
		FVector Extent;
		CurrentTarget->GetActorBounds(true, AimPoint, Extent);
		SetActorRotation((AimPoint - GetActorLocation()).Rotation());
	}
	else if (const AActor* Parent = GetParentActor())
	{
		SetActorRotation(Parent->GetActorRotation());
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (bReloading && Now >= ReloadEndTime)
	{
		AmmoInMagazine = MagazineSize;
		bReloading = false;
	}
	if (bFiring && !bReloading && AmmoInMagazine > 0 && Now >= NextFireTime)
	{
		NextFireTime = Now + FireInterval;
		if (Fire())
		{
			--AmmoInMagazine;
			if (AmmoInMagazine == 0)
			{
				bReloading = true;
				ReloadEndTime = Now + ReloadTime;
			}
			OnTurretFired.Broadcast();
		}
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

bool AMadRunTurret::Fire()
{
	AMadRunPawn* Vehicle = Cast<AMadRunPawn>(GetParentActor());
	if (!IsValid(Vehicle) || !ProjectileClass)
	{
		UE_LOG(LogMadRun, Error, TEXT("Turret '%s' cannot fire without a vehicle and projectile class."), *GetNameSafe(this));
		return false;
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
	const FTransform SpawnTransform(GetActorRotation(), SpawnLocation);
	Params.bDeferConstruction = true;
	AMadRunProjectile* Projectile = GetWorld()->SpawnActor<AMadRunProjectile>(ProjectileClass, SpawnTransform, Params);
	if (!Projectile)
	{
		UE_LOG(LogMadRun, Error, TEXT("Turret '%s' failed to spawn a projectile."), *GetNameSafe(this));
		return false;
	}
	if (!Projectile->SetDamage(Damage))
	{
		Projectile->Destroy();
		return false;
	}
	UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
	return true;
}
