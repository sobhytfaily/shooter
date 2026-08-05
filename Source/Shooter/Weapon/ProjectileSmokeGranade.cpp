// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileSmokeGranade.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/BoxComponent.h"
#include "TimerManager.h"

AProjectileSmokeGranade::AProjectileSmokeGranade()
{
	// Smoke grenades don't deal damage
	damage = 0.f;
	DamageInnerRadius = 0.f;
	DamageOuterRadius = 0.f;

	// Extend destroy time to allow smoke to deploy while grenade is visible
	DestroyTime = 6.f;  // 4 second base + 2 seconds for smoke deployment
}

void AProjectileSmokeGranade::Destroyed()
{
	// Deploy smoke
	DeploySmokeCloud();

	// Don't call parent Destroyed() to avoid explosion damage
	AActor::Destroyed();
}
void AProjectileSmokeGranade::DeploySmokeCloud()
{
	// Play smoke deployment sound
	if (SmokeDeploySound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			SmokeDeploySound,
			GetActorLocation()
		);
	}

	// Spawn particle-based smoke effect
	if (SmokeEffect)
	{
		UParticleSystemComponent* SmokeComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			SmokeEffect,
			GetActorLocation(),
			GetActorRotation(),
			FVector(SmokeRadius / 100.f),  // Scale based on radius
			true,                          // bAutoDestroy
			EPSCPoolMethod::None,
			true                           // bAutoActivate
		);
	}

	// Spawn Niagara-based smoke fog system (optional, better visuals)
	if (SmokeFogSystem)
	{
		UNiagaraComponent* NiagaraSmokeComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			SmokeFogSystem,
			GetActorLocation(),
			GetActorRotation(),
			FVector(SmokeRadius / 100.f),
			true  // bAutoDestroy
		);

		if (NiagaraSmokeComponent)
		{
			// Set smoke parameters
			NiagaraSmokeComponent->SetFloatParameter(FName("SmokeRadius"), SmokeRadius);
			NiagaraSmokeComponent->SetFloatParameter(FName("SmokeDuration"), SmokeDuration);
		}
	}
}
