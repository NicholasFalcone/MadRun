// Copyright Epic Games, Inc. All Rights Reserved.

#include "MadRunPawn.h"
#include "MadRunFuelTank.h"
#include "MadRunWheelBase.h"
#include "MadRunTurret.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "MadRun.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

AMadRunPawn::AMadRunPawn()
{
	FuelTank = CreateDefaultSubobject<UChildActorComponent>(TEXT("Fuel Tank Actor"));
	FuelTank->SetupAttachment(GetMesh());
	FuelTank->SetChildActorClass(AMadRunFuelTank::StaticClass());
	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

}

void AMadRunPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// steering 
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &AMadRunPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &AMadRunPawn::Steering);

		// throttle 
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AMadRunPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &AMadRunPawn::Throttle);

		// break 
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &AMadRunPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &AMadRunPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &AMadRunPawn::StopBrake);

		// handbrake 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &AMadRunPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &AMadRunPawn::StopHandbrake);

		// look around 
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &AMadRunPawn::LookAround);

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &AMadRunPawn::ToggleCamera);

		// reset the vehicle 
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &AMadRunPawn::ResetVehicle);
	}
	else
	{
		UE_LOG(LogMadRun, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMadRunPawn::BeginPlay()
{
	for (UMadRunEquipment* Initial : { static_cast<UMadRunEquipment*>(InitialEngineEquipment.Get()),
		static_cast<UMadRunEquipment*>(InitialWeaponEquipment.Get()),
		static_cast<UMadRunEquipment*>(InitialTiresEquipment.Get()),
		static_cast<UMadRunEquipment*>(InitialFuelTankEquipment.Get()) })
	{
		if (Initial && !Equip(Initial))
		{
			UE_LOG(LogMadRun, Error, TEXT("Vehicle '%s' failed to equip initial equipment '%s'."), *GetNameSafe(this), *GetNameSafe(Initial));
		}
	}

	Super::BeginPlay();

	if (!GetFuelTank())
	{
		UE_LOG(LogMadRun, Error, TEXT("Vehicle '%s' has no valid fuel tank actor; engine torque will be disabled."), *GetNameSafe(this));
	}

	// set up the flipped check timer
	GetWorld()->GetTimerManager().SetTimer(FlipCheckTimer, this, &AMadRunPawn::FlippedCheck, FlipCheckTime, true);
}

void AMadRunPawn::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	DoFireStop();
	// clear the flipped check timer
	GetWorld()->GetTimerManager().ClearTimer(FlipCheckTimer);

	Super::EndPlay(EndPlayReason);
}

void AMadRunPawn::UnPossessed()
{
	DoFireStop();
	Super::UnPossessed();
}

void AMadRunPawn::DoFireStart()
{
	if (AMadRunTurret* Turret = GetTurret())
	{
		Turret->StartFire();
	}
}

void AMadRunPawn::DoFireStop()
{
	if (AMadRunTurret* Turret = GetTurret())
	{
		Turret->StopFire();
	}
}

void AMadRunPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	const float Throttle = ChaosVehicleMovement->GetThrottleInput();
	const float ReverseThrottle = ChaosVehicleMovement->bReverseAsBrake && ChaosVehicleMovement->GetTargetGear() < 0
		? ChaosVehicleMovement->GetBrakeInput() : 0.0f;
	AMadRunFuelTank* Tank = GetFuelTank();
	if (Tank)
	{
		Tank->ConsumeFuel(Delta, FMath::Max(Throttle, ReverseThrottle));
	}

	if (!Tank || Tank->GetFuel() <= 0.0f)
	{
		ChaosVehicleMovement->SetMaxEngineTorque(0.0f);
		bEngineTorqueDisabledForFuel = true;
	}
	else if (bEngineTorqueDisabledForFuel)
	{
		ChaosVehicleMovement->SetMaxEngineTorque(ChaosVehicleMovement->EngineSetup.MaxTorque);
		bEngineTorqueDisabledForFuel = false;
	}

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
}

AMadRunFuelTank* AMadRunPawn::GetFuelTank() const
{
	return IsValid(FuelTank) ? Cast<AMadRunFuelTank>(FuelTank->GetChildActor()) : nullptr;
}

bool AMadRunPawn::Equip(UMadRunEquipment* Equipment)
{
	if (!IsValid(Equipment))
	{
		UE_LOG(LogMadRun, Warning, TEXT("Vehicle '%s' cannot equip invalid equipment '%s'."), *GetNameSafe(this), *GetNameSafe(Equipment));
		return false;
	}
	if (UMadRunEngineEquipment* Engine = Cast<UMadRunEngineEquipment>(Equipment))
	{
		EngineEquipment = Engine;
		return true;
	}
	if (UMadRunWeaponEquipment* Weapon = Cast<UMadRunWeaponEquipment>(Equipment))
	{
		AMadRunTurret* Turret = GetTurret();
		if (!IsValid(Turret) || !Turret->ApplyEquipmentVisual(Weapon))
		{
			UE_LOG(LogMadRun, Warning, TEXT("Vehicle '%s' cannot apply weapon equipment '%s'."), *GetNameSafe(this), *GetNameSafe(Weapon));
			return false;
		}
		WeaponEquipment = Weapon;
		return true;
	}
	if (UMadRunTiresEquipment* Tires = Cast<UMadRunTiresEquipment>(Equipment))
	{
		if (!IsValid(Tires->Mesh) || !ApplyTiresMesh(Tires->Mesh))
		{
			UE_LOG(LogMadRun, Warning, TEXT("Vehicle '%s' cannot apply tires equipment '%s'."), *GetNameSafe(this), *GetNameSafe(Tires));
			return false;
		}
		TiresEquipment = Tires;
		return true;
	}
	if (UMadRunFuelTankEquipment* TankEquipment = Cast<UMadRunFuelTankEquipment>(Equipment))
	{
		AMadRunFuelTank* Tank = GetFuelTank();
		if (!IsValid(Tank) || !Tank->ApplyEquipmentVisual(TankEquipment))
		{
			UE_LOG(LogMadRun, Warning, TEXT("Vehicle '%s' cannot apply fuel tank equipment '%s'."), *GetNameSafe(this), *GetNameSafe(TankEquipment));
			return false;
		}
		FuelTankEquipment = TankEquipment;
		return true;
	}
	UE_LOG(LogMadRun, Warning, TEXT("Vehicle '%s' cannot equip unsupported equipment '%s'."), *GetNameSafe(this), *GetNameSafe(Equipment));
	return false;
}

bool AMadRunPawn::ApplyTiresMesh(UStaticMesh* TireMesh)
{
	UE_LOG(LogMadRun, Warning, TEXT("Vehicle '%s' does not support tire visuals."), *GetNameSafe(this));
	return false;
}

UMadRunWheelBase* AMadRunPawn::GetWheelByIndex(int32 WheelIndex) const
{
	if (!ChaosVehicleMovement || !ChaosVehicleMovement->Wheels.IsValidIndex(WheelIndex))
	{
		UE_LOG(LogMadRun, Warning, TEXT("Vehicle '%s' has no wheel at index %d."), *GetNameSafe(this), WheelIndex);
		return nullptr;
	}
	UMadRunWheelBase* Wheel = Cast<UMadRunWheelBase>(ChaosVehicleMovement->Wheels[WheelIndex]);
	if (!Wheel)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Wheel %d on vehicle '%s' does not implement MadRun wheel health."), WheelIndex, *GetNameSafe(this));
	}
	return Wheel;
}

void AMadRunPawn::Steering(const FInputActionValue& Value)
{
	// route the input
	DoSteering(Value.Get<float>());
}

void AMadRunPawn::Throttle(const FInputActionValue& Value)
{
	// route the input
	DoThrottle(Value.Get<float>());
}

void AMadRunPawn::Brake(const FInputActionValue& Value)
{
	// route the input
	DoBrake(Value.Get<float>());
}

void AMadRunPawn::StartBrake(const FInputActionValue& Value)
{
	// route the input
	DoBrakeStart();
}

void AMadRunPawn::StopBrake(const FInputActionValue& Value)
{
	// route the input
	DoBrakeStop();
}

void AMadRunPawn::StartHandbrake(const FInputActionValue& Value)
{
	// route the input
	DoHandbrakeStart();
}

void AMadRunPawn::StopHandbrake(const FInputActionValue& Value)
{
	// route the input
	DoHandbrakeStop();
}

void AMadRunPawn::LookAround(const FInputActionValue& Value)
{
	// route the input
	DoLookAround(Value.Get<float>());
}

void AMadRunPawn::ToggleCamera(const FInputActionValue& Value)
{
	// route the input
	DoToggleCamera();
}

void AMadRunPawn::ResetVehicle(const FInputActionValue& Value)
{
	// route the input
	DoResetVehicle();
}

void AMadRunPawn::DoSteering(float SteeringValue)
{
	// add the input
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void AMadRunPawn::DoThrottle(float ThrottleValue)
{
	// add the input
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);

	// reset the brake input
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void AMadRunPawn::DoBrake(float BrakeValue)
{
	// add the input
	ChaosVehicleMovement->SetBrakeInput(BrakeValue);

	// reset the throttle input
	ChaosVehicleMovement->SetThrottleInput(0.0f);
}

void AMadRunPawn::DoBrakeStart()
{
	// call the Blueprint hook for the brake lights
	BrakeLights(true);
}

void AMadRunPawn::DoBrakeStop()
{
	// call the Blueprint hook for the brake lights
	BrakeLights(false);

	// reset brake input to zero
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void AMadRunPawn::DoHandbrakeStart()
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(true);

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void AMadRunPawn::DoHandbrakeStop()
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(false);

	// call the Blueprint hook for the break lights
	BrakeLights(false);
}

void AMadRunPawn::DoLookAround(float YawDelta)
{
	// rotate the spring arm
	BackSpringArm->AddLocalRotation(FRotator(0.0f, YawDelta, 0.0f));
}

void AMadRunPawn::DoToggleCamera()
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void AMadRunPawn::DoResetVehicle()
{
	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;

	// teleport the actor to the reset spot and reset physics
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
}

void AMadRunPawn::FlippedCheck()
{
	// check the difference in angle between the mesh's up vector and world up
	const float UpDot = FVector::DotProduct(FVector::UpVector, GetMesh()->GetUpVector());

	if (UpDot < FlipCheckMinDot)
	{
		// is this the second time we've checked that the vehicle is still flipped?
		if (bPreviousFlipCheck)
		{
			// reset the vehicle to upright
			DoResetVehicle();
		}
		
		// set the flipped check flag so the next check resets the car
		bPreviousFlipCheck = true;

	} else {

		// we're upright. reset the flipped check flag
		bPreviousFlipCheck = false;
	}
}

#undef LOCTEXT_NAMESPACE