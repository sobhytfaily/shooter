// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon.h"
#include "ShooterAnimInstance.h"
#include "Shooter/GameState/ShooterGameState.h"
#include "Shooter/PlayerStart/TeamPlayerStart.h"
#include "Shooter/ShooterComponent/CombatComponent.h"
#include "Shooter/ShooterComponent/BuffComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Shooter/Shooter.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "ShooterGameMode.h"
#include "TimerManager.h"
#include "Shooter/PlayerState/ShooterPlayerState.h"
#include "Shooter/Weapon/WeaponTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/boxComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Shooter/ShooterComponent/LagCompensationComponent.h"
#include "OverheadWidget.h"

AShooterCharacter::AShooterCharacter()
{

	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->bUsePawnControlRotation = true;
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent); 

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Ignore);

	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);

	// Movement component settings
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// Network smoothing settings to prevent jitter
	GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Linear;
	GetCharacterMovement()->NetworkMaxSmoothUpdateDistance = 256.f;  // Increased from default 64
	GetCharacterMovement()->NetworkNoSmoothUpdateDistance = 512.f;   // Increased from default 128
	GetCharacterMovement()->ListenServerNetworkSimulatedSmoothLocationTime = 0.04f; // Faster smoothing
	GetCharacterMovement()->ListenServerNetworkSimulatedSmoothRotationTime = 0.04f; // Faster smoothing

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	SetNetUpdateFrequency(66.f);
	SetMinNetUpdateFrequency(33.f);
	
	DesolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DesolveTimelineComponent"));
	AttachedGernade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GernadeMesh"));
	AttachedGernade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachedGernade->SetupAttachment(GetMesh(), FName("RightHandSocket"));
	MaskMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MaskMesh"));
	MaskMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MaskMesh->SetupAttachment(GetMesh(), FName("headSocket"));
	MaskMesh->SetVisibility(true);

	//HitBoxies for server side rewind

	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	HitCollisionBoxies.Add(FName("head"), head);


	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitCollisionBoxies.Add(FName("pelvis"), pelvis);

	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxies.Add(FName("spine_02"), spine_02);

	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxies.Add(FName("spine_03"), spine_03);

	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxies.Add(FName("upperarm_l"), upperarm_l);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxies.Add(FName("upperarm_r"), upperarm_r);

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxies.Add(FName("lowerarm_l"), lowerarm_l);

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxies.Add(FName("lowerarm_r"), lowerarm_r);

	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxies.Add(FName("hand_l"), hand_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxies.Add(FName("hand_r"), hand_r);

	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxies.Add(FName("thigh_l"), thigh_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitCollisionBoxies.Add(FName("thigh_r"), thigh_r);

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxies.Add(FName("calf_l"), calf_l);

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxies.Add(FName("calf_r"), calf_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxies.Add(FName("foot_l"), foot_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxies.Add(FName("foot_r"), foot_r);

	for (auto Box : HitCollisionBoxies)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}
}


void AShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(AShooterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(AShooterCharacter, Health);
	DOREPLIFETIME(AShooterCharacter, bDisableGameplay);
	DOREPLIFETIME(AShooterCharacter, bIsSprinting);
}
void AShooterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{ 
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;	
	}
	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<AShooterPlayerController>(Controller);
		}
	}
	
}

void AShooterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Initialize walk speed to correct value on spawn
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}

	UpdateHudHealth();
	GetMesh()->UnHideBoneByName(FName("neck_01"));

	// Save Blueprint/editor camera values before any code touches them
	if (CameraBoom)
	{
		DefaultArmLength    = CameraBoom->TargetArmLength;
		DefaultSocketOffset = CameraBoom->SocketOffset;
	}

	// Start in third person - camera values managed by the engine/blueprint
	if (IsLocallyControlled())
	{
		inFirst = false;
		SetFirstPersonBodyVisibility(false);
		if (MaskMesh) MaskMesh->SetVisibility(false);
	}

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AShooterCharacter::RecieveDamage);
	}
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 100.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint
		);
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSoundSpawn,
			GetActorLocation()
		);
	}
	if (AttachedGernade)
	{
		AttachedGernade->SetVisibility(false);
	}
	SpawnDefaultWeapon();
	UpdateHudAmmo();
	// Do NOT call ChangeFOV() on respawn — camera state is managed by the engine/blueprint.
	// Calling it here would flip to first-person on every respawn.
	Changefov = false;
	inFirst = false;
	IntializedHud = false;

	// Ensure weapon visibility is correct after spawning default weapon in first person
	if (IsLocallyControlled() && inFirst)
	{
		HideCameraWhenCharacterClose();
	}
}
void AShooterCharacter::UpdateHudHealth()
{
	PlayerController = PlayerController == nullptr ? Cast<AShooterPlayerController>(Controller) : PlayerController;
	if (PlayerController)
	{
		// Health bar removed - commented out
		// PlayerController->SetHudHealth(Health, MaxHealth);

		// Update damage overlay (blood effect) based on health
		PlayerController->SetHudDamageOverlay(Health, MaxHealth);
	}
}

void AShooterCharacter::RegenerateHealth(float DeltaTime)
{
	// Only regenerate on server or in single player
	if (!HasAuthority()) return;

	// Don't regenerate if dead or already at full health
	if (bElimed || Health >= MaxHealth) 
	{
		bIsRegeneratingHealth = false;
		return;
	}

	// Increment time since last damage
	TimeSinceLastDamage += DeltaTime;

	// Check if we should start regenerating
	if (TimeSinceLastDamage >= HealthRegenDelay)
	{
		bIsRegeneratingHealth = true;

		// Regenerate health
		float HealthToAdd = HealthRegenRate * DeltaTime;
		Health = FMath::Clamp(Health + HealthToAdd, 0.f, MaxHealth);

		// Update HUD
		UpdateHudHealth();
	}
	else
	{
		bIsRegeneratingHealth = false;
	}
}

void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Combat && Combat->EquippedWeapon && !IntializedHud)
	{
		Combat->EquippedWeapon->ShowPickUpWidget(false);
		Combat->UpdateCarriedAmmo();
		Combat->UpdateHudGerandes();
		IntializedHud = true;
	}
	UpdateHudAmmo();
	if (!Combat)
	{
		Combat = Cast<UCombatComponent>(getCombatComponent());
		Combat->Character = this;
	}

	// Handle health regeneration
	RegenerateHealth(DeltaTime);

	// Handle sprint
	UpdateSprint();

	RotateInPlace(DeltaTime);
	DeltaTimeFov = DeltaTime;
	if (inFirst)
	{
		if (bIsCrouched)
		{
			CameraBoom->SocketOffset.Z = FMath::FInterpTo(CameraBoom->SocketOffset.Z, -20.f, DeltaTime, 20.f); 
		}
		else
		{
			CameraBoom->SocketOffset.Z = FMath::FInterpTo(CameraBoom->SocketOffset.Z, 10.f, DeltaTime, 20.f);
		}
	}
	HideCameraWhenCharacterClose();

	PollInit();
	if (Changefov)
	{
		if (!inFirst)
		{
			// Switching TO First Person
			if (CameraBoom && FollowCamera && GetMesh())
			{
				CameraBoom->TargetArmLength = 0.f;
				CameraBoom->SocketOffset = FVector(0.f, 0.f, 0.f);
				FollowCamera->SetFieldOfView(90.f);
				inFirst = true;

				if (IsLocallyControlled())
				{
					SetFirstPersonBodyVisibility(true);
					MaskMesh->SetVisibility(false);
				}
				Changefov = false;
			}
		}
		else
		{
			// Switching TO Third Person
			// Show meshes immediately at start of transition
			if (IsLocallyControlled() && inFirst)
			{
				SetFirstPersonBodyVisibility(false);
				MaskMesh->SetVisibility(true);
			}

			CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, DefaultArmLength, DeltaTime, 10.f);
			CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, DefaultSocketOffset, DeltaTime, 20.f);

			if (FMath::IsNearlyEqual(CameraBoom->TargetArmLength, DefaultArmLength, 1.f))
			{
				CameraBoom->TargetArmLength = DefaultArmLength;
				CameraBoom->SocketOffset = DefaultSocketOffset;
				Changefov = false;
				inFirst = false;
			}
		}
	}

}
void AShooterCharacter::RotateInPlace(float DeltaTime)
{
	if (Combat && Combat->bHoldingTheFlag)
	{
		bUseControllerRotationYaw = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	if (Combat && Combat->EquippedWeapon)
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		bUseControllerRotationYaw = true;
	}
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateYO_Ptich();
	}
}
void AShooterCharacter::UpdateHudAmmo()
{
	PlayerController = PlayerController == nullptr ? Cast<AShooterPlayerController>(Controller) : PlayerController;
	if (PlayerController && Combat && Combat->EquippedWeapon)
	{
		PlayerController->SetHudCarriedAmmo(Combat->CarriedAmmo);
		PlayerController->SetHudWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
	}

}
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AShooterCharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUp);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &AShooterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AShooterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AShooterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &AShooterCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AShooterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AShooterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AShooterCharacter::ReloadButtonPressed);

	// Grenade - hold to aim, release to throw
	PlayerInputComponent->BindAction("Gernade", IE_Pressed, this, &AShooterCharacter::GernadeButtonPressed);
	PlayerInputComponent->BindAction("Gernade", IE_Released, this, &AShooterCharacter::GernadeButtonReleased);

	PlayerInputComponent->BindAction("ChangeFOV", IE_Pressed, this, &AShooterCharacter::ChangeFOV);
	PlayerInputComponent->BindAction("SwapToPrimary", IE_Pressed, this, &AShooterCharacter::SwapToPrimaryWeapon);
	PlayerInputComponent->BindAction("SwapToSecondary", IE_Pressed, this, &AShooterCharacter::SwapToSecondaryWeapon);

	// Smoke Grenade - hold to aim, release to throw
	PlayerInputComponent->BindAction("SmokeGernade", IE_Pressed, this, &AShooterCharacter::SmokeGernadeButtonPressed);
	PlayerInputComponent->BindAction("SmokeGernade", IE_Released, this, &AShooterCharacter::SmokeGernadeButtonReleased);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AShooterCharacter::SprintButtonPressed);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AShooterCharacter::SprintButtonReleased);
	PlayerInputComponent->BindAction("SprintToggle", IE_Pressed, this, &AShooterCharacter::SprintToggleButtonPressed);

}
void AShooterCharacter::MoveForward(float Value)
{
	if (bDisableGameplay)
	{
		return;
	}

	// Handle ladder climbing
	if (bIsOnLadder)
	{
		AddMovementInput(FVector::UpVector, Value);
		return;
	}

	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotator(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotator).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}
void AShooterCharacter::MoveRight(float Value)
{
	if (bDisableGameplay)
	{
		return;
	}
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotator(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotator).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}
void AShooterCharacter::Turn(float Value)
{
	float Sensitivity = 1.0f;
	if (GConfig)
	{
		GConfig->GetFloat(TEXT("Shooter.Settings"), TEXT("MouseSensitivity"), Sensitivity, GGameUserSettingsIni);
	}
	// Reduce look speed when scoped in with sniper on controller
	if (Combat && Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle &&
		Combat->bAiming)
	{
		Sensitivity *= 0.35f;
	}
	AddControllerYawInput(Value * Sensitivity);
}
void AShooterCharacter::LookUp(float Value)
{
	float Sensitivity = 1.0f;
	if (GConfig)
	{
		GConfig->GetFloat(TEXT("Shooter.Settings"), TEXT("MouseSensitivity"), Sensitivity, GGameUserSettingsIni);
	}
	// Reduce look speed when scoped in with sniper on controller
	if (Combat && Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle &&
		Combat->bAiming)
	{
		Sensitivity *= 0.35f;
	}
	AddControllerPitchInput(Value * Sensitivity);
}
void AShooterCharacter::EquipButtonPressed()
{
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		if (Combat->bHoldingTheFlag)
		{
			return;
		}
		// Only pick up weapon if overlapping one
		if (OverlappingWeapon && Combat->CombatState == ECombatState::ECS_Unoccupied)
		{
			ServerEquipButtonPressed();
		}
	}
}
void AShooterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon); 
		}
		else
		{
			if (Combat->ShouldSwapWeapons())
			{
				Combat->SwapWeapon();
			}
		}
	}
}
void AShooterCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat && Combat->bHoldingTheFlag)
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else{
		Crouch();
	}
}
void AShooterCharacter::CalculateYO_Ptich()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from range {270, 360} to { -90, 0} 
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}
void AShooterCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;

	// In first-person, always rotate with controller (no turn-in-place delay)
	if (inFirst)
	{
		bUseControllerRotationYaw = true;
		bRotateRootBone = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		AO_Yaw = 0.f;
		return;
	}

	float speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	if (speed == 0.f && !bIsInAir)
	{
		   bRotateRootBone = true;
		   FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		   FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		   AO_Yaw = DeltaAimRotation.Yaw;
		   if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		   {
			   InterpAO_Yaw = AO_Yaw;
		   }
		   bUseControllerRotationYaw = true;
		   TurnInPlace(DeltaTime);
	}
	if (speed > 0.f || bIsInAir)
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 5.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);

		}
	}
	CalculateYO_Ptich();
}
void AShooterCharacter::ChangeFOV()
{
	Changefov = true;
}

void AShooterCharacter::SetFirstPersonBodyVisibility(bool bFirstPerson)
{
	if (!GetMesh()) return;

	EPhysBodyOp PhysOp = EPhysBodyOp::PBO_None;

	if (bFirstPerson)
	{
		// FIRST PERSON: Hide head, neck, and legs only
		// DON'T hide spine - it would hide arms too (children)
		// The camera is inside the body so you won't see the torso anyway

		// Hide head and neck
		GetMesh()->HideBoneByName(FName("head"), PhysOp);
		GetMesh()->HideBoneByName(FName("neck_01"), PhysOp);

		// Hide legs (these don't have arms as children)
		GetMesh()->HideBoneByName(FName("thigh_l"), PhysOp);
		GetMesh()->HideBoneByName(FName("thigh_r"), PhysOp);

		// Arms stay visible because we don't hide spine!
	}
	else
	{
		// THIRD PERSON: Show everything
		GetMesh()->UnHideBoneByName(FName("head"));
		GetMesh()->UnHideBoneByName(FName("neck_01"));
		GetMesh()->UnHideBoneByName(FName("thigh_l"));
		GetMesh()->UnHideBoneByName(FName("thigh_r"));
	}
}

float AShooterCharacter::CalculateSpeed() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}
void AShooterCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	float speed = CalculateSpeed();
	bRotateRootBone = false;
	if (speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	ProxyRotatorLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotatorLastFrame, ProxyRotation).Yaw;
	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}
void AShooterCharacter::GernadeButtonPressed()
{
	if (Combat)
	{
		if (Combat->bHoldingTheFlag)
		{
			return;
		}
		Combat->GrenadeButtonPressed();
	}
}

void AShooterCharacter::GernadeButtonReleased()
{
	if (Combat)
	{
		Combat->GrenadeButtonReleased();
	}
}

void AShooterCharacter::SmokeGernadeButtonPressed()
{
	if (Combat)
	{
		if (Combat->bHoldingTheFlag)
		{
			return;
		}
		Combat->SmokeGrenadeButtonPressed();
	}
}

void AShooterCharacter::SmokeGernadeButtonReleased()
{
	if (Combat)
	{
		Combat->SmokeGrenadeButtonReleased();
	}
}

void AShooterCharacter::RecieveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	GameMode = GameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : GameMode;
	if (bElimed || GameMode == nullptr) return;

	// Debug: Log damage received
	UE_LOG(LogTemp, Warning, TEXT("RecieveDamage called: Damage=%f, Health Before=%f"), Damage, Health);

	Damage = GameMode->CalculateDamage(InstigatorController, Controller ,Damage);

	// Debug: Log calculated damage
	UE_LOG(LogTemp, Warning, TEXT("Calculated Damage=%f"), Damage);

	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);

	// Debug: Log health after
	UE_LOG(LogTemp, Warning, TEXT("Health After=%f"), Health);

	// Reset regeneration timer when taking damage
	TimeSinceLastDamage = 0.f;
	bIsRegeneratingHealth = false;

	UpdateHudHealth();
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && !AnimInstance->IsAnyMontagePlaying())
	{
		PlayHitReactMontage();
	}
	if(Health == 0.f) 
	{
			PlayerController = PlayerController == nullptr ? Cast<AShooterPlayerController>(Controller) : PlayerController;
			AShooterPlayerController* AttackerContoller = Cast<AShooterPlayerController>(InstigatorController);
			// AttackerContoller can be null for environmental damage (fall damage, etc.) - guard the deref.
			if (AttackerContoller)
			{
				AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(AttackerContoller->GetCharacter());
				if (ShooterCharacter && ShooterCharacter->IsLocallyControlled())
				{
					ShooterCharacter->ShowKillMarker();
				}
			}
			GameMode->PlayerEliminated(this, PlayerController, AttackerContoller);
	}
	ServerShowDamageIndicator(DamagedActor, DamageCauser);
}

void AShooterCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// Server-authoritative: only the server applies fall damage.
	if (!HasAuthority()) return;
	if (bElimed) return;

	// Impact speed: |Z velocity| at the moment of landing (falling = negative Z).
	const float FallSpeed = FMath::Abs(GetVelocity().Z);
	if (FallSpeed <= 0.f) return;

	// Convert impact speed back to equivalent free-fall height: h = v^2 / (2g)
	float Gravity = GetCharacterMovement() ? FMath::Abs(GetCharacterMovement()->GetGravityZ()) : 980.f;
	if (Gravity <= 1.f) Gravity = 980.f; // safety
	const float FallHeight = (FallSpeed * FallSpeed) / (2.f * Gravity);

	if (FallHeight < FallDamageMinHeight) return;

	// Linear ramp: 0 damage at MinHeight, MaxHealth (lethal) at LethalHeight, clamped past that.
	const float Range = FMath::Max(1.f, FallDamageLethalHeight - FallDamageMinHeight);
	const float Alpha = FMath::Clamp((FallHeight - FallDamageMinHeight) / Range, 0.f, 1.f);
	const float Damage = Alpha * MaxHealth;

	UE_LOG(LogTemp, Warning, TEXT("[FallDamage] FallSpeed=%.1f FallHeight=%.1f Damage=%.1f"),
		FallSpeed, FallHeight, Damage);

	// nullptr InstigatorController = no kill credit to anyone. DamageCauser = self (environmental).
	UGameplayStatics::ApplyDamage(this, Damage, nullptr, this, UDamageType::StaticClass());
}

void AShooterCharacter::Jump()
{
	if (Combat && Combat->bHoldingTheFlag)
	{
		return;
	}
	if (bDisableGameplay)
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}
void AShooterCharacter::PollInit()
{
	if (ShooterPlayerState == nullptr)
	{
		ShooterPlayerState = GetPlayerState<AShooterPlayerState>();
		if (ShooterPlayerState)
		{
			OnPlayerStateIntialized();
		}
	}
}

FVector AShooterCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

void AShooterCharacter::TurnInPlace(float DeltaTime)
{
	if(AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
}
void AShooterCharacter::HideCameraWhenCharacterClose()
{
	if (!IsLocallyControlled()) return;

	// In first-person mode, always show weapon, hide mask
	if (inFirst)
	{
		// While scoped with a sniper, the scope widget covers the whole screen.
		// Hide weapon + arms/body from the owner so nothing pokes into the lens
		// when pitching down. Other players still see them normally (bOwnerNoSee
		// only hides from the local owner).
		const bool bScopedSniper =
			Combat && Combat->bAiming && Combat->EquippedWeapon
			&& Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;

		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = bScopedSniper;
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true; // Hide secondary in FPS
		}

		// Hide the character mesh (arms, body) from the owner while scoped.
		// Keep shadow casting so the player still casts a local shadow.
		if (GetMesh())
		{
			GetMesh()->SetOwnerNoSee(bScopedSniper);
			GetMesh()->bCastHiddenShadow = true;
		}

		if (MaskMesh)
		{
			MaskMesh->SetVisibility(false);
		}
		return; // Don't do the distance-based hiding in first person
	}

	// Leaving FPS branch — make sure mesh owner-visibility is restored for TPS handling below.
	if (GetMesh())
	{
		GetMesh()->SetOwnerNoSee(false);
	}

	// Third-person distance-based hiding
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
		if (MaskMesh)
		{
			MaskMesh->SetVisibility(false);
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
		if (MaskMesh)
		{
			MaskMesh->SetVisibility(true);
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}
void AShooterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHudHealth();	
	GetHitTarget();

	// Only play hit animation if health DECREASED (taking damage), not if it increased (healing)
	if (Health < LastHealth)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && !AnimInstance->IsAnyMontagePlaying())
		{
			PlayHitReactMontage();
		}
	}
}

void AShooterCharacter::UpdateDesolveMaterial(float DesolveValue)
{
	if (DynamicDesolveMaterialInstance)
	{
		DynamicDesolveMaterialInstance->SetScalarParameterValue(TEXT("Desolve"), DesolveValue);	
	}
}

void AShooterCharacter::StartDesolve()
{
	DisolveTrack.BindDynamic(this, &AShooterCharacter::UpdateDesolveMaterial);
	if(DesolveCurve && DesolveTimeline)
	{	
		DesolveTimeline->AddInterpFloat(DesolveCurve, DisolveTrack);
		DesolveTimeline->Play();	
	}
}

void AShooterCharacter::SetSpawnPoint()
{
	if (HasAuthority() && ShooterPlayerState->GetTeam() != ETeam::ET_NoTeam)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, ATeamPlayerStart::StaticClass(), PlayerStarts);
		TArray<ATeamPlayerStart*> TeamPlayersStarts;
		for (auto Start : PlayerStarts)
		{
			ATeamPlayerStart* TeamStart = Cast<ATeamPlayerStart>(Start);
			if (TeamStart && TeamStart->Team == ShooterPlayerState->GetTeam())
			{
				TeamPlayersStarts.Add(TeamStart);
			}
		}
		if (TeamPlayersStarts.Num() > 0)
		{
			ATeamPlayerStart* ChosenPlayerStart = TeamPlayersStarts[FMath::RandRange(0, TeamPlayersStarts.Num() - 1)];
			SetActorLocationAndRotation(ChosenPlayerStart->GetActorLocation(), ChosenPlayerStart->GetActorRotation());
		}
	}
}

void AShooterCharacter::OnPlayerStateIntialized()
{
	ShooterPlayerState->AddToScore(0.f);
	ShooterPlayerState->AddToDeaths(0);
	SetTeamColor(ShooterPlayerState->GetTeam());
	SetSpawnPoint();

	// Update overhead widget with player name
	if (OverheadWidget)
	{
		UOverheadWidget* OverheadWidgetComponent = Cast<UOverheadWidget>(OverheadWidget->GetUserWidgetObject());
		if (OverheadWidgetComponent)
		{
			OverheadWidgetComponent->ShowPlayerName(this);
		}
	}
}

void AShooterCharacter::SpawnDefaultWeapon()
{
	GameMode = GameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : GameMode;
	UWorld* World = GetWorld();
	if (GameMode && World && !bElimed && DefaultWeaponClass)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
		StartingWeapon->bDestroyWeapon = true;
		if (Combat)
		{
			Combat->EquipWeapon(StartingWeapon);
			StartingWeapon->GetPickUpWidget()->DestroyComponent();
		}
	}
}

void AShooterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickUpWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickUpWidget(true);
		}
	}
}
void AShooterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickUpWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickUpWidget(false);
	}
}
bool AShooterCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}
bool AShooterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}
void AShooterCharacter::PlayGernadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}
AWeapon* AShooterCharacter::GetEqiuppedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}
void AShooterCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;


	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
			if (FireMontage)
			{
			AnimInstance->Montage_Play(FireMontage);
			FName SectionName;
			SectionName = bAiming ? FName("AimRifle") : FName("RifleHip");
			AnimInstance->Montage_JumpToSection(SectionName);
			}
	}
}
void AShooterCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		if (ReloadMontage)
		{			
			AnimInstance->Montage_Play(ReloadMontage);
			FName SectionName;
			switch (Combat->EquippedWeapon->GetWeaponType())
			{
			case EWeaponType::EWT_AssualtRifle:
				SectionName = FName("Rifle");
				break;

			case EWeaponType::EWT_RocketLuncher:
				SectionName = FName("RocketLuncher");
				break;
			case EWeaponType::EWT_Pistol:
				SectionName = FName("Pistol");
				break;
			case EWeaponType::EWT_SMG:
				SectionName = FName("SMG");
				break;
			case EWeaponType::EWT_ShotGun:
				SectionName = FName("Shotgun");
				break;
			case EWeaponType::EWT_SniperRifle:
				SectionName = FName("Sniper");
				break;
			case EWeaponType::EWT_GranadeLuncher:
				SectionName = FName("GranadeLuncher");
				break;
			default:
				break;
			}

			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}
void AShooterCharacter::PlayHitReactMontage()
{
if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}
void AShooterCharacter::PlayElimMontage()
{

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}
void AShooterCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SwapMontage)
	{
		AnimInstance->Montage_Play(SwapMontage);
	}
}
void AShooterCharacter::MulticaseElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;	
	bElimed = true;
	PlayElimMontage();
	//Start DesolveEffect
	if (DesolveMaterialInstance)
	{
		DynamicDesolveMaterialInstance = UMaterialInstanceDynamic::Create(DesolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDesolveMaterialInstance);
		DynamicDesolveMaterialInstance->SetScalarParameterValue(TEXT("Desolve"), 0.55f);
		DynamicDesolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 15.f);
	}
	StartDesolve();
	//Disabling Character Movement
	bDisableGameplay = true;
	GetCharacterMovement()->DisableMovement();
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
	//Disabling Collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (AttachedGernade)
	{
		AttachedGernade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Spawn elim bot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 100.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint
			
		);
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}
	bool bHideSniperScope = IsLocallyControlled() &&
		Combat &&
		Combat->bAiming && Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	if (bHideSniperScope)
	{
		ShowSnipeScopeWidget(false);
	}
	if (MaskMesh)
	{
		MaskMesh->SetVisibility(false);
	}
	//Ammo when elimed
	if (PlayerController)
	{
		PlayerController->SetHudWeaponAmmo(0);
	}
	GetWorldTimerManager().SetTimer(ElimTimer, this, &AShooterCharacter::ElimTimerFinished, ElimDelay);

}

void AShooterCharacter::Elim(bool bPlayerLeftGame)
{
	DropOrDestroyWeapons();
	MulticaseElim(bPlayerLeftGame);
}
void AShooterCharacter::SetTeamColor(ETeam Team)
{
	if (GetMesh() == nullptr && GreenMaterial == nullptr) return;
	switch (Team)
	{
	case ETeam::ET_GreenTeam:
		GetMesh()->SetMaterial(0, GreenMaterial);
		DesolveMaterialInstance = TeamDesolveMaterialInstance;
		break;
	case ETeam::ET_GreyTeam:
		GetMesh()->SetMaterial(0, GreyMaterial);
		DesolveMaterialInstance = TeamDesolveMaterialInstance;
		break;
	case ETeam::ET_NoTeam:
		GetMesh()->SetMaterial(0, GreenMaterial);
		DesolveMaterialInstance = TeamDesolveMaterialInstance;
		break;
	case ETeam::ET_MAX:
		break;
	default:
		break;
	}
}
void AShooterCharacter::Destroyed()
{
	Super::Destroy();	 
	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}
	AShooterGameMode* ShooterGameMode = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = ShooterGameMode && ShooterGameMode->GetMatchState() != MatchState::InProgress;
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}
void AShooterCharacter::ElimTimerFinished()
{
	GameMode = GameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : GameMode;
	if (GameMode && !bLeftGame)
	{
		GameMode->RequestRespawn(this, Controller);
	}
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}
void AShooterCharacter::ServerLeaveGame_Implementation()
{
	GameMode = GameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : GameMode;
	ShooterPlayerState = ShooterPlayerState == nullptr ? GetPlayerState<AShooterPlayerState>() : ShooterPlayerState;
	if (GameMode && ShooterPlayerState)
	{
		GameMode->PlayerLeftGame(ShooterPlayerState);
	}
}
void AShooterCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
	if (Weapon == nullptr) return;
	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy();
	}
	else
	{
		Weapon->Dropped();
	}
}
void AShooterCharacter::DropOrDestroyWeapons()
{
	if (Combat)
	{
		if (Combat->EquippedWeapon)
		{
			DropOrDestroyWeapon(Combat->EquippedWeapon);
		}
		if (Combat->SecondaryWeapon)
		{
			DropOrDestroyWeapon(Combat->SecondaryWeapon);
		}
	}
}
void AShooterCharacter::SwapToPrimaryWeapon()
{
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		if (Combat->bHoldingTheFlag)
		{
			return;
		}
		if (Combat->ShouldSwapWeapons() && Combat->CombatState == ECombatState::ECS_Unoccupied)
		{
			if (HasAuthority())
			{
				Combat->SwapWeapon();
			}
			else
			{
				PlaySwapMontage();
				Combat->CombatState = ECombatState::ECS_SwapingWeapons;
				bFinishedSwaping = false;
				ServerEquipButtonPressed();
			}
		}
	}
}
void AShooterCharacter::SwapToSecondaryWeapon()
{
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		if (Combat->bHoldingTheFlag)
		{
			return;
		}
		if (Combat->ShouldSwapWeapons() && Combat->CombatState == ECombatState::ECS_Unoccupied)
		{
			if (HasAuthority())
			{
				Combat->SwapWeapon();
			}
			else
			{
				PlaySwapMontage();
				Combat->CombatState = ECombatState::ECS_SwapingWeapons;
				bFinishedSwaping = false;
				ServerEquipButtonPressed();
			}
		}
	}
}
void AShooterCharacter::AimButtonPressed()
{
	if (Combat && Combat->bHoldingTheFlag)
	{
		return;
	}
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		// Stop sprinting when aiming
		if (bIsSprinting)
		{
			bIsSprinting = false;
			GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		}
		Combat->SetAiming(true);
	}
}
void AShooterCharacter::AimButtonReleased()
{
	if (Combat && Combat->bHoldingTheFlag)
	{
		return;
	}
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}
void AShooterCharacter::FireButtonPressed()
{
	if (bIsOnLadder) return; // Can't shoot while climbing
	if (Combat && Combat->bHoldingTheFlag)
	{
		return;
	}
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}
void AShooterCharacter::FireButtonReleased()
{

	if (Combat && Combat->bHoldingTheFlag)
	{
		return;
	}
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}
void AShooterCharacter::ReloadButtonPressed()
{
	if (Combat && Combat->bHoldingTheFlag)
	{
		return;
	}
	if (bDisableGameplay)
	{
		return;
	}
	if (Combat)
	{
		Combat->Reload();
	}

}
ECombatState AShooterCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}
void AShooterCharacter::SetHoldingTheFlag(bool bHolding)
{
	if (Combat == nullptr) return;
	Combat->bHoldingTheFlag = bHolding;
}
bool AShooterCharacter::IsHoldingTheFlag() const
{
	if (Combat == nullptr) return false;
	return Combat->bHoldingTheFlag;
}
bool AShooterCharacter::IsLocallyReloading()
{
	if (Combat == nullptr) return false;
	return Combat->bLocallyReloading;
}
ETeam AShooterCharacter::GetTeam()
{
	ShooterPlayerState = ShooterPlayerState == nullptr ? GetPlayerState<AShooterPlayerState>() : ShooterPlayerState;
	if (ShooterPlayerState == nullptr) return ETeam::ET_NoTeam;
	return ShooterPlayerState->GetTeam();
}

void AShooterCharacter::ServerShowDamageIndicator(AActor* DamagedActor, AActor* DamageCauser)
{
	MulticastShowDamageIndicator(DamagedActor, DamageCauser);
}

void AShooterCharacter::MulticastShowDamageIndicator_Implementation(AActor* DamagedActor, AActor* DamageCauser)
{
		AShooterCharacter* DamagedCharacter = Cast<AShooterCharacter>(DamagedActor);
	if (DamagedCharacter && DamageCauser)
	{
		FRotator Direction = UKismetMathLibrary::FindLookAtRotation(DamagedActor->GetActorLocation(), DamageCauser->GetActorLocation());
		FRotator PlayerRot = DamagedCharacter->GetControlRotation();
		float AngleDamaged = Direction.Yaw - PlayerRot.Yaw;
		if (DamagedCharacter->IsLocallyControlled())
		{
			DamagedCharacter->Angle = AngleDamaged;
			DamagedCharacter->ShowHitIndicator();
		}
	}
}

void AShooterCharacter::SetIsOnLadder(bool bOnLadder)
{
	bIsOnLadder = bOnLadder;

	if (bIsOnLadder)
	{
		// Enable flying movement for climbing
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		GetCharacterMovement()->MaxFlySpeed = LadderClimbSpeed;

		// Configure movement to stop immediately when input stops (no sliding/drift)
		GetCharacterMovement()->BrakingDecelerationFlying = 4000.f;  // High deceleration for instant stop
		GetCharacterMovement()->MaxAcceleration = 2048.f;  // Responsive acceleration
		GetCharacterMovement()->GravityScale = 0.f;  // No gravity while on ladder
	}
	else
	{
		// Return to walking movement
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);

		// Restore default movement settings
		GetCharacterMovement()->BrakingDecelerationFlying = 0.f;  // Reset to default
		GetCharacterMovement()->MaxAcceleration = 2048.f;  // Default acceleration

		// Restore gravity from the BP default (so BP-tuned GravityScale survives ladder use).
		float RestoreGravity = 1.f;
		if (ACharacter* DefaultChar = GetClass()->GetDefaultObject<ACharacter>())
		{
			if (UCharacterMovementComponent* DefaultMove = DefaultChar->GetCharacterMovement())
			{
				RestoreGravity = DefaultMove->GravityScale;
			}
		}
		GetCharacterMovement()->GravityScale = RestoreGravity;
	}
}

// ================== SPRINT SYSTEM ==================

void AShooterCharacter::SprintButtonPressed()
{
	// Keyboard (Left Shift) — hold to sprint.
	bSprintButtonHeld = true;
}

void AShooterCharacter::SprintButtonReleased()
{
	bSprintButtonHeld = false;
}

void AShooterCharacter::SprintToggleButtonPressed()
{
	// Controller (L3) — toggle: press to start running, press again to stop.
	bSprintButtonHeld = !bSprintButtonHeld;
}

bool AShooterCharacter::CanSprint() const
{
	// Can't sprint if disabled
	if (bDisableGameplay) return false;

	// Can't sprint while on ladder
	if (bIsOnLadder) return false;

	// Can't sprint while crouching
	if (bIsCrouched) return false;

	// Can't sprint while aiming
	if (Combat && Combat->bAiming) return false;

	// Can't sprint while reloading
	if (Combat && Combat->CombatState == ECombatState::ECS_Reloading) return false;

	// Can't sprint while throwing grenade
	if (Combat && Combat->CombatState == ECombatState::ECS_ThrowingGernade) return false;

	// Can't sprint while holding flag
	if (Combat && Combat->bHoldingTheFlag) return false;

	// Can't sprint while eliminated
	if (bElimed) return false;

	// Must be moving forward to sprint
	float Speed = CalculateSpeed();
	if (Speed < 10.f) return false;

	return true;
}

void AShooterCharacter::UpdateSprint()
{
	// Only run on locally controlled characters
	if (!IsLocallyControlled()) return;

	// Determine if we should be sprinting
	bool bShouldSprint = bSprintButtonHeld && CanSprint();

	// Update sprint state
	if (bShouldSprint != bIsSprinting)
	{
		bIsSprinting = bShouldSprint;

		// Update speed locally for immediate response
		GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;

		// Tell the server about the change
		if (!HasAuthority())
		{
			ServerSetSprinting(bIsSprinting);
		}
	}
}

void AShooterCharacter::ServerSetSprinting_Implementation(bool bSprinting)
{
	bIsSprinting = bSprinting;

	// RepNotify doesn't run on the server, so we need to update speed manually here
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
	}
}

void AShooterCharacter::OnRep_IsSprinting()
{
	// Update movement speed when sprint state replicates to clients
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
	}
}

// ==================== AUDIO FUNCTIONS ====================

void AShooterCharacter::PlayCharacterSound(USoundBase* Sound, float VolumeMultiplier)
{
	if (!Sound) return;

	// For locally controlled characters, play 2D sound (no attenuation)
	if (IsLocallyControlled())
	{
		UGameplayStatics::PlaySound2D(this, Sound, VolumeMultiplier);
	}
	// For other characters (server or other clients), play 3D spatial sound
	else
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			Sound,
			GetActorLocation(),
			VolumeMultiplier
		);
	}
}
