// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

// Forward declarations
class UTextBlock;
class UImage;
class UWidgetAnimation;

/**
 * 
 */
UCLASS()
class SHOOTER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
	

public:

	UPROPERTY(meta = (BindWidget))
		class UImage* LowHealth;

	UPROPERTY(meta = (BindWidget))
		 UTextBlock* ScoreAmount;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* GreenTeamScore;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* GreyTeamScore;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* DeathAmount;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* ScoreSpacerText;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* WeaponAmmoAmount;
	UPROPERTY(meta = (BindWidget))
		UTextBlock* CarriedWeaponAmmount;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* MatchCountDownText;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* GernadeText;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* SmokeGernadeText;

	UPROPERTY(meta = (BindWidget))
		 UImage* HighPingImage;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
		UWidgetAnimation* HighPingAnimation;

	UPROPERTY(meta = (BindWidget))
		 UImage* DamageIndicator;
private:

};
