

#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "Shooter/ShooterCharacter.h"
#include "Shooter/Shooter.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);


}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}

	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);

		// Ignore the instigator (shooter) for a brief period to prevent self-collision jitter
		if (GetInstigator())
		{
			CollisionBox->IgnoreActorWhenMoving(GetInstigator(), true);

			// Re-enable collision with instigator after 0.5 seconds (for grenades/rockets that can damage owner later)
			GetWorldTimerManager().SetTimer(
				InstigatorIgnoreTimerHandle,
				this,
				&AProjectile::ReEnableInstigatorCollision,
				0.5f,
				false
			);
		}
	}

}

void AProjectile::ReEnableInstigatorCollision()
{
	if (GetInstigator() && CollisionBox)
	{
		CollisionBox->IgnoreActorWhenMoving(GetInstigator(), false);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitCom, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{	
	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);

	// Allow subclasses to handle hit events before destruction
	// Subclasses may override this method to add custom behavior (damage, explosions, etc.)
	// Call Destroy() in subclass implementation if they want immediate destruction
}
void AProjectile::DestroyTimerFinshed()
{
	Destroy();
}

void AProjectile::ExplodeDamage()
{
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn && HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();	
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this, // WOrld context object
				damage, // Base Damage
				10.f, // MinimumDamage
				GetActorLocation(), //Origin
				DamageInnerRadius, // Damage inner radius
				DamageOuterRadius, // damage outer radius
				1.f, // Damage Fall off
				UDamageType::StaticClass(), // DamageTypeClass
				TArray<AActor*>(), // Actors to ignore - grenades/rockets CAN damage owner
				this,	// Damage Causer	
				FiringController // InsitigaorController
			);
		}
	}
}


void AProjectile::SpawnTrailSystem()
{
	if (TrailSystem)
	{

		TrailComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);

	}
}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimerHandle,
		this,
		&AProjectile::DestroyTimerFinshed,
		DestroyTime);
}

void AProjectile::Destroyed()
{
	Super::Destroyed();
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());

	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


