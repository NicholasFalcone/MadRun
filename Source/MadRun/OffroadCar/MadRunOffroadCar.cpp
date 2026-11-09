// Copyright Epic Games, Inc. All Rights Reserved.


#include "MadRunOffroadCar.h"
#include "MadRunOffroadWheelFront.h"
#include "MadRunOffroadWheelRear.h"
#include "MadRunWheelHitActor.h"
#include "MadRunTurret.h"
#include "MadRun.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/ChildActorComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"

AMadRunOffroadCar::AMadRunOffroadCar()
{
	// construct the mesh components
	Chassis = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chassis"));
	Chassis->SetupAttachment(GetMesh());

	// NOTE: tire sockets are set from the Blueprint class
	TireFrontLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tire Front Left"));
	TireFrontLeft->SetupAttachment(GetMesh(), FName("VisWheel_FL"));
	TireFrontLeft->SetCollisionProfileName(FName("NoCollision"));

	TireFrontRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tire Front Right"));
	TireFrontRight->SetupAttachment(GetMesh(), FName("VisWheel_FR"));
	TireFrontRight->SetCollisionProfileName(FName("NoCollision"));
	TireFrontRight->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	TireRearLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tire Rear Left"));
	TireRearLeft->SetupAttachment(GetMesh(), FName("VisWheel_BL"));
	TireRearLeft->SetCollisionProfileName(FName("NoCollision"));

	TireRearRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tire Rear Right"));
	TireRearRight->SetupAttachment(GetMesh(), FName("VisWheel_BR"));
	TireRearRight->SetCollisionProfileName(FName("NoCollision"));
	TireRearRight->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	FrontLeftWheelHitbox = CreateDefaultSubobject<UChildActorComponent>(TEXT("Front Left Wheel Hitbox"));
	FrontLeftWheelHitbox->SetupAttachment(TireFrontLeft);
	FrontLeftWheelHitbox->SetChildActorClass(AMadRunWheelHitActor::StaticClass());

	FrontRightWheelHitbox = CreateDefaultSubobject<UChildActorComponent>(TEXT("Front Right Wheel Hitbox"));
	FrontRightWheelHitbox->SetupAttachment(TireFrontRight);
	FrontRightWheelHitbox->SetChildActorClass(AMadRunWheelHitActor::StaticClass());

	RearLeftWheelHitbox = CreateDefaultSubobject<UChildActorComponent>(TEXT("Rear Left Wheel Hitbox"));
	RearLeftWheelHitbox->SetupAttachment(TireRearLeft);
	RearLeftWheelHitbox->SetChildActorClass(AMadRunWheelHitActor::StaticClass());

	RearRightWheelHitbox = CreateDefaultSubobject<UChildActorComponent>(TEXT("Rear Right Wheel Hitbox"));
	RearRightWheelHitbox->SetupAttachment(TireRearRight);
	RearRightWheelHitbox->SetChildActorClass(AMadRunWheelHitActor::StaticClass());

	RoofTurret = CreateDefaultSubobject<UChildActorComponent>(TEXT("Roof Turret"));
	RoofTurret->SetupAttachment(GetMesh());
	RoofTurret->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
	RoofTurret->SetChildActorClass(AMadRunTurret::StaticClass());

	// adjust the cameras
	GetFrontSpringArm()->SetRelativeLocation(FVector(-5.0f, -30.0f, 135.0f));
	GetBackSpringArm()->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));

	// Note: for faster iteration times, the vehicle setup can be tweaked in the Blueprint instead

	// Set up the chassis
	GetChaosVehicleMovement()->ChassisHeight = 160.0f;
	GetChaosVehicleMovement()->DragCoefficient = 0.1f;
	GetChaosVehicleMovement()->DownforceCoefficient = 0.1f;
	GetChaosVehicleMovement()->CenterOfMassOverride = FVector(0.0f, 0.0f, 75.0f);
	GetChaosVehicleMovement()->bEnableCenterOfMassOverride = true;

	// Set up the wheels
	GetChaosVehicleMovement()->bLegacyWheelFrictionPosition = false;
	GetChaosVehicleMovement()->WheelSetups.SetNum(4);

	GetChaosVehicleMovement()->WheelSetups[0].WheelClass = UMadRunOffroadWheelFront::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[0].BoneName = FName("PhysWheel_FL");
	GetChaosVehicleMovement()->WheelSetups[0].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosVehicleMovement()->WheelSetups[1].WheelClass = UMadRunOffroadWheelFront::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[1].BoneName = FName("PhysWheel_FR");
	GetChaosVehicleMovement()->WheelSetups[1].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosVehicleMovement()->WheelSetups[2].WheelClass = UMadRunOffroadWheelRear::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[2].BoneName = FName("PhysWheel_BL");
	GetChaosVehicleMovement()->WheelSetups[2].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosVehicleMovement()->WheelSetups[3].WheelClass = UMadRunOffroadWheelRear::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[3].BoneName = FName("PhysWheel_BR");
	GetChaosVehicleMovement()->WheelSetups[3].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	// Set up the engine
	// NOTE: Check the Blueprint asset for the Torque Curve
	GetChaosVehicleMovement()->EngineSetup.MaxTorque = 600.0f;
	GetChaosVehicleMovement()->EngineSetup.MaxRPM = 5000.0f;
	GetChaosVehicleMovement()->EngineSetup.EngineIdleRPM = 1200.0f;
	GetChaosVehicleMovement()->EngineSetup.EngineBrakeEffect = 0.05f;
	GetChaosVehicleMovement()->EngineSetup.EngineRevUpMOI = 5.0f;
	GetChaosVehicleMovement()->EngineSetup.EngineRevDownRate = 600.0f;

	// Set up the differential
	GetChaosVehicleMovement()->DifferentialSetup.DifferentialType = EVehicleDifferential::AllWheelDrive;
	GetChaosVehicleMovement()->DifferentialSetup.FrontRearSplit = 0.5f;

	// Set up the steering
	// NOTE: Check the Blueprint asset for the Steering Curve
	GetChaosVehicleMovement()->SteeringSetup.SteeringType = ESteeringType::AngleRatio;
	GetChaosVehicleMovement()->SteeringSetup.AngleRatio = 0.7f;
}

AMadRunTurret* AMadRunOffroadCar::GetTurret() const
{
	return IsValid(RoofTurret) ? Cast<AMadRunTurret>(RoofTurret->GetChildActor()) : nullptr;
}

bool AMadRunOffroadCar::ApplyTiresMesh(UStaticMesh* TireMesh)
{
	UStaticMeshComponent* Tires[] = { TireFrontLeft, TireFrontRight, TireRearLeft, TireRearRight };
	for (const UStaticMeshComponent* Tire : Tires)
	{
		if (!IsValid(Tire))
		{
			UE_LOG(LogMadRun, Warning, TEXT("Vehicle '%s' has a missing tire mesh component."), *GetNameSafe(this));
			return false;
		}
	}
	UStaticMesh* Previous[UE_ARRAY_COUNT(Tires)];
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Tires); ++Index)
	{
		Previous[Index] = Tires[Index]->GetStaticMesh();
		if (Previous[Index] != TireMesh && !Tires[Index]->SetStaticMesh(TireMesh))
		{
			for (int32 Rollback = 0; Rollback < Index; ++Rollback)
			{
				if (Tires[Rollback]->GetStaticMesh() != Previous[Rollback] && !Tires[Rollback]->SetStaticMesh(Previous[Rollback]))
				{
					UE_LOG(LogMadRun, Error, TEXT("Vehicle '%s' could not restore tire mesh %d."), *GetNameSafe(this), Rollback);
				}
			}
			return false;
		}
	}
	return true;
}

void AMadRunOffroadCar::BeginPlay()
{
	Super::BeginPlay();
	UChildActorComponent* Hitboxes[] = { FrontLeftWheelHitbox, FrontRightWheelHitbox, RearLeftWheelHitbox, RearRightWheelHitbox };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Hitboxes); ++Index)
	{
		AMadRunWheelHitActor* HitActor = Hitboxes[Index] ? Cast<AMadRunWheelHitActor>(Hitboxes[Index]->GetChildActor()) : nullptr;
		if (!HitActor)
		{
			UE_LOG(LogMadRun, Error, TEXT("Vehicle '%s' has no hit actor for wheel %d."), *GetNameSafe(this), Index);
			continue;
		}

		HitActor->InitializeWheel(this, Index);
	}
}

float AMadRunOffroadCar::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointDamage = static_cast<const FPointDamageEvent&>(DamageEvent);
		if (PointDamage.HitInfo.GetComponent() == GetMesh() && !PointDamage.HitInfo.BoneName.IsNone())
		{
			const TArray<FChaosWheelSetup>& Setups = GetChaosVehicleMovement()->WheelSetups;
			UChildActorComponent* Hitboxes[] = { FrontLeftWheelHitbox, FrontRightWheelHitbox, RearLeftWheelHitbox, RearRightWheelHitbox };
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(Hitboxes) && Setups.IsValidIndex(Index); ++Index)
			{
				if (Setups[Index].BoneName == PointDamage.HitInfo.BoneName)
				{
					AMadRunWheelHitActor* HitActor = Hitboxes[Index] ? Cast<AMadRunWheelHitActor>(Hitboxes[Index]->GetChildActor()) : nullptr;
					if (!HitActor)
					{
						UE_LOG(LogMadRun, Error, TEXT("Cannot route hit on wheel %d of '%s': missing hit actor."), Index, *GetNameSafe(this));
						return 0.0f;
					}
					return HitActor->TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
				}
			}
		}
	}
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}