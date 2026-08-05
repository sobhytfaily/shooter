// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenu.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API UReturnToMainMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	void MenuSetup();
	void MenuTearDown();

protected:
	virtual bool Initialize() override;

	UFUNCTION()
		void OnDestroySession(bool bWasSuccessful);

	UFUNCTION()
		void OnPlayerLeftGame();

	UFUNCTION()
		void OnSensitivityChanged(float Value);

	UFUNCTION()
		void On1080pButtonClicked();

	UFUNCTION()
		void On1440pButtonClicked();

	UFUNCTION()
		void On4KButtonClicked();

	UFUNCTION()
		void OnLowQualityButtonClicked();

	UFUNCTION()
		void OnMediumQualityButtonClicked();

	UFUNCTION()
		void OnHighQualityButtonClicked();

private:
	UPROPERTY(meta = (BindWidget))
		class UButton* ReturnButton;

	UPROPERTY(meta = (BindWidget))
		class USlider* SensitivitySlider;

	UPROPERTY(meta = (BindWidget))
		class UTextBlock* SensitivityText;

	// Resolution buttons
	UPROPERTY(meta = (BindWidget))
		class UButton* Resolution1080pButton;

	UPROPERTY(meta = (BindWidget))
		class UButton* Resolution1440pButton;

	UPROPERTY(meta = (BindWidget))
		class UButton* Resolution4KButton;

	UPROPERTY(meta = (BindWidget))
		class UTextBlock* CurrentResolutionText;

	// Graphics quality buttons
	UPROPERTY(meta = (BindWidget))
		class UButton* LowQualityButton;

	UPROPERTY(meta = (BindWidget))
		class UButton* MediumQualityButton;

	UPROPERTY(meta = (BindWidget))
		class UButton* HighQualityButton;

	UPROPERTY(meta = (BindWidget))
		class UTextBlock* CurrentQualityText;

	UFUNCTION()
		void ReturnButtonClicked();

	void LoadSensitivity();
	void SaveSensitivity(float Sensitivity);
	void UpdateSensitivityText(float Value);

	void SetResolution(int32 Width, int32 Height);
	void UpdateResolutionText();
	void HighlightCurrentResolution();

	void SetGraphicsQuality(int32 QualityLevel);
	void UpdateGraphicsQualityText();
	void HighlightCurrentGraphicsQuality();

	UPROPERTY()
		class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	UPROPERTY()
		APlayerController* PlayerController;

	// Track current graphics quality level
	int32 CurrentGraphicsQuality = 1; // Default to Medium


};
