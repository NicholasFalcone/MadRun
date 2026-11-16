// Copyright Epic Games, Inc. All Rights Reserved.

#include "MadRunCopilotRepairComponent.h"
#include "MadRunCopilotStats.h"

UCopilotRepairComponent::UCopilotRepairComponent()
{
	PrimaryComponentTick.TickInterval = 0.1f;
	PrimaryComponentTick.bCanEverTick = true;
}

void UCopilotRepairComponent::BeginPlay()
{
	Super::BeginPlay();

	// Ensure tick is disabled until we're repairing
	SetComponentTickEnabled(false);
}

void UCopilotRepairComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentState != ERepairState::Repairing || !CurrentTarget)
	{
		return;
	}

	ElapsedRepairTime += DeltaTime;

	// Check if repair is complete
	if (ElapsedRepairTime >= AdjustedRepairTime)
	{
		CompleteRepair(true);
	}
}

void UCopilotRepairComponent::Initialize(const FCopilotStats& InStats)
{
	CopilotStats = InStats;
}

void UCopilotRepairComponent::StartRepair(AActor* InTarget, float BaseRepairTime, float InMaxRepairAmount)
{
	if (!InTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCopilotRepairComponent::StartRepair - Invalid target"));
		return;
	}

	// Cancel any ongoing repair
	if (CurrentState == ERepairState::Repairing)
	{
		CancelRepair();
	}

	CurrentTarget = InTarget;
	CurrentState = ERepairState::Repairing;
	TotalRepairTime = BaseRepairTime;
	MaxRepairAmount = InMaxRepairAmount;
	ElapsedRepairTime = 0.0f;

	// Apply copilot's mechanics skill to adjust repair time
	AdjustedRepairTime = CopilotStats.GetAdjustedRepairTime(BaseRepairTime);

	// Enable ticking
	SetComponentTickEnabled(true);

	// Broadcast event
	OnRepairStarted.Broadcast(CurrentTarget, AdjustedRepairTime);

	UE_LOG(LogTemp, Log,
		TEXT("UCopilotRepairComponent::StartRepair - Target: %s, Base Time: %.2f, Adjusted Time: %.2f"),
		*InTarget->GetName(), TotalRepairTime, AdjustedRepairTime);
}

void UCopilotRepairComponent::CancelRepair()
{
	if (CurrentState != ERepairState::Repairing)
	{
		return;
	}

	const float AmountRepaired = (ElapsedRepairTime / AdjustedRepairTime) * MaxRepairAmount;
	CurrentState = ERepairState::Cancelled;

	// Disable ticking
	SetComponentTickEnabled(false);

	// Broadcast event
	OnRepairCancelled.Broadcast(CurrentTarget, AmountRepaired);

	UE_LOG(LogTemp, Log, TEXT("UCopilotRepairComponent::CancelRepair - Repair cancelled, %.2f%% complete"), 
		GetRepairProgress() * 100.0f);

	CurrentTarget = nullptr;
}

float UCopilotRepairComponent::GetRepairProgress() const
{
	if (AdjustedRepairTime <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(ElapsedRepairTime / AdjustedRepairTime, 0.0f, 1.0f);
}

void UCopilotRepairComponent::CompleteRepair(bool bSuccess)
{
	CurrentState = ERepairState::Completed;
	SetComponentTickEnabled(false);

	OnRepairCompleted.Broadcast(CurrentTarget, bSuccess);

	UE_LOG(LogTemp, Log, TEXT("UCopilotRepairComponent::CompleteRepair - Target: %s, Success: %d"),
		*CurrentTarget->GetName(), bSuccess);

	CurrentTarget = nullptr;
}
