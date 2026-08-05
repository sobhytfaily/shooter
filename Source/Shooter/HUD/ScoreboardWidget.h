// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScoreboardWidget.generated.h"

/**
 * Full scoreboard shown while Tab is held.
 *
 * In WBP_Scoreboard (Widget Blueprint derived from this class) you need:
 *   - A UVerticalBox named "PlayerListBox"  (bound via BindWidget)
 *   - A TSubclassOf<UScoreboardRow> set in the "Row Class" property
 */
UCLASS()
class SHOOTER_API UScoreboardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Reads GameState->PlayerArray, sorts by kills, populates the list.
	// Called automatically in NativeConstruct and whenever the widget is shown.
	UFUNCTION(BlueprintCallable, Category = "Scoreboard")
	void RefreshScoreboard();

	// The vertical box that rows are added into — bind this in the Widget Blueprint
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* PlayerListBox;

	// Set this to WBP_ScoreboardRow in the Widget Blueprint default properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	TSubclassOf<class UScoreboardRow> RowClass;

protected:
	virtual void NativeConstruct() override;
};
