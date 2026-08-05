// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectileGranade.h"
#include "ProjectileSmokeGranade.generated.h"

/**
 * Smoke grenade that creates a smoke cloud instead of explosion
 */
UCLASS()
class SHOOTER_API AProjectileSmokeGranade : public AProjectileGranade
{
	GENERATED_BODY()

public:
	AProjectileSmokeGranade();
	virtual void Destroyed() override;

protected:
	// Smoke effect settings
	UPROPERTY(EditAnywhere, Category = "Smoke")
	class UParticleSystem* SmokeEffect;

	UPROPERTY(EditAnywhere, Category = "Smoke")
	class UNiagaraSystem* SmokeFogSystem;

	UPROPERTY(EditAnywhere, Category = "Smoke")
	float SmokeDuration = 10.f;  // How long smoke lasts

	UPROPERTY(EditAnywhere, Category = "Smoke")
	float SmokeRadius = 500.f;   // Radius of smoke cloud

	UPROPERTY(EditAnywhere, Category = "Smoke")
	class USoundCue* SmokeDeploySound;

private:
	void DeploySmokeCloud();
};
