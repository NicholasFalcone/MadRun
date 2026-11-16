// Copyright Epic Games, Inc. All Rights Reserved.

#include "MadRunCopilot.h"
#include "MadRunCopilotData.h"
#include "MadRunCopilotRepairComponent.h"
#include "MadRunCopilotCombatComponent.h"
#include "MadRunCopilotScavengingComponent.h"
#include "MadRunPawn.h"
#include "MadRun.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"

ACopilot::ACopilot()
{
	PrimaryActorTick.bCanEverTick = true;

	// Don't rotate character with camera
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	GetCharacterMovement()->MaxWalkSpeedCrouched = 300.0f;

	// Initialize components
	RepairComponent = CreateDefaultSubobject<UCopilotRepairComponent>(TEXT("RepairComponent"));
	CombatComponent = CreateDefaultSubobject<UCopilotCombatComponent>(TEXT("CombatComponent"));
	ScavengingComponent = CreateDefaultSubobject<UCopilotScavengingComponent>(TEXT("ScavengingComponent"));

	// Set default stats
	CopilotStats.MechanicsSkill = 65.0f;   // Good at repairs
	CopilotStats.AccuracySkill = 75.0f;    // Good at shooting
	CopilotStats.ScavengingSkill = 60.0f;  // Decent at scavenging
}

void ACopilot::PostInitializeComponents()
{
	const bool bInVehicle = Cast<AMadRunPawn>(GetParentActor()) != nullptr;
	if (bInVehicle)
	{
		AutoPossessAI = EAutoPossessAI::Disabled;
		AutoPossessPlayer = EAutoReceiveInput::Disabled;
	}

	Super::PostInitializeComponents();

	if (bInVehicle)
	{
		SetActorEnableCollision(false);
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->Deactivate();
	}
}

void ACopilot::BeginPlay()
{
	Super::BeginPlay();

	InitializeComponents();
}

void ACopilot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool ACopilot::ApplyCopilotData(const UMadRunCopilotData* Data)
{
	if (!IsValid(Data) || !IsValid(Data->BodyMesh))
	{
		UE_LOG(LogMadRun, Error, TEXT("Copilot '%s' requires a valid data asset with a body skeletal mesh."),
			*GetNameSafe(this));
		return false;
	}

	const FCopilotStats& Stats = Data->Stats;
	for (const float Skill : { Stats.MechanicsSkill, Stats.AccuracySkill, Stats.ScavengingSkill })
	{
		if (!FMath::IsFinite(Skill) || Skill < 0.0f || Skill > 100.0f)
		{
			UE_LOG(LogMadRun, Error, TEXT("Copilot data '%s' has an invalid skill; skills must be finite and between 0 and 100."),
				*GetNameSafe(Data));
			return false;
		}
	}

	GetMesh()->SetSkeletalMesh(Data->BodyMesh);
	SetCopilotStats(Stats);
	return true;
}

void ACopilot::InitializeComponents()
{
	// Recalculate derived stats
	CopilotStats.RecalculateDerivedStats();

	// Initialize components with stats
	if (RepairComponent)
	{
		RepairComponent->Initialize(CopilotStats);
	}

	if (CombatComponent)
	{
		CombatComponent->Initialize(CopilotStats);
	}

	if (ScavengingComponent)
	{
		ScavengingComponent->Initialize(CopilotStats);
	}

	UE_LOG(LogTemp, Log,
		TEXT("ACopilot::InitializeComponents - Mechanics: %.1f, Accuracy: %.1f, Scavenging: %.1f"),
		CopilotStats.MechanicsSkill, CopilotStats.AccuracySkill, CopilotStats.ScavengingSkill);
}

void ACopilot::SetCopilotStats(const FCopilotStats& NewStats)
{
	CopilotStats = NewStats;
	InitializeComponents();
}

void ACopilot::SetMechanicsSkill(float NewSkill)
{
	CopilotStats.MechanicsSkill = FMath::Clamp(NewSkill, 0.0f, 100.0f);
	CopilotStats.RecalculateDerivedStats();

	if (RepairComponent)
	{
		RepairComponent->Initialize(CopilotStats);
	}
}

void ACopilot::SetAccuracySkill(float NewSkill)
{
	CopilotStats.AccuracySkill = FMath::Clamp(NewSkill, 0.0f, 100.0f);
	CopilotStats.RecalculateDerivedStats();

	if (CombatComponent)
	{
		CombatComponent->Initialize(CopilotStats);
	}
}

void ACopilot::SetScavengingSkill(float NewSkill)
{
	CopilotStats.ScavengingSkill = FMath::Clamp(NewSkill, 0.0f, 100.0f);
	CopilotStats.RecalculateDerivedStats();

	if (ScavengingComponent)
	{
		ScavengingComponent->Initialize(CopilotStats);
	}
}
