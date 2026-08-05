// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScoreboardRow.generated.h"

/**
 * One row in the scoreboard: Player Name | Kills | Deaths | K/D
 * Bind the four Text Blocks in the Widget Blueprint to these properties.
 */
UCLASS()
class SHOOTER_API UScoreboardRow : public UUserWidget
{
	GENERATED_BODY()

public:
	// Call this to populate the row with player data
	void SetRowData(const FString& PlayerName, int32 Kills, int32 Deaths);

	// Bound in WBP_ScoreboardRow via the "Is Variable" checkbox
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* KillsText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DeathsText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* KDRatioText;
};
