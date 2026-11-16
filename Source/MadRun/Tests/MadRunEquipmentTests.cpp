#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MadRunEquipment.h"
#include "MadRunFuelTank.h"
#include "MadRunTurret.h"
#include "MadRunWheelBase.h"
#include "MadRunVehicleMovementComponent.h"
#include "OffroadCar/MadRunOffroadCar.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMadRunEquipmentTest, "MadRun.Vehicle.Equipment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMadRunEquipmentTest::RunTest(const FString& Parameters)
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
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (TestNotNull(TEXT("Vehicle"), Car) && TestNotNull(TEXT("Mesh"), Mesh))
	{
		auto* Engine = NewObject<UMadRunEngineEquipment>(Car);
		auto* Weapon = NewObject<UMadRunWeaponEquipment>(Car);
		auto* SecondWeapon = NewObject<UMadRunWeaponEquipment>(Car);
		auto* Tires = NewObject<UMadRunTiresEquipment>(Car);
		auto* TankEquipment = NewObject<UMadRunFuelTankEquipment>(Car);
		auto* InvalidWeapon = NewObject<UMadRunWeaponEquipment>(Car);
		Weapon->Mesh = Mesh;
		SecondWeapon->Mesh = Mesh;
		SecondWeapon->MeshTransform.SetLocation(FVector(80.0f, 0.0f, 0.0f));
		Tires->FrictionForceMultiplier = 20.0f;
		Tires->SuspensionMaxRaise = 25.0f;
		Tires->SuspensionMaxDrop = 30.0f;
		TankEquipment->Mesh = Mesh;
		AMadRunFuelTank* Tank = Car->GetFuelTank();
		AMadRunTurret* Turret = Car->GetTurret();
		UMadRunWheelBase* FirstWheel = Car->GetWheelByIndex(0);
		UMadRunWheelBase* LastWheel = Car->GetWheelByIndex(3);
		const FName TireComponentNames[] = {
			FName("Tire Front Left"), FName("Tire Front Right"),
			FName("Tire Rear Left"), FName("Tire Rear Right")
		};
		UStaticMesh* OriginalTireMeshes[UE_ARRAY_COUNT(TireComponentNames)] = {};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(TireComponentNames); ++Index)
		{
			if (UStaticMeshComponent* Tire = FindObject<UStaticMeshComponent>(Car, *TireComponentNames[Index].ToString()))
			{
				OriginalTireMeshes[Index] = Tire->GetStaticMesh();
			}
		}
		if (TestNotNull(TEXT("Tank"), Tank) && TestNotNull(TEXT("Turret"), Turret)
			&& TestNotNull(TEXT("First wheel"), FirstWheel) && TestNotNull(TEXT("Last wheel"), LastWheel))
		{
			TestTrue(TEXT("Equip engine"), Car->Equip(Engine));
			TestTrue(TEXT("Equip weapon"), Car->Equip(Weapon));
			TestTrue(TEXT("Equip tires"), Car->Equip(Tires));
			TestTrue(TEXT("Equip tank"), Car->Equip(TankEquipment));
			TestTrue(TEXT("Engine selection"), Car->GetEngineEquipment() == Engine);
			TestTrue(TEXT("Turret selection"), Turret->GetEquipment() == Weapon);
			TestTrue(TEXT("Tank selection"), Tank->GetEquipment() == TankEquipment);
			TestTrue(TEXT("Tires shared by both wheels"), FirstWheel->GetEquipment() == Tires && LastWheel->GetEquipment() == Tires);
			TestTrue(TEXT("Tire friction statistics applied to all wheels"),
				FMath::IsNearlyEqual(FirstWheel->FrictionForceMultiplier, 20.0f)
				&& FMath::IsNearlyEqual(LastWheel->FrictionForceMultiplier, 20.0f));
			TestTrue(TEXT("Tire suspension statistics applied to all wheels"),
				FMath::IsNearlyEqual(FirstWheel->SuspensionMaxRaise, 25.0f)
				&& FMath::IsNearlyEqual(LastWheel->SuspensionMaxRaise, 25.0f)
				&& FMath::IsNearlyEqual(FirstWheel->SuspensionMaxDrop, 30.0f)
				&& FMath::IsNearlyEqual(LastWheel->SuspensionMaxDrop, 30.0f));
			UMadRunVehicleMovementComponent* Movement = Cast<UMadRunVehicleMovementComponent>(Car->GetVehicleMovement());
			if (TestNotNull(TEXT("MadRun movement component"), Movement))
			{
				float Friction = 0.0f;
				float MaxRaise = 0.0f;
				float MaxDrop = 0.0f;
				float SpringRate = 0.0f;
				float SpringPreload = 0.0f;
				float Grip = 0.0f;
				for (int32 Index = 0; Index < 4; ++Index)
				{
					if (TestTrue(FString::Printf(TEXT("Wheel %d has a physics simulation"), Index),
						Movement->GetSimulatedTireStats(Index, Friction, MaxRaise, MaxDrop, SpringRate, SpringPreload, Grip)))
					{
						TestEqual(FString::Printf(TEXT("Wheel %d simulated friction"), Index), Friction, 20.0f);
						TestEqual(FString::Printf(TEXT("Wheel %d simulated grip at fixed load"), Index), Grip, 10000.0f);
						TestEqual(FString::Printf(TEXT("Wheel %d simulated suspension raise"), Index), MaxRaise, 25.0f);
						TestEqual(FString::Printf(TEXT("Wheel %d simulated suspension drop"), Index), MaxDrop, 30.0f);
						UMadRunWheelBase* Wheel = Car->GetWheelByIndex(Index);
						if (TestNotNull(TEXT("Simulated wheel object"), Wheel))
						{
							TestEqual(FString::Printf(TEXT("Wheel %d original spring rate"), Index), SpringRate, Wheel->SpringRate * 100.0f);
							TestEqual(FString::Printf(TEXT("Wheel %d original spring preload"), Index), SpringPreload, Wheel->SpringPreload * 100.0f);
						}
					}
				}
			}
			TestTrue(TEXT("Tank mesh changed"), Cast<UStaticMeshComponent>(Tank->GetRootComponent())->GetStaticMesh() == Mesh);
			UStaticMeshComponent* Barrel = Turret->FindComponentByClass<UStaticMeshComponent>();
			if (TestNotNull(TEXT("Turret barrel"), Barrel))
			{
				TestTrue(TEXT("Weapon mesh changed"), Barrel->GetStaticMesh() == Mesh);
				TestTrue(TEXT("Choose a different weapon"), Car->Equip(SecondWeapon));
				TestTrue(TEXT("Weapon transform changed"), Barrel->GetRelativeLocation().Equals(FVector(80.0f, 0.0f, 0.0f)));
				TestTrue(TEXT("Weapon selection changed"), Turret->GetEquipment() == SecondWeapon);
			}
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(TireComponentNames); ++Index)
			{
				UStaticMeshComponent* Tire = FindObject<UStaticMeshComponent>(Car, *TireComponentNames[Index].ToString());
				TestTrue(FString::Printf(TEXT("%s mesh unchanged"), *TireComponentNames[Index].ToString()),
					Tire && Tire->GetStaticMesh() == OriginalTireMeshes[Index]);
			}
			AddExpectedError(TEXT("requires a valid equipment mesh"), EAutomationExpectedErrorFlags::Contains, 1);
			AddExpectedError(TEXT("cannot apply weapon equipment"), EAutomationExpectedErrorFlags::Contains, 1);
			TestFalse(TEXT("Reject weapon without mesh"), Car->Equip(InvalidWeapon));
			TestTrue(TEXT("Failed selection preserves previous weapon"), Turret->GetEquipment() == SecondWeapon);
			AddExpectedError(TEXT("cannot equip invalid equipment"), EAutomationExpectedErrorFlags::Contains, 1);
			TestFalse(TEXT("Reject null equipment"), Car->Equip(nullptr));
		}

		AMadRunOffroadCar* InitialCar = World->SpawnActorDeferred<AMadRunOffroadCar>(
			CarClass, FTransform(FVector(3000.0f, 0.0f, 0.0f)));
		if (TestNotNull(TEXT("Deferred vehicle"), InitialCar))
		{
			auto SetInitial = [&](const TCHAR* Name, UObject* Value)
			{
				FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(AMadRunPawn::StaticClass(), Name);
				if (TestNotNull(FString::Printf(TEXT("Initial slot %s"), Name), Property))
				{
					Property->SetObjectPropertyValue_InContainer(InitialCar, Value);
				}
			};
			SetInitial(TEXT("InitialEngineEquipment"), Engine);
			SetInitial(TEXT("InitialWeaponEquipment"), Weapon);
			SetInitial(TEXT("InitialTiresEquipment"), Tires);
			SetInitial(TEXT("InitialFuelTankEquipment"), TankEquipment);
			InitialCar->FinishSpawning(FTransform(FVector(3000.0f, 0.0f, 0.0f)));
			TestTrue(TEXT("Initial engine equipped"), InitialCar->GetEngineEquipment() == Engine);
			TestTrue(TEXT("Initial weapon equipped"), InitialCar->GetWeaponEquipment() == Weapon);
			TestTrue(TEXT("Initial tires equipped"), InitialCar->GetTiresEquipment() == Tires);
			TestTrue(TEXT("Initial tank equipped"), InitialCar->GetFuelTankEquipment() == TankEquipment);
			UMadRunWheelBase* InitialWheel = InitialCar->GetWheelByIndex(0);
			AMadRunTurret* InitialTurret = InitialCar->GetTurret();
			AMadRunFuelTank* InitialTank = InitialCar->GetFuelTank();
			UStaticMeshComponent* InitialBarrel = InitialTurret ? InitialTurret->FindComponentByClass<UStaticMeshComponent>() : nullptr;
			UStaticMeshComponent* InitialTankMesh = InitialTank ? Cast<UStaticMeshComponent>(InitialTank->GetRootComponent()) : nullptr;
			TestTrue(TEXT("Initial weapon mesh applied"), InitialBarrel && InitialBarrel->GetStaticMesh() == Mesh);
			TestTrue(TEXT("Initial tank mesh applied"), InitialTankMesh && InitialTankMesh->GetStaticMesh() == Mesh);
			TestTrue(TEXT("Initial tire statistics applied"),
				InitialWheel && FMath::IsNearlyEqual(InitialWheel->FrictionForceMultiplier, 20.0f)
				&& FMath::IsNearlyEqual(InitialWheel->SuspensionMaxRaise, 25.0f)
				&& FMath::IsNearlyEqual(InitialWheel->SuspensionMaxDrop, 30.0f));

			UMadRunVehicleMovementComponent* InitialMovement = Cast<UMadRunVehicleMovementComponent>(InitialCar->GetVehicleMovement());
			UMadRunVehicleMovementComponent* Movement = Cast<UMadRunVehicleMovementComponent>(Car->GetVehicleMovement());
			if (TestNotNull(TEXT("Initial movement component"), InitialMovement) && TestNotNull(TEXT("Original movement component"), Movement))
			{
				float Friction = 0.0f;
				float Raise = 0.0f;
				float Drop = 0.0f;
				float Rate = 0.0f;
				float Preload = 0.0f;
				float Grip = 0.0f;
				TestTrue(TEXT("Initial tires reach the physics simulation"),
					InitialMovement->GetSimulatedTireStats(0, Friction, Raise, Drop, Rate, Preload, Grip));
				TestEqual(TEXT("Initial simulated friction"), Friction, 20.0f);
				TestEqual(TEXT("Initial simulated grip at fixed load"), Grip, 10000.0f);
				TestEqual(TEXT("Initial simulated raise"), Raise, 25.0f);
				TestEqual(TEXT("Initial simulated drop"), Drop, 30.0f);

				auto* DifferentTires = NewObject<UMadRunTiresEquipment>(Car);
				DifferentTires->FrictionForceMultiplier = 2.0f;
				DifferentTires->SuspensionMaxRaise = 10.0f;
				DifferentTires->SuspensionMaxDrop = 15.0f;
				TestTrue(TEXT("Change tires at runtime"), Car->Equip(DifferentTires));
				TestTrue(TEXT("Runtime tire simulation is available"),
					Movement->GetSimulatedTireStats(0, Friction, Raise, Drop, Rate, Preload, Grip));
				TestEqual(TEXT("Runtime simulated friction changed"), Friction, 2.0f);
				TestEqual(TEXT("Runtime simulated grip changed at fixed load"), Grip, 1000.0f);
				TestEqual(TEXT("Runtime simulated raise changed"), Raise, 10.0f);
				TestEqual(TEXT("Runtime simulated drop changed"), Drop, 15.0f);
				TestTrue(TEXT("Other vehicle simulation is available"),
					InitialMovement->GetSimulatedTireStats(0, Friction, Raise, Drop, Rate, Preload, Grip));
				TestEqual(TEXT("Other vehicle friction unchanged"), Friction, 20.0f);
				TestEqual(TEXT("Other vehicle raise unchanged"), Raise, 25.0f);
				TestEqual(TEXT("Other vehicle drop unchanged"), Drop, 30.0f);

				Movement->RecreatePhysicsState();
				TestTrue(TEXT("Recreated physics has tire simulation"),
					Movement->GetSimulatedTireStats(0, Friction, Raise, Drop, Rate, Preload, Grip));
				TestEqual(TEXT("Recreated physics retains friction"), Friction, 2.0f);
				TestEqual(TEXT("Recreated physics retains raise"), Raise, 10.0f);
				TestEqual(TEXT("Recreated physics retains drop"), Drop, 15.0f);
			}
		}
	}
	World->BeginTearingDown();
	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

// Physics tick functions must advance on separate engine frames.
class FMadRunTireGripSimulation : public IAutomationLatentCommand
{
public:
	FMadRunTireGripSimulation(FAutomationTestBase* InTest, UWorld* InWorld,
		AMadRunOffroadCar* InGrippyCar, AMadRunOffroadCar* InSlipperyCar)
		: Test(InTest), World(InWorld), GrippyCar(InGrippyCar), SlipperyCar(InSlipperyCar)
	{
	}

	virtual bool Update() override
	{
		World->Tick(LEVELTICK_All, 1.0f / 60.0f);
		++Frame;
		if (Frame == 240)
		{
			const UChaosWheeledVehicleMovementComponent* GrippyMovement = Cast<UChaosWheeledVehicleMovementComponent>(GrippyCar->GetVehicleMovement());
			const UChaosWheeledVehicleMovementComponent* SlipperyMovement = Cast<UChaosWheeledVehicleMovementComponent>(SlipperyCar->GetVehicleMovement());
			if (Test->TestNotNull(TEXT("Grippy movement"), GrippyMovement) && Test->TestNotNull(TEXT("Slippery movement"), SlipperyMovement))
			{
				Test->TestTrue(TEXT("Grippy tires contact the ground"), GrippyMovement->GetWheelState(0).bInContact);
				Test->TestTrue(TEXT("Slippery tires contact the ground"), SlipperyMovement->GetWheelState(0).bInContact);
			}
			GrippyCar->GetMesh()->SetPhysicsLinearVelocity(FVector(0.0f, 1000.0f, 0.0f));
			SlipperyCar->GetMesh()->SetPhysicsLinearVelocity(FVector(0.0f, 1000.0f, 0.0f));
		}
		if (Frame < 300)
		{
			return false;
		}

		const float GrippySpeed = FMath::Abs(GrippyCar->GetMesh()->GetPhysicsLinearVelocity().Y);
		const float SlipperySpeed = FMath::Abs(SlipperyCar->GetMesh()->GetPhysicsLinearVelocity().Y);
		Test->AddInfo(FString::Printf(TEXT("Lateral speeds after one second: grippy %.1f, slippery %.1f cm/s"), GrippySpeed, SlipperySpeed));
		Test->TestTrue(TEXT("Grippy tires reduce lateral sliding more than frictionless tires"), GrippySpeed + 100.0f < SlipperySpeed);
		World->BeginTearingDown();
		World->EndPlay(EEndPlayReason::Quit);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return true;
	}

private:
	FAutomationTestBase* Test;
	UWorld* World;
	AMadRunOffroadCar* GrippyCar;
	AMadRunOffroadCar* SlipperyCar;
	int32 Frame = 0;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMadRunTireGripTest, "MadRun.Vehicle.TireGrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMadRunTireGripTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues WorldSettings = UWorld::InitializationValues()
		.CreatePhysicsScene(true).ShouldSimulatePhysics(true).EnableTraceCollision(true)
		.CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
		ERHIFeatureLevel::Num, &WorldSettings);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();

	AActor* Ground = World->SpawnActor<AActor>();
	UBoxComponent* GroundCollision = NewObject<UBoxComponent>(Ground);
	Ground->SetRootComponent(GroundCollision);
	GroundCollision->SetBoxExtent(FVector(10000.0f, 10000.0f, 50.0f));
	GroundCollision->SetCollisionProfileName(TEXT("BlockAll"));
	GroundCollision->RegisterComponent();
	Ground->SetActorLocation(FVector(0.0f, 0.0f, -50.0f));

	UClass* CarClass = LoadClass<AMadRunOffroadCar>(nullptr, TEXT("/Game/VehicleTemplate/Blueprints/OffroadCar/BP_OffroadCar_Pawn.BP_OffroadCar_Pawn_C"));
	AMadRunOffroadCar* GrippyCar = CarClass ? World->SpawnActor<AMadRunOffroadCar>(CarClass, FVector(0.0f, -1200.0f, 150.0f), FRotator::ZeroRotator) : nullptr;
	AMadRunOffroadCar* SlipperyCar = CarClass ? World->SpawnActor<AMadRunOffroadCar>(CarClass, FVector(0.0f, 1200.0f, 150.0f), FRotator::ZeroRotator) : nullptr;
	if (TestNotNull(TEXT("Grippy vehicle"), GrippyCar) && TestNotNull(TEXT("Slippery vehicle"), SlipperyCar))
	{
		auto* GrippyTires = NewObject<UMadRunTiresEquipment>(GrippyCar);
		auto* SlipperyTires = NewObject<UMadRunTiresEquipment>(SlipperyCar);
		GrippyTires->FrictionForceMultiplier = 20.0f;
		SlipperyTires->FrictionForceMultiplier = 0.0f;
		TestTrue(TEXT("Equip grippy tires"), GrippyCar->Equip(GrippyTires));
		TestTrue(TEXT("Equip slippery tires"), SlipperyCar->Equip(SlipperyTires));

		ADD_LATENT_AUTOMATION_COMMAND(FMadRunTireGripSimulation(this, World, GrippyCar, SlipperyCar));
		return true;
	}
	World->BeginTearingDown();
	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
