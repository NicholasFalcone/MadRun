// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MadRunPawn.h"
#include "MadRunOffroadCar.generated.h"

class UChildActorComponent;

/**
 *  Offroad car wheeled vehicle implementation
 */
UCLASS(abstract)
class AMadRunOffroadCar : public AMadRunPawn
{
	GENERATED_BODY()
	
	/** Chassis static mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Chassis;

	/** FL Tire static mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* TireFrontLeft;

	/** FR Tire static mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* TireFrontRight;

	/** RL Tire static mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* TireRearLeft;

	/** RR Tire static mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* TireRearRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wheel Hitboxes", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UChildActorComponent> FrontLeftWheelHitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wheel Hitboxes", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UChildActorComponent> FrontRightWheelHitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wheel Hitboxes", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UChildActorComponent> RearLeftWheelHitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wheel Hitboxes", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UChildActorComponent> RearRightWheelHitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UChildActorComponent> RoofTurret;

public:

	AMadRunOffroadCar();
	virtual void BeginPlay() override;
	virtual AMadRunTurret* GetTurret() const override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual bool ApplyTiresMesh(UStaticMesh* TireMesh) override;
};
