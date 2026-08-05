// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Shooter/ShooterCharacter.h"
#include "Shooter/Weapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"
void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket =  GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	UWorld* world = GetWorld();

	if (!InstigatorPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("AProjectileWeapon::Fire - InstigatorPawn is null!"));
		return;
	}

	if (MuzzleFlashSocket && world)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = InstigatorPawn;

		AProjectile* SpawnedProjectile = nullptr;
		if (bUseServerSideRewind)
		{
			if (InstigatorPawn->HasAuthority())
			{
				if (InstigatorPawn->IsLocallyControlled())
				{
					if (ProjectileClass)
					{
						SpawnedProjectile = world->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
						if (SpawnedProjectile)
						{
							SpawnedProjectile->bUseServerSideRewind = false;
							SpawnedProjectile->damage = Damage;
							SpawnedProjectile->headShotDamage = HeadShotDamage;
						}
					}
				}
				else
				{
					if (ServerSideRewindProjectileClass)
					{
						SpawnedProjectile = world->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
						if (SpawnedProjectile)
						{
							SpawnedProjectile->bUseServerSideRewind = true;
						}
					}
				}	
			}
			else
			{
				// !HasAuthority(): client machine
				if (InstigatorPawn->IsLocallyControlled())
				{
					// Locally controlled client: spawn a cosmetic (non-authority) projectile for visual prediction.
					// The server will spawn the real replicated projectile via ServerFire → MulticastFire.
					if (ServerSideRewindProjectileClass)
					{
						SpawnedProjectile = world->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
						if (SpawnedProjectile)
						{
							SpawnedProjectile->bUseServerSideRewind = true;
							SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
							SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
						}
					}
				}
				// Simulated proxy: do NOT spawn anything locally.
				// The server-spawned replicated projectile will appear here automatically via Actor replication.
			}
		}
		else
		{
			if (InstigatorPawn->HasAuthority())
			{
				if (ProjectileClass)
				{
					SpawnedProjectile = world->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					if (SpawnedProjectile)
					{
						SpawnedProjectile->bUseServerSideRewind = false;
						SpawnedProjectile->damage = Damage;	
						SpawnedProjectile->headShotDamage = HeadShotDamage;
						SpawnedProjectile->Owner = InstigatorPawn;
						// Set TraceStart so damage falloff calculation in OnHit uses correct distance
						SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
						SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
					}
				}
			}
		}

		if (!SpawnedProjectile)
		{
			UE_LOG(LogTemp, Warning, TEXT("AProjectileWeapon::Fire - Failed to spawn projectile! Check if ProjectileClass is set in Blueprint."));
		}
	}
}
