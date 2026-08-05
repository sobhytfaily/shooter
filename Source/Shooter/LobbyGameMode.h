// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* newPlayer) override;
	virtual void BeginPlay() override;

	// Timer-based lobby transition settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	float LobbyWaitTime = 30.f;  // Seconds to wait before starting match

private:
	FTimerHandle LobbyTimerHandle;
	void StartMatch();
	bool bMatchStarted = false;
};
