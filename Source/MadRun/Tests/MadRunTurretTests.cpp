#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MadRunTurret.h"
#include "MadRunProjectile.h"
#include "MadRunFuelTank.h"
#include "OffroadCar/MadRunOffroadCar.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMadRunTurretTest, "MadRun.Vehicle.Turret",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMadRunTurretTest::RunTest(const FString& Parameters)
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
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto SpawnCar = [&](const FVector& Location)
	{
		AMadRunOffroadCar* Car = CarClass
			? World->SpawnActor<AMadRunOffroadCar>(CarClass, Location, FRotator::ZeroRotator, SpawnParams) : nullptr;
		if (Car)
		{
			Car->GetMesh()->SetSimulatePhysics(false);
		}
		return Car;
	};
	AMadRunOffroadCar* Shooter = SpawnCar(FVector::ZeroVector);
	AMadRunOffroadCar* Near = SpawnCar(FVector(3000.0f, 0.0f, 0.0f));
	AMadRunOffroadCar* Far = SpawnCar(FVector(3000.0f, 3000.0f, 0.0f));
	AMadRunOffroadCar* OutOfRange = SpawnCar(FVector(20000.0f, 0.0f, 0.0f));
	AMadRunTurret* Turret = Shooter ? Shooter->GetTurret() : nullptr;
	if (TestNotNull(TEXT("Shooter"), Shooter) && TestNotNull(TEXT("Near target"), Near)
		&& TestNotNull(TEXT("Far target"), Far) && TestNotNull(TEXT("Out-of-range target"), OutOfRange)
		&& TestNotNull(TEXT("Roof turret"), Turret))
	{
		Turret->Tick(0.0f);
		TestTrue(TEXT("Selects nearest other vehicle"), Turret->GetTarget() == Near);
		const FVector ExpectedDirection = (Near->GetMesh()->Bounds.Origin - Turret->GetActorLocation()).GetSafeNormal();
		TestTrue(TEXT("Barrel aims at target"), FVector::DotProduct(ExpectedDirection, Turret->GetActorForwardVector()) > 0.999f);

		AActor* Blocker = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Blocker);
		Blocker->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(100.0f, 300.0f, 1000.0f));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->RegisterComponent();
		Blocker->SetActorLocation(FVector(1500.0f, 0.0f, 0.0f));
		Turret->Tick(0.0f);
		TestTrue(TEXT("Occluded nearest vehicle is skipped"), Turret->GetTarget() == Far);
		Far->Destroy();
		Turret->Tick(0.0f);
		TestNull(TEXT("Blocked and out-of-range vehicles are not targets"), Turret->GetTarget());
		Blocker->Destroy();
		Near->Destroy();
		Turret->Tick(0.0f);
		TestTrue(TEXT("No target resets aim to vehicle forward"), FVector::DotProduct(Turret->GetActorForwardVector(), Shooter->GetActorForwardVector()) > 0.999f);

		int32 SpawnedProjectiles = 0;
		const FDelegateHandle SpawnHandle = World->AddOnActorSpawnedHandler(
			FOnActorSpawned::FDelegate::CreateLambda([&](AActor* Actor)
			{
				if (Actor->IsA<AMadRunProjectile>())
				{
					++SpawnedProjectiles;
				}
			}));
		auto CountProjectiles = [&]()
		{
			int32 Count = 0;
			for (TActorIterator<AMadRunProjectile> It(World); It; ++It)
			{
				++Count;
			}
			return Count;
		};
		AMadRunFuelTank* ImpactTank = World->SpawnActor<AMadRunFuelTank>(
			AMadRunFuelTank::StaticClass(), Turret->GetActorLocation() + Shooter->GetActorForwardVector() * 350.0f,
			FRotator::ZeroRotator, SpawnParams);
		TestNotNull(TEXT("Fuel tank in projectile path"), ImpactTank);
		AMadRunFuelTank* OwnTank = Shooter->GetFuelTank();
		TestNotNull(TEXT("Shooter fuel tank"), OwnTank);
		const int32 OwnTankHealth = OwnTank ? IMadRunPartHealth::Execute_GetHealth(OwnTank) : 0;
		Shooter->DoFireStart();
		Turret->Tick(0.0f);
		TestEqual(TEXT("Fire produces a projectile without a target"), CountProjectiles(), 1);
		for (TActorIterator<AMadRunProjectile> It(World); It; ++It)
		{
			TestTrue(TEXT("Vehicle owns the projectile"), It->GetOwner() == Shooter);
			TestTrue(TEXT("Vehicle instigates the projectile"), It->GetInstigator() == Shooter);
			TestTrue(TEXT("Projectile travels forward"), FVector::DotProduct(It->GetActorForwardVector(), Shooter->GetActorForwardVector()) > 0.999f);
		}
		Turret->Tick(0.0f);
		TestEqual(TEXT("Cooldown prevents extra shots"), CountProjectiles(), 1);
		TestTrue(TEXT("Turret has begun play"), Turret->HasActorBegunPlay());
		TestTrue(TEXT("Turret tick is enabled"), Turret->IsActorTickEnabled());
		World->Tick(LEVELTICK_All, 0.1f);
		Turret->Tick(0.0f);
		TestEqual(TEXT("No shot before interval"), SpawnedProjectiles, 1);
		if (ImpactTank)
		{
			TestEqual(TEXT("Turret projectile damages the tank it hits"), IMadRunPartHealth::Execute_GetHealth(ImpactTank), 5);
		}
		if (OwnTank)
		{
			TestEqual(TEXT("Turret projectile does not damage own tank"), IMadRunPartHealth::Execute_GetHealth(OwnTank), OwnTankHealth);
		}
		World->Tick(LEVELTICK_All, 0.11f);
		// Multiple world ticks in one automation frame do not reschedule actor ticks.
		Turret->Tick(0.0f);
		TestEqual(FString::Printf(TEXT("Held fire repeats after interval (world time %.3f)"), World->GetTimeSeconds()), SpawnedProjectiles, 2);
		Shooter->DoFireStop();
		World->Tick(LEVELTICK_All, 0.3f);
		Turret->Tick(0.0f);
		TestEqual(TEXT("Release stops firing"), SpawnedProjectiles, 2);
		World->RemoveOnActorSpawnedHandler(SpawnHandle);
	}

	World->BeginTearingDown();
	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
