// Fill out your copyright notice in the Description page of Project Settings.


#include "shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Shooter/ShooterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "Shooter/ShooterComponent/LagCompensationComponent.h"
#include "Particles/ParticleSystemComponent.h"



void Ashotgun::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets, bool bIsAiming)
{
    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (MuzzleFlashSocket == nullptr) return;

    const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
    const FVector TraceStart = SocketTransform.GetLocation();
    const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();

    // Use hip fire or aiming scatter parameters
    float CurrentDistanceToSphere = bIsAiming ? AimingDistanceToSphere : HipFireDistanceToSphere;
    float CurrentSphereRadius = bIsAiming ? AimingSphereRadius : HipFireSphereRadius;

    const FVector SphereCenter = TraceStart + ToTargetNormalized * CurrentDistanceToSphere;

    for (uint32 i = 0; i < numberOfPellets; i++)
    {
        const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, CurrentSphereRadius);
        const FVector EndLoc = SphereCenter + RandVec;
        FVector ToEndLoc = EndLoc - TraceStart;
        ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGHT / ToEndLoc.Size();
        HitTargets.Add(ToEndLoc);
    }
}

void Ashotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
    AWeapon::Fire(FVector());
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn == nullptr) return;
    AController* InstigatorController = OwnerPawn->GetController();
    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if (MuzzleFlashSocket)
    {
        const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        const FVector Start = SocketTransform.GetLocation();
        TMap<AShooterCharacter*, uint32> HitMap; //Maps hit character to number of hits
        TMap<AShooterCharacter*, uint32> HeadShotHitMap; //Maps hit character to number of headshots
        TMap<AShooterCharacter*, float> DistanceMap; //Maps hit character to average distance (for falloff)

        for (auto HitTarget : HitTargets)
        {
            FHitResult FireHit;
            WeaponTraceHit(Start, HitTarget, FireHit);
            AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(FireHit.GetActor());
            if (ShooterCharacter)
            {
                const bool bHeadShot = FireHit.BoneName == FName("head") || FireHit.BoneName == FName("neck_01");
                const float HitDistance = (FireHit.ImpactPoint - Start).Size();

                // Track distance for falloff calculation (average distance of all pellets that hit)
                if (DistanceMap.Contains(ShooterCharacter))
                {
                    DistanceMap[ShooterCharacter] = (DistanceMap[ShooterCharacter] + HitDistance) / 2.f;
                }
                else
                {
                    DistanceMap.Emplace(ShooterCharacter, HitDistance);
                }

                if (bHeadShot)
                {
                    if (HeadShotHitMap.Contains(ShooterCharacter)) HeadShotHitMap[ShooterCharacter]++;
                    else HeadShotHitMap.Emplace(ShooterCharacter, 1);
                }
                else
                {
                    if (HitMap.Contains(ShooterCharacter)) HitMap[ShooterCharacter]++;
                    else HitMap.Emplace(ShooterCharacter, 1);
                }

                if (ImpactParticle)
                {
                    UGameplayStatics::SpawnEmitterAtLocation(
                        GetWorld(),
                        ImpactParticle,
                        FireHit.ImpactPoint,
                        FireHit.ImpactNormal.Rotation()
                    );
                }
                if (HitSound)
                {
                    UGameplayStatics::PlaySoundAtLocation(
                        this,
                        HitSound,
                        FireHit.ImpactPoint,
                        .5f,
                        FMath::FRandRange(-.5f, .5f)
                    );
                }
            }

        }
        TArray<AShooterCharacter*> HitCharacters;
        //maps character hit to total damage
        TMap<AShooterCharacter*, float> DamageMap;
        //Calculates body damage to each character hit
        for (auto& Hitpair : HitMap)
        {
            if (Hitpair.Key)
            {
                float HitDistance = DistanceMap.Contains(Hitpair.Key) ? DistanceMap[Hitpair.Key] : 0.f;
                float BaseDamagePerPellet = CalculateDamageWithFalloff(HitDistance, Damage);
                DamageMap.Emplace(Hitpair.Key, Hitpair.Value * BaseDamagePerPellet);

                HitCharacters.AddUnique(Hitpair.Key);
                if (PlayerCharacter && PlayerCharacter->IsLocallyControlled())
                {
                    PlayerCharacter->ShowHitMarker();
                }
            }
        }
        //Calculates headshot damage to each character hit
        for (auto& HeadShotHitpair : HeadShotHitMap)
        {
            if (HeadShotHitpair.Key)
            {
                float HitDistance = DistanceMap.Contains(HeadShotHitpair.Key) ? DistanceMap[HeadShotHitpair.Key] : 0.f;
                float HeadShotDamagePerPellet = CalculateDamageWithFalloff(HitDistance, HeadShotDamage);

                if (DamageMap.Contains(HeadShotHitpair.Key)) DamageMap[HeadShotHitpair.Key] += HeadShotHitpair.Value * HeadShotDamagePerPellet;
                else DamageMap.Emplace(HeadShotHitpair.Key, HeadShotHitpair.Value * HeadShotDamagePerPellet);

                HitCharacters.AddUnique(HeadShotHitpair.Key);
                if (PlayerCharacter && PlayerCharacter->IsLocallyControlled())
                {
                    PlayerCharacter->ShowHeadShotHitMarker();
                }
            }
        }
        PlayerCharacter = PlayerCharacter == nullptr ? Cast<AShooterCharacter>(GetOwner()) : PlayerCharacter;
        //loop through damage map to calculate total damage for each character
        for (auto& DamagePair : DamageMap)
        {
            if (DamagePair.Key && InstigatorController)
            {
                bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
                if (HasAuthority() && bCauseAuthDamage)
                {
                    // Applies damage to each character seperatly 
                    UGameplayStatics::ApplyDamage(
                        DamagePair.Key,
                        DamagePair.Value,
                        InstigatorController,
                        this,
                        UDamageType::StaticClass()
                    );
                }
            }
        }
        if (!HasAuthority() && bUseServerSideRewind )
        {
            PlayerCharacter = PlayerCharacter == nullptr ? Cast<AShooterCharacter>(OwnerPawn) : PlayerCharacter;
            PlayerController = PlayerController == nullptr ? Cast<AShooterPlayerController>(InstigatorController) : PlayerController;
            if (PlayerCharacter && PlayerController && PlayerCharacter->GetLagCompansationComponent() && PlayerCharacter->IsLocallyControlled() && PlayerCharacter->IsLocallyControlled())
            {
                PlayerCharacter->GetLagCompansationComponent()->ShotgunServerScoreRequest(
                    HitCharacters,
                    Start,
                    HitTargets,
                    PlayerController->GetServerTime() - PlayerController->SingleTripTime
                );
            }
        }
    }
}


