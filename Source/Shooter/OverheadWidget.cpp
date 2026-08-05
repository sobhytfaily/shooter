// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"
#include "Shooter/ShooterCharacter.h"
#include "Shooter/PlayerState/ShooterPlayerState.h"
#include "Components/TextBlock.h"

void UOverheadWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!FWorldDelegates::LevelRemovedFromWorld.IsBoundToObject(this))
    {
        FWorldDelegates::LevelRemovedFromWorld.AddUObject(this, &UOverheadWidget::OnLevelRemovedFromWorld);
    }
}

void UOverheadWidget::NativeDestruct()
{
    FWorldDelegates::LevelRemovedFromWorld.RemoveAll(this);
    Super::NativeDestruct();
}

void UOverheadWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    FWorldDelegates::LevelRemovedFromWorld.RemoveAll(this);
}

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	SetDisplayText("fd");
}

void UOverheadWidget::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	RemoveFromParent();
	FWorldDelegates::LevelRemovedFromWorld.RemoveAll(this);
}
void UOverheadWidget::ShowPlayerName(AShooterCharacter* InCharacter)
{
    if (InCharacter)
    {
        APlayerState* PlayerState = InCharacter->GetPlayerState();
        if (PlayerState)
        {
            FString PlayerName = PlayerState->GetPlayerName();
            SetDisplayText(PlayerName);
        }
    }
}