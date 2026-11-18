// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Shooter/Weapon.h"
#include "Shooter/ShooterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Components/LineBatchComponent.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "Shooter/ShooterHUD.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Shooter/ShooterAnimInstance.h"
#include "Sound/SoundCue.h"
#include "Shooter/Weapon/Projectile.h"
#include "Shooter/Weapon/Shotgun.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStaticsTypes.h"


UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
		
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Gerandes);
	DOREPLIFETIME(UCombatComponent, SmokeGerandes);
	DOREPLIFETIME(UCombatComponent, bHoldingTheFlag);

}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Controller)
	{
		Character = Character == nullptr ? Cast<AShooterCharacter>(Controller->GetCharacter()) : Character;
	}

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (Character->GetFollowCamera())
		{
			DefualtFov  = 120.f;
			CurrentFov = DefualtFov;
		}

		// Initialize ammo if on server
		if (Character->HasAuthority())
		{
			IntializeCarriedAmmo();
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
		SetHudCrosshairs(DeltaTime);
		InterpFov(DeltaTime);

		// Update grenade trajectory while aiming
		UpdateGrenadeTrajectory();

		// Only the locally-controlled player should trigger auto-reload
		ReloadEmptyWeapon();
	}
}

void UCombatComponent::SetHudCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;

	Controller = Controller == nullptr ? Cast<AShooterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<AShooterHud>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
			}
			//Calculate Crosshair Spread
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);

			HUDPackage.CrosshairSpread =
				0.5f +
				CrosshairVelocityFactor +
				CrosshairInAirFactor -
				CrosshairAimFactor +
				CrosshairShootingFactor;

			HUD->setHUDPackage(HUDPackage);
		}
	}
}
void UCombatComponent::InterpFov(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;
	if (bAiming)
	{
		if (!Character->inFirst)
		{
			CurrentFov = FMath::FInterpTo(CurrentFov, EquippedWeapon->GetZoomedFov(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
		}
		if (Character->inFirst && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
		{
			CurrentFov = FMath::FInterpTo(CurrentFov, EquippedWeapon->GetZoomedFov(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
		}
	}
	else
	{

		CurrentFov = FMath::FInterpTo(CurrentFov, DefualtFov, DeltaTime, ZoomInterpSpeed);

		
	}
	
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFov);
	}
}
void UCombatComponent::FireTimerFinished()
{
	// Always restore bCanFire first — a null weapon or any early-return below
	// must never leave the player permanently unable to fire.
	bCanFire = true;

	if (EquippedWeapon == nullptr || Character == nullptr) return;

	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
	ReloadEmptyWeapon();
}
void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr ) return;
	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
	);
}
void UCombatComponent::OnRep_EuippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

		AttachActorToRightHand(EquippedWeapon);

		if (EquippedWeapon->GetWeaponMesh())
		{
			EquippedWeapon->GetWeaponMesh()->SetVisibility(true, true);
			EquippedWeapon->GetWeaponMesh()->SetOwnerNoSee(false);
			EquippedWeapon->GetWeaponMesh()->MarkRenderStateDirty();
		}

		// Ensure pickup widget stays hidden after mesh visibility changes
		EquippedWeapon->ShowPickUpWidget(false);

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		PlayEquipedWeaponSound(EquippedWeapon);
		EquippedWeapon->SetHudAmmo();
	}
}
void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && Character)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);

		AttachActorToBackPack(SecondaryWeapon);

		if (SecondaryWeapon->GetWeaponMesh())
		{
			SecondaryWeapon->GetWeaponMesh()->SetVisibility(true, true);
			SecondaryWeapon->GetWeaponMesh()->MarkRenderStateDirty();
		}

		// Ensure pickup widget stays hidden
		SecondaryWeapon->ShowPickUpWidget(false);

		PlayEquipedWeaponSound(SecondaryWeapon);
	}
}
void UCombatComponent::Fire()
{
	
	if(CanFire())
	{
		bCanFire = false;
		if (EquippedWeapon)
		{
			CrosshairShootingFactor = .75f;
			switch (EquippedWeapon->FireType)
			{
			case EFireType::EFT_Projectile:
				FireProjectileWeapon();
				break;
			case EFireType::EFT_HitScan:
				FireHitScanWeapon();
				break;
			case EFireType::EFT_Shotgun:
				FireShotgun();
				break;
			default:
				break;
			}
		}
		StartFireTimer();
	}
}
void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon)
	{
		// Check if scatter should be applied based on aiming state
		bool bShouldUseScatter = bAiming ? EquippedWeapon->bAimingScatter : EquippedWeapon->bHipFireScatter;
		HitTarget = bShouldUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget, bAiming) : HitTarget;
		if(Character && !Character->HasAuthority())	LocalFire(HitTarget);
		ServerFire(HitTarget, EquippedWeapon->FireDelay);
	}

}
void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon)
	{
		// Check if scatter should be applied based on aiming state
		bool bShouldUseScatter = bAiming ? EquippedWeapon->bAimingScatter : EquippedWeapon->bHipFireScatter;

		HitTarget = bShouldUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget, bAiming) : HitTarget;
		if (Character && !Character->HasAuthority())	LocalFire(HitTarget);
		ServerFire(HitTarget, EquippedWeapon->FireDelay);
	}
}
void UCombatComponent::FireShotgun()
{
	Ashotgun* Shotgun = Cast<Ashotgun>(EquippedWeapon);
	if (Shotgun)
	{
		TArray<FVector_NetQuantize> HitTargets;
		Shotgun->ShotgunTraceEndWithScatter(HitTarget, HitTargets, bAiming);
		if (Character && !Character->HasAuthority())	LocalShotgunFire(HitTargets);
		ServerShotgunFire(HitTargets, EquippedWeapon->FireDelay);
	}
}
void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if(bFireButtonPressed)
	{
		
		Fire();
	}
}
void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}
void UCombatComponent::JumpToShotgunEnd()
{
	if (Character == nullptr)  return;
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		if (Character->GetReloadMontage())
		{
			AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
		}
	}
}
void UCombatComponent::PickUpAmmo(EWeaponType weaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(weaponType))
	{
		CarriedAmmoMap[weaponType] = FMath::Clamp(CarriedAmmoMap[weaponType] + AmmoAmount, 0, MaxCarriedAmmo);
		UpdateCarriedAmmo();
	}
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == weaponType)
	{
		Reload();
	}
}
void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize(0.f, 0.f);
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;

		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGHT;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairInterface>())
		{
			HUDPackage.CrosshairColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairColor = FLinearColor::White;
		}
		if (!TraceHitResult.bBlockingHit) TraceHitResult.ImpactPoint = End;
	}
	

}
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	MulticastFire(TraceHitTarget);
}
void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	LocalFire(TraceHitTarget);
}
void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	MulticastShotgunFire(TraceHitTargets);
}
void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	LocalShotgunFire(TraceHitTargets);
}
void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{	
	if (EquippedWeapon == nullptr) return;
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_AssualtRifle && HUDPackage.CrosshairColor == FColor::Red)
		{
			Character->ShowHitMarker();
		}
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	Ashotgun* Shotgun = Cast<Ashotgun>(EquippedWeapon);
	if (Shotgun == nullptr || Character == nullptr) return;
	if (CombatState == ECombatState::ECS_Reloading || CombatState == ECombatState::ECS_Unoccupied)
	{
		bLocallyReloading = false;
		Character->PlayFireMontage(bAiming);
		Shotgun->FireShotgun(TraceHitTargets);
		CombatState = ECombatState::ECS_Unoccupied;
	}
}

void UCombatComponent::SeverSetAiming_Implementation(bool bisAiming)
{
	bAiming = bisAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bisAiming ? AimWalkSpeed : BaseWalkSpeed;
	}

}
void UCombatComponent::SetAiming(bool bisAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	bAiming = bisAiming;
	SeverSetAiming(bisAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bisAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSnipeScopeWidget(bisAiming);
	}
	if (Character->IsLocallyControlled())
	{
		bAimButtonPressed = bisAiming;
	}
}
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	
	if (WeaponToEquip->GetWeaponType() == EWeaponType::EWT_Flag)
	{
		Character->Crouch();
		bHoldingTheFlag = true;
		WeaponToEquip->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachFlagToLeftHand(WeaponToEquip);
		WeaponToEquip->SetOwner(Character);
		Character->GetCharacterMovement()->bOrientRotationToMovement = true;
		Character->bUseControllerRotationYaw = false;

	}
	else
	{
		if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr)
		{
			EquipSecondayWeapon(WeaponToEquip);
		}
		else
		{
			EquipPrimaryWeapon(WeaponToEquip);
			UpdateCarriedAmmo();
			UpdateAmmoValues();
		}

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}

}
void UCombatComponent::SwapWeapon()
{
	if (CombatState != ECombatState::ECS_Unoccupied || Character == nullptr) return;

	Character->PlaySwapMontage();
	Character->bFinishedSwaping = false;
	CombatState = ECombatState::ECS_SwapingWeapons;

	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;
}
void UCombatComponent::ServerSwapWeapon_Implementation()
{
	if (ShouldSwapWeapons())
	{
		SwapWeapon();
	}
}
void UCombatComponent::ServerFinishSwap_Implementation()
{
	if (CombatState == ECombatState::ECS_SwapingWeapons)
	{
		FinishSwapWeapons();
		CombatState = ECombatState::ECS_Unoccupied;
	}
}
void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr || Character == nullptr) return;
	// Clear any stuck fire-timer or reload state from the previously equipped weapon.
	// If a reload montage was interrupted (elim, swap, respawn), FinishReloading never
	// fires and both bLocallyReloading and CombatState stay locked. Reset them here.
	bCanFire = true;
	bLocallyReloading = false;
	if (Character->HasAuthority() && CombatState == ECombatState::ECS_Reloading)
	{
		CombatState = ECombatState::ECS_Unoccupied;
	}
	DropEquippedWeapon();
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	AttachActorToRightHand(EquippedWeapon);

	// Ensure weapon is visible when equipped - AFTER attachment
	if (EquippedWeapon->GetWeaponMesh())
	{
		EquippedWeapon->GetWeaponMesh()->SetVisibility(true, true);
		EquippedWeapon->GetWeaponMesh()->SetOwnerNoSee(false);
		EquippedWeapon->GetWeaponMesh()->MarkRenderStateDirty();
	}

	// Hide pickup widget AFTER visibility propagation
	EquippedWeapon->ShowPickUpWidget(false);

	EquippedWeapon->SetHudAmmo();
	UpdateCarriedAmmo();
	PlayEquipedWeaponSound(EquippedWeapon);
	// Note: do NOT call ReloadEmptyWeapon() here.
	// TickComponent already calls it every frame. Calling it during equip
	// races with the tick and causes a double ServerReload → infinite reload loop.
	EquippedWeapon->EnableCustomDepth(false);
}
void UCombatComponent::EquipSecondayWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr || Character == nullptr) return;
	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetOwner(Character);
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);

	AttachActorToBackPack(WeaponToEquip);

	// Ensure weapon is visible when equipped as secondary - AFTER attachment
	if (SecondaryWeapon->GetWeaponMesh())
	{
		SecondaryWeapon->GetWeaponMesh()->SetVisibility(true, true);
		SecondaryWeapon->GetWeaponMesh()->MarkRenderStateDirty();
	}

	// Hide pickup widget AFTER visibility propagation
	SecondaryWeapon->ShowPickUpWidget(false);

	PlayEquipedWeaponSound(SecondaryWeapon);
}
void UCombatComponent::OnRep_Aiming()
{
	if (Character && Character->IsLocallyControlled())
	{
		bAiming = bAimButtonPressed;
	}
}
void UCombatComponent::ReloadEmptyWeapon()
{

	if (EquippedWeapon && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}
void UCombatComponent::PlayEquipedWeaponSound(AWeapon* WeaponToEquip)
{
	
	if (Character && WeaponToEquip && WeaponToEquip->EquipeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponToEquip->EquipeSound, Character->GetActorLocation());
	}
}
void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller  == nullptr ? Cast<AShooterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHudCarriedAmmo(CarriedAmmo);
	}
}
void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || ActorToAttach == nullptr || Character->GetMesh() == nullptr) return;
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}
void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || ActorToAttach == nullptr || Character->GetMesh() == nullptr || EquippedWeapon == nullptr) return;
	bool bUsePistolSocket = EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol || EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SMG;
	FName SocketName = bUsePistolSocket ? FName("PistolSocket") : FName("LeftHandSocket");
	const USkeletalMeshSocket* HandSocket =  Character->GetMesh()->GetSocketByName(SocketName);
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}
void UCombatComponent::AttachActorToBackPack(AActor* ActorToAttach)
{
	if (Character == nullptr || ActorToAttach == nullptr || Character->GetMesh() == nullptr) return;
	const USkeletalMeshSocket* BackPackSocket = Character->GetMesh()->GetSocketByName(FName("BackSocket"));
	if (BackPackSocket)
	{
		BackPackSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}

}
void UCombatComponent::AttachFlagToLeftHand(AWeapon* Flag)
{
	if (Character == nullptr || Flag == nullptr || Character->GetMesh() == nullptr) return;
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("FlagSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(Flag, Character->GetMesh());
	}
}
void UCombatComponent::DropEquippedWeapon()	
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}
void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull() && !bLocallyReloading)
	{
		ServerReload();
		HandleReload();
		bLocallyReloading = true;
	}
}
void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	// Reject if already reloading — prevents double-reload from race conditions
	if (CombatState == ECombatState::ECS_Reloading) return;

	CombatState = ECombatState::ECS_Reloading;
	if (!Character->IsLocallyControlled())
	{
		HandleReload();
	}
}
void UCombatComponent::HandleReload()
{
	if (Character)
	{
		Character->PlayReloadMontage();
	}
}
int32 UCombatComponent::AmountToReload() 
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmmountCarried);
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}
void UCombatComponent::ThrowGernade()
{
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr || Gerandes == 0) return;

	bThrowingSmokeGernade = false;  // Regular grenade
	bGrenadeHeld = true;
	bGrenadeLaunched = false;
	CombatState = ECombatState::ECS_ThrowingGernade;
	if (Character)
	{
		Character->PlayGernadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGernade(true);
	}
	StartGrenadeHoldTimer();
	if (Character && !Character->HasAuthority())
	{
		ServerThrowGernade();
	}
	if (Character && Character->HasAuthority())
	{
		Gerandes = FMath::Clamp(Gerandes - 1, 0, MaxGerandes);
		UpdateHudGerandes();
	}

}

void UCombatComponent::ServerThrowGernade_Implementation()
{
	if (Gerandes == 0) return;
	bThrowingSmokeGernade = false;  // Regular grenade
	bGrenadeHeld = true;
	bGrenadeLaunched = false;
	CombatState = ECombatState::ECS_ThrowingGernade;
	if (Character)
	{
		Character->PlayGernadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGernade(true);
	}
	Gerandes = FMath::Clamp(Gerandes - 1, 0, MaxGerandes);
	UpdateHudGerandes();
}
void UCombatComponent::UpdateHudGerandes()
{
	Controller = Controller ? Cast<AShooterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHudGernades(Gerandes);
	}
}

void UCombatComponent::UpdateHudSmokeGerandes()
{
	Controller = Controller ? Cast<AShooterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHudSmokeGernades(SmokeGerandes);
	}
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Unoccupied:
		// Remote view of a grenade thrown on release: cut the throw montage short like the owner did.
		if (Character && !Character->IsLocallyControlled() && Character->StopGrenadeMontage())
		{
			ShowAttachedGernade(false);
			AttachActorToRightHand(EquippedWeapon);
		}
		if (bFireButtonPressed)
		{
			Fire();
		}
		break; 
	case ECombatState::ECS_Reloading:
		if (Character && !Character->IsLocallyControlled())
		{
			HandleReload();
		}
		break;
		case ECombatState::ECS_ThrowingGernade:
			if (Character && !Character->IsLocallyControlled())
			{
				Character->PlayGernadeMontage();
				AttachActorToLeftHand(EquippedWeapon);	
				ShowAttachedGernade(true);
			}
		break;
		case ECombatState::ECS_SwapingWeapons:
			if (Character && !Character->IsLocallyControlled())
			{
				Character->PlaySwapMontage();
			}
			break;
	default:
		break;
	}
}
void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<AShooterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHudCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(ReloadAmount);
}
void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{

		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller ? Cast<AShooterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHudCarriedAmmo(CarriedAmmo);
	}

	EquippedWeapon->AddAmmo(1);
	bCanFire = true;
	if (EquippedWeapon->IsFull() || CarriedAmmo == 0)
	{
		JumpToShotgunEnd();
	}
}
void UCombatComponent::ShowAttachedGernade(bool bShowGerande)
{
	if (Character && Character->GetAttachedGernade())
	{
		Character->GetAttachedGernade()->SetVisibility(bShowGerande);
	}
}
void UCombatComponent::OnRep_Gernades()
{
	UpdateHudGerandes();
}

void UCombatComponent::OnRep_SmokeGernades()
{
	UpdateHudSmokeGerandes();
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;
	bLocallyReloading = false;
	if(Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	if(bFireButtonPressed)
	{
		Fire();
	}
}
void UCombatComponent::FinishSwap()
{
	if (Character && Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
	}
	// For locally-controlled non-authority clients: reset the state we set locally
	// and tell the server to reset too (in case the server-side anim notify didn't fire).
	if (Character && !Character->HasAuthority() && Character->IsLocallyControlled())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		ServerFinishSwap();
	}
	if (Character)
	{
		Character->bFinishedSwaping = true;
	}
}
void UCombatComponent::FinishSwapWeapons()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);

		if (EquippedWeapon->GetWeaponMesh())
		{
			EquippedWeapon->GetWeaponMesh()->SetVisibility(true, true);
			EquippedWeapon->GetWeaponMesh()->SetOwnerNoSee(false);
			EquippedWeapon->GetWeaponMesh()->MarkRenderStateDirty();
		}

		// Ensure pickup widget stays hidden
		EquippedWeapon->ShowPickUpWidget(false);

		EquippedWeapon->SetHudAmmo();
		UpdateCarriedAmmo();
		PlayEquipedWeaponSound(EquippedWeapon);
	}

	if (SecondaryWeapon)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
		AttachActorToBackPack(SecondaryWeapon);

		if (SecondaryWeapon->GetWeaponMesh())
		{
			SecondaryWeapon->GetWeaponMesh()->SetVisibility(true, true);
			SecondaryWeapon->GetWeaponMesh()->MarkRenderStateDirty();
		}

		// Ensure pickup widget stays hidden
		SecondaryWeapon->ShowPickUpWidget(false);
	}
}
void UCombatComponent::ThrowGernadeFinish()
{
	// Montage notify; also called directly by FinishGrenadeThrow. Ignore late calls (e.g. during blend-out)
	// so we never clobber a state the player has already moved on to.
	if (CombatState != ECombatState::ECS_ThrowingGernade) return;
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHand(EquippedWeapon);
	bThrowingSmokeGernade = false;  // Reset flag
}
void UCombatComponent::LunchGernade()
{
	// Montage notify at the hand-release frame of the throw.
	if (bGrenadeHeld)
	{
		// Safety net (normally the hold timer froze the montage earlier): keep waiting for release.
		if (Character) Character->PauseGrenadeMontage();
		return;
	}
	if (bGrenadeLaunched) return;

	bIsAimingGrenade = false;
	bIsAimingSmokeGrenade = false;
	if (Character && Character->IsLocallyControlled())
	{
		LaunchGrenadeNow(); // button was released; the throw just reached the release frame
	}
	else
	{
		ShowAttachedGernade(false); // remote view: the grenade has left the hand
	}
}

void UCombatComponent::ServerLunchSmokeGernade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && SmokeGernadeClass && Character->GetAttachedGernade())
	{
		const FVector StartingLocation = Character->GetAttachedGernade()->GetComponentLocation();
		FVector ToTarget = Target - StartingLocation;
		FVector ThrowDirection = ToTarget.GetSafeNormal();

		// Spawn grenade with forward offset to clear character's body
		FVector SpawnLocation = StartingLocation + FVector(0.f, 0.f, 20.f) + (ThrowDirection * 50.f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		UWorld* World = GetWorld();
		if (World)
		{
			World->SpawnActor<AProjectile>(SmokeGernadeClass, SpawnLocation, ToTarget.Rotation(), SpawnParams);
		}
	}
}

void UCombatComponent::OnRep_HoldingTheFlag()
{
	if (bHoldingTheFlag && Character && Character->IsLocallyControlled())
	{
		Character->Crouch();
	}
}
bool UCombatComponent::ShouldSwapWeapons()
{
	return EquippedWeapon != nullptr && SecondaryWeapon != nullptr;
}
void UCombatComponent::ServerLunchGernade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && GernadeClass && Character->GetAttachedGernade())
	{
		const FVector StartingLocation = Character->GetAttachedGernade()->GetComponentLocation();
		FVector ToTarget = Target - StartingLocation;
		FVector ThrowDirection = ToTarget.GetSafeNormal();

		// Spawn grenade with forward offset to clear character's body
		FVector SpawnLocation = StartingLocation + FVector(0.f, 0.f, 20.f) + (ThrowDirection * 50.f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		UWorld* World = GetWorld();
		if (World)
		{
			World->SpawnActor<AProjectile>(GernadeClass, SpawnLocation, ToTarget.Rotation(), SpawnParams);
		}
	}
}

void UCombatComponent::ThrowSmokeGernade()
{
	if (SmokeGerandes == 0) return;
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr) return;

	bThrowingSmokeGernade = true;  // Set flag for smoke grenade
	bGrenadeHeld = true;
	bGrenadeLaunched = false;
	CombatState = ECombatState::ECS_ThrowingGernade;
	if (Character)
	{
		Character->PlayGernadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGernade(true);
	}
	StartGrenadeHoldTimer();
	if (Character && !Character->HasAuthority())
	{
		ServerThrowSmokeGernade();
	}
	if (Character && Character->HasAuthority())
	{
		SmokeGerandes = FMath::Clamp(SmokeGerandes - 1, 0, MaxSmokeGerandes);
		UpdateHudSmokeGerandes();
	}
}

void UCombatComponent::ServerThrowSmokeGernade_Implementation()
{
	if (SmokeGerandes == 0) return;
	bThrowingSmokeGernade = true;  // Set flag for smoke grenade
	bGrenadeHeld = true;
	bGrenadeLaunched = false;
	CombatState = ECombatState::ECS_ThrowingGernade;
	if (Character)
	{
		Character->PlayGernadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGernade(true);
	}
	SmokeGerandes = FMath::Clamp(SmokeGerandes - 1, 0, MaxSmokeGerandes);
	UpdateHudSmokeGerandes();
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	if (!EquippedWeapon->IsEmpty() &&
		bCanFire &&
		CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_ShotGun
		)
	{
		return true;
	}
	if (bLocallyReloading) return false;
	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}
void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<AShooterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHudCarriedAmmo(CarriedAmmo);
	}
	bool bJumpToShotgunEnd =
		CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon != nullptr &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_ShotGun &&
		CarriedAmmo == 0;
	if (bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}
void UCombatComponent::IntializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssualtRifle, StartingAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLuncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SMG, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_ShotGun, StartingShotGunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperRifleAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GranadeLuncher, StartingGranadeLuncherAmmo);
}

// ==================== GRENADE TRAJECTORY SYSTEM ====================

void UCombatComponent::GrenadeButtonPressed()
{
	if (!Character || !Character->IsLocallyControlled()) return;
	if (Gerandes <= 0) return;
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr) return;
	if (bHoldingTheFlag) return;

	bIsAimingGrenade = true;
	bIsAimingSmokeGrenade = false;
	ShowGrenadeTrajectory(true);
	// Start the throw animation immediately so windup plays while the player aims.
	// LunchGernade notify fires naturally during the animation.
	ThrowGernade();
}

void UCombatComponent::GrenadeButtonReleased()
{
	if (!bIsAimingGrenade) return;

	bIsAimingGrenade = false;
	ShowGrenadeTrajectory(false);
	ReleaseGrenade();
}

void UCombatComponent::SmokeGrenadeButtonPressed()
{
	if (!Character || !Character->IsLocallyControlled()) return;
	if (SmokeGerandes <= 0) return;
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr) return;
	if (bHoldingTheFlag) return;

	bIsAimingSmokeGrenade = true;
	bIsAimingGrenade = false;
	ShowGrenadeTrajectory(true);
	// Start the throw animation immediately so windup plays while the player aims.
	ThrowSmokeGernade();
}

void UCombatComponent::SmokeGrenadeButtonReleased()
{
	if (!bIsAimingSmokeGrenade) return;

	bIsAimingSmokeGrenade = false;
	ShowGrenadeTrajectory(false);
	ReleaseGrenade();
}

void UCombatComponent::StartGrenadeHoldTimer()
{
	// Owner only: freeze the wind-up at GrenadeHoldPoseTime while the button is held.
	if (Character == nullptr || !Character->IsLocallyControlled()) return;
	Character->GetWorldTimerManager().SetTimer(GrenadeHoldTimer, this, &UCombatComponent::GrenadeHoldTimerFinished, FMath::Max(GrenadeHoldPoseTime, 0.01f), false);
}

void UCombatComponent::GrenadeHoldTimerFinished()
{
	if (bGrenadeHeld && Character)
	{
		Character->PauseGrenadeMontage();
	}
}

void UCombatComponent::ReleaseGrenade()
{
	// Owning client (or listen-server host): play the throw; the grenade launches at the LunchGernade notify.
	if (Character == nullptr || !bGrenadeHeld) return;
	bGrenadeHeld = false;
	Character->GetWorldTimerManager().ClearTimer(GrenadeHoldTimer);

	if (!Character->ResumeGrenadeMontage(GrenadeReleasePlayRate))
	{
		LaunchGrenadeNow(); // montage missing/ended: don't lose the throw
	}
}

void UCombatComponent::LaunchGrenadeNow()
{
	// Owner only. Spawns on the server and cuts the rest of the montage so the player can act right away.
	if (Character == nullptr || bGrenadeLaunched) return;
	bGrenadeLaunched = true;
	if (Character->HasAuthority())
	{
		SpawnGrenade(HitTarget, bThrowingSmokeGernade);
		FinishGrenadeThrow();
	}
	else
	{
		FinishGrenadeThrow(); // predict locally
		ServerReleaseGrenade(HitTarget);
	}
}

void UCombatComponent::ServerReleaseGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (bGrenadeLaunched || CombatState != ECombatState::ECS_ThrowingGernade) return;
	bGrenadeHeld = false;
	bGrenadeLaunched = true;
	SpawnGrenade(Target, bThrowingSmokeGernade); // server's own flag decides the grenade type
	FinishGrenadeThrow();
}

void UCombatComponent::SpawnGrenade(const FVector_NetQuantize& Target, bool bSmoke)
{
	// Server only. Reuses the existing spawn code.
	if (bSmoke)
	{
		ServerLunchSmokeGernade_Implementation(Target);
	}
	else
	{
		ServerLunchGernade_Implementation(Target);
	}
}

void UCombatComponent::FinishGrenadeThrow()
{
	ShowAttachedGernade(false);
	if (Character)
	{
		Character->StopGrenadeMontage();
	}
	ThrowGernadeFinish(); // back to Unoccupied, weapon to right hand
}

void UCombatComponent::ShowGrenadeTrajectory(bool bShow)
{
	// Lines are drawn with LifeTime=0 (single frame) in UpdateGrenadeTrajectory,
	// so they self-clear when we stop calling it. Nothing to flush.
}

void UCombatComponent::UpdateGrenadeTrajectory()
{
	if (!bIsAimingGrenade && !bIsAimingSmokeGrenade) return;
	if (!Character) return;

	// Get starting position (from hand socket, offset up a bit)
	FVector StartLocation;
	if (Character->GetAttachedGernade())
	{
		StartLocation = Character->GetAttachedGernade()->GetComponentLocation() + FVector(0.f, 0.f, 20.f);
	}
	else
	{
		StartLocation = Character->GetActorLocation() + FVector(0, 0, 70);
	}

	// Calculate throw direction - same as actual grenade launch
	FVector ToTarget = HitTarget - StartLocation;
	FVector LaunchDirection = ToTarget.GetSafeNormal();

	// Use the correct speed and gravity based on grenade type
	float ThrowSpeed = bIsAimingSmokeGrenade ? SmokeGrenadeThrowSpeed : GrenadeThrowSpeed;
	float GravityScale = bIsAimingSmokeGrenade ? SmokeGrenadeGravityScale : GrenadeGravityScale;
	FVector LaunchVelocity = LaunchDirection * ThrowSpeed;

	// Predict projectile path
	FPredictProjectilePathParams PredictParams;
	PredictParams.StartLocation = StartLocation;
	PredictParams.LaunchVelocity = LaunchVelocity;
	PredictParams.bTraceWithCollision = true;
	PredictParams.ProjectileRadius = 10.f;
	PredictParams.MaxSimTime = 4.f;
	PredictParams.bTraceWithChannel = true;
	PredictParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	PredictParams.ActorsToIgnore.Add(Character);
	PredictParams.SimFrequency = 30.f;
	PredictParams.DrawDebugType = EDrawDebugTrace::None;

	// Apply custom gravity scale
	PredictParams.OverrideGravityZ = GetWorld()->GetGravityZ() * GravityScale;

	FPredictProjectilePathResult PredictResult;
	bool bHit = UGameplayStatics::PredictProjectilePath(this, PredictParams, PredictResult);

	// Draw sleek dotted grey trajectory line.
	// NOTE: Use ULineBatchComponent directly instead of DrawDebugLine, because
	// DrawDebug* functions are stripped out in Shipping builds (ENABLE_DRAW_DEBUG=0).
	TArray<FPredictProjectilePathPointData>& PathPoints = PredictResult.PathData;

	UWorld* World = GetWorld();
	ULineBatchComponent* LineBatcher = World ? World->GetLineBatcher(UWorld::ELineBatcherType::World) : nullptr;
	if (LineBatcher && PathPoints.Num() > 1)
	{
		for (int32 i = 0; i < PathPoints.Num() - 1; i++)
		{
			// Skip every other segment to create dotted effect
			if (i % 2 == 0)
			{
				// Fade opacity along the path
				float Alpha = 1.0f - (float(i) / float(PathPoints.Num())) * 0.5f;
				uint8 Grey = FMath::Clamp(int32(150 * Alpha), 80, 150);
				FLinearColor FadedGrey = FLinearColor(FColor(Grey, Grey, Grey));

				LineBatcher->DrawLine(
					PathPoints[i].Location,
					PathPoints[i + 1].Location,
					FadedGrey,
					SDPG_World,
					1.5f,   // Thickness
					0.f     // LifeTime: 0 = single frame, redrawn next tick
				);
			}
		}
	}
}
