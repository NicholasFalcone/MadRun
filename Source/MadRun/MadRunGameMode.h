// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MadRunGameMode.generated.h"

class APlayerController;
class AMadRunRaceCheckpoint;

struct FMadRunRaceProgress
{
	bool bStarted = false;
	bool bFinished = false;
	int32 NextWaypointIndex = 1;
	double StartTime = 0.0;
};

UCLASS(abstract)
class AMadRunGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMadRunGameMode();

	virtual void BeginPlay() override;
	void HandleRaceCheckpoint(AActor* OverlappedActor, AMadRunRaceCheckpoint* Checkpoint);

private:
	void InitializeRaceRoute();
	void UpdateRaceStatus(APlayerController* PlayerController, const FText& Status) const;

	UPROPERTY(Transient)
	TObjectPtr<AMadRunRaceCheckpoint> StartCheckpoint;

	UPROPERTY(Transient)
	TObjectPtr<AMadRunRaceCheckpoint> FinishCheckpoint;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AMadRunRaceCheckpoint>> Waypoints;

	TMap<TWeakObjectPtr<APlayerController>, FMadRunRaceProgress> PlayerProgress;
	bool bRaceRouteValid = false;
};

