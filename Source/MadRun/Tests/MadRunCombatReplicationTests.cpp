#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MadRunFuelTank.h"
#include "MadRunPawn.h"
#include "MadRunProjectile.h"
#include "MadRunTurret.h"
#include "MadRunWheelBase.h"
#include "OffroadCar/MadRunOffroadCar.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Net/UnrealNetwork.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMadRunCombatReplicationTest, "MadRun.Multiplayer.CombatState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMadRunCombatReplicationTest::RunTest(const FString& Parameters)
{
	const UFunction* FireRPC = AMadRunPawn::StaticClass()->FindFunctionByName(TEXT("ServerSetTurretFire"));
	TestTrue(TEXT("Fire input is a reliable server RPC"),
		FireRPC && FireRPC->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer | FUNC_NetReliable));
	const AMadRunProjectile* ProjectileDefaults = GetDefault<AMadRunProjectile>();
	TestTrue(TEXT("Projectiles replicate"), ProjectileDefaults->GetIsReplicated());
	TestTrue(TEXT("Projectile movement replicates"), ProjectileDefaults->IsReplicatingMovement());
	TArray<FLifetimeProperty> Properties;
	GetDefault<AMadRunPawn>()->GetLifetimeReplicatedProps(Properties);
	const FProperty* CombatProperty = FindFProperty<FProperty>(AMadRunPawn::StaticClass(), TEXT("CombatState"));
	TestTrue(TEXT("Combat state is registered for replication"),
		CombatProperty && Properties.ContainsByPredicate([CombatProperty](const FLifetimeProperty& Property)
		{
			return Property.RepIndex == CombatProperty->RepIndex;
		}));

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
	if (TestNotNull(TEXT("Vehicle"), Car))
	{
		AMadRunFuelTank* Tank = Car->GetFuelTank();
		AMadRunTurret* Turret = Car->GetTurret();
		UMadRunWheelBase* Wheel = Car->GetWheelByIndex(0);
		if (TestNotNull(TEXT("Fuel tank"), Tank))
		{
			const int32 MaxHealth = IMadRunPartHealth::Execute_GetMaxHealth(Tank);
			Tank->ApplyReplicatedState(15.0f, 0);
			TestEqual(TEXT("Replicated fuel applied"), Tank->GetFuel(), 15.0f);
			TestEqual(TEXT("Replicated tank damage applied"), IMadRunPartHealth::Execute_GetHealth(Tank), 0);
			Tank->ApplyReplicatedState(20.0f, MaxHealth);
			TestEqual(TEXT("Replicated tank repair applied"), IMadRunPartHealth::Execute_GetHealth(Tank), MaxHealth);
		}
		if (TestNotNull(TEXT("Wheel"), Wheel))
		{
			const int32 MaxHealth = IMadRunPartHealth::Execute_GetMaxHealth(Wheel);
			Wheel->ApplyReplicatedHealth(0);
			TestEqual(TEXT("Replicated wheel damage applied"), IMadRunPartHealth::Execute_GetHealth(Wheel), 0);
			Wheel->ApplyReplicatedHealth(MaxHealth);
			TestEqual(TEXT("Replicated wheel repair applied"), IMadRunPartHealth::Execute_GetHealth(Wheel), MaxHealth);
		}
		if (TestNotNull(TEXT("Turret"), Turret))
		{
			Turret->ApplyReplicatedState(2, true, FRotator(0.0f, 45.0f, 0.0f));
			TestEqual(TEXT("Replicated ammo applied"), Turret->GetAmmoInMagazine(), 2);
			TestTrue(TEXT("Replicated reload applied"), Turret->IsReloading());
			TestTrue(TEXT("Replicated aim applied"), Turret->GetActorRotation().Equals(FRotator(0.0f, 45.0f, 0.0f), 0.1f));
		}
	}
	World->BeginTearingDown();
	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
