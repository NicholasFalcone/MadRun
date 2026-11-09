// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "MadRunEquipment.h"
#include "MadRunPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UChaosWheeledVehicleMovementComponent;
class AMadRunFuelTank;
class UMadRunWheelBase;
class UChildActorComponent;
class AMadRunTurret;
struct FInputActionValue;

/**
 *  Vehicle Pawn class
 *  Handles common functionality for all vehicle types,
 *  including input handling and camera management.
 *  
 *  Specific vehicle configurations are handled in subclasses.
 */
UCLASS(abstract)
class AMadRunPawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

	/** Spring Arm for the front camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* FrontSpringArm;

	/** Front Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FrontCamera;

	/** Spring Arm for the back camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* BackSpringArm;

	/** Back Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* BackCamera;

	/** Cast pointer to the Chaos Vehicle movement component */
	TObjectPtr<UChaosWheeledVehicleMovementComponent> ChaosVehicleMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UChildActorComponent> FuelTank;

	bool bEngineTorqueDisabledForFuel = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle|Equipment|Initial", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMadRunEngineEquipment> InitialEngineEquipment;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle|Equipment|Initial", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMadRunWeaponEquipment> InitialWeaponEquipment;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle|Equipment|Initial", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMadRunTiresEquipment> InitialTiresEquipment;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Vehicle|Equipment|Initial", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMadRunFuelTankEquipment> InitialFuelTankEquipment;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Vehicle|Equipment", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMadRunEngineEquipment> EngineEquipment;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Vehicle|Equipment", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMadRunWeaponEquipment> WeaponEquipment;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Vehicle|Equipment", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMadRunTiresEquipment> TiresEquipment;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Vehicle|Equipment", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMadRunFuelTankEquipment> FuelTankEquipment;

protected:

	/** Steering Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SteeringAction;

	/** Throttle Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ThrottleAction;

	/** Brake Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* BrakeAction;

	/** Handbrake Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* HandbrakeAction;

	/** Look Around Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAroundAction;

	/** Toggle Camera Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleCameraAction;

	/** Reset Vehicle Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ResetVehicleAction;

	/** Keeps track of which camera is active */
	bool bFrontCameraActive = false;

	/** Keeps track of whether the car is flipped. If this is true for two flip checks, resets the vehicle automatically */
	bool bPreviousFlipCheck = false;

	/** Time between automatic flip checks */
	UPROPERTY(EditAnywhere, Category="Flip Check", meta = (Units = "s"))
	float FlipCheckTime = 3.0f;

	/** Minimum dot product value for the vehicle's up direction that we still consider upright */
	UPROPERTY(EditAnywhere, Category="Flip Check")
	float FlipCheckMinDot = -0.2f;

	/** Flip check timer */
	FTimerHandle FlipCheckTimer;

public:
	AMadRunPawn();

	// Begin Pawn interface

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual void UnPossessed() override;

	// End Pawn interface

	// Begin Actor interface

	/** Initialization */
	virtual void BeginPlay() override;

	/** Cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Update */
	virtual void Tick(float Delta) override;

	// End Actor interface

protected:

	/** Handles steering input */
	void Steering(const FInputActionValue& Value);

	/** Handles throttle input */
	void Throttle(const FInputActionValue& Value);

	/** Handles brake input */
	void Brake(const FInputActionValue& Value);

	/** Handles brake start/stop inputs */
	void StartBrake(const FInputActionValue& Value);
	void StopBrake(const FInputActionValue& Value);

	/** Handles handbrake start/stop inputs */
	void StartHandbrake(const FInputActionValue& Value);
	void StopHandbrake(const FInputActionValue& Value);

	/** Handles look around input */
	void LookAround(const FInputActionValue& Value);

	/** Handles toggle camera input */
	void ToggleCamera(const FInputActionValue& Value);

	/** Handles reset vehicle input */
	void ResetVehicle(const FInputActionValue& Value);

public:

	/** Handle steering input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSteering(float SteeringValue);

	/** Handle throttle input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoThrottle(float ThrottleValue);

	/** Handle brake input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoBrake(float BrakeValue);

	/** Handle brake start input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoBrakeStart();

	/** Handle brake stop input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoBrakeStop();

	/** Handle handbrake start input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoHandbrakeStart();

	/** Handle handbrake stop input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoHandbrakeStop();

	/** Handle look input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoLookAround(float YawDelta);

	/** Handle toggle camera input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoToggleCamera();

	/** Handle reset vehicle input by input actions or mobile interface */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoResetVehicle();

protected:

	/** Called when the brake lights are turned on or off */
	UFUNCTION(BlueprintImplementableEvent, Category="Vehicle")
	void BrakeLights(bool bBraking);

	/** Checks if the car is flipped upside down and automatically resets it */
	UFUNCTION()
	void FlippedCheck();

public:
	/** Returns the front spring arm subobject */
	FORCEINLINE USpringArmComponent* GetFrontSpringArm() const { return FrontSpringArm; }
	/** Returns the front camera subobject */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FrontCamera; }
	/** Returns the back spring arm subobject */
	FORCEINLINE USpringArmComponent* GetBackSpringArm() const { return BackSpringArm; }
	/** Returns the back camera subobject */
	FORCEINLINE UCameraComponent* GetBackCamera() const { return BackCamera; }
	/** Returns the cast Chaos Vehicle Movement subobject */
	FORCEINLINE const TObjectPtr<UChaosWheeledVehicleMovementComponent>& GetChaosVehicleMovement() const { return ChaosVehicleMovement; }
	UFUNCTION(BlueprintPure, Category="Fuel")
	AMadRunFuelTank* GetFuelTank() const;

	/** Returns a runtime wheel by its Wheel Setups index, or null if unavailable. */
	UFUNCTION(BlueprintCallable, Category="Vehicle|Wheels")
	UMadRunWheelBase* GetWheelByIndex(int32 WheelIndex) const;

	UFUNCTION(BlueprintPure, Category="Vehicle|Equipment")
	UMadRunEngineEquipment* GetEngineEquipment() const { return EngineEquipment; }

	UFUNCTION(BlueprintPure, Category="Vehicle|Equipment")
	UMadRunWeaponEquipment* GetWeaponEquipment() const { return WeaponEquipment; }

	UFUNCTION(BlueprintPure, Category="Vehicle|Equipment")
	UMadRunTiresEquipment* GetTiresEquipment() const { return TiresEquipment; }

	UFUNCTION(BlueprintPure, Category="Vehicle|Equipment")
	UMadRunFuelTankEquipment* GetFuelTankEquipment() const { return FuelTankEquipment; }

	/** Equips an asset in its corresponding slot and applies its visual, if any. */
	UFUNCTION(BlueprintCallable, Category="Vehicle|Equipment")
	bool Equip(UMadRunEquipment* Equipment);

protected:
	virtual bool ApplyTiresMesh(UStaticMesh* TireMesh);

public:

	UFUNCTION(BlueprintPure, Category="Vehicle|Turret")
	virtual AMadRunTurret* GetTurret() const { return nullptr; }

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoFireStart();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoFireStop();
};
