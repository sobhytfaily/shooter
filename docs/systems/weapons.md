# System: Weapons & projectiles

**Owns:** weapon actors, ammo-in-magazine, firing implementations (hitscan / projectile / shotgun), projectiles,
grenades, rockets, smoke, shell casings, damage fall-off.
**Driven by:** `UCombatComponent` (combat.md) which decides *when* to fire; weapons decide *how*.

## Class tree

```
AActor
├─ AWeapon                         Source/Shooter/Weapon.h/.cpp            (BP_Weapon)
│  ├─ AHitScanWeapon               Weapon/HitScanWeapon.*                  (BP_AssualtRifle, BP_Pistol, BP_SMG, BP_SniperRifle — verify in BP)
│  │  └─ Ashotgun                  Weapon/shotgun.*                        (BP_shotgun)
│  ├─ AProjectileWeapon            Weapon/ProjectileWeapon.*               (BP_RocketLuncher, BP_GranadeLuncher)
│  └─ AFlag                        Flag.*                                  (BP_Flag, BP_GreenFlag, BP_GreyFlag)
├─ AProjectile                     Weapon/Projectile.*                     (BP_Projectile)
│  ├─ AProjectileBullet            Weapon/ProjectileBullet.*               (BP_ProjectileBullet, BP_ServerSideRewindProjectileBullet)
│  ├─ AProjectileRocket            Weapon/ProjectileRocket.* + URocketMovementComponent (BP_Rocket)
│  └─ AProjectileGranade           Weapon/ProjectileGranade.*              (BP_ProjectileGranade, BP_ThrowGernade)
│     └─ AProjectileSmokeGranade   Weapon/ProjectileSmokeGranade.*         (BP_SmokeGrenade)
└─ ABulletShell                    Weapon/BulletShell.*                    (BulletShells/BP_*)
```
Which C++ class each weapon BP derives from is set in the editor — confirm in the BP before relying on the mapping above.

## AWeapon (Weapon.cpp)

Components: `WeaponMesh` (root, skeletal), `AreaSphere` (pickup overlap, enabled in `BeginPlay`), `PickupWidget`.

**State machine** — `SetWeaponState` (:321) → `OnWeaponStateSet` (:337), replicated via `OnRep_WeaponState`:

| State | Handler | Effect |
|---|---|---|
| `EWS_Equipped` | `OnWeaponEquipped` (:444) | no physics/collision, hide outline, clear `DestroyWeaponTimer`, `AddUniqueDynamic` to `HighPingDelegate` if SSR |
| `EWS_EquippedSecondary` | `OnEquippedSecondary` (:402) | same, attached to back (also clears `DestroyWeaponTimer`, binds ping delegate) |
| `EWS_Dropped` | `OnWeaponDropped` (:509) | physics on, outline on, unbind ping delegate, start `DestroyWeaponTimer` (`DestroyWeapon` destroys on authority only if still `EWS_Dropped`; `AFlag` overrides and never despawns) |

SMG is special-cased (strap physics: `QueryAndPhysics` + gravity, ignores all channels).

**Ammo (client prediction with sequence numbers):**
- `Ammo` replicates `COND_OwnerOnly`; `MagCapacity` is BP-set.
- `SpendRound` (:108): decrement locally; server sends `ClientUpdateAmmo(Ammo)`; client increments `Sequence`.
- `ClientUpdateAmmo` (:137): `Ammo = ServerAmmo - (--Sequence)` → reconciles unacknowledged shots.
- `AddAmmo` (:145) server-side + `ClientAddAmmo` (:188) (which also triggers `JumpToShotgunEnd` when full).
- HUD push: `SetHudAmmo` (:220) → `AShooterPlayerController::SetHudWeaponAmmo`.

**Firing base:** `Fire` (:562) plays `FireAnimation`, spawns `BulletShellClass` at `AmmoEject`, calls `SpendRound`.
Subclasses call `Super::Fire` then do their own trace/spawn.

**Scatter:** `TraceEndWithScatter` (:236) – random point in a sphere `*DistanceToSphere` in front of the muzzle,
separate hip-fire (`bHipFireScatter`, 80/80) and aim (`bAimingScatter`, 40/40) settings.

**Damage fall-off:** `CalculateDamageWithFalloff(Distance, Base)` (:270): full damage ≤ `MinDamageDistance` (500),
linear to `Base*MinDamageMultiplier` (0.5) at `MaxDamageDistance` (5000). Only if `bUseDamageFalloff`.

**Server-side rewind flag:** `bUseServerSideRewind` (replicated owner-only). `bWantsServerSideRewind` captures the BP
value in `BeginPlay`; weapons that want SSR bind `HighPingDelegate` (`AddUniqueDynamic`) on every equip and always
unbind on drop. `OnPingTooHigh` sets `bUseServerSideRewind = bWantsServerSideRewind && !bPingTooHigh`, so high ping can
turn SSR off and back on, but never on for a weapon configured without it.

**Despawn & respawn (T-0004):** a dropped weapon is destroyed after `DestroyWeaponTime` (50 s) unless picked up again
(timer cleared on equip; flags never despawn). Map-placed weapons (`EWS_intial` at `BeginPlay`) remember their
`SpawnTransform`; the first pickup calls `ScheduleRespawn()`, which sets a **world** timer (`RespawnDelay`, 60 s;
`bRespawnAfterPickup` toggle) that spawns a fresh actor of the same class at that spot. Once per actor; starting weapons
(`bDestroyWeapon`) and flags are skipped. The copy uses class defaults, so per-instance level overrides (e.g. ammo) are
not carried over. An in-place level reset without travel would not cancel a pending respawn.

Key tunables (EditAnywhere): `FireDelay`, `bAutomatic`, `Damage`, `HeadShotDamage`, `ZoomedFov`, `ZoomInterpSpeed`,
`MagCapacity`, `WeaponType`, `FireType`, `Team` (flags), crosshair textures, `EquipeSound`, `DestroyWeaponTime`,
`bRespawnAfterPickup`, `RespawnDelay`.

## AHitScanWeapon::Fire (HitScanWeapon.cpp:14)
Line trace from `MuzzleFlash` socket toward `HitTarget * 1.25` (`WeaponTraceHit`, :111, ignores owner).
- Locally controlled shooter → hit-marker / headshot-marker (BlueprintImplementableEvents on character).
- **Server** applies damage if `!bUseServerSideRewind || shooter is the listen-server host`.
- **Client with SSR** sends `LagCompensation->ServerScoreRequest(HitChar, Start, HitTarget, ServerTime - SingleTripTime)`.
- Cosmetics: `ImpactParticle`, `HitSound`, `MuzzleFlash`, `FireSound`, `BeamParticle` (beam to impact).

## Ashotgun (shotgun.cpp)
`ShotgunTraceEndWithScatter` (:15) generates `numberOfPellets` (10) scattered endpoints (client), sent as an array.
`FireShotgun` (:40) traces each pellet, accumulates body/head hits per character in maps, applies one
`ApplyDamage` per character on the server, or sends `ShotgunServerScoreRequest` when SSR. Shotgun reload is
one-shell-at-a-time (see combat.md).

## AProjectileWeapon::Fire (ProjectileWeapon.cpp:9)
Spawns from `MuzzleFlash` toward the target. Spawn matrix:

| SSR? | Machine | Spawns |
|---|---|---|
| no | server | `ProjectileClass` (replicated, deals damage) |
| no | client | nothing (sees server's replicated projectile) |
| yes | server, local host | `ProjectileClass`, no SSR |
| yes | server, remote shooter | `ServerSideRewindProjectileClass` (`bUseServerSideRewind=true`, no damage) |
| yes | client, local shooter | `ServerSideRewindProjectileClass` cosmetic + `TraceStart`/`InitialVelocity` for rewind |
| yes | simulated proxy | nothing |

## Projectiles (Projectile.cpp)
- `AProjectile`: `CollisionBox` root (blocks Visibility/WorldStatic/Pawn/SkeletalMesh), optional `Tracer`,
  Niagara trail (`SpawnTrailSystem`), `OnHit` bound on the server (rocket also binds it on clients, cosmetic only), instigator collision ignored for 0.5 s.
- `ExplodeDamage` (:89): radial damage with fall-off (`damage`, min 10, `DamageInnerRadius` 200, `DamageOuterRadius` 500),
  **can damage the instigator**.
- `Destroyed` (:142): impact particles + sound (skipped on dedicated server).
- `AProjectileBullet::OnHit` (ProjectileBullet.cpp:40): server damage (fall-off via `SourceWeapon`, set with `TraceStart`/`InitialVelocity` in every `AProjectileWeapon::Fire` spawn branch) when not SSR; locally-controlled SSR shooter sends
  `ProjectileServerScoreRequest`. Movement not replicated (client-predicted).
- `AProjectileRocket`: `URocketMovementComponent` keeps flying through blocking hits (`AdvanceNextSubstep`);
  `OnHit` (server and clients, guarded by `bExploded`) explodes (server-only damage), plays FX/sound, hides mesh, stops loop sound, server destroys after `DestroyTime`; `Destroyed` only calls `AActor::Destroyed` (no duplicate FX).
- `AProjectileGranade`: bouncy, replicated movement; explodes in `Destroyed` after `DestroyTime`; bounce sound with
  velocity/cooldown gate.
- `AProjectileSmokeGranade`: no damage; `Destroyed` spawns Cascade and/or Niagara smoke scaled by `SmokeRadius`;
  skips `AProjectile::Destroyed` on purpose.
- `ABulletShell`: physics casing, plays `ShellSound` and destroys on first hit.

## Open issues
(BUG-004/006/007/029 fixed in T-0003, unverified in editor.)
BUG-009 (ping delegate bound once), BUG-023 (dropped weapons never despawn), PERF-007 (empty ticks),
PERF-008 (log spam). See `docs/KNOWN_ISSUES.md`.

## How to add a weapon
1. Add `EWT_*` to `EWeaponType` (WeaponTypes.h) — append before `EWT_MAX` to keep BP enum values stable.
2. Add starting ammo in `UCombatComponent::IntializeCarriedAmmo` (+ a `Starting*Ammo` UPROPERTY).
3. Add a reload montage section name in `AShooterCharacter::PlayReloadMontage` and the section in the montage.
4. Create a BP from `AHitScanWeapon` / `Ashotgun` / `AProjectileWeapon`, set `FireType`, `WeaponType`, sockets
   (`MuzzleFlash`, `AmmoEject`, `LeftHandSocket`), crosshairs, sounds.
5. Optional pickup: BP from `AAmmoPickUp` with the new `weaponType`; add to a `BP_PickUpSpawnPoint`.
