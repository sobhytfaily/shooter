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
	ProjectileMovementComponent->SetIsReplicated(true);
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

	// Validate hit data and OtherActor before accessing hit properties
	if (!OtherActor || !Hit.IsValidBlockingHit())
	{
		Super::OnHit(HitCom, OtherActor, OtherComp, NormalImpulse, Hit);
		return;
	}

	AShooterCharacter* OwnerCharacter = Cast<AShooterCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		AShooterPlayerController* OwnerContoller = Cast<AShooterPlayerController>(OwnerCharacter->Controller);
		if (OwnerContoller)
		{
			if (OwnerCharacter->HasAuthority() && !bUseServerSideRewind)
			{
				const bool bHeadShot = Hit.BoneName == FName("head") || Hit.BoneName == FName("neck_01");
				const float BaseDamage = bHeadShot ? headShotDamage : damage;

				// Calculate distance from spawn point for damage falloff
				AWeapon* OwnerWeapon = Cast<AWeapon>(GetOwner()->GetOwner());
				float DamageToCause = BaseDamage;

				if (OwnerWeapon)
				{
					const float Distance = (Hit.ImpactPoint - TraceStart).Size();
					DamageToCause = OwnerWeapon->CalculateDamageWithFalloff(Distance, BaseDamage);
				}

				UGameplayStatics::ApplyDamage(OtherActor, DamageToCause, OwnerContoller, this, UDamageType::StaticClass());
				Super::OnHit(HitCom, OtherActor, OtherComp, NormalImpulse, Hit);
				return;
			}
			AShooterCharacter* HitCharacter = Cast<AShooterCharacter>(OtherActor);
			if (bUseServerSideRewind && OwnerCharacter->GetLagCompansationComponent() && OwnerCharacter->IsLocallyControlled() && HitCharacter)
			{
				OwnerCharacter->GetLagCompansationComponent()->ProjectileServerScoreRequest(HitCharacter, TraceStart, InitialVelocity, OwnerContoller->GetServerTime() - OwnerContoller->SingleTripTime);
			}
		}
	}
	Super::OnHit(HitCom, OtherActor, OtherComp, NormalImpulse, Hit);
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
