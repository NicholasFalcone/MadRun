// Copyright Epic Games, Inc. All Rights Reserved.


#include "MadRunPlayerController.h"
#include "MadRunPawn.h"
#include "MadRunFuelTank.h"
#include "MadRunUI.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "MadRun.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AMadRunPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// ensure we're attached to the vehicle pawn so that World Partition streaming works correctly
	bAttachToPawn = true;

	// only spawn UI on local player controllers
	if (IsLocalPlayerController())
	{
		if (ShouldUseTouchControls())
		{
			// spawn the mobile controls widget
			MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

			if (MobileControlsWidget)
			{
				// add the controls to the player screen
				MobileControlsWidget->AddToPlayerScreen(0);

			} else {

				UE_LOG(LogMadRun, Error, TEXT("Could not spawn mobile controls widget."));

			}
		}
		

		// spawn the UI widget and add it to the viewport
		VehicleUI = CreateWidget<UMadRunUI>(this, VehicleUIClass);

		if (VehicleUI)
		{
			VehicleUI->AddToViewport();

		} else {

			UE_LOG(LogMadRun, Error, TEXT("Could not spawn vehicle UI widget."));

		}
	}
}

void AMadRunPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!FireAction)
	{
		FireAction = NewObject<UInputAction>(this, TEXT("TurretFireAction"));
		FireAction->ValueType = EInputActionValueType::Boolean;
		FireMappingContext = NewObject<UInputMappingContext>(this, TEXT("TurretFireMapping"));
		if (FireKey.IsValid())
		{
			FireMappingContext->MapKey(FireAction, FireKey);
		}
		else
		{
			UE_LOG(LogMadRun, Error, TEXT("Cannot map turret fire: Fire Key is invalid."));
		}
	}
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &AMadRunPlayerController::StartTurretFire);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, this, &AMadRunPlayerController::StopTurretFire);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Canceled, this, &AMadRunPlayerController::StopTurretFire);
	}
	else
	{
		UE_LOG(LogMadRun, Error, TEXT("Cannot bind turret fire: controller has no Enhanced Input component."));
	}
	
	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(FireMappingContext, 0);
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
			else
			{
				UE_LOG(LogMadRun, Error, TEXT("Cannot map turret fire: Enhanced Input local player subsystem is unavailable."));
			}
		}
	}
}

void AMadRunPlayerController::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	StopTurretFire();
	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(FireMappingContext);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AMadRunPlayerController::StartTurretFire()
{
	if (AMadRunPawn* ControlledVehicle = Cast<AMadRunPawn>(GetPawn()))
	{
		ControlledVehicle->DoFireStart();
	}
}

void AMadRunPlayerController::StopTurretFire()
{
	if (AMadRunPawn* ControlledVehicle = Cast<AMadRunPawn>(GetPawn()))
	{
		ControlledVehicle->DoFireStop();
	}
}

void AMadRunPlayerController::OnUnPossess()
{
	StopTurretFire();
	Super::OnUnPossess();
}

void AMadRunPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	if (IsValid(VehicleUI))
	{
		if (IsValid(VehiclePawn) && !VehiclePawn->IsActorBeingDestroyed() && GetPawn() == VehiclePawn)
		{
			VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
			VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
			if (AMadRunFuelTank* Tank = VehiclePawn->GetFuelTank())
			{
				VehicleUI->UpdateFuel(Tank->GetFuel(), Tank->GetCapacity());
				return;
			}
		}
		VehicleUI->ClearFuel();
	}
}

void AMadRunPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// get a pointer to the controlled pawn
	VehiclePawn = CastChecked<AMadRunPawn>(InPawn);

	// subscribe to the pawn's OnDestroyed delegate
	VehiclePawn->OnDestroyed.AddDynamic(this, &AMadRunPlayerController::OnPawnDestroyed);
}

void AMadRunPlayerController::OnPawnDestroyed(AActor* DestroyedPawn)
{
	// find the player start
	TArray<AActor*> ActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), ActorList);

	if (ActorList.Num() > 0)
	{
		// spawn a vehicle at the player start
		const FTransform SpawnTransform = ActorList[0]->GetActorTransform();

		if (AMadRunPawn* RespawnedVehicle = GetWorld()->SpawnActor<AMadRunPawn>(VehiclePawnClass, SpawnTransform))
		{
			// possess the vehicle
			Possess(RespawnedVehicle);
		}
	}
}

bool AMadRunPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
