// Copyright Epic Games, Inc. All Rights Reserved.

#include "MadRunRaceCheckpoint.h"
#include "MadRunGameMode.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

AMadRunRaceCheckpoint::AMadRunRaceCheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(500.0f, 500.0f, 300.0f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionObjectType(ECC_WorldDynamic);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->SetHiddenInGame(true);
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AMadRunRaceCheckpoint::HandleOverlapBegin);

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Trigger);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMadRunRaceCheckpoint::HandleOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Log, TEXT("Overlap detected with actor: %s"), *OtherActor->GetName());
	if (AMadRunGameMode* GameMode = GetWorld()->GetAuthGameMode<AMadRunGameMode>())
	{
		GameMode->HandleRaceCheckpoint(OtherActor, this);
	}
}
