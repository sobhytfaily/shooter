// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h" 
#include "Shooter/ShooterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimationAsset.h"
#include "Weapon/BulletShell.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"
#include "Shooter/ShooterComponent/CombatComponent.h"	
#include "Shooter/PlayerController/ShooterPlayerController.h"

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon Mesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EnableCustomDepth(true);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	
	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickUpWidget"));
	PickupWidget->SetupAttachment(RootComponent);

}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeapon, Ammo, COND_OwnerOnly);
}



// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if(AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}

	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}
}



void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
	if (ShooterCharacter)
	{
		if (WeaponType == EWeaponType::EWT_Flag && ShooterCharacter->GetTeam() == Team) return;		
		if (ShooterCharacter->IsHoldingTheFlag()) return;
		ShooterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
	if (ShooterCharacter)
	{
		if (WeaponType == EWeaponType::EWT_Flag && ShooterCharacter->GetTeam() == Team) return;
		if (ShooterCharacter->IsHoldingTheFlag()) return;
		ShooterCharacter->SetOverlappingWeapon(nullptr);
	}
}



void AWeapon::SpendRound()
{
	// Guard clause: prevent spending ammo if magazine is empty
	if (Ammo <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot spend round - Weapon %s has no ammo!"), *GetName());
		return;
	}

	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHudAmmo();

	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else
	{
		Sequence++;
	}

	// Log and notify if weapon is now empty after firing
	if (IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Weapon %s magazine is now empty - reload needed!"), *GetName());
		// Note: Reload logic should be handled by CombatComponent based on reserve ammo availability
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	Sequence--;
	Ammo -= Sequence;
	SetHudAmmo();
}
void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	// Check if weapon was empty before adding ammo
	bool bWasEmpty = IsEmpty();

	// Clamp the ammo value to valid range
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);

	SetHudAmmo();
	ClientAddAmmo(AmmoToAdd);

	// Handle empty-to-full transition (weapon went from empty to having ammo)
	if (bWasEmpty && Ammo > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Weapon %s received ammo - now has %d/%d rounds"), *GetName(), Ammo, MagCapacity);

		// Trigger reload logic on server if weapon had no ammo but now has some
		if (HasAuthority())
		{
			// Notify the combat component or character that this weapon can now fire
			// This prevents the reload loop by making the transition explicit
			AActor* OwnerActor = GetOwner();
			if (OwnerActor)
			{
				AShooterCharacter* Character = Cast<AShooterCharacter>(OwnerActor);
				if (Character && Character->getCombatComponent())
				{
					// Signal that weapon has ammo available for firing
					// Actual reload trigger should be handled by CombatComponent
					UE_LOG(LogTemp, Log, TEXT("Notifying CombatComponent that %s has ammo available"), *GetName());
				}
			}
		}
	}
	else if (IsFull())
	{
		UE_LOG(LogTemp, Log, TEXT("Weapon %s is now full with %d/%d rounds"), *GetName(), Ammo, MagCapacity);
	}
	else if (Ammo > 0 && Ammo < MagCapacity)
	{
		UE_LOG(LogTemp, Log, TEXT("Weapon %s has %d/%d rounds"), *GetName(), Ammo, MagCapacity);
	}
}
void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority()) return;

	// Check if weapon was empty before adding ammo
	bool bWasEmpty = IsEmpty();

	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	PlayerCharacter = PlayerCharacter == nullptr ? Cast<AShooterCharacter>(OwnerActor) : PlayerCharacter;
	if (PlayerCharacter && PlayerCharacter->getCombatComponent())
	{
		// If weapon was empty and now has ammo, notify combat component
		if (bWasEmpty && Ammo > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("Client: Weapon %s now has ammo after being empty"), *GetName());
			// Combat component can trigger reload based on this state change
		}

		// If magazine is now full, trigger end animation (e.g., for shotgun reload)
		if (IsFull())
		{
			PlayerCharacter->getCombatComponent()->JumpToShotgunEnd();
		}
	}

	SetHudAmmo();
}

void AWeapon::SetHudAmmo()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	PlayerCharacter = PlayerCharacter == nullptr ? Cast<AShooterCharacter>(OwnerActor) : PlayerCharacter;
	if (PlayerCharacter)
	{
		PlayerController = PlayerController == nullptr ? Cast<AShooterPlayerController>(PlayerCharacter->Controller) : PlayerController;
		if (PlayerController)
		{
			PlayerController->SetHudWeaponAmmo(Ammo);
		}
	}
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget, bool bIsAiming)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return FVector();

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();

	// Determine scatter parameters based on aiming state
	float CurrentDistanceToSphere;
	float CurrentSphereRadius;

	if (bIsAiming)
	{
		if (!bAimingScatter) return HitTarget;
		CurrentDistanceToSphere = AimingDistanceToSphere;
		CurrentSphereRadius = AimingSphereRadius;
	}
	else
	{
		if (!bHipFireScatter) return HitTarget;
		CurrentDistanceToSphere = HipFireDistanceToSphere;
		CurrentSphereRadius = HipFireSphereRadius;
	}

	const FVector SphereCenter = TraceStart + ToTargetNormalized * CurrentDistanceToSphere;
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, CurrentSphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	return FVector(TraceStart + ToEndLoc * TRACE_LENGHT / ToEndLoc.Size());
}

float AWeapon::CalculateDamageWithFalloff(float Distance, float BaseDamage) const
{
	if (!bUseDamageFalloff)
	{
		return BaseDamage;
	}

	// No falloff if within min distance
	if (Distance <= MinDamageDistance)
	{
		return BaseDamage;
	}

	// Apply min damage multiplier if beyond max distance
	if (Distance >= MaxDamageDistance)
	{
		return BaseDamage * MinDamageMultiplier;
	}

	// Linear interpolation between min and max distance
	float Alpha = (Distance - MinDamageDistance) / (MaxDamageDistance - MinDamageDistance);
	float DamageMultiplier = FMath::Lerp(1.0f, MinDamageMultiplier, Alpha);

	return BaseDamage * DamageMultiplier;
}



void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr)
	{
		PlayerCharacter = nullptr;
		PlayerController = nullptr;
	}
	else
	{
		SetHudAmmo();
	}
}


void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	// Validate state transitions
	if (HasAuthority())
	{
		// Log state change information
		const FString OldState = StaticEnum<EWeaponState>()->GetValueAsString(WeaponState);
		const FString NewState = StaticEnum<EWeaponState>()->GetValueAsString(State);
		UE_LOG(LogTemp, Log, TEXT("Weapon %s state changing from %s to %s (Ammo: %d/%d)"), 
			*GetName(), *OldState, *NewState, Ammo, MagCapacity);
	}

	WeaponState = State;
	OnWeaponStateSet();
}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		OnWeaponEquipped();
		break;
	case EWeaponState::EWS_Dropped:
		OnWeaponDropped();
		break;
	case EWeaponState::EWS_EquippedSecondary:
		OnEquippedSecondary();
		break;
	}

	// Update ammo display whenever weapon state changes
	SetHudAmmo();
}

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh;
}



void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::OnRep_Ammo()
{
	// Update HUD when ammo is replicated from server
	SetHudAmmo();

	// Handle ammo state changes
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	PlayerCharacter = PlayerCharacter == nullptr ? Cast<AShooterCharacter>(OwnerActor) : PlayerCharacter;
	if (PlayerCharacter)
	{
		PlayerController = PlayerController == nullptr ? Cast<AShooterPlayerController>(PlayerCharacter->Controller) : PlayerController;
		if (PlayerController)
		{
			// Update weapon empty/full status on HUD
			PlayerController->SetHudWeaponAmmo(Ammo);

			// Notify if weapon became empty
			if (IsEmpty())
			{
				// Could add additional logic here for empty state (e.g., disable firing, play sound)
				UE_LOG(LogTemp, Warning, TEXT("Weapon %s is now empty"), *GetName());
			}
			// Notify if weapon is full
			else if (IsFull())
			{
				UE_LOG(LogTemp, Log, TEXT("Weapon %s is now full"), *GetName());
			}
		}
	}
}


void AWeapon::OnEquippedSecondary()
{
	ShowPickUpWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Explicitly hide pickup widget
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}

	if (WeaponType == EWeaponType::EWT_SMG)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}
	EnableCustomDepth(false);

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	PlayerCharacter = PlayerCharacter == nullptr ? Cast<AShooterCharacter>(OwnerActor) : PlayerCharacter;
	if (PlayerCharacter)
	{
		// Update HUD with current ammo state for secondary weapon
		SetHudAmmo();

		if (bUseServerSideRewind)
		{
			PlayerController = PlayerController == nullptr ? Cast<AShooterPlayerController>(PlayerCharacter->Controller) : PlayerController;
			if (PlayerController && HasAuthority() && !PlayerController->HighPingDelegate.IsBound())
			{
				PlayerController->HighPingDelegate.AddDynamic(this, &AWeapon::OnPingTooHigh);
			}
		}
	}
}

void AWeapon::OnWeaponEquipped()
{
	ShowPickUpWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Ensure weapon mesh is visible when equipped
	WeaponMesh->SetVisibility(true);
	WeaponMesh->bOwnerNoSee = false;

	// Explicitly hide pickup widget after mesh visibility changes
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}

	if (WeaponType == EWeaponType::EWT_SMG)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}
	EnableCustomDepth(false);

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	PlayerCharacter = PlayerCharacter == nullptr ? Cast<AShooterCharacter>(OwnerActor) : PlayerCharacter;
	if (PlayerCharacter)
	{
		// Update HUD with current ammo state
		SetHudAmmo();

		// Check if the weapon has ammo before attempting any reload logic
		if (Ammo > 0)
		{
			// Weapon has ammo - can proceed with normal firing
			UE_LOG(LogTemp, Log, TEXT("Equipped weapon %s with ammo (%d/%d)"), *GetName(), Ammo, MagCapacity);
		}
		else
		{
			// Weapon has no ammo - inform player and potentially trigger reload
			UE_LOG(LogTemp, Warning, TEXT("Equipped weapon %s but it has no ammo - please reload"), *GetName());
			// Note: Actual reload logic should be handled by CombatComponent based on available reserve ammo
		}

		// Log additional state for full magazine
		if (IsFull())
		{
			UE_LOG(LogTemp, Log, TEXT("Equipped weapon %s with full magazine"), *GetName());
		}

		if (bUseServerSideRewind)
		{
			PlayerController = PlayerController == nullptr ? Cast<AShooterPlayerController>(PlayerCharacter->Controller) : PlayerController;
			if (PlayerController && HasAuthority() && !PlayerController->HighPingDelegate.IsBound())
			{
				PlayerController->HighPingDelegate.AddDynamic(this, &AWeapon::OnPingTooHigh);
			}
		}
	}
}

void AWeapon::OnWeaponDropped()
{
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	PlayerCharacter = PlayerCharacter == nullptr ? Cast<AShooterCharacter>(OwnerActor) : PlayerCharacter;
	if (PlayerCharacter && bUseServerSideRewind)
	{
		PlayerController = PlayerController == nullptr ? Cast<AShooterPlayerController>(PlayerCharacter->Controller) : PlayerController;
		if (PlayerController && HasAuthority() && PlayerController->HighPingDelegate.IsBound())
		{
			PlayerController->HighPingDelegate.RemoveDynamic(this, &AWeapon::OnPingTooHigh);
		}
	}
	GetWorldTimerManager().SetTimer(DestroyWeaponTimer,
		this,
		&AWeapon::DestroyWeapon,
		DestroyWeaponTime);
}

void AWeapon::DestroyWeapon()
{
	/*if (WeaponState == EWeaponState::EWS_Dropped)
	{
		Destroy();
	}
	else {

	}*/
}

void AWeapon::ShowPickUpWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
	if (BulletShellClass)
	{
		const USkeletalMeshSocket* AmmoEject = GetWeaponMesh()->GetSocketByName(FName("AmmoEject"));
		if (AmmoEject)
		{
			FTransform SocketTransform = AmmoEject->GetSocketTransform(WeaponMesh);
				UWorld* world = GetWorld();
				if (world)
				{
					world->SpawnActor<ABulletShell>(
						BulletShellClass,
						SocketTransform.GetLocation(),
						SocketTransform.GetRotation().Rotator()
						);
				}
			
		}
	}
	SpendRound();
	
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	PlayerCharacter = nullptr;
	PlayerController = nullptr;
}

