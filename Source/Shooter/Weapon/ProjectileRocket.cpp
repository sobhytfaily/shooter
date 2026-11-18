// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "RocketMovementComponent.h"

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("Rocket movement"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
	// Replicate actor position/rotation so all clients see the rocket moving correctly
	SetReplicateMovement(true);
}



void AProjectileRocket::BeginPlay()
{
	// Note: Super::BeginPlay() already registers OnHit on the server via AProjectile::BeginPlay.
	// We must NOT register it again here — that would cause double damage/explosion on every hit.
	Super::BeginPlay();

	if (!HasAuthority())
	{
		// Clients bind the hit locally so they see the impact FX/sound (cosmetic only; damage is server-side)
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
		if (GetInstigator())
		{
			CollisionBox->IgnoreActorWhenMoving(GetInstigator(), true);
			GetWorldTimerManager().SetTimer(ClientInstigatorIgnoreTimer, this, &AProjectileRocket::ReEnableInstigatorCollision, 0.5f, false);
		}
	}

	SpawnTrailSystem();
	if(ProjectileLoop && LoopAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			LoopAttenuation,
			(USoundConcurrency*)nullptr,
			false
		);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitCom, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bExploded)
	{
		return;
	}
	bExploded = true;

	ExplodeDamage(); // authority-checked internally
	if (HasAuthority())
	{
		StartDestroyTimer();
	}
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());

	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	if(ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
		
	}
	if(CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if(TrailComponent)
	{
		TrailComponent->Deactivate();
	}
	if(ProjectileLoopComponent && ProjectileLoop && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}
void AProjectileRocket::Destroyed()
{
	// Impact FX/sound are played in OnHit on every machine; skip AProjectile::Destroyed so they don't play twice.
	// Fallback: a client that never registered the hit locally still gets the explosion here.
	if (!bExploded && GetNetMode() != NM_DedicatedServer)
	{
		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
		}
		if (ImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
		}
	}
	AActor::Destroyed();
}