// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MadRunGameInstance.generated.h"

class FOnlineSessionSearch;

UCLASS()
class UMadRunGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void LoadComplete(const float LoadTime, const FString& MapName) override;

	void HostRace();
	void FindRaces();
	void JoinRace(int32 Index);
	void LeaveRace();

private:
	IOnlineSessionPtr GetSteamSessions() const;
	void Report(const FString& Message) const;
	void OnCreateComplete(FName SessionName, bool bSuccess);
	void OnFindComplete(bool bSuccess);
	void OnJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroyComplete(FName SessionName, bool bSuccess);
	void DestroyRaceSession(bool bReturnToMap);
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& Error);
	void OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error);
	void ClearDelegates();

	TSharedPtr<FOnlineSessionSearch> Search;
	IOnlineSessionPtr ActiveSessions;
	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle JoinHandle;
	FDelegateHandle DestroyHandle;
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
	bool bBusy = false;
	bool bReturnToMapAfterDestroy = false;
	bool bAwaitingConnection = false;
};
