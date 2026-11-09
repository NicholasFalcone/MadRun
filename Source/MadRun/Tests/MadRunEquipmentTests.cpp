#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MadRunEquipment.h"
#include "MadRunFuelTank.h"
#include "MadRunTurret.h"
#include "MadRunWheelBase.h"
#include "OffroadCar/MadRunOffroadCar.h"
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
		Tires->Mesh = Mesh;
		TankEquipment->Mesh = Mesh;
		AMadRunFuelTank* Tank = Car->GetFuelTank();
		AMadRunTurret* Turret = Car->GetTurret();
		UMadRunWheelBase* FirstWheel = Car->GetWheelByIndex(0);
		UMadRunWheelBase* LastWheel = Car->GetWheelByIndex(3);
		if (TestNotNull(TEXT("Tank"), Tank) && TestNotNull(TEXT("Turret"), Turret)
			&& TestNotNull(TEXT("First wheel"), FirstWheel) && TestNotNull(TEXT("Last wheel"), LastWheel))
		{
			TestNull(TEXT("Engine starts unequipped"), Car->GetEngineEquipment());
			TestNull(TEXT("Tank starts unequipped"), Tank->GetEquipment());
			TestNull(TEXT("Turret starts unequipped"), Turret->GetEquipment());
			TestNull(TEXT("Wheel starts unequipped"), FirstWheel->GetEquipment());
			TestTrue(TEXT("Equip engine"), Car->Equip(Engine));
			TestTrue(TEXT("Equip weapon"), Car->Equip(Weapon));
			TestTrue(TEXT("Equip tires"), Car->Equip(Tires));
			TestTrue(TEXT("Equip tank"), Car->Equip(TankEquipment));
			TestTrue(TEXT("Engine selection"), Car->GetEngineEquipment() == Engine);
			TestTrue(TEXT("Turret selection"), Turret->GetEquipment() == Weapon);
			TestTrue(TEXT("Tank selection"), Tank->GetEquipment() == TankEquipment);
			TestTrue(TEXT("Tires shared by both wheels"), FirstWheel->GetEquipment() == Tires && LastWheel->GetEquipment() == Tires);
			TestTrue(TEXT("Tank mesh changed"), Cast<UStaticMeshComponent>(Tank->GetRootComponent())->GetStaticMesh() == Mesh);
			UStaticMeshComponent* Barrel = Turret->FindComponentByClass<UStaticMeshComponent>();
			if (TestNotNull(TEXT("Turret barrel"), Barrel))
			{
				TestTrue(TEXT("Weapon mesh changed"), Barrel->GetStaticMesh() == Mesh);
				TestTrue(TEXT("Choose a different weapon"), Car->Equip(SecondWeapon));
				TestTrue(TEXT("Weapon transform changed"), Barrel->GetRelativeLocation().Equals(FVector(80.0f, 0.0f, 0.0f)));
				TestTrue(TEXT("Weapon selection changed"), Turret->GetEquipment() == SecondWeapon);
			}
			for (FName Name : { FName("Tire Front Left"), FName("Tire Front Right"), FName("Tire Rear Left"), FName("Tire Rear Right") })
			{
				UStaticMeshComponent* Tire = FindObject<UStaticMeshComponent>(Car, *Name.ToString());
				TestTrue(FString::Printf(TEXT("%s mesh changed"), *Name.ToString()), Tire && Tire->GetStaticMesh() == Mesh);
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
			AMadRunTurret* InitialTurret = InitialCar->GetTurret();
			AMadRunFuelTank* InitialTank = InitialCar->GetFuelTank();
			UStaticMeshComponent* InitialBarrel = InitialTurret ? InitialTurret->FindComponentByClass<UStaticMeshComponent>() : nullptr;
			UStaticMeshComponent* InitialTankMesh = InitialTank ? Cast<UStaticMeshComponent>(InitialTank->GetRootComponent()) : nullptr;
			UStaticMeshComponent* InitialTire = FindObject<UStaticMeshComponent>(InitialCar, TEXT("Tire Front Left"));
			TestTrue(TEXT("Initial weapon mesh applied"), InitialBarrel && InitialBarrel->GetStaticMesh() == Mesh);
			TestTrue(TEXT("Initial tank mesh applied"), InitialTankMesh && InitialTankMesh->GetStaticMesh() == Mesh);
			TestTrue(TEXT("Initial tire mesh applied"), InitialTire && InitialTire->GetStaticMesh() == Mesh);
		}
	}
	World->BeginTearingDown();
	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
