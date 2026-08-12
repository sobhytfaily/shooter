// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileGranade.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AProjectileGranade::AProjectileGranade()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Granade mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement Component"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->bShouldBounce = true;
	// Replicate actor position/rotation so all clients see the grenade bouncing correctly
	SetReplicateMovement(true);
	ProjectileMovementComponent->Bounciness = 0.15f;  // Very low bounce (barely bounces)
	ProjectileMovementComponent->Friction = 0.8f;     // High friction to stop quickly
}


void AProjectileGranade::BeginPlay()
{
	Super::BeginPlay();

	StartDestroyTimer();
	SpawnTrailSystem();

	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileGranade::OnBounce);

}

void AProjectileGranade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float VelocityMagnitude = ImpactVelocity.Size();

	// Check if sound is set and velocity is sufficient before playing
	if (BounceSound != nullptr && 
		VelocityMagnitude >= MinBounceVelocity && 
		(CurrentTime - LastBounceSoundTime) >= BounceSoundCooldown)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			BounceSound,
			GetActorLocation(),
			FRotator::ZeroRotator
		);
		LastBounceSoundTime = CurrentTime;
	}
}
void AProjectileGranade::Destroyed()
{
	ExplodeDamage();
	Super::Destroyed();

}