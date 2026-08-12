// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Shooter/ShooterCharacter.h"
#include "Shooter/Weapon.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "Shooter/ShooterComponent/LagCompensationComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement Component"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	// Bullets are client-predicted; movement replication is unnecessary bandwidth.
	// The server spawns the authoritative projectile; only the hit result matters.
	ProjectileMovementComponent->SetIsReplicated(false);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& event)
{
	Super::PostEditChangeProperty(event);

	FName PropertyName = event.Property != nullptr ? event.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif
void AProjectileBullet::OnHit(UPrimitiveComponent *HitCom, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse, const FHitResult &Hit)
{
	// Bullets don't damage owner
	if (OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	// Always destroy on any valid blocking hit — prevents bullets sticking in the world
	if (!OtherActor || !Hit.IsValidBlockingHit())
	{
		Super::OnHit(HitCom, OtherActor, OtherComp, NormalImpulse, Hit);
		Destroy();
		return;
	}

	// Owner chain: this projectile's Owner is the AWeapon, the weapon's Owner is the AShooterCharacter
	AShooterCharacter* OwnerCharacter = Cast<AShooterCharacter>(GetOwner()) ;
	if (OwnerCharacter == nullptr)
	{
		AWeapon* OwnerWeapon = Cast<AWeapon>(GetOwner());
		if (OwnerWeapon)
		{
			OwnerCharacter = Cast<AShooterCharacter>(OwnerWeapon->GetOwner());
		}
	}

	if (OwnerCharacter)
	{
		AShooterPlayerController* OwnerController = Cast<AShooterPlayerController>(OwnerCharacter->Controller);
		if (OwnerController)
		{
			if (OwnerCharacter->HasAuthority() && !bUseServerSideRewind)
			{
				const bool bHeadShot = Hit.BoneName == FName("head") || Hit.BoneName == FName("neck_01");
				const float BaseDamage = bHeadShot ? headShotDamage : damage;

				AWeapon* OwnerWeapon = Cast<AWeapon>(GetOwner());
				float DamageToCause = BaseDamage;
				if (OwnerWeapon)
				{
					const float Distance = (Hit.ImpactPoint - TraceStart).Size();
					DamageToCause = OwnerWeapon->CalculateDamageWithFalloff(Distance, BaseDamage);
				}

				UGameplayStatics::ApplyDamage(OtherActor, DamageToCause, OwnerController, this, UDamageType::StaticClass());
				Super::OnHit(HitCom, OtherActor, OtherComp, NormalImpulse, Hit);
				Destroy();
				return;
			}
			AShooterCharacter* HitCharacter = Cast<AShooterCharacter>(OtherActor);
			if (bUseServerSideRewind && OwnerCharacter->GetLagCompansationComponent() && OwnerCharacter->IsLocallyControlled() && HitCharacter)
			{
				OwnerCharacter->GetLagCompansationComponent()->ProjectileServerScoreRequest(
					HitCharacter, TraceStart, InitialVelocity,
					OwnerController->GetServerTime() - OwnerController->SingleTripTime);
			}
		}
	}

	// Always call Super and Destroy — handles world/static hits and cosmetic effects
	Super::OnHit(HitCom, OtherActor, OtherComp, NormalImpulse, Hit);
	Destroy();
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	//FPredictProjectilePathParams PathParams;
	//PathParams.bTraceWithChannel = true;
	//PathParams.bTraceWithCollision = true;
	//PathParams.DrawDebugTime = 5.f;
	//PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	//PathParams.LaunchVelocity = GetActorForwardVector() * IntialSpeed;
	//PathParams.MaxSimTime = 4.f;
	//PathParams.ProjectileRadius = 5.f;
	//PathParams.SimFrequency = 30.f;
	//PathParams.StartLocation = GetActorLocation();
	//PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	//PathParams.ActorsToIgnore.Add(this);
	//FPredictProjectilePathResult PathResult;
	//UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
}
