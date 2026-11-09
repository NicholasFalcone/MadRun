#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MadRunProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProjectileHit);
UCLASS(Blueprintable)
class AMadRunProjectile : public AActor
{
	GENERATED_BODY()

public:
	AMadRunProjectile();
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage", meta=(AllowPrivateAccess="true", ClampMin="0.0"))
	float Damage = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Effects", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UParticleSystem> HitEffect;

public:
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnProjectileHit OnProjectileHit;
};
