#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MadRunFuelTank.h"
#include "MadRunPartHealthDisplay.h"
#include "MadRunPartHealthWidget.h"
#include "MadRunWheelBase.h"
#include "MadRunWheelHitActor.h"
#include "OffroadCar/MadRunOffroadCar.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMadRunPartHealthDisplayTest, "MadRun.Vehicle.PartHealthDisplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMadRunPartHealthDisplayTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();

	UClass* CarClass = LoadClass<AMadRunOffroadCar>(nullptr, TEXT("/Game/VehicleTemplate/Blueprints/OffroadCar/BP_OffroadCar_Pawn.BP_OffroadCar_Pawn_C"));
	AMadRunOffroadCar* Car = CarClass ? World->SpawnActor<AMadRunOffroadCar>(CarClass) : nullptr;
	AMadRunFuelTank* Tank = Car ? Car->GetFuelTank() : nullptr;
	UMadRunWheelBase* Wheel = Car ? Car->GetWheelByIndex(0) : nullptr;
	UMadRunWheelBase* OtherWheel = Car ? Car->GetWheelByIndex(1) : nullptr;
	UMadRunPartHealthDisplay* TankDisplay = Tank ? Tank->FindComponentByClass<UMadRunPartHealthDisplay>() : nullptr;
	UMadRunPartHealthDisplay* WheelDisplay = nullptr;
	UMadRunPartHealthDisplay* OtherDisplay = nullptr;
	if (Car)
	{
		TArray<AActor*> Attached;
		Car->GetAttachedActors(Attached, true, true);
		for (AActor* Actor : Attached)
		{
			AMadRunWheelHitActor* HitActor = Cast<AMadRunWheelHitActor>(Actor);
			if (HitActor && HitActor->GetWheel() == Wheel)
			{
				WheelDisplay = HitActor->FindComponentByClass<UMadRunPartHealthDisplay>();
			}
			else if (HitActor && HitActor->GetWheel() == OtherWheel)
			{
				OtherDisplay = HitActor->FindComponentByClass<UMadRunPartHealthDisplay>();
			}
		}
	}

	if (!TestNotNull(TEXT("Tank"), Tank) || !TestNotNull(TEXT("Wheel"), Wheel)
		|| !TestNotNull(TEXT("Tank display"), TankDisplay) || !TestNotNull(TEXT("Wheel display"), WheelDisplay)
		|| !TestNotNull(TEXT("Other wheel display"), OtherDisplay))
	{
		World->BeginTearingDown();
		World->EndPlay(EEndPlayReason::Quit);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	TestFalse(TEXT("Tank display starts hidden"), TankDisplay->IsVisible());
	TestFalse(TEXT("Wheel display starts hidden"), WheelDisplay->IsVisible());
	TestTrue(TEXT("Tank uses a world-space widget"), TankDisplay->GetWidgetSpace() == EWidgetSpace::World);
	TestTrue(TEXT("Wheel uses a world-space widget"), WheelDisplay->GetWidgetSpace() == EWidgetSpace::World);
	UGameplayStatics::ApplyDamage(Tank, 2.0f, nullptr, nullptr, nullptr);
	IMadRunPartHealth::Execute_SetHealth(Wheel, 6);
	TestTrue(TEXT("Tank health change shows its display"), TankDisplay->IsVisible());
	TestTrue(TEXT("Direct wheel health change shows its display"), WheelDisplay->IsVisible());
	TestFalse(TEXT("Other wheel stays hidden"), OtherDisplay->IsVisible());
	TestTrue(TEXT("Tank indicator is above its root in world space"),
		TankDisplay->GetComponentLocation().Equals(TankDisplay->GetAttachParent()->GetComponentLocation() + FVector(0.0f, 0.0f, 80.0f), 0.1f));
	TestTrue(TEXT("Wheel indicator is above its hitbox in world space"),
		WheelDisplay->GetComponentLocation().Equals(WheelDisplay->GetAttachParent()->GetComponentLocation() + FVector(0.0f, 0.0f, 100.0f), 0.1f));
	USceneComponent* WheelParent = WheelDisplay->GetAttachParent();
	const FTransform PreviousTransform = WheelParent->GetComponentTransform();
	WheelParent->SetWorldRotation(FRotator(60.0f, 35.0f, 90.0f));
	WheelDisplay->TickComponent(0.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Wheel indicator stays above a rotating wheel"),
		WheelDisplay->GetComponentLocation().Equals(WheelParent->GetComponentLocation() + FVector(0.0f, 0.0f, 100.0f), 0.1f));
	WheelParent->SetWorldTransform(PreviousTransform);
	WheelDisplay->TickComponent(0.0f, LEVELTICK_All, nullptr);

	UMadRunPartHealthWidget* Widget = Cast<UMadRunPartHealthWidget>(WheelDisplay->GetUserWidgetObject());
	UTextBlock* Label = Widget ? Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("HealthLabel"))) : nullptr;
	UProgressBar* HealthBar = Widget ? Cast<UProgressBar>(Widget->GetWidgetFromName(TEXT("HealthBar"))) : nullptr;
	if (TestNotNull(TEXT("Health label"), Label) && TestNotNull(TEXT("Health bar"), HealthBar))
	{
		TestTrue(TEXT("Label displays current and max health"), Label->GetText().ToString().Contains(TEXT("6 / 10")));
		TestTrue(TEXT("Progress bar displays health fraction"), FMath::IsNearlyEqual(HealthBar->GetPercent(), 0.6f));
		TestTrue(TEXT("Progress bar does not intercept input"), HealthBar->GetVisibility() == ESlateVisibility::HitTestInvisible);
	}

	int32 Step = 0;
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, World, Car, Tank, Wheel, TankDisplay, WheelDisplay, OtherDisplay, Step]() mutable
	{
		FTimerManager& Timers = World->GetTimerManager();
		switch (Step++)
		{
		case 0:
			Timers.Tick(0.0f);
			break;
		case 1:
			Timers.Tick(2.0f);
			TestTrue(TEXT("Display remains visible before three seconds"), TankDisplay->IsVisible());
			IMadRunPartHealth::Execute_DoRepair(Tank);
			IMadRunPartHealth::Execute_TakeDamage(Wheel, 1);
			break;
		case 2:
			Timers.Tick(1.1f);
			TestTrue(TEXT("Repair resets tank timer"), TankDisplay->IsVisible());
			TestTrue(TEXT("Further wheel damage resets wheel timer"), WheelDisplay->IsVisible());
			TestFalse(TEXT("Unchanged wheel remains hidden"), OtherDisplay->IsVisible());
			IMadRunPartHealth::Execute_SetHealth(Tank, 10);
			break;
		case 3:
			Timers.Tick(1.8f);
			TestTrue(TEXT("Tank remains visible just before renewed timer ends"), TankDisplay->IsVisible());
			break;
		default:
			Timers.Tick(0.2f);
			TestFalse(TEXT("Tank hides three seconds after actual change"), TankDisplay->IsVisible());
			TestFalse(TEXT("Wheel hides three seconds after last damage"), WheelDisplay->IsVisible());
			IMadRunPartHealth::Execute_DoRepair(Wheel);
			TestTrue(TEXT("Wheel repair shows display again"), WheelDisplay->IsVisible());
			Car->Destroy();
			World->BeginTearingDown();
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			return true;
		}
		return false;
	}));
	return true;
}

#endif
