// Fill out your copyright notice in the Description page of Project Settings.

#include "ScoreboardWidget.h"
#include "ScoreboardRow.h"
#include "Components/VerticalBox.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Shooter/PlayerState/ShooterPlayerState.h"

void UScoreboardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshScoreboard();
}

void UScoreboardWidget::RefreshScoreboard()
{
	if (PlayerListBox == nullptr || RowClass == nullptr) return;

	// Clear old rows
	PlayerListBox->ClearChildren();

	// Collect all ShooterPlayerStates from the replicated PlayerArray
	TArray<AShooterPlayerState*> SortedPlayers;
	AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (GS == nullptr) return;

	for (APlayerState* PS : GS->PlayerArray)
	{
		AShooterPlayerState* SPS = Cast<AShooterPlayerState>(PS);
		if (SPS)
		{
			SortedPlayers.Add(SPS);
		}
	}

	// Sort descending by kills (Score)
	SortedPlayers.Sort([](const AShooterPlayerState& A, const AShooterPlayerState& B)
	{
		return A.GetScore() > B.GetScore();
	});

	// Create one row widget per player
	for (AShooterPlayerState* SPS : SortedPlayers)
	{
		UScoreboardRow* Row = CreateWidget<UScoreboardRow>(GetOwningPlayer(), RowClass);
		if (Row)
		{
			Row->SetRowData(
				SPS->GetPlayerName(),
				FMath::FloorToInt(SPS->GetScore()),
				SPS->GetDeaths()
			);
			PlayerListBox->AddChildToVerticalBox(Row);
		}
	}
}
