// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MadRunRaceCheckpoint.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EMadRunRaceCheckpointType : uint8
{
	Start,
	Waypoint,
	Finish
};

UCLASS(Blueprintable)
class AMadRunRaceCheckpoint : public AActor
{
	GENERATED_BODY()

public:
	AMadRunRaceCheckpoint();

	EMadRunRaceCheckpointType GetCheckpointType() const { return CheckpointType; }
	int32 GetWaypointIndex() const { return WaypointIndex; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Race", meta=(AllowPrivateAccess="true"))
	EMadRunRaceCheckpointType CheckpointType = EMadRunRaceCheckpointType::Waypoint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Race", meta=(AllowPrivateAccess="true", EditCondition="CheckpointType == EMadRunRaceCheckpointType::Waypoint", ClampMin="1"))
	int32 WaypointIndex = 1;

	UFUNCTION()
	void HandleOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);
};
