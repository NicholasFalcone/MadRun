// Copyright Epic Games, Inc. All Rights Reserved.

#include "MadRunGameInstance.h"
#include "MadRun.h"
#include "Engine/Engine.h"
#include "Engine/PendingNetGame.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "SocketSubsystem.h"

namespace
{
	const FName RaceTag(TEXT("MADRUNGAME"));
	const FString RaceTagValue(TEXT("MadRunRaceV1"));
	const TCHAR* RaceMap = TEXT("/Game/Variant_OffRoad/Maps/Lvl_Offroad");
}

void UMadRunGameInstance::Init()
{
	Super::Init();
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UMadRunGameInstance::OnNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UMadRunGameInstance::OnTravelFailure);
	}
}

void UMadRunGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}
	ClearDelegates();
	Super::Shutdown();
}

void UMadRunGameInstance::LoadComplete(const float LoadTime, const FString& MapName)
{
	Super::LoadComplete(LoadTime, MapName);
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		bAwaitingConnection = false;
	}
}

IOnlineSessionPtr UMadRunGameInstance::GetSteamSessions() const
{
	if (!IsRunningGame())
	{
		Report(TEXT("Steam richiede un processo di gioco standalone, non Play in Editor."));
		return nullptr;
	}
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem || Subsystem->GetSubsystemName() != STEAM_SUBSYSTEM)
	{
		Report(TEXT("Steam non disponibile: avvia Steam e usa una build standalone."));
		return nullptr;
	}
	if (!ISocketSubsystem::Get(FName(TEXT("SteamSockets"))))
	{
		Report(TEXT("SteamSockets non disponibile: impossibile usare le lobby online."));
		return nullptr;
	}
	IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();
	if (!Sessions.IsValid())
	{
		Report(TEXT("Interfaccia sessioni Steam non disponibile."));
	}
	return Sessions;
}

void UMadRunGameInstance::Report(const FString& Message) const
{
	UE_LOG(LogMadRun, Display, TEXT("Steam race: %s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, Message);
	}
}

void UMadRunGameInstance::HostRace()
{
	if (bBusy)
	{
		Report(TEXT("Operazione Steam gia in corso."));
		return;
	}
	IOnlineSessionPtr Sessions = GetSteamSessions();
	if (!Sessions.IsValid())
	{
		return;
	}
	if (!GetWorld() || GetWorld()->GetNetMode() != NM_Standalone || Sessions->GetNamedSession(NAME_GameSession))
	{
		Report(TEXT("Esci dalla sessione corrente prima di crearne un'altra."));
		return;
	}

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = 2;
	Settings.bIsLANMatch = false;
	Settings.bIsDedicated = false;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.Set(RaceTag, RaceTagValue, EOnlineDataAdvertisementType::ViaOnlineService);

	bBusy = true;
	ActiveSessions = Sessions;
	CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UMadRunGameInstance::OnCreateComplete));
	if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
	{
		ClearDelegates();
		bBusy = false;
		Report(TEXT("Impossibile avviare la creazione della sessione Steam."));
	}
	else
	{
		Report(TEXT("Creazione della lobby Steam in corso..."));
	}
}

void UMadRunGameInstance::OnCreateComplete(FName SessionName, bool bSuccess)
{
	ClearDelegates();
	bBusy = false;
	if (!bSuccess)
	{
		Report(TEXT("Creazione della lobby Steam fallita."));
		return;
	}
	if (!GetWorld() || !GetWorld()->ServerTravel(FString::Printf(TEXT("%s?listen?MaxPlayers=2"), RaceMap), true))
	{
		Report(TEXT("Lobby creata, ma avvio della mappa fallito; chiusura della sessione."));
		LeaveRace();
		return;
	}
	Report(TEXT("Lobby creata; avvio della gara come host."));
}

void UMadRunGameInstance::FindRaces()
{
	if (bBusy)
	{
		Report(TEXT("Operazione Steam gia in corso."));
		return;
	}
	IOnlineSessionPtr Sessions = GetSteamSessions();
	if (!Sessions.IsValid())
	{
		return;
	}
	if (!GetWorld() || GetWorld()->GetNetMode() != NM_Standalone || Sessions->GetNamedSession(NAME_GameSession))
	{
		Report(TEXT("Esci dalla sessione corrente prima di cercare una gara."));
		return;
	}

	Search = MakeShared<FOnlineSessionSearch>();
	Search->MaxSearchResults = 100;
	Search->bIsLanQuery = false;
	Search->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	Search->QuerySettings.Set(RaceTag, RaceTagValue, EOnlineComparisonOp::Equals);
	bBusy = true;
	ActiveSessions = Sessions;
	FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UMadRunGameInstance::OnFindComplete));
	if (!Sessions->FindSessions(0, Search.ToSharedRef()))
	{
		ClearDelegates();
		bBusy = false;
		Search.Reset();
		Report(TEXT("Impossibile avviare la ricerca Steam."));
	}
	else
	{
		Report(TEXT("Ricerca delle lobby Steam in corso..."));
	}
}

void UMadRunGameInstance::OnFindComplete(bool bSuccess)
{
	ClearDelegates();
	bBusy = false;
	if (!bSuccess || !Search.IsValid())
	{
		Search.Reset();
		Report(TEXT("Ricerca delle lobby Steam fallita."));
		return;
	}
	Report(FString::Printf(TEXT("Lobby trovate: %d. Usa MadRunJoin <indice>."), Search->SearchResults.Num()));
	for (int32 Index = 0; Index < Search->SearchResults.Num(); ++Index)
	{
		const FOnlineSessionSearchResult& Result = Search->SearchResults[Index];
		Report(FString::Printf(TEXT("%d: %s - posti liberi: %d"), Index,
			*Result.Session.OwningUserName, Result.Session.NumOpenPublicConnections));
	}
}

void UMadRunGameInstance::JoinRace(int32 Index)
{
	if (bBusy)
	{
		Report(TEXT("Operazione Steam gia in corso."));
		return;
	}
	IOnlineSessionPtr Sessions = GetSteamSessions();
	if (!Sessions.IsValid())
	{
		return;
	}
	if (!GetWorld() || GetWorld()->GetNetMode() != NM_Standalone || Sessions->GetNamedSession(NAME_GameSession))
	{
		Report(TEXT("Esci dalla sessione corrente prima di unirti a una gara."));
		return;
	}
	if (!Search.IsValid() || !Search->SearchResults.IsValidIndex(Index))
	{
		Report(TEXT("Indice non valido: esegui MadRunFind e scegli un risultato."));
		return;
	}

	bBusy = true;
	ActiveSessions = Sessions;
	JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UMadRunGameInstance::OnJoinComplete));
	if (!Sessions->JoinSession(0, NAME_GameSession, Search->SearchResults[Index]))
	{
		ClearDelegates();
		bBusy = false;
		Report(TEXT("Impossibile avviare l'accesso alla lobby Steam."));
	}
	else
	{
		Report(TEXT("Accesso alla lobby Steam in corso..."));
	}
}

void UMadRunGameInstance::OnJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Sessions = ActiveSessions;
	ClearDelegates();
	bBusy = false;
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		Report(FString::Printf(TEXT("Accesso alla lobby Steam fallito (codice %d)."), static_cast<int32>(Result)));
		return;
	}

	FString Address;
	APlayerController* Controller = GetFirstLocalPlayerController();
	if (!Sessions.IsValid() || !Sessions->GetResolvedConnectString(SessionName, Address) || Address.IsEmpty() || !Controller)
	{
		Report(TEXT("Impossibile risolvere l'indirizzo della lobby; chiusura della sessione."));
		LeaveRace();
		return;
	}
	Report(TEXT("Connessione all'host in corso..."));
	bAwaitingConnection = true;
	Controller->ClientTravel(Address, TRAVEL_Absolute);
}

void UMadRunGameInstance::LeaveRace()
{
	DestroyRaceSession(true);
}

void UMadRunGameInstance::DestroyRaceSession(bool bReturnToMap)
{
	if (bBusy)
	{
		Report(TEXT("Operazione Steam gia in corso."));
		return;
	}
	IOnlineSessionPtr Sessions = GetSteamSessions();
	if (!Sessions.IsValid())
	{
		return;
	}
	if (!Sessions->GetNamedSession(NAME_GameSession))
	{
		Report(TEXT("Non sei in una sessione Steam."));
		return;
	}
	bReturnToMapAfterDestroy = bReturnToMap && GetWorld() && GetWorld()->GetNetMode() != NM_Standalone;
	bBusy = true;
	ActiveSessions = Sessions;
	DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UMadRunGameInstance::OnDestroyComplete));
	if (!Sessions->DestroySession(NAME_GameSession))
	{
		ClearDelegates();
		bBusy = false;
		bReturnToMapAfterDestroy = false;
		Report(TEXT("Impossibile chiudere la sessione Steam."));
	}
}

void UMadRunGameInstance::OnDestroyComplete(FName SessionName, bool bSuccess)
{
	ClearDelegates();
	bBusy = false;
	if (!bSuccess)
	{
		bReturnToMapAfterDestroy = false;
		Report(TEXT("Chiusura della sessione Steam fallita."));
		return;
	}
	Search.Reset();
	bAwaitingConnection = false;
	if (bReturnToMapAfterDestroy)
	{
		bReturnToMapAfterDestroy = false;
		if (APlayerController* Controller = GetFirstLocalPlayerController())
		{
			Controller->ClientTravel(RaceMap, TRAVEL_Absolute);
		}
		else
		{
			Report(TEXT("Sessione chiusa, ma il controller locale non e disponibile."));
		}
	}
	Report(TEXT("Sessione Steam chiusa."));
}

void UMadRunGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType, const FString& Error)
{
	const FWorldContext* Context = GetWorldContext();
	const bool bOurPendingConnection = !World && (bAwaitingConnection || (Context && Context->PendingNetGame
		&& Context->PendingNetGame->GetNetDriver() == NetDriver));
	if ((!bOurPendingConnection && (!World || World->GetGameInstance() != this))
		|| (World && World->GetNetMode() == NM_ListenServer))
	{
		return;
	}
	bAwaitingConnection = false;
	Report(FString::Printf(TEXT("Connessione interrotta: %s"), *Error));
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	IOnlineSessionPtr Sessions = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (!bBusy && Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession))
	{
		DestroyRaceSession(false);
	}
}

void UMadRunGameInstance::OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error)
{
	if (!World || World->GetGameInstance() != this)
	{
		return;
	}
	bAwaitingConnection = false;
	Report(FString::Printf(TEXT("Caricamento della mappa fallito: %s"), *Error));
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	IOnlineSessionPtr Sessions = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (!bBusy && Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession))
	{
		DestroyRaceSession(false);
	}
}

void UMadRunGameInstance::ClearDelegates()
{
	if (ActiveSessions.IsValid())
	{
		if (CreateHandle.IsValid()) ActiveSessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		if (FindHandle.IsValid()) ActiveSessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		if (JoinHandle.IsValid()) ActiveSessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		if (DestroyHandle.IsValid()) ActiveSessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}
	CreateHandle.Reset();
	FindHandle.Reset();
	JoinHandle.Reset();
	DestroyHandle.Reset();
	ActiveSessions.Reset();
}
