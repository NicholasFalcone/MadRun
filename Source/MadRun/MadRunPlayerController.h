// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "MadRunPlayerController.generated.h"

class UInputMappingContext;
class AMadRunPawn;
class UMadRunUI;
class UInputAction;
class AMadRunScavengerPoint;
class UMadRunEquipment;
class UMadRunScavengerWidget;

/**
 *  Vehicle Player Controller class
 *  Handles input mapping and user interface
 */
UCLASS(abstract, Config="Game")
class AMadRunPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category="Input|Turret")
	FKey FireKey = EKeys::LeftMouseButton;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> FireMappingContext;

	UPROPERTY(EditDefaultsOnly, Category="Input|Scavenging")
	FKey ScavengeKey = EKeys::E;

	UPROPERTY(EditDefaultsOnly, Category="Input|Scavenging")
	FKey AcceptEquipmentKey = EKeys::Y;

	UPROPERTY(EditDefaultsOnly, Category="Input|Scavenging")
	FKey RejectEquipmentKey = EKeys::N;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ScavengeAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> AcceptEquipmentAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RejectEquipmentAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ScavengingMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UMadRunScavengerWidget> ScavengerWidget;

	TWeakObjectPtr<AMadRunScavengerPoint> PendingScavengerPoint;
	TWeakObjectPtr<UMadRunEquipment> PendingEquipment;
	bool bScavengerPromptOpen = false;
	bool bScavengerHintVisible = false;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** If true, the optional steering wheel input mapping context will be registered */
	UPROPERTY(EditAnywhere, Category = "Input|Steering Wheel Controls")
	bool bUseSteeringWheelControls = false;

	/** Optional Input Mapping Context to be used for steering wheel input.
	 *  This is added alongside the default Input Mapping Context and does not block other forms of input.
	 */
	UPROPERTY(EditAnywhere, Category = "Input|Steering Wheel Controls", meta = (EditCondition = "bUseSteeringWheelControls"))
	UInputMappingContext* SteeringWheelInputMappingContext;

	/** Type of vehicle to automatically respawn when it's destroyed */
	UPROPERTY(EditAnywhere, Category="Vehicle|Respawn")
	TSubclassOf<AMadRunPawn> VehiclePawnClass;

	/** Pointer to the controlled vehicle pawn */
	TObjectPtr<AMadRunPawn> VehiclePawn;

	/** Type of the UI to spawn */
	UPROPERTY(EditAnywhere, Category="Vehicle|UI")
	TSubclassOf<UMadRunUI> VehicleUIClass;

	/** Pointer to the UI widget */
	UPROPERTY()
	TObjectPtr<UMadRunUI> VehicleUI;
		
protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input setup */
	virtual void SetupInputComponent() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

public:

	/** Update vehicle UI on tick */
	virtual void Tick(float Delta) override;

	UFUNCTION(BlueprintCallable, Category="Scavenging")
	void StartScavenging();

	UFUNCTION(BlueprintCallable, Category="Scavenging")
	void AcceptScavengedEquipment();

	UFUNCTION(BlueprintCallable, Category="Scavenging")
	void CancelScavenging();

	void UpdateRaceStatus(const FText& Status) const;

protected:

	/** Pawn setup */
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	void StartTurretFire();
	void StopTurretFire();
	AMadRunScavengerPoint* FindNearbyScavengerPoint() const;

	/** Handles pawn destruction and respawning */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedPawn);

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;
};
