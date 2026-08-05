// Fill out your copyright notice in the Description page of Project Settings.


#include "ReturnToMainMenu.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Shooter/ShooterCharacter.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"

void UReturnToMainMenu::MenuSetup()
{
	AddToViewport(100);  // High Z-order to ensure it's on top
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	UWorld* World = GetWorld();
	if (World)
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController)
		{
			FInputModeGameAndUI InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputModeData.SetHideCursorDuringCapture(false);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);

			// Force focus to the widget
			SetKeyboardFocus();
		}
	}

	if (ReturnButton && !ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}

	if (SensitivitySlider && !SensitivitySlider->OnValueChanged.IsBound())
	{
		SensitivitySlider->OnValueChanged.AddDynamic(this, &UReturnToMainMenu::OnSensitivityChanged);
		LoadSensitivity();
	}

	// Bind resolution buttons
	if (Resolution1080pButton && !Resolution1080pButton->OnClicked.IsBound())
	{
		Resolution1080pButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::On1080pButtonClicked);
	}
	if (Resolution1440pButton && !Resolution1440pButton->OnClicked.IsBound())
	{
		Resolution1440pButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::On1440pButtonClicked);
	}
	if (Resolution4KButton && !Resolution4KButton->OnClicked.IsBound())
	{
		Resolution4KButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::On4KButtonClicked);
	}

	// Update resolution display
	UpdateResolutionText();
	HighlightCurrentResolution();

	// Bind graphics quality buttons
	if (LowQualityButton && !LowQualityButton->OnClicked.IsBound())
	{
		LowQualityButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::OnLowQualityButtonClicked);
	}
	if (MediumQualityButton && !MediumQualityButton->OnClicked.IsBound())
	{
		MediumQualityButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::OnMediumQualityButtonClicked);
	}
	if (HighQualityButton && !HighQualityButton->OnClicked.IsBound())
	{
		HighQualityButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::OnHighQualityButtonClicked);
	}

	// Load current graphics quality
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		CurrentGraphicsQuality = Settings->GetOverallScalabilityLevel();
		// If it returns -1 or invalid, default to Medium
		if (CurrentGraphicsQuality < 0 || CurrentGraphicsQuality > 3)
		{
			CurrentGraphicsQuality = 1; // Medium
		}
	}

	// Update graphics quality display
	UpdateGraphicsQualityText();
	HighlightCurrentGraphicsQuality();


	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &UReturnToMainMenu::OnDestroySession);
		}
	}
}

bool UReturnToMainMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	return true;
}

void UReturnToMainMenu::OnDestroySession(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] OnDestroySession bWasSuccessful=%d"), bWasSuccessful);

	if (!bWasSuccessful)
	{
		if (ReturnButton)
		{
			ReturnButton->SetIsEnabled(true);
		}
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// Host: kick everyone (RPC clients to leave) before we travel ourselves.
	AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>();
	if (GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] Host path: ReturnToMainMenuHost()"));
		GameMode->ReturnToMainMenuHost();
		// ReturnToMainMenuHost iterates PCs and calls ClientReturnToMainMenu on each,
		// which includes the host's local PC. That's enough for the host.
		return;
	}

	// Client path: instead of relying on ClientReturnToMainMenuWithTextReason
	// (which is fragile in packaged builds), travel directly to the main menu map.
	UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] Client path: OpenLevel(StartGameMap)"));
	UGameplayStatics::OpenLevel(this, FName(TEXT("StartGameMap")));
}


void UReturnToMainMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController)
		{
			FInputModeGameOnly InputModData;
			PlayerController->SetInputMode(InputModData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
	if (ReturnButton && ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}
	if (SensitivitySlider && SensitivitySlider->OnValueChanged.IsBound())
	{
		SensitivitySlider->OnValueChanged.RemoveDynamic(this, &UReturnToMainMenu::OnSensitivityChanged);
	}
	if (Resolution1080pButton && Resolution1080pButton->OnClicked.IsBound())
	{
		Resolution1080pButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::On1080pButtonClicked);
	}
	if (Resolution1440pButton && Resolution1440pButton->OnClicked.IsBound())
	{
		Resolution1440pButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::On1440pButtonClicked);
	}
	if (Resolution4KButton && Resolution4KButton->OnClicked.IsBound())
	{
		Resolution4KButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::On4KButtonClicked);
	}
	if (LowQualityButton && LowQualityButton->OnClicked.IsBound())
	{
		LowQualityButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::OnLowQualityButtonClicked);
	}
	if (MediumQualityButton && MediumQualityButton->OnClicked.IsBound())
	{
		MediumQualityButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::OnMediumQualityButtonClicked);
	}
	if (HighQualityButton && HighQualityButton->OnClicked.IsBound())
	{
		HighQualityButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::OnHighQualityButtonClicked);
	}
	if (MultiplayerSessionsSubsystem && MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
	{
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(this, &UReturnToMainMenu::OnDestroySession);
	}

}

void UReturnToMainMenu::ReturnButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] ReturnButtonClicked"));
	ReturnButton->SetIsEnabled(false);

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* FirstPlayerController = World->GetFirstPlayerController();
		if (FirstPlayerController)
		{
			AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(FirstPlayerController->GetPawn());
			if (ShooterCharacter)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] Calling ServerLeaveGame on character"));
				ShooterCharacter->ServerLeaveGame();
				ShooterCharacter->OnLeftGame.AddDynamic(this, &UReturnToMainMenu::OnPlayerLeftGame);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] No ShooterCharacter found! Re-enabling button"));
				ReturnButton->SetIsEnabled(true);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] No PlayerController"));
		}
	}
}

void UReturnToMainMenu::OnPlayerLeftGame()
{
	UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] OnPlayerLeftGame fired - destroying session"));
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->DestroySession();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ReturnToMainMenu] No MultiplayerSessionsSubsystem!"));
	}
}

void UReturnToMainMenu::OnSensitivityChanged(float Value)
{
	// Slider range 0.0 to 1.0, map to sensitivity range 0.1 to 3.0
	float Sensitivity = FMath::Lerp(0.1f, 3.0f, Value);

	// Update the text display - show slider value (0-1) not internal value
	if (SensitivityText)
	{
		FString SensitivityString = FString::Printf(TEXT("%.2f"), Value);
		SensitivityText->SetText(FText::FromString(SensitivityString));
	}

	// Save to config
	SaveSensitivity(Sensitivity);
}

void UReturnToMainMenu::LoadSensitivity()
{
	if (GConfig)
	{
		float Sensitivity = 1.0f; // Default sensitivity
		GConfig->GetFloat(TEXT("Shooter.Settings"), TEXT("MouseSensitivity"), Sensitivity, GGameUserSettingsIni);

		// Set slider value (map from 0.1-3.0 to 0.0-1.0)
		float SliderValue = FMath::GetMappedRangeValueClamped(FVector2D(0.1f, 3.0f), FVector2D(0.0f, 1.0f), Sensitivity);
		if (SensitivitySlider)
		{
			SensitivitySlider->SetValue(SliderValue);
		}

		// Update text with slider value (0-1), not internal sensitivity value
		if (SensitivityText)
		{
			FString SensitivityString = FString::Printf(TEXT("%.2f"), SliderValue);
			SensitivityText->SetText(FText::FromString(SensitivityString));
		}
	}
}

void UReturnToMainMenu::SaveSensitivity(float Sensitivity)
{
	if (GConfig)
	{
		GConfig->SetFloat(TEXT("Shooter.Settings"), TEXT("MouseSensitivity"), Sensitivity, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void UReturnToMainMenu::UpdateSensitivityText(float Value)
{
	if (SensitivityText)
	{
		FString SensitivityString = FString::Printf(TEXT("%.2f"), Value);
		SensitivityText->SetText(FText::FromString(SensitivityString));
	}
}

// ==================== RESOLUTION FUNCTIONS ====================

void UReturnToMainMenu::On1080pButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("1080p Button Clicked!"));
	SetResolution(1920, 1080);
}

void UReturnToMainMenu::On1440pButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("1440p Button Clicked!"));
	SetResolution(2560, 1440);
}

void UReturnToMainMenu::On4KButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("4K Button Clicked!"));
	SetResolution(3840, 2160);
}

void UReturnToMainMenu::SetResolution(int32 Width, int32 Height)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		// Set screen resolution
		Settings->SetScreenResolution(FIntPoint(Width, Height));

		// Set to fullscreen (you can change to Windowed or WindowedFullscreen if preferred)
		Settings->SetFullscreenMode(EWindowMode::Fullscreen);

		// Apply and save settings
		Settings->ApplySettings(false);
		Settings->SaveSettings();

		// Update UI
		UpdateResolutionText();
		HighlightCurrentResolution();
	}
}

void UReturnToMainMenu::UpdateResolutionText()
{
	if (CurrentResolutionText)
	{
		UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
		if (Settings)
		{
			FIntPoint Resolution = Settings->GetScreenResolution();
			FString ResolutionString = FString::Printf(TEXT("Current: %dx%d"), Resolution.X, Resolution.Y);
			CurrentResolutionText->SetText(FText::FromString(ResolutionString));
		}
	}
}

void UReturnToMainMenu::HighlightCurrentResolution()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings) return;

	FIntPoint CurrentRes = Settings->GetScreenResolution();

	// Reset all button colors to normal
	if (Resolution1080pButton)
	{
		Resolution1080pButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
	if (Resolution1440pButton)
	{
		Resolution1440pButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
	if (Resolution4KButton)
	{
		Resolution4KButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}

	// Highlight the current resolution button
	if (CurrentRes.X == 1920 && CurrentRes.Y == 1080 && Resolution1080pButton)
	{
		Resolution1080pButton->SetBackgroundColor(FLinearColor(0.0f, 1.0f, 0.0f, 1.0f)); // Green
	}
	else if (CurrentRes.X == 2560 && CurrentRes.Y == 1440 && Resolution1440pButton)
	{
		Resolution1440pButton->SetBackgroundColor(FLinearColor(0.0f, 1.0f, 0.0f, 1.0f)); // Green
	}
	else if (CurrentRes.X == 3840 && CurrentRes.Y == 2160 && Resolution4KButton)
	{
		Resolution4KButton->SetBackgroundColor(FLinearColor(0.0f, 1.0f, 0.0f, 1.0f)); // Green
	}
}

// ==================== GRAPHICS QUALITY FUNCTIONS ====================

void UReturnToMainMenu::OnLowQualityButtonClicked()
{
	SetGraphicsQuality(0); // Low
}

void UReturnToMainMenu::OnMediumQualityButtonClicked()
{
	SetGraphicsQuality(1); // Medium
}

void UReturnToMainMenu::OnHighQualityButtonClicked()
{
	SetGraphicsQuality(2); // High
}

void UReturnToMainMenu::SetGraphicsQuality(int32 QualityLevel)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		// Store the quality level
		CurrentGraphicsQuality = QualityLevel;

		// Set overall scalability quality
		// 0 = Low, 1 = Medium, 2 = High, 3 = Epic
		Settings->SetOverallScalabilityLevel(QualityLevel);

		// Apply and save settings
		Settings->ApplySettings(false);
		Settings->SaveSettings();

		// Update UI
		UpdateGraphicsQualityText();
		HighlightCurrentGraphicsQuality();
	}
}																				

void UReturnToMainMenu::UpdateGraphicsQualityText()
{
	if (CurrentQualityText)
	{
		FString QualityString;

		switch (CurrentGraphicsQuality)
		{
		case 0:
			QualityString = TEXT("Current: Low");
			break;
		case 1:
			QualityString = TEXT("Current: Medium");
			break;
		case 2:
			QualityString = TEXT("Current: High");
			break;
		case 3:
			QualityString = TEXT("Current: Epic");
			break;
		default:
			QualityString = TEXT("Current: Medium");
			break;
		}

		CurrentQualityText->SetText(FText::FromString(QualityString));
	}
}

void UReturnToMainMenu::HighlightCurrentGraphicsQuality()
{
	// Reset all button colors to normal
	if (LowQualityButton)
	{
		LowQualityButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
	if (MediumQualityButton)
	{
		MediumQualityButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
	if (HighQualityButton)
	{
		HighQualityButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}

	// Highlight the current quality button
	if (CurrentGraphicsQuality == 0 && LowQualityButton)
	{
		LowQualityButton->SetBackgroundColor(FLinearColor(0.0f, 1.0f, 0.0f, 1.0f)); // Green
	}
	else if (CurrentGraphicsQuality == 1 && MediumQualityButton)
	{
		MediumQualityButton->SetBackgroundColor(FLinearColor(0.0f, 1.0f, 0.0f, 1.0f)); // Green
	}
	else if (CurrentGraphicsQuality == 2 && HighQualityButton)
	{
		HighQualityButton->SetBackgroundColor(FLinearColor(0.0f, 1.0f, 0.0f, 1.0f)); // Green
	}
}
