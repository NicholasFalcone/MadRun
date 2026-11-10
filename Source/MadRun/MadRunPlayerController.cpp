// Copyright Epic Games, Inc. All Rights Reserved.


#include "MadRunPlayerController.h"
#include "MadRunPawn.h"
#include "MadRunUI.h"
#include "MadRunScavengerPoint.h"
#include "MadRunScavengerWidget.h"
#include "MadRunEquipment.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "MadRun.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AMadRunPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// ensure we're attached to the vehicle pawn so that World Partition streaming works correctly
	bAttachToPawn = true;

	// only spawn UI on local player controllers
	if (IsLocalPlayerController())
	{
		ScavengerWidget = CreateWidget<UMadRunScavengerWidget>(this, UMadRunScavengerWidget::StaticClass());
		if (!ScavengerWidget || !ScavengerWidget->AddToPlayerScreen(10))
		{
			UE_LOG(LogMadRun, Error, TEXT("Could not display scavenging UI."));
			ScavengerWidget = nullptr;
		}
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
	if (!ScavengeAction)
	{
		ScavengingMappingContext = NewObject<UInputMappingContext>(this, TEXT("ScavengingMapping"));
		ScavengeAction = NewObject<UInputAction>(this, TEXT("ScavengeAction"));
		AcceptEquipmentAction = NewObject<UInputAction>(this, TEXT("AcceptEquipmentAction"));
		RejectEquipmentAction = NewObject<UInputAction>(this, TEXT("RejectEquipmentAction"));
		for (UInputAction* Action : { ScavengeAction.Get(), AcceptEquipmentAction.Get(), RejectEquipmentAction.Get() })
		{
			Action->ValueType = EInputActionValueType::Boolean;
		}
		if (ScavengeKey.IsValid() && AcceptEquipmentKey.IsValid() && RejectEquipmentKey.IsValid()
			&& ScavengeKey != AcceptEquipmentKey && ScavengeKey != RejectEquipmentKey && AcceptEquipmentKey != RejectEquipmentKey)
		{
			ScavengingMappingContext->MapKey(ScavengeAction, ScavengeKey);
			ScavengingMappingContext->MapKey(AcceptEquipmentAction, AcceptEquipmentKey);
			ScavengingMappingContext->MapKey(RejectEquipmentAction, RejectEquipmentKey);
		}
		else
		{
			UE_LOG(LogMadRun, Error, TEXT("Scavenging requires three valid, distinct input keys."));
		}
	}
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &AMadRunPlayerController::StartTurretFire);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, this, &AMadRunPlayerController::StopTurretFire);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Canceled, this, &AMadRunPlayerController::StopTurretFire);
		EnhancedInput->BindAction(ScavengeAction, ETriggerEvent::Started, this, &AMadRunPlayerController::StartScavenging);
		EnhancedInput->BindAction(AcceptEquipmentAction, ETriggerEvent::Started, this, &AMadRunPlayerController::AcceptScavengedEquipment);
		EnhancedInput->BindAction(RejectEquipmentAction, ETriggerEvent::Started, this, &AMadRunPlayerController::CancelScavenging);
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
			Subsystem->AddMappingContext(ScavengingMappingContext, 0);
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
	CancelScavenging();
	if (ScavengerWidget)
	{
		ScavengerWidget->RemoveFromParent();
		ScavengerWidget = nullptr;
	}
	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(FireMappingContext);
			Subsystem->RemoveMappingContext(ScavengingMappingContext);
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
	CancelScavenging();
	Super::OnUnPossess();
}

AMadRunScavengerPoint* AMadRunPlayerController::FindNearbyScavengerPoint() const
{
	const AMadRunPawn* Vehicle = Cast<AMadRunPawn>(GetPawn());
	if (!IsValid(Vehicle))
	{
		return nullptr;
	}
	AMadRunScavengerPoint* Closest = nullptr;
	double BestDistance = TNumericLimits<double>::Max();
	for (TActorIterator<AMadRunScavengerPoint> It(GetWorld()); It; ++It)
	{
		AMadRunScavengerPoint* Point = *It;
		if (IsValid(Point) && !Point->IsActorBeingDestroyed() && Point->IsVehicleInRange(Vehicle))
		{
			const double Distance = FVector::DistSquared(Vehicle->GetActorLocation(), Point->GetActorLocation());
			if (Distance < BestDistance)
			{
				Closest = Point;
				BestDistance = Distance;
			}
		}
	}
	return Closest;
}

void AMadRunPlayerController::StartScavenging()
{
	if (!IsLocalPlayerController() || bScavengerPromptOpen || !IsValid(ScavengerWidget))
	{
		return;
	}
	if (AMadRunScavengerPoint* Point = FindNearbyScavengerPoint())
	{
		PendingScavengerPoint = Point;
		PendingEquipment = Point->GetOrSelectEquipment();
		bScavengerPromptOpen = true;
		bScavengerHintVisible = false;
		if (UMadRunEquipment* Offer = PendingEquipment.Get())
		{
			ScavengerWidget->ShowOffer(Offer, AcceptEquipmentKey.GetDisplayName(), RejectEquipmentKey.GetDisplayName());
		}
		else
		{
			ScavengerWidget->ShowFailure(RejectEquipmentKey.GetDisplayName());
		}
	}
}

void AMadRunPlayerController::AcceptScavengedEquipment()
{
	if (!bScavengerPromptOpen)
	{
		return;
	}
	AMadRunScavengerPoint* Point = PendingScavengerPoint.Get();
	AMadRunPawn* Vehicle = Cast<AMadRunPawn>(GetPawn());
	if (!IsValid(Point) || Point->IsActorBeingDestroyed() || !Point->IsVehicleInRange(Vehicle))
	{
		CancelScavenging();
		return;
	}
	if (Point->TryEquip(Vehicle, PendingEquipment.Get()))
	{
		CancelScavenging();
	}
	else if (IsValid(ScavengerWidget))
	{
		ScavengerWidget->ShowFailure(RejectEquipmentKey.GetDisplayName());
	}
}

void AMadRunPlayerController::CancelScavenging()
{
	bScavengerPromptOpen = false;
	bScavengerHintVisible = false;
	PendingScavengerPoint.Reset();
	PendingEquipment.Reset();
	if (IsValid(ScavengerWidget))
	{
		ScavengerWidget->Clear();
	}
}

void AMadRunPlayerController::UpdateRaceStatus(const FText& Status) const
{
	if (IsLocalPlayerController() && IsValid(VehicleUI))
	{
		VehicleUI->UpdateRaceStatus(Status);
	}
}

void AMadRunPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	if (IsLocalPlayerController() && IsValid(ScavengerWidget))
	{
		AMadRunScavengerPoint* Point = PendingScavengerPoint.Get();
		if (bScavengerPromptOpen && (!IsValid(Point) || Point->IsActorBeingDestroyed()
			|| !Point->IsVehicleInRange(Cast<AMadRunPawn>(GetPawn()))))
		{
			CancelScavenging();
		}
		if (!bScavengerPromptOpen)
		{
			const bool bShouldShowHint = FindNearbyScavengerPoint() != nullptr;
			if (bShouldShowHint != bScavengerHintVisible)
			{
				bScavengerHintVisible = bShouldShowHint;
				if (bShouldShowHint)
				{
					ScavengerWidget->ShowHint(ScavengeKey.GetDisplayName());
				}
				else
				{
					ScavengerWidget->Clear();
				}
			}
		}
	}

	if (IsValid(VehicleUI))
	{
		if (IsValid(VehiclePawn) && !VehiclePawn->IsActorBeingDestroyed() && GetPawn() == VehiclePawn)
		{
			VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
			VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
			VehicleUI->UpdateVehicleStatus(VehiclePawn);
		}
		else
		{
			VehicleUI->UpdateVehicleStatus(nullptr);
		}
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
