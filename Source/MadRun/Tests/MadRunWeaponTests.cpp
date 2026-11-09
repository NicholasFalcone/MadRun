#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MadRunEquipment.h"
#include "MadRunFuelTank.h"
#include "MadRunProjectile.h"
#include "MadRunTurret.h"
#include "OffroadCar/MadRunOffroadCar.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMadRunWeaponTest, "MadRun.Vehicle.Weapon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMadRunWeaponTest::RunTest(const FString& Parameters)
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
	UClass* CarClass = LoadClass<AMadRunOffroadCar>(nullptr, TEXT("/Game/VehicleTemplate/Blueprints/OffroadCar/BP_OffroadCar_Pawn.BP_OffroadCar_Pawn_C"));
	AMadRunOffroadCar* Car = CarClass ? World->SpawnActor<AMadRunOffroadCar>(CarClass) : nullptr;
	AMadRunTurret* Turret = Car ? Car->GetTurret() : nullptr;
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (TestNotNull(TEXT("Car"), Car) && TestNotNull(TEXT("Turret"), Turret) && TestNotNull(TEXT("Mesh"), Mesh))
	{
		UMadRunWeaponEquipment* Weapon = NewObject<UMadRunWeaponEquipment>(Car);
		Weapon->Mesh = Mesh;
		Weapon->FireInterval = 0.4f;
		Weapon->Damage = 3.0f;
		Weapon->MagazineSize = 2;
		Weapon->ReloadTime = 1.0f;
		TestTrue(TEXT("Equip weapon"), Car->Equip(Weapon));
		TestEqual(TEXT("Magazine starts full"), Turret->GetAmmoInMagazine(), 2);
		int32 Shots = 0;
		float LastDamage = 0.0f;
		FDelegateHandle SpawnHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda(
			[&](AActor* Actor)
			{
				if (AMadRunProjectile* Projectile = Cast<AMadRunProjectile>(Actor))
				{
					++Shots;
					LastDamage = Projectile->GetDamage();
				}
			}));
		Turret->StartFire();
		Turret->Tick(0.0f);
		TestEqual(TEXT("First shot"), Shots, 1);
		TestEqual(TEXT("Ammo decremented"), Turret->GetAmmoInMagazine(), 1);
		AMadRunProjectile* Shot = nullptr;
		for (TActorIterator<AMadRunProjectile> It(World); It; ++It)
		{
			Shot = *It;
		}
		if (TestNotNull(TEXT("Projectile exists"), Shot))
		{
			TestEqual(TEXT("Projectile damage uses equipment"), Shot->GetDamage(), 3.0f);
		}
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AMadRunFuelTank* ImpactTank = World->SpawnActor<AMadRunFuelTank>(
			AMadRunFuelTank::StaticClass(), Turret->GetActorLocation() + Turret->GetActorForwardVector() * 500.0f,
			FRotator::ZeroRotator, SpawnParams);
		TestNotNull(TEXT("Tank in projectile path"), ImpactTank);
		World->Tick(LEVELTICK_All, 0.2f);
		Turret->Tick(0.0f);
		TestEqual(TEXT("Cannot fire before configured interval"), Shots, 1);
		if (ImpactTank)
		{
			TestEqual(TEXT("Impact applies equipment damage"), IMadRunPartHealth::Execute_GetHealth(ImpactTank), 7);
			ImpactTank->Destroy();
		}
		World->Tick(LEVELTICK_All, 0.21f);
		Turret->Tick(0.0f);
		TestEqual(TEXT("Second shot after interval"), Shots, 2);
		TestEqual(TEXT("Magazine empty"), Turret->GetAmmoInMagazine(), 0);
		TestTrue(TEXT("Empty magazine reloads"), Turret->IsReloading());
		World->Tick(LEVELTICK_All, 0.2f);
		World->Tick(LEVELTICK_All, 0.2f);
		Turret->Tick(0.0f);
		TestEqual(TEXT("Cannot fire during reload"), Shots, 2);
		Turret->StopFire();
		World->Tick(LEVELTICK_All, 0.2f);
		World->Tick(LEVELTICK_All, 0.2f);
		World->Tick(LEVELTICK_All, 0.21f);
		Turret->Tick(0.0f);
		TestFalse(TEXT("Reload completes without held fire"), Turret->IsReloading());
		TestEqual(TEXT("Infinite ammunition refills magazine"), Turret->GetAmmoInMagazine(), 2);
		TestEqual(TEXT("Release prevents automatic shot"), Shots, 2);
		Turret->StartFire();
		Turret->Tick(0.0f);
		TestEqual(TEXT("Can fire after reload"), Shots, 3);
		TestEqual(TEXT("Damage is set before projectile spawn notification"), LastDamage, 3.0f);
		World->Tick(LEVELTICK_All, 0.41f);
		Turret->Tick(0.0f);
		TestTrue(TEXT("Second reload begins"), Turret->IsReloading());
		UMadRunWeaponEquipment* Replacement = NewObject<UMadRunWeaponEquipment>(Car);
		Replacement->Mesh = Mesh;
		Replacement->MagazineSize = 4;
		Replacement->Damage = 7.0f;
		TestTrue(TEXT("Switch weapon while reloading"), Car->Equip(Replacement));
		TestFalse(TEXT("Switch cancels reload"), Turret->IsReloading());
		TestEqual(TEXT("Replacement magazine starts full"), Turret->GetAmmoInMagazine(), 4);
		World->Tick(LEVELTICK_All, 0.41f);
		Turret->Tick(0.0f);
		TestEqual(TEXT("Replacement damage applied"), LastDamage, 7.0f);
		Turret->StopFire();
		UMadRunWeaponEquipment* SingleShot = NewObject<UMadRunWeaponEquipment>(Car);
		SingleShot->Mesh = Mesh;
		SingleShot->FireInterval = 0.1f;
		SingleShot->MagazineSize = 1;
		SingleShot->ReloadTime = 0.3f;
		TestTrue(TEXT("Equip single-shot weapon"), Car->Equip(SingleShot));
		World->Tick(LEVELTICK_All, 0.21f);
		Turret->StartFire();
		Turret->Tick(0.0f);
		const int32 ShotsBeforeReload = Shots;
		TestTrue(TEXT("Single shot starts reload"), Turret->IsReloading());
		World->Tick(LEVELTICK_All, 0.2f);
		Turret->Tick(0.0f);
		TestEqual(TEXT("Held fire waits for reload"), Shots, ShotsBeforeReload);
		World->Tick(LEVELTICK_All, 0.11f);
		Turret->Tick(0.0f);
		TestEqual(TEXT("Held fire resumes after reload"), Shots, ShotsBeforeReload + 1);
		Turret->StopFire();
		Replacement->MagazineSize = 0;
		AddExpectedError(TEXT("invalid weapon statistics"), EAutomationExpectedErrorFlags::Contains, 1);
		AddExpectedError(TEXT("cannot apply weapon equipment"), EAutomationExpectedErrorFlags::Contains, 1);
		TestFalse(TEXT("Invalid statistics rejected"), Car->Equip(Replacement));
		TestEqual(TEXT("Invalid equipment leaves runtime magazine size unchanged"), Turret->GetMagazineSize(), 1);
		World->RemoveOnActorSpawnedHandler(SpawnHandle);
	}
	World->BeginTearingDown();
	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
