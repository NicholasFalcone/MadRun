#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MadRunGameInstance.h"
#include "MadRunGameMode.h"
#include "MadRunPawn.h"
#include "MadRunPlayerController.h"
#include "ChaosVehicleMovementComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ConfigCacheIni.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMadRunMultiplayerConfigurationTest, "MadRun.Multiplayer.Configuration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMadRunMultiplayerConfigurationTest::RunTest(const FString& Parameters)
{
	UClass* GameModeClass = LoadClass<AMadRunGameMode>(nullptr,
		TEXT("/Game/Variant_OffRoad/Blueprints/BP_OffRoadGameMode.BP_OffRoadGameMode_C"));
	if (!TestNotNull(TEXT("Offroad GameMode"), GameModeClass))
	{
		return false;
	}

	const AMadRunGameMode* GameMode = GameModeClass->GetDefaultObject<AMadRunGameMode>();
	if (TestNotNull(TEXT("Configured player controller"), GameMode->PlayerControllerClass.Get()))
	{
		TestTrue(TEXT("Controller supports MadRun multiplayer"),
			GameMode->PlayerControllerClass->IsChildOf(AMadRunPlayerController::StaticClass()));
	}
	if (TestNotNull(TEXT("Configured vehicle class"), GameMode->DefaultPawnClass.Get()))
	{
		const AMadRunPawn* Vehicle = Cast<AMadRunPawn>(GameMode->DefaultPawnClass->GetDefaultObject());
		if (TestNotNull(TEXT("Configured pawn is a MadRun vehicle"), Vehicle))
		{
			TestTrue(TEXT("Vehicle replicates"), Vehicle->GetIsReplicated());
			TestTrue(TEXT("Vehicle movement replicates"), Vehicle->IsReplicatingMovement());
			TestTrue(TEXT("Chaos movement component replicates"), Vehicle->GetVehicleMovementComponent()->GetIsReplicated());
		}
	}

	const UFunction* RaceMessage = AMadRunPlayerController::StaticClass()->FindFunctionByName(TEXT("ClientUpdateRaceStatus"));
	if (TestNotNull(TEXT("Race message RPC"), RaceMessage))
	{
		TestTrue(TEXT("Race message is a reliable client RPC"),
			RaceMessage->HasAllFunctionFlags(FUNC_Net | FUNC_NetClient | FUNC_NetReliable));
	}
	for (const FName Command : { FName(TEXT("MadRunHost")), FName(TEXT("MadRunFind")),
		FName(TEXT("MadRunJoin")), FName(TEXT("MadRunLeave")) })
	{
		const UFunction* Function = AMadRunPlayerController::StaticClass()->FindFunctionByName(Command);
		TestTrue(FString::Printf(TEXT("%s is an executable console command"), *Command.ToString()),
			Function && Function->HasAllFunctionFlags(FUNC_Exec));
	}

	FString GameInstanceClass;
	GConfig->GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("GameInstanceClass"), GameInstanceClass, GEngineIni);
	TestEqual(TEXT("Configured game instance"), GameInstanceClass, FString(TEXT("/Script/MadRun.MadRunGameInstance")));
	FString PlatformService;
	GConfig->GetString(TEXT("OnlineSubsystem"), TEXT("DefaultPlatformService"), PlatformService, GEngineIni);
	TestEqual(TEXT("Configured online service"), PlatformService, FString(TEXT("Steam")));
	bool bRelay = false;
	GConfig->GetBool(TEXT("OnlineSubsystemSteam"), TEXT("bAllowP2PPacketRelay"), bRelay, GEngineIni);
	TestTrue(TEXT("Steam P2P relay enabled"), bRelay);
	int32 GameDrivers = 0;
	bool bHasBeaconDriver = false;
	bool bHasDemoDriver = false;
	for (const FNetDriverDefinition& Driver : GEngine->NetDriverDefinitions)
	{
		if (Driver.DefName == NAME_GameNetDriver)
		{
			++GameDrivers;
			TestEqual(TEXT("Game driver uses Steam Sockets"), Driver.DriverClassName,
				FName(TEXT("/Script/SteamSockets.SteamSocketsNetDriver")));
			TestEqual(TEXT("Game driver has IP fallback"), Driver.DriverClassNameFallback,
				FName(TEXT("/Script/OnlineSubsystemUtils.IpNetDriver")));
		}
		bHasBeaconDriver |= Driver.DefName == FName(TEXT("BeaconNetDriver"));
		bHasDemoDriver |= Driver.DefName == FName(TEXT("DemoNetDriver"));
	}
	TestEqual(TEXT("Exactly one game net driver"), GameDrivers, 1);
	TestTrue(TEXT("Beacon driver is preserved"), bHasBeaconDriver);
	TestTrue(TEXT("Demo driver is preserved"), bHasDemoDriver);
	return true;
}

#endif
