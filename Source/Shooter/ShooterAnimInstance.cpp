// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon.h"
#include "Shooter/CombatState.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
void UShooterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}
void UShooterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);
	if (ShooterCharacter == nullptr)
	{
		ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
	}
	if (ShooterCharacter == nullptr) return;

	// Set sprint animation play rate
	bIsSprinting = ShooterCharacter->IsSprinting();
	SprintAnimPlayRate = bIsSprinting ? ShooterCharacter->SprintAnimationPlayRate : 1.0f;

	// DON'T use GlobalAnimRateScale - it affects montages (fire, reload) and causes jitter!
	// Instead, the sprint play rate should be applied in the AnimBP to ONLY the locomotion blend space
	// Leave this commented out:
	// if (USkeletalMeshComponent* SkelMesh = ShooterCharacter->GetMesh())
	// {
	// 	SkelMesh->GlobalAnimRateScale = SprintAnimPlayRate;
	// }

		FVector Velocity = ShooterCharacter->GetVelocity();
		Velocity.Z = 0.f;
		speed = Velocity.Size();
		IsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();
		isAccerlating = ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
		bWeaponEquipped = ShooterCharacter->IsWeaponEquipped();
		EquippedWeapon = ShooterCharacter->GetEqiuppedWeapon();
		bIsCrouched = ShooterCharacter->bIsCrouched;
		bAiming = ShooterCharacter->IsAiming();
		TurningInPlace = ShooterCharacter->GetTurningInPlace();
		bRotateRootBone = ShooterCharacter->ShouldRotateRootBone();
		bElimed = ShooterCharacter->IsElimed();
		bHoldingTheFlag = ShooterCharacter->IsHoldingTheFlag();

		// Sprint animation
		bIsSprinting = ShooterCharacter->IsSprinting();
		SprintAnimPlayRate = bIsSprinting ? ShooterCharacter->SprintAnimationPlayRate : 1.0f;

		//Offset Yaw for Strafing
		FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();
		FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity());
		FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
		DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 15.f);
		YawOffset = DeltaRotation.Yaw;
		//Offset Lean
		CharacterRotationLastFrame = CharacterRotation;
		CharacterRotation = ShooterCharacter->GetActorRotation();
		const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
		const float target = Delta.Yaw / DeltaTime;
		const float Interp = FMath::FInterpTo(Lean, target, DeltaTime, 6.f);
		Lean = FMath::Clamp(Interp, -90.f, 90.f);
		AO_Yaw = ShooterCharacter->GetAO_Yaw();
		AO_Pitch = ShooterCharacter->GetAO_Pitch();


		if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && ShooterCharacter->GetMesh())
		{
			LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
			FVector OutPosition;
			FRotator OutRotation;
			ShooterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
			LeftHandTransform.SetLocation(OutPosition);
			LeftHandTransform.SetRotation(FQuat(OutRotation));
			if (ShooterCharacter->IsLocallyControlled())
			{
				bLocallyControlled = true;
				FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);
				FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - ShooterCharacter->GetHitTarget()));
				RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 100.f);
			}
		}

		// Safe checks for Combat component access
		ECombatState CombatState = ShooterCharacter->GetCombatState();
		bUseFabrick = CombatState == ECombatState::ECS_Unoccupied;
		bool bFabrickOverride =
			ShooterCharacter->IsLocallyControlled() &&
			CombatState != ECombatState::ECS_ThrowingGernade &&
			ShooterCharacter->bFinishedSwaping;
		if (bFabrickOverride)
		{
			bUseFabrick = !ShooterCharacter->IsLocallyReloading();
		}
			bUseAimOffsets = CombatState == ECombatState::ECS_Unoccupied && !ShooterCharacter->getDisabledGameplay();
			bTransformRightHand = CombatState == ECombatState::ECS_Unoccupied && !ShooterCharacter->getDisabledGameplay();
		}

		void UShooterAnimInstance::AnimNotify_PlaySound(USoundBase* Sound, float VolumeMultiplier, float PitchMultiplier)
		{
			if (!Sound || !ShooterCharacter) return;

			// For locally controlled characters (your own character), always play sound
			if (ShooterCharacter->IsLocallyControlled())
			{
				UGameplayStatics::PlaySound2D(this, Sound, VolumeMultiplier, PitchMultiplier);
			}
			// For non-locally controlled characters (other players), use 3D spatial audio
			else
			{
				// Only play if the character mesh is valid
				if (USkeletalMeshComponent* Mesh = ShooterCharacter->GetMesh())
				{
					UGameplayStatics::PlaySoundAtLocation(
						this,
						Sound,
						Mesh->GetComponentLocation(),
						VolumeMultiplier,
						PitchMultiplier
					);
				}
			}
		}

