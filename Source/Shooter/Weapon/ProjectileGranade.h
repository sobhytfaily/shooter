// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileGranade.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AProjectileGranade : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileGranade();
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;
	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);
private:
	UPROPERTY(EditAnywhere)
	USoundCue* BounceSound;

	// Bounce sound settings
	UPROPERTY(EditAnywhere, Category = "Bounce")
	float MinBounceVelocity = 200.f;  // Minimum velocity to play sound

	UPROPERTY(EditAnywhere, Category = "Bounce")
	float BounceSoundCooldown = 0.2f;  // Cooldown between bounce sounds

	float LastBounceSoundTime = 0.f;

};
