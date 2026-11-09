#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MadRunFuelTank.h"
#include "MadRunUI.h"
#include "MadRunWheelBase.h"
#include "MadRunWheelHitActor.h"
#include "MadRunProjectile.h"
#include "MadRunCollision.h"
#include "OffroadCar/MadRunOffroadCar.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
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
	TestNotNull(TEXT("Static mesh is the tank root"), Cast<UStaticMeshComponent>(Tank->GetRootComponent()));
	UBoxComponent* Hitbox = Tank->FindComponentByClass<UBoxComponent>();
	TestTrue(TEXT("Hitbox is attached to the mesh root"), Hitbox && Hitbox->GetAttachParent() == Tank->GetRootComponent());
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
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AMadRunProjectile* Projectile = World->SpawnActor<AMadRunProjectile>(
			AMadRunProjectile::StaticClass(), FVector(-120.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParams);
		if (TestNotNull(TEXT("Projectile spawned for fuel tank"), Projectile))
		{
			for (int32 Frame = 0; Frame < 8 && IsValid(Projectile); ++Frame)
			{
				World->Tick(LEVELTICK_All, 1.0f / 60.0f);
			}
			TestFalse(TEXT("Projectile is destroyed on tank impact"), IsValid(Projectile));
			TestEqual(TEXT("Projectile impact damages fuel tank"), IMadRunPartHealth::Execute_GetHealth(Tank), 2);
		}
	}

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
		UMadRunWheelBase* RuntimeWheel = Car->GetWheelByIndex(0);
		UMadRunWheelBase* OtherWheel = Car->GetWheelByIndex(1);
		if (TestNotNull(TEXT("Front wheel supports damage"), RuntimeWheel)
			&& TestNotNull(TEXT("Other front wheel is available"), OtherWheel))
		{
			IMadRunPartHealth::Execute_TakeDamage(RuntimeWheel, 4);
			TestEqual(TEXT("Damage is applied to the selected wheel"), IMadRunPartHealth::Execute_GetHealth(RuntimeWheel), 6);
			TestEqual(TEXT("Other wheel is unaffected"), IMadRunPartHealth::Execute_GetHealth(OtherWheel), 10);
			IMadRunPartHealth::Execute_TakeDamage(RuntimeWheel, 20);
			TestEqual(TEXT("Destroyed wheel health clamps to zero"), IMadRunPartHealth::Execute_GetHealth(RuntimeWheel), 0);
			IMadRunPartHealth::Execute_TakeDamage(RuntimeWheel, 1);
			TestEqual(TEXT("Damage on a broken wheel stays at zero"), IMadRunPartHealth::Execute_GetHealth(RuntimeWheel), 0);
			IMadRunPartHealth::Execute_DoRepair(RuntimeWheel);
			TestEqual(TEXT("Repair restores the wheel"), IMadRunPartHealth::Execute_GetHealth(RuntimeWheel), 10);
			IMadRunPartHealth::Execute_SetHealth(RuntimeWheel, 0);
			IMadRunPartHealth::Execute_SetHealth(RuntimeWheel, 1);
			TestEqual(TEXT("Restoring health above zero repairs the wheel"), IMadRunPartHealth::Execute_GetHealth(RuntimeWheel), 1);
			IMadRunPartHealth::Execute_DoRepair(RuntimeWheel);

			TArray<AActor*> AttachedActors;
			Car->GetAttachedActors(AttachedActors, true, true);
			TArray<AMadRunWheelHitActor*> WheelHits;
			for (AActor* Attached : AttachedActors)
			{
				if (AMadRunWheelHitActor* HitActor = Cast<AMadRunWheelHitActor>(Attached))
				{
					WheelHits.Add(HitActor);
				}
			}
			TestEqual(TEXT("Four hit actors are attached to the vehicle"), WheelHits.Num(), 4);
			for (int32 WheelIndex = 0; WheelIndex < 4; ++WheelIndex)
			{
				UMadRunWheelBase* Target = Car->GetWheelByIndex(WheelIndex);
				AMadRunWheelHitActor* TargetHit = nullptr;
				for (AMadRunWheelHitActor* HitActor : WheelHits)
				{
					if (HitActor->GetWheel() == Target)
					{
						TargetHit = HitActor;
					}
				}
				if (!TestNotNull(FString::Printf(TEXT("Hit actor for wheel %d"), WheelIndex), TargetHit))
				{
					continue;
				}
				USphereComponent* Shape = TargetHit->FindComponentByClass<USphereComponent>();
				if (!TestNotNull(TEXT("Wheel collision sphere"), Shape))
				{
					continue;
				}
				const FVector DirectionOut = Car->GetActorRightVector() * (WheelIndex % 2 == 0 ? -1.0f : 1.0f);
				const FVector Start = Shape->GetComponentLocation() + DirectionOut * (Shape->GetScaledSphereRadius() + 15.0f);
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AMadRunProjectile* Projectile = World->SpawnActor<AMadRunProjectile>(
					AMadRunProjectile::StaticClass(), Start, (-DirectionOut).Rotation(), SpawnParams);
				if (TestNotNull(TEXT("Projectile spawned for mounted wheel"), Projectile))
				{
					for (int32 Frame = 0; Frame < 8 && IsValid(Projectile); ++Frame)
					{
						World->Tick(LEVELTICK_All, 1.0f / 60.0f);
					}
					TestFalse(FString::Printf(TEXT("Projectile hits mounted wheel %d"), WheelIndex), IsValid(Projectile));
					TestEqual(FString::Printf(TEXT("Mounted wheel %d receives damage"), WheelIndex),
						IMadRunPartHealth::Execute_GetHealth(Target), 5);
					IMadRunPartHealth::Execute_DoRepair(Target);
				}
			}
			AMadRunWheelHitActor* FrontLeftHit = nullptr;
			for (AMadRunWheelHitActor* HitActor : WheelHits)
			{
				if (HitActor->GetWheel() == RuntimeWheel)
				{
					FrontLeftHit = HitActor;
				}
			}
			if (TestNotNull(TEXT("Front left hit actor targets the correct wheel"), FrontLeftHit))
			{
				USphereComponent* WheelHitbox = FrontLeftHit->FindComponentByClass<USphereComponent>();
				TestTrue(TEXT("Wheel hitbox blocks projectile channel"), WheelHitbox
					&& WheelHitbox->GetCollisionResponseToChannel(MadRunProjectileChannel) == ECR_Block);
				if (WheelHitbox)
				{
					FActorSpawnParameters OwnShotParams;
					OwnShotParams.Owner = Car;
					OwnShotParams.Instigator = Car;
					OwnShotParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					AMadRunProjectile* OwnShot = World->SpawnActor<AMadRunProjectile>(
						AMadRunProjectile::StaticClass(), WheelHitbox->GetComponentLocation(),
						(-Car->GetActorRightVector()).Rotation(), OwnShotParams);
					if (TestNotNull(TEXT("Spawned own projectile"), OwnShot))
					{
						World->Tick(LEVELTICK_All, 1.0f / 60.0f);
						TestTrue(TEXT("Own projectile ignores the vehicle and its wheel actors"), IsValid(OwnShot));
						TestEqual(TEXT("Own projectile does not damage the wheel"), IMadRunPartHealth::Execute_GetHealth(RuntimeWheel), 10);
						OwnShot->Destroy();
					}
				}
				UGameplayStatics::ApplyPointDamage(FrontLeftHit, 2.5f, FVector::ForwardVector, FHitResult(),
					nullptr, nullptr, nullptr);
				TestEqual(TEXT("Point damage affects only the hit wheel"), IMadRunPartHealth::Execute_GetHealth(RuntimeWheel), 7);
				TestEqual(TEXT("Point damage leaves the other wheel intact"), IMadRunPartHealth::Execute_GetHealth(OtherWheel), 10);
				IMadRunPartHealth::Execute_DoRepair(RuntimeWheel);

				AMadRunWheelHitActor* IsolatedHit = World->SpawnActor<AMadRunWheelHitActor>(FVector(5000.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
				if (TestNotNull(TEXT("Spawned isolated wheel hitbox"), IsolatedHit))
				{
					IsolatedHit->InitializeWheel(Car, 0);
					FActorSpawnParameters SpawnParams;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					AMadRunProjectile* Projectile = World->SpawnActor<AMadRunProjectile>(
						AMadRunProjectile::StaticClass(), FVector(4800.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParams);
					if (TestNotNull(TEXT("Spawned projectile"), Projectile))
					{
						for (int32 Frame = 0; Frame < 8 && IsValid(Projectile); ++Frame)
						{
							World->Tick(LEVELTICK_All, 1.0f / 60.0f);
						}
						TestFalse(TEXT("Projectile is destroyed on wheel impact"), IsValid(Projectile));
						TestEqual(TEXT("Projectile impact damages the wheel"), IMadRunPartHealth::Execute_GetHealth(RuntimeWheel), 5);
						TestEqual(TEXT("Projectile leaves the other wheel intact"), IMadRunPartHealth::Execute_GetHealth(OtherWheel), 10);
					}
				}
			}
		}
		AMadRunFuelTank* ChildTank = Car->GetFuelTank();
		if (TestNotNull(TEXT("Vehicle owns a fuel tank actor"), ChildTank))
		{
			TestTrue(TEXT("Tank is attached to vehicle mesh"), ChildTank->GetAttachParentActor() == Car);
			TestTrue(TEXT("Vehicle tank has fuel"), ChildTank->GetFuel() > 0.0f);
			TestNotNull(TEXT("Vehicle tank uses a static mesh root"), Cast<UStaticMeshComponent>(ChildTank->GetRootComponent()));
			UBoxComponent* ChildHitbox = ChildTank->FindComponentByClass<UBoxComponent>();
			TestTrue(TEXT("Vehicle tank blocks projectile collision"), ChildHitbox
				&& ChildHitbox->GetCollisionResponseToChannel(MadRunProjectileChannel) == ECR_Block
				&& ChildHitbox->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
			if (ChildHitbox)
			{
				const FVector Center = ChildHitbox->GetComponentLocation();
				const FVector Directions[] = { FVector::ForwardVector, -FVector::ForwardVector,
					FVector::RightVector, -FVector::RightVector, FVector::UpVector, -FVector::UpVector };
				FVector ShotStart;
				FVector ShotDirection;
				bool bFoundExposedSide = false;
				for (const FVector& Direction : Directions)
				{
					const FVector Start = Center + Direction * (ChildHitbox->Bounds.SphereRadius + 10.0f);
					FHitResult TraceHit;
					if (World->LineTraceSingleByChannel(TraceHit, Start, Center, ECC_Visibility)
						&& TraceHit.GetActor() == ChildTank)
					{
						ShotStart = Start;
						ShotDirection = -Direction;
						bFoundExposedSide = true;
						break;
					}
				}
				if (TestTrue(TEXT("Mounted tank has an exposed side"), bFoundExposedSide))
				{
					const int32 OriginalHealth = IMadRunPartHealth::Execute_GetHealth(ChildTank);
					FActorSpawnParameters SpawnParams;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					AMadRunProjectile* Projectile = World->SpawnActor<AMadRunProjectile>(
						AMadRunProjectile::StaticClass(), ShotStart, ShotDirection.Rotation(), SpawnParams);
					if (TestNotNull(TEXT("Projectile spawned for mounted fuel tank"), Projectile))
					{
						for (int32 Frame = 0; Frame < 8 && IsValid(Projectile); ++Frame)
						{
							World->Tick(LEVELTICK_All, 1.0f / 60.0f);
						}
						TestFalse(TEXT("Projectile hits mounted tank"), IsValid(Projectile));
						TestEqual(TEXT("Projectile damages mounted tank"),
							IMadRunPartHealth::Execute_GetHealth(ChildTank), FMath::Max(0, OriginalHealth - 5));
					}
				}
			}
			Car->Destroy();
			TestFalse(TEXT("Tank is destroyed with its vehicle"), IsValid(ChildTank));
		}
		UClass* UIClass = LoadClass<UMadRunUI>(nullptr, TEXT("/Game/VehicleTemplate/Blueprints/UI/BP_VehicleAdvUI.BP_VehicleAdvUI_C"));
		UMadRunUI* UI = UIClass ? CreateWidget<UMadRunUI>(World, UIClass) : nullptr;
		if (TestNotNull(TEXT("Vehicle UI Blueprint can be instantiated"), UI))
		{
			if (!UI->GetWidgetFromName(TEXT("FuelText")))
			{
				UI->NativeOnInitialized();
			}
			UOverlay* Overlay = Cast<UOverlay>(UI->GetRootWidget());
			if (TestNotNull(TEXT("HUD has an overlay root"), Overlay))
			{
				if (TestEqual(TEXT("Original HUD and status display are present"), Overlay->GetChildrenCount(), 2))
				{
					UOverlaySlot* OriginalSlot = Cast<UOverlaySlot>(Overlay->GetChildAt(0)->Slot);
					UOverlaySlot* StatusSlot = Cast<UOverlaySlot>(Overlay->GetChildAt(1)->Slot);
					if (TestNotNull(TEXT("Original HUD slot"), OriginalSlot) && TestNotNull(TEXT("Status slot"), StatusSlot))
					{
						TestTrue(TEXT("Original HUD is at the bottom right"),
							OriginalSlot->GetHorizontalAlignment() == HAlign_Right && OriginalSlot->GetVerticalAlignment() == VAlign_Bottom);
						TestTrue(TEXT("Status panel is at the top left"),
							StatusSlot->GetHorizontalAlignment() == HAlign_Left && StatusSlot->GetVerticalAlignment() == VAlign_Top);
					}
				}
			}
			UTextBlock* FuelText = Cast<UTextBlock>(UI->GetWidgetFromName(TEXT("FuelText")));
			UTextBlock* TankHealthText = Cast<UTextBlock>(UI->GetWidgetFromName(TEXT("TankHealthText")));
			UVerticalBox* WheelBox = Cast<UVerticalBox>(UI->GetWidgetFromName(TEXT("WheelHealthBox")));
			AMadRunOffroadCar* UICar = CarClass
				? World->SpawnActor<AMadRunOffroadCar>(CarClass, FVector(3000.0f, 0.0f, 0.0f), FRotator::ZeroRotator)
				: nullptr;
			if (TestNotNull(TEXT("Fuel text exists"), FuelText)
				&& TestNotNull(TEXT("Tank health text exists"), TankHealthText)
				&& TestNotNull(TEXT("Wheel health box exists"), WheelBox)
				&& TestNotNull(TEXT("UI test vehicle"), UICar))
			{
				TestEqual(TEXT("Fuel is unavailable before the first update"), FuelText->GetText().ToString(), FString(TEXT("Fuel: --")));
				TestEqual(TEXT("Tank health is unavailable before the first update"), TankHealthText->GetText().ToString(), FString(TEXT("Fuel tank: --")));
				AMadRunFuelTank* UITank = UICar->GetFuelTank();
				UMadRunWheelBase* UIWheel = UICar->GetWheelByIndex(0);
				if (TestNotNull(TEXT("UI test tank"), UITank) && TestNotNull(TEXT("UI test wheel"), UIWheel))
				{
					const TSharedRef<SWidget> SlateWidget = UI->TakeWidget();
					IMadRunPartHealth::Execute_SetHealth(UITank, 4);
					IMadRunPartHealth::Execute_SetHealth(UIWheel, 0);
					UI->UpdateVehicleStatus(UICar);
					FNumberFormattingOptions Options;
					Options.SetMinimumFractionalDigits(1);
					Options.SetMaximumFractionalDigits(1);
					const FString Expected = FText::Format(FText::FromString(TEXT("Fuel: {0} / {1} L")),
						FText::AsNumber(UITank->GetFuel(), &Options), FText::AsNumber(UITank->GetCapacity(), &Options)).ToString();
					TestEqual(TEXT("Fuel display shows current amount and capacity"), FuelText->GetText().ToString(), Expected);
					TestEqual(TEXT("Tank health displays current and max health"), TankHealthText->GetText().ToString(), FString(TEXT("Fuel tank: 4 / 10")));
					TestEqual(TEXT("All four wheel rows are present"), WheelBox->GetChildrenCount(), 4);
					UTextBlock* WheelText = Cast<UTextBlock>(WheelBox->GetChildAt(0));
					if (TestNotNull(TEXT("First wheel health text"), WheelText))
					{
						TestEqual(TEXT("Broken wheel displays zero health"), WheelText->GetText().ToString(), FString(TEXT("Wheel 0: 0 / 10")));
						IMadRunPartHealth::Execute_DoRepair(UIWheel);
						IMadRunPartHealth::Execute_DoRepair(UITank);
						UI->UpdateVehicleStatus(UICar);
						TestEqual(TEXT("Wheel health updates after repair"), WheelText->GetText().ToString(), FString(TEXT("Wheel 0: 10 / 10")));
						TestEqual(TEXT("Tank health updates after repair"), TankHealthText->GetText().ToString(), FString(TEXT("Fuel tank: 10 / 10")));
					}
					UI->UpdateVehicleStatus(nullptr);
					TestEqual(TEXT("Missing vehicle clears fuel"), FuelText->GetText().ToString(), FString(TEXT("Fuel: --")));
					TestEqual(TEXT("Missing vehicle clears tank health"), TankHealthText->GetText().ToString(), FString(TEXT("Fuel tank: --")));
					TestTrue(TEXT("Missing vehicle hides wheel rows"), WheelText && WheelText->GetVisibility() == ESlateVisibility::Collapsed);
					UI->UpdateVehicleStatus(UICar);
					TestTrue(TEXT("Wheel rows return on possession"), WheelText && WheelText->GetVisibility() != ESlateVisibility::Collapsed);
					TestEqual(TEXT("Wheel rows are reused"), WheelBox->GetChildrenCount(), 4);

					UI->ForceLayoutPrepass();
					if (Overlay && Overlay->GetChildrenCount() == 2)
					{
						const FVector2D OriginalSize = Overlay->GetChildAt(0)->GetDesiredSize();
						const FVector2D StatusSize = Overlay->GetChildAt(1)->GetDesiredSize();
						TestTrue(FString::Printf(TEXT("HUD and status have nonzero desired sizes (HUD %s, status %s)"),
							*OriginalSize.ToString(), *StatusSize.ToString()),
							OriginalSize.X > 0.0 && OriginalSize.Y > 0.0 && StatusSize.X > 0.0 && StatusSize.Y > 0.0);
						TestTrue(TEXT("Panels fit without overlapping at 1280x720"),
							OriginalSize.X + StatusSize.X + 48.0 <= 1280.0 || OriginalSize.Y + StatusSize.Y + 48.0 <= 720.0);
					}
					UITank->Destroy();
					UI->UpdateVehicleStatus(UICar);
					TestEqual(TEXT("Missing tank clears fuel"), FuelText->GetText().ToString(), FString(TEXT("Fuel: --")));
					TestEqual(TEXT("Missing tank clears tank health"), TankHealthText->GetText().ToString(), FString(TEXT("Fuel tank: --")));
					TestTrue(TEXT("Wheel health remains visible without a tank"), WheelText && WheelText->GetVisibility() != ESlateVisibility::Collapsed);
				}
			}
		}
	}
	World->BeginTearingDown();
	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
