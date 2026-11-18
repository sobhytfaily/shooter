// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Shooter/ShooterHud.h"
#include "Shooter/Weapon/WeaponTypes.h"
#include "Shooter/CombatState.h"
#include "CombatComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHOOTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UCombatComponent();
	friend class AShooterCharacter;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void EquipWeapon(class AWeapon* Weapon);
	void SwapWeapon();
	void ReloadEmptyWeapon();
	void PlayEquipedWeaponSound(AWeapon* WeaponToEquip);
	void UpdateCarriedAmmo();
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void AttachActorToBackPack(AActor* ActorToAttach);
	void AttachFlagToLeftHand(AWeapon* Flag);

	void DropEquippedWeapon();
	void Reload();
	void FireButtonPressed(bool bPressed);
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();
	void JumpToShotgunEnd();
	void PickUpAmmo(EWeaponType weaponType, int32 AmmoAmount);

	bool bLocallyReloading = false;

	UPROPERTY()
		class AShooterHud* HUD;

	FHUDPackage HUDPackage;

	// Grenade aiming
	void GrenadeButtonPressed();
	void GrenadeButtonReleased();
	void SmokeGrenadeButtonPressed();
	void SmokeGrenadeButtonReleased();
protected:

	virtual void BeginPlay() override;
	void SetAiming(bool bisAiming);
	UFUNCTION(Server, Reliable)
	void SeverSetAiming(bool bisAiming);

	UFUNCTION(Server, Reliable)
	void ServerSwapWeapon();

	UFUNCTION(Server, Reliable)
	void ServerFinishSwap();

	UFUNCTION()void OnRep_EuippedWeapon();
	UFUNCTION()void OnRep_SecondaryWeapon();

	void Fire();
	void FireProjectileWeapon();
	void FireHitScanWeapon();
	void FireShotgun();

	void LocalFire(const FVector_NetQuantize& TraceHitTarget);
	void LocalShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	UFUNCTION(Server, Reliable)
		void ServerFire(const FVector_NetQuantize& TraceHitTarget, float FireDelay );

	UFUNCTION(NetMulticast, Reliable)
		void MulticastFire(const FVector_NetQuantize& TraceHitTarget);
	UFUNCTION(Server, Reliable)
		void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay);

	UFUNCTION(NetMulticast, Reliable)
		void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);


		void TraceUnderCrosshairs(FHitResult& TraceHitResult);

		void SetHudCrosshairs(float DeltaTime);

		UFUNCTION(Server, Reliable)
			void ServerReload();
		void HandleReload();
		int32 AmountToReload();

		void ThrowGernade();
		UFUNCTION(Server, Reliable)
		void ServerThrowGernade();
		UPROPERTY(EditAnywhere)

		TSubclassOf<class AProjectile> GernadeClass;

		// Smoke grenade support
		void ThrowSmokeGernade();
		UFUNCTION(Server, Reliable)
		void ServerThrowSmokeGernade();
		UPROPERTY(EditAnywhere)
		TSubclassOf<class AProjectile> SmokeGernadeClass;

			// Track which grenade type is being thrown
				bool bThrowingSmokeGernade = false;

				// Hold-to-aim grenades: the throw montage pauses at the LunchGernade notify while the
				// button is held; releasing launches immediately. Set on the owning client and the server.
				bool bGrenadeHeld = false;
				bool bGrenadeLaunched = false;
				// Seconds into ThrowGrenadeMontage where the wind-up freezes while the button is held (tune in BP)
				UPROPERTY(EditAnywhere, Category = "Grenade")
				float GrenadeHoldPoseTime = 0.35f;
				// Montage play rate for the throw after the button is released (tune in BP)
				UPROPERTY(EditAnywhere, Category = "Grenade")
				float GrenadeReleasePlayRate = 1.75f;
				FTimerHandle GrenadeHoldTimer;
				void StartGrenadeHoldTimer();
				void GrenadeHoldTimerFinished();
				void LaunchGrenadeNow();
				void ReleaseGrenade();
				UFUNCTION(Server, Reliable)
				void ServerReleaseGrenade(const FVector_NetQuantize& Target);
				void SpawnGrenade(const FVector_NetQuantize& Target, bool bSmoke);
				void FinishGrenadeThrow();

				void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
				void EquipSecondayWeapon(AWeapon* WeaponToEquip);

				// Grenade trajectory prediction
				void UpdateGrenadeTrajectory();
				void ShowGrenadeTrajectory(bool bShow);

		private:
			UPROPERTY()
			class AShooterCharacter* Character;
			UPROPERTY()
			class AShooterPlayerController* Controller;

			// Grenade aiming state
			bool bIsAimingGrenade = false;
			bool bIsAimingSmokeGrenade = false;

			// Trajectory visualization
			UPROPERTY(EditAnywhere, Category = "Grenade|Trajectory")
			class USplineComponent* TrajectorySpline;

			UPROPERTY(EditAnywhere, Category = "Grenade|Trajectory")
			TArray<class USplineMeshComponent*> TrajectoryMeshes;

			UPROPERTY(EditAnywhere, Category = "Grenade|Trajectory")
			class UStaticMesh* TrajectoryMesh;

			UPROPERTY(EditAnywhere, Category = "Grenade|Trajectory")
			class UMaterialInterface* TrajectoryMaterial;

			UPROPERTY(EditAnywhere, Category = "Grenade|Trajectory")
			class UDecalComponent* LandingDecal;

			UPROPERTY(EditAnywhere, Category = "Grenade|Trajectory")
			class UMaterialInterface* LandingDecalMaterial;

			// Speed of regular grenade - matches BP_Grenade's ProjectileMovementComponent->InitialSpeed
			UPROPERTY(EditAnywhere, Category = "Grenade|Trajectory")
			float GrenadeThrowSpeed = 2500.f;

			// Gravity scale for regular grenade (1.0 = full gravity - realistic)
			UPROPERTY(EditAnywhere, Category = "Grenade|Trajectory")
			float GrenadeGravityScale = 1.0f;

			// Speed of smoke grenade - matches BP_SmokeGrenade's ProjectileMovementComponent->InitialSpeed
			UPROPERTY(EditAnywhere, Category = "Grenade|Trajectory")
			float SmokeGrenadeThrowSpeed = 2200.f;

			// Gravity scale for smoke grenade (1.0 = full gravity - realistic)
			UPROPERTY(EditAnywhere, Category = "Grenade|Trajectory")
			float SmokeGrenadeGravityScale = 1.0f;

			UPROPERTY(EditAnywhere, Category = "Grenade|Trajectory")
			int32 TrajectorySegments = 30;



	UPROPERTY(ReplicatedUsing = OnRep_EuippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
		AWeapon* SecondaryWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bAiming = false;


	bool bAimButtonPressed = false;

	UFUNCTION()
	void OnRep_Aiming();
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	// HUD and Crosshairs




	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;

	FVector HitTarget;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;



	//Aiming And FOV
	float DefualtFov;  //no aiming
	UPROPERTY(EditAnywhere, Category ="Combat")
		float ZoomedFov = 30.f;
	UPROPERTY(EditAnywhere, Category = "Combat")
		float ZoomInterpSpeed = 20.f;

	void InterpFov(float DeltaTime);

	float CurrentFov;

	//Automatic Firing
	FTimerHandle FireTimer;
	void FireTimerFinished();
	void StartFireTimer();


	bool bCanFire = true;
	bool CanFire();

	//Ammo for currently Equipped Weapon 
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
		int32	CarriedAmmo;

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 300.f;
	TMap<EWeaponType, int32> CarriedAmmoMap;
	UPROPERTY(EditAnywhere)
	int32 StartingAmmo = 30;
	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 4;
	UPROPERTY(EditAnywhere)
		int32 StartingPistolAmmo = 15;
	UPROPERTY(EditAnywhere)
		int32 StartingSMGAmmo = 40;
	UPROPERTY(EditAnywhere)
		int32 StartingShotGunAmmo = 10;
	UPROPERTY(EditAnywhere)
		int32 StartingSniperRifleAmmo = 5;
	UPROPERTY(EditAnywhere)
		int32 StartingGranadeLuncherAmmo = 5;
	void IntializeCarriedAmmo();
	UFUNCTION()
	void OnRep_CarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState  = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();
	void UpdateShotgunAmmoValues();
	void ShowAttachedGernade(bool bShowGerande);

	// Regular grenades
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Gernades)
	int32 Gerandes = 4;
	UPROPERTY(EditAnywhere)
	int32 MaxGerandes = 5;

	// Smoke grenades (separate count)
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_SmokeGernades)
	int32 SmokeGerandes = 2;
	UPROPERTY(EditAnywhere)
	int32 MaxSmokeGerandes = 3;

	UFUNCTION()
	void OnRep_SmokeGernades();
	void UpdateHudSmokeGerandes();
	UFUNCTION()
	void OnRep_Gernades();
	void UpdateHudGerandes();
public:	
	UFUNCTION(BlueprintCallable )
	void FinishReloading();

	UFUNCTION(BlueprintCallable)
		void FinishSwap();

	UFUNCTION(BlueprintCallable)
		void FinishSwapWeapons();

	UFUNCTION(BlueprintCallable)
		void ThrowGernadeFinish();
	UFUNCTION(BlueprintCallable)
		void LunchGernade();

	UFUNCTION(Server, Reliable)
	void ServerLunchGernade(const FVector_NetQuantize& Target);

	UFUNCTION(Server, Reliable)
	void ServerLunchSmokeGernade(const FVector_NetQuantize& Target);

	UPROPERTY(ReplicatedUsing = OnRep_HoldingTheFlag)
	bool bHoldingTheFlag = false;

	UFUNCTION()
	void OnRep_HoldingTheFlag();
public:
	FORCEINLINE int32 GetGernades() const { return Gerandes; }
	FORCEINLINE int32 GetSmokeGernades() const { return SmokeGerandes; }
	bool ShouldSwapWeapons();

};
