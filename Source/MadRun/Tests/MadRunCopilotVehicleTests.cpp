#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MadRunCopilot.h"
#include "MadRunCopilotData.h"
#include "MadRunCopilotCombatComponent.h"
#include "MadRunCopilotRepairComponent.h"
#include "MadRunCopilotScavengingComponent.h"
#include "OffroadCar/MadRunOffroadCar.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMadRunCopilotVehicleTest, "MadRun.Vehicle.Copilot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMadRunCopilotVehicleTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("World"), World))
	{
		return false;
	}
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();

	UClass* CarClass = LoadClass<AMadRunOffroadCar>(nullptr,
		TEXT("/Game/VehicleTemplate/Blueprints/OffroadCar/BP_OffroadCar_Pawn.BP_OffroadCar_Pawn_C"));
	AMadRunOffroadCar* Car = CarClass ? World->SpawnActor<AMadRunOffroadCar>(CarClass) : nullptr;
	if (TestNotNull(TEXT("Blueprint vehicle"), Car))
	{
		ACopilot* Copilot = Car->GetCopilot();
		if (TestNotNull(TEXT("Vehicle has a copilot"), Copilot))
		{
			UChildActorComponent* Component = Copilot->GetParentComponent();
			if (TestNotNull(TEXT("Copilot child component"), Component))
			{
				TestTrue(TEXT("Component is attached to vehicle mesh"), Component->GetAttachParent() == Car->GetMesh());
				TestTrue(TEXT("Child belongs to vehicle"), Copilot->GetParentActor() == Car);
				TestFalse(TEXT("Mounted copilot collision disabled"), Copilot->GetActorEnableCollision());
				TestFalse(TEXT("Mounted copilot movement inactive"), Copilot->GetCharacterMovement()->IsActive());
				TestNull(TEXT("Mounted copilot has no controller"), Copilot->GetController());
				Component->SetRelativeLocation(FVector(40.0, 25.0, 100.0));
				TestTrue(TEXT("Copilot follows seat position"),
					Copilot->GetActorLocation().Equals(Component->GetComponentLocation(), 0.1));
				Car->SetActorLocation(FVector(500.0, 300.0, 100.0), false, nullptr, ETeleportType::TeleportPhysics);
				TestTrue(TEXT("Copilot follows vehicle movement"),
					Copilot->GetActorLocation().Equals(Component->GetComponentLocation(), 0.1));
			}
			Car->Destroy();
			TestFalse(TEXT("Destroying vehicle destroys its copilot"), IsValid(Copilot));
		}
	}

	USkeletalMesh* BodyMesh = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/Vehicles/OffroadCar/SKM_Offroad.SKM_Offroad"));
	UMadRunCopilotData* Data = NewObject<UMadRunCopilotData>(World);
	Data->BodyMesh = BodyMesh;
	Data->Stats.MechanicsSkill = 80.0f;
	Data->Stats.AccuracySkill = 60.0f;
	Data->Stats.ScavengingSkill = 20.0f;
	if (TestNotNull(TEXT("Test skeletal mesh"), BodyMesh) && TestNotNull(TEXT("Vehicle class"), CarClass))
	{
		const FTransform Transform(FVector(3000.0, 0.0, 100.0));
		AMadRunOffroadCar* InitialCar = World->SpawnActorDeferred<AMadRunOffroadCar>(CarClass, Transform);
		FObjectPropertyBase* InitialProperty = FindFProperty<FObjectPropertyBase>(AMadRunPawn::StaticClass(), TEXT("InitialCopilotData"));
		if (TestNotNull(TEXT("Deferred vehicle"), InitialCar) && TestNotNull(TEXT("Initial copilot data slot"), InitialProperty))
		{
			InitialProperty->SetObjectPropertyValue_InContainer(InitialCar, Data);
			InitialCar->FinishSpawning(Transform);
			ACopilot* InitialCopilot = InitialCar->GetCopilot();
			if (TestNotNull(TEXT("Configured copilot"), InitialCopilot))
			{
				TestTrue(TEXT("Body mesh applied"), InitialCopilot->GetMesh()->GetSkeletalMeshAsset() == BodyMesh);
				const FCopilotStats& Stats = InitialCopilot->GetCopilotStats();
				TestEqual(TEXT("Mechanics skill applied"), Stats.MechanicsSkill, 80.0f);
				TestEqual(TEXT("Accuracy skill applied"), Stats.AccuracySkill, 60.0f);
				TestEqual(TEXT("Scavenging skill applied"), Stats.ScavengingSkill, 20.0f);
				TestTrue(TEXT("Repair speed recalculated"), FMath::IsNearlyEqual(Stats.RepairSpeedMultiplier, 1.4f));
				InitialCopilot->RepairComponent->StartRepair(InitialCar, 10.0f);
				InitialCopilot->RepairComponent->TickComponent(1.0f, LEVELTICK_All, nullptr);
				TestTrue(TEXT("Repair component uses asset skill"),
					FMath::IsNearlyEqual(InitialCopilot->RepairComponent->GetRepairProgress(), 0.14f));
				InitialCopilot->RepairComponent->CancelRepair();
				TestTrue(TEXT("Combat component updated"),
					FMath::IsNearlyEqual(InitialCopilot->CombatComponent->GetCurrentAccuracyDeviation(), 7.0f));
				TestTrue(TEXT("Scavenging component updated"),
					FMath::IsNearlyEqual(InitialCopilot->ScavengingComponent->GetLootFindChanceModifier(), 0.5f));

				UMadRunCopilotData* InvalidData = NewObject<UMadRunCopilotData>(World);
				AddExpectedError(TEXT("requires a valid data asset with a body skeletal mesh"), EAutomationExpectedErrorFlags::Contains, 1);
				TestFalse(TEXT("Reject asset without mesh"), InitialCopilot->ApplyCopilotData(InvalidData));
				InvalidData->BodyMesh = BodyMesh;
				InvalidData->Stats.MechanicsSkill = 101.0f;
				AddExpectedError(TEXT("has an invalid skill"), EAutomationExpectedErrorFlags::Contains, 1);
				TestFalse(TEXT("Reject invalid skill"), InitialCopilot->ApplyCopilotData(InvalidData));
				TestTrue(TEXT("Invalid asset leaves mesh unchanged"), InitialCopilot->GetMesh()->GetSkeletalMeshAsset() == BodyMesh);
				TestEqual(TEXT("Invalid asset leaves stats unchanged"), InitialCopilot->GetCopilotStats().MechanicsSkill, 80.0f);
			}
		}
	}

	ACopilot* Standalone = World->SpawnActor<ACopilot>(FVector(2000.0, 0.0, 100.0), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("Standalone copilot"), Standalone))
	{
		TestTrue(TEXT("Standalone copilot retains collision"), Standalone->GetActorEnableCollision());
		TestTrue(TEXT("Standalone copilot retains movement"), Standalone->GetCharacterMovement()->IsActive());
	}
	World->BeginTearingDown();
	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
