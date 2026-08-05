// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "MultiplayerSessionsSubsystem.h"
#include "TimerManager.h"

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Start the lobby timer when the lobby map loads
	GetWorldTimerManager().SetTimer(
		LobbyTimerHandle,
		this,
		&ALobbyGameMode::StartMatch,
		LobbyWaitTime,
		false  // Don't loop
	);
}

void ALobbyGameMode::PostLogin(APlayerController* newPlayer)
{
	Super::PostLogin(newPlayer);

	int32 numberOfPlayers = GameState.Get()->PlayerArray.Num();

	// Optional: Start match early if lobby is full
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (Subsystem && numberOfPlayers >= Subsystem->DesiredNumPublicConnections)
		{
			// Lobby is full, start match immediately
			if (!bMatchStarted)
			{
				// Clear the timer and start now
				GetWorldTimerManager().ClearTimer(LobbyTimerHandle);
				StartMatch();
			}
		}
	}
}

void ALobbyGameMode::StartMatch()
{
	if (bMatchStarted) return;  // Prevent multiple calls

	bMatchStarted = true;

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (Subsystem)
		{
			UWorld* world = GetWorld();
			if (world)
			{
				bUseSeamlessTravel = true;
				FString MatchType = Subsystem->DesiredMatchType;

				if (MatchType == "FreeForAll")
				{
					world->ServerTravel(FString("/Game/Maps/ShooterMap?listen"));
				}
				else if (MatchType == "Teams")
				{
					world->ServerTravel(FString("/Game/Maps/Teams?listen"));
				}
				else if (MatchType == "CaptureTheFlag")
				{
					world->ServerTravel(FString("/Game/Maps/CaptureTheFlag?listen"));
				}
			}
		}
	}
}
