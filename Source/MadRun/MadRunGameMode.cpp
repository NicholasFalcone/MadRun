// Copyright Epic Games, Inc. All Rights Reserved.

#include "MadRunGameMode.h"
#include "MadRunPlayerController.h"
#include "MadRunPawn.h"
#include "MadRunRaceCheckpoint.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "MadRun.h"

#define LOCTEXT_NAMESPACE "MadRunRace"

AMadRunGameMode::AMadRunGameMode()
{
	PlayerControllerClass = AMadRunPlayerController::StaticClass();
}

void AMadRunGameMode::BeginPlay()
{
	Super::BeginPlay();
	InitializeRaceRoute();
}

void AMadRunGameMode::InitializeRaceRoute()
{
	StartCheckpoint = nullptr;
	FinishCheckpoint = nullptr;
	Waypoints.Reset();
	PlayerProgress.Reset();
	bRaceRouteValid = false;

	TMap<int32, TObjectPtr<AMadRunRaceCheckpoint>> WaypointsByIndex;
	bool bHasInvalidCheckpoint = false;
	for (TActorIterator<AMadRunRaceCheckpoint> It(GetWorld()); It; ++It)
	{
		UE_LOG(LogMadRun, Log, TEXT("Found checkpoint: %s"), *It->GetName());
		AMadRunRaceCheckpoint* Checkpoint = *It;
		UE_LOG(LogMadRun, Log, TEXT("Checkpoint type: %d"), static_cast<int32>(Checkpoint->GetCheckpointType()));
		switch (Checkpoint->GetCheckpointType())
		{
		case EMadRunRaceCheckpointType::Start:
			if (StartCheckpoint)
			{
				UE_LOG(LogMadRun, Error, TEXT("Race route has more than one start checkpoint."));
				bHasInvalidCheckpoint = true;
			}
			else
			{
				StartCheckpoint = Checkpoint;
			}
			break;
		case EMadRunRaceCheckpointType::Waypoint:
			if (Checkpoint->GetWaypointIndex() < 1 || WaypointsByIndex.Contains(Checkpoint->GetWaypointIndex()))
			{
				UE_LOG(LogMadRun, Error, TEXT("Race route has an invalid or duplicate waypoint index: %d."),
					Checkpoint->GetWaypointIndex());
				bHasInvalidCheckpoint = true;
			}
			else
			{
				WaypointsByIndex.Add(Checkpoint->GetWaypointIndex(), Checkpoint);
			}
			break;
		case EMadRunRaceCheckpointType::Finish:
			if (FinishCheckpoint)
			{
				UE_LOG(LogMadRun, Error, TEXT("Race route has more than one finish checkpoint."));
				bHasInvalidCheckpoint = true;
			}
			else
			{
				FinishCheckpoint = Checkpoint;
			}
			break;
		}
	}

	if (!StartCheckpoint || !FinishCheckpoint)
	{
		UE_LOG(LogMadRun, Warning, TEXT("Race route requires exactly one start and one finish checkpoint."));
		return;
	}

	WaypointsByIndex.GenerateValueArray(Waypoints);
	Waypoints.Sort([](const AMadRunRaceCheckpoint& A, const AMadRunRaceCheckpoint& B)
	{
		return A.GetWaypointIndex() < B.GetWaypointIndex();
	});

	for (int32 Index = 0; Index < Waypoints.Num(); ++Index)
	{
		if (!Waypoints[Index] || Waypoints[Index]->GetWaypointIndex() != Index + 1)
		{
			UE_LOG(LogMadRun, Error, TEXT("Race waypoint indices must be consecutive, starting at 1."));
			bHasInvalidCheckpoint = true;
			break;
		}
	}

	if (!bHasInvalidCheckpoint)
	{
		bRaceRouteValid = true;
		UE_LOG(LogMadRun, Log, TEXT("Race route ready with %d waypoint(s)."), Waypoints.Num());
	}
}

void AMadRunGameMode::HandleRaceCheckpoint(AActor* OverlappedActor, AMadRunRaceCheckpoint* Checkpoint)
{
	if (!bRaceRouteValid || !IsValid(Checkpoint))
	{
		return;
	}

	AMadRunPawn* Vehicle = Cast<AMadRunPawn>(OverlappedActor);
	APlayerController* PlayerController = Vehicle ? Cast<APlayerController>(Vehicle->GetController()) : nullptr;
	if (!PlayerController)
	{
		return;
	}

	FMadRunRaceProgress& Progress = PlayerProgress.FindOrAdd(PlayerController);
	if (Checkpoint == StartCheckpoint)
	{
		if (Progress.bStarted || Progress.bFinished)
		{
			return;
		}

		Progress.bStarted = true;
		Progress.StartTime = GetWorld()->GetTimeSeconds();
		UpdateRaceStatus(PlayerController, FText::Format(
			LOCTEXT("RaceStarted", "Gara iniziata - waypoint: 0/{0}"), FText::AsNumber(Waypoints.Num())));
		return;
	}

	if (!Progress.bStarted || Progress.bFinished)
	{
		return;
	}

	if (Checkpoint->GetCheckpointType() == EMadRunRaceCheckpointType::Waypoint)
	{
		if (!Waypoints.IsValidIndex(Progress.NextWaypointIndex - 1)
			|| Waypoints[Progress.NextWaypointIndex - 1] != Checkpoint)
		{
			UpdateRaceStatus(PlayerController, FText::Format(
				LOCTEXT("WrongWaypoint", "Waypoint errato - prossimo: {0}/{1}"),
				FText::AsNumber(Progress.NextWaypointIndex), FText::AsNumber(Waypoints.Num())));
			return;
		}

		++Progress.NextWaypointIndex;
		UpdateRaceStatus(PlayerController, FText::Format(
			LOCTEXT("WaypointReached", "Waypoint: {0}/{1}"),
			FText::AsNumber(Progress.NextWaypointIndex - 1), FText::AsNumber(Waypoints.Num())));
		return;
	}

	if (Checkpoint == FinishCheckpoint && Progress.NextWaypointIndex == Waypoints.Num() + 1)
	{
		Progress.bFinished = true;
		const double ElapsedTime = GetWorld()->GetTimeSeconds() - Progress.StartTime;
		FNumberFormattingOptions TimeOptions;
		TimeOptions.SetUseGrouping(false);
		TimeOptions.SetMinimumFractionalDigits(2);
		TimeOptions.SetMaximumFractionalDigits(2);
		UpdateRaceStatus(PlayerController, FText::Format(
			LOCTEXT("RaceFinished", "Gara completata! Tempo: {0} s"),
			FText::AsNumber(ElapsedTime, &TimeOptions)));
	}
	else
	{
		UpdateRaceStatus(PlayerController, FText::Format(
			LOCTEXT("WaypointsRemaining", "Completa prima tutti i waypoint: {0}/{1}"),
			FText::AsNumber(Progress.NextWaypointIndex - 1), FText::AsNumber(Waypoints.Num())));
	}
}

void AMadRunGameMode::UpdateRaceStatus(APlayerController* PlayerController, const FText& Status) const
{
	if (AMadRunPlayerController* MadRunController = Cast<AMadRunPlayerController>(PlayerController))
	{
		MadRunController->UpdateRaceStatus(Status);
	}
}

#undef LOCTEXT_NAMESPACE
