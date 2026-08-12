// Fill out your copyright notice in the Description page of Project Settings.

#include "ScoreboardRow.h"
#include "Components/TextBlock.h"

void UScoreboardRow::SetRowData(const FString& PlayerName, int32 Kills, int32 Deaths)
{
	if (PlayerNameText)
	{
		PlayerNameText->SetText(FText::FromString(PlayerName));
	}
	if (KillsText)
	{
		KillsText->SetText(FText::AsNumber(Kills));
	}
	if (DeathsText)
	{
		DeathsText->SetText(FText::AsNumber(Deaths));
	}
	if (KDRatioText)
	{
		float KD = Deaths > 0 ? static_cast<float>(Kills) / static_cast<float>(Deaths) : static_cast<float>(Kills);
		KDRatioText->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), KD)));
	}
}
