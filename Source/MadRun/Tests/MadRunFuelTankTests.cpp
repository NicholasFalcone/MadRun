#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MadRunFuelTank.h"
#include "MadRunWheelBase.h"
#include "OffroadCar/MadRunOffroadCar.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMadRunFuelTankTest, "MadRun.Vehicle.FuelTank",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMadRunFuelTankTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
	AMadRunFuelTank* Tank = World->SpawnActor<AMadRunFuelTank>();
	if (!TestNotNull(TEXT("Spawned fuel tank actor"), Tank))
	{
		World->BeginTearingDown();
		World->EndPlay(EEndPlayReason::Quit);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	TestEqual(TEXT("Tank starts full"), Tank->GetFuel(), 100.0f);
	Tank->AddFuel(200.0f);
	TestEqual(TEXT("Refueling is limited by capacity"), Tank->GetFuel(), 100.0f);

	Tank->ConsumeFuel(10.0f, 0.0f);
	TestEqual(TEXT("Healthy tank does not consume fuel at idle"), Tank->GetFuel(), 100.0f);
	Tank->ConsumeFuel(10.0f, 0.5f);
	TestEqual(TEXT("Half throttle consumes half the full-throttle rate"), Tank->GetFuel(), 99.0f);
	Tank->ConsumeFuel(10.0f, 1.0f);
	TestEqual(TEXT("Full throttle consumes the configured rate"), Tank->GetFuel(), 97.0f);

	IMadRunPartHealth::Execute_TakeDamage(Tank, 5);
	Tank->ConsumeFuel(10.0f, 0.0f);
	TestEqual(TEXT("Half damage leaks at half the maximum rate"), Tank->GetFuel(), 96.5f);
	IMadRunPartHealth::Execute_SetHealth(Tank, 0);
	Tank->ConsumeFuel(10.0f, 1.0f);
	TestEqual(TEXT("Consumption and maximum leak add up"), Tank->GetFuel(), 93.5f);
	IMadRunPartHealth::Execute_DoRepair(Tank);
	TestEqual(TEXT("Repair restores tank health"), IMadRunPartHealth::Execute_GetHealth(Tank), 10);
	Tank->ConsumeFuel(1000.0f, 1.0f);
	TestEqual(TEXT("Fuel cannot go below zero"), Tank->GetFuel(), 0.0f);
	Tank->AddFuel(1.0f);
	TestEqual(TEXT("Refueling after empty restores fuel"), Tank->GetFuel(), 1.0f);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, FVector(-100.0f, 0.0f, 0.0f),
		FVector(100.0f, 0.0f, 0.0f), ECC_Visibility);
	TestTrue(TEXT("Visibility trace hits the tank"), bHit && Hit.GetActor() == Tank);
	UGameplayStatics::ApplyPointDamage(Tank, 2.0f, FVector::ForwardVector, Hit, nullptr, nullptr, nullptr);
	TestEqual(TEXT("Actor point damage reduces health"), IMadRunPartHealth::Execute_GetHealth(Tank), 8);
	Tank->ConsumeFuel(10.0f, 0.0f);
	TestTrue(TEXT("Actor damage causes a fuel leak"), FMath::IsNearlyEqual(Tank->GetFuel(), 0.8f));
	UGameplayStatics::ApplyDamage(Tank, 0.5f, nullptr, nullptr, nullptr);
	TestEqual(TEXT("Fractional actor damage rounds up"), IMadRunPartHealth::Execute_GetHealth(Tank), 7);

	UMadRunWheelBase* Wheel = NewObject<UMadRunWheelBase>();
	IMadRunPartHealth::Execute_TakeDamage(Wheel, 4);
	TestEqual(TEXT("Wheel uses the shared health interface"), IMadRunPartHealth::Execute_GetHealth(Wheel), 6);
	IMadRunPartHealth::Execute_DoRepair(Wheel);
	TestEqual(TEXT("Wheel repair restores maximum health"), IMadRunPartHealth::Execute_GetHealth(Wheel), 10);
	UClass* CarClass = LoadClass<AMadRunOffroadCar>(nullptr, TEXT("/Game/VehicleTemplate/Blueprints/OffroadCar/BP_OffroadCar_Pawn.BP_OffroadCar_Pawn_C"));
	AMadRunOffroadCar* Car = CarClass
		? World->SpawnActor<AMadRunOffroadCar>(CarClass, FVector(1000.0f, 0.0f, 0.0f), FRotator::ZeroRotator)
		: nullptr;
	if (TestNotNull(TEXT("Spawned OffRoad car"), Car))
	{
		AMadRunFuelTank* ChildTank = Car->GetFuelTank();
		if (TestNotNull(TEXT("Vehicle owns a fuel tank actor"), ChildTank))
		{
			TestTrue(TEXT("Tank is attached to vehicle mesh"), ChildTank->GetAttachParentActor() == Car);
			TestEqual(TEXT("Vehicle tank starts full"), ChildTank->GetFuel(), 100.0f);
			Car->Destroy();
			TestFalse(TEXT("Tank is destroyed with its vehicle"), IsValid(ChildTank));
		}
	}
	World->BeginTearingDown();
	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
