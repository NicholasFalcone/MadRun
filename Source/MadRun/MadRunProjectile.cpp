#include "MadRunProjectile.h"
#include "MadRunCollision.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AMadRunProjectile::AMadRunProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 10.0f;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(3.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionObjectType(MadRunProjectileChannel);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(MadRunProjectileChannel, ECR_Ignore);
	Collision->SetNotifyRigidBodyCollision(true);
	Collision->OnComponentHit.AddDynamic(this, &AMadRunProjectile::HandleHit);

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetRelativeScale3D(FVector(0.06f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		Visual->SetStaticMesh(Sphere.Object);
	}

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	ProjectileMovement->InitialSpeed = 10000.0f;
	ProjectileMovement->MaxSpeed = 10000.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void AMadRunProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Shooter = GetOwner())
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
		TArray<AActor*> AttachedActors;
		Shooter->GetAttachedActors(AttachedActors, true, true);
		for (AActor* AttachedActor : AttachedActors)
		{
			Collision->IgnoreActorWhenMoving(AttachedActor, true);
		}
	}
}

void AMadRunProjectile::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (IsValid(OtherActor) && OtherActor != this && Damage > 0.0f)
	{
		UGameplayStatics::ApplyPointDamage(OtherActor, Damage, GetVelocity().GetSafeNormal(),
			Hit, GetInstigatorController(), this, nullptr);
	}
	Destroy();
}
