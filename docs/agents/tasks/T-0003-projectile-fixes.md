# T-0003: Projectile fixes (rocket FX on clients, bullet fall-off, shotgun sound/markers)

| Field | Value |
|---|---|
| Status | done (build passed 2026-11-19; unverified in editor) |
| Assignee | ue-worker (sonnet) |
| Reviewer | ue-reviewer (sonnet) |
| Issues | BUG-004, BUG-007, BUG-006, BUG-029 |
| Build | orchestrator |
| Created | 2026-11-19 by orchestrator |
| Branch | current working tree (`fix/T-0002-shotgun-ssr`; other uncommitted changes in CombatComponent/ShooterCharacter/LagCompensation are NOT yours — don't touch or revert them) |

## Goal
Every player sees rocket explosions; projectile-bullet damage fall-off actually applies; shotgun pellet hit sounds play
at a sane pitch and hit markers work on the first shot.

## Read first
- `CLAUDE.md`, `docs/GLOSSARY.md`
- `docs/systems/weapons.md` (AProjectileWeapon spawn matrix, Projectiles, Ashotgun, Damage fall-off)
- `docs/KNOWN_ISSUES.md` rows BUG-004, BUG-006, BUG-007, BUG-029

## Files (read)
- `Source/Shooter/Weapon.h/.cpp`, `Source/Shooter/Weapon/HitScanWeapon.cpp`, `Source/Shooter/Weapon/RocketMovementComponent.*`

## Files (write) — binding lock
- `Source/Shooter/Weapon/Projectile.h`, `Source/Shooter/Weapon/Projectile.cpp`
- `Source/Shooter/Weapon/ProjectileBullet.cpp`
- `Source/Shooter/Weapon/ProjectileRocket.h`, `Source/Shooter/Weapon/ProjectileRocket.cpp`
- `Source/Shooter/Weapon/ProjectileWeapon.cpp`
- `Source/Shooter/Weapon/shotgun.cpp`
- `docs/KNOWN_ISSUES.md` (Status column of the 4 listed issues only), `docs/systems/weapons.md` (lines that become untrue)

## Required changes
1. **BUG-004 rocket impact on clients.** Today `AProjectile::BeginPlay` binds `OnHit` only on the server and
   `AProjectileRocket::Destroyed` is empty, so clients never get impact FX/sound.
   - In `AProjectileRocket::BeginPlay`, on non-authority also bind `CollisionBox->OnComponentHit` to `OnHit`
     (cosmetic only) and ignore the instigator when moving, as the server does.
   - `AProjectileRocket::OnHit` must be safe to run on clients: `ExplodeDamage` already checks authority; keep FX/sound,
     hide mesh, disable collision, deactivate trail, stop loop sound; guard against running twice. `StartDestroyTimer`
     only on authority.
   - `AProjectileRocket::Destroyed` must call `AActor::Destroyed()` (skip `AProjectile::Destroyed` cosmetics so FX
     don't play twice). Clients that missed the local hit (e.g. rocket spawned replicated late) must not crash.
2. **BUG-007 bullet fall-off.** Add a weak pointer to the firing weapon on `AProjectile` (e.g.
   `TWeakObjectPtr<AWeapon> SourceWeapon`, not a UPROPERTY-replicated field) and set it in every spawn branch of
   `AProjectileWeapon::Fire`. Also set `TraceStart` (+ `InitialVelocity` where SSR is used) in **every** branch.
   In `AProjectileBullet::OnHit`, use `SourceWeapon` (fall back to the current owner-chain lookup) for
   `CalculateDamageWithFalloff`.
3. **BUG-006** `Ashotgun::FireShotgun`: pellet hit sound pitch → `FMath::FRandRange(0.85f, 1.15f)` (keep volume 0.5).
4. **BUG-029** `Ashotgun::FireShotgun`: assign `PlayerCharacter` (from `GetOwner()`) before the hit-marker code uses it.

## Constraints
- Don't change `AProjectile` UPROPERTY names (BPs set `damage`, `headShotDamage`, `InitialSpeed`, `DestroyTime`, FX).
- No changes outside the write-set; no new logs; no Tick work.
- Grenade/smoke projectile behavior must be unchanged.

## Acceptance criteria
- [ ] Rocket impact FX + sound appear once on host, once on each client.
- [ ] Projectile bullets get fall-off damage when the weapon BP has `bUseDamageFalloff`.
- [ ] Shotgun hit sounds have positive pitch; first-shot hit markers show.
- [ ] Compiles — orchestrator builds.
- [ ] KNOWN_ISSUES rows → `fixed, unverified in editor (T-0003)`; weapons.md updated.

## Playtest checklist (for the user)
1. PIE 2 players, Listen Server. Client fires a rocket at a wall: explosion FX + sound on **both** windows, once each.
   Host fires a rocket: same.
2. Rocket hits a player: damage applied once (check health), no double explosion.
3. Shotgun: pellet hit sounds sound normal; hit marker shows on the very first hit after spawning.
4. Grenade launcher + hand grenades + smoke still behave as before.

---
## Worker report
<!-- appended by ue-worker -->
**Status:** review. Build: not run (orchestrator builds).

**Files changed**
- `Weapon/Projectile.h`: added non-replicated `TWeakObjectPtr<class AWeapon> SourceWeapon`.
- `Weapon/ProjectileWeapon.cpp` `AProjectileWeapon::Fire`: `SourceWeapon = this` set in all 4 spawn branches (SSR+authority+local, SSR+authority+remote, SSR client-local, non-SSR authority); `TraceStart` + `InitialVelocity` now set in every branch (previously missing in SSR server branches).
- `Weapon/ProjectileBullet.cpp` `OnHit`: falloff weapon = `SourceWeapon`, falling back to `Cast<AWeapon>(GetOwner())`.
- `Weapon/ProjectileRocket.h/.cpp`: `bExploded` guard; `BeginPlay` binds `CollisionBox->OnComponentHit` to `OnHit` and ignores instigator on non-authority; `OnHit` guarded against double run, `StartDestroyTimer` authority-only (FX/sound/mesh/trail/loop run everywhere, `ExplodeDamage` already authority-checked); `Destroyed` now calls `AActor::Destroyed()`.
- `Weapon/shotgun.cpp` `FireShotgun`: pitch `FRandRange(0.85f, 1.15f)`; `PlayerCharacter` assigned (from `GetOwner()`) before the hit-marker loops.
- Docs: KNOWN_ISSUES BUG-004/006/007/029 -> `fixed, unverified in editor (T-0003)`; `systems/weapons.md` updated.

**Unverified / notes**
- Nothing compiled or playtested. Clients that never run a local hit (rocket replicated late / destroyed by server first) get no FX, by design, and don't crash.
- Server rocket still replicates; client-side hit depends on `URocketMovementComponent` generating hit events on clients (replicated movement component). If clients don't get hit events in PIE, a multicast fallback in `Destroyed` may be needed.
- Weak ptr is not set for grenade/smoke (they don't use Fire's bullet path differently); their behavior is unchanged except they also receive `SourceWeapon`/`TraceStart` (unused by them).

**Questions for orchestrator:** none.

---
## Review
<!-- appended by ue-reviewer -->

**Verdict:** approve (with non-blocking notes; items 1-3 are runtime risks to check in the PIE playtest)

**Findings**
1. `ProjectileRocket.cpp:34-42,66-70 (BeginPlay/OnHit)` — Damage is unaffected: `ExplodeDamage` is gated by `HasAuthority()`, `StartDestroyTimer` is now authority-only, and the client hit only does cosmetics. Listen-server host binds `OnHit` once (Super::BeginPlay, `!HasAuthority()` is false), `bExploded` guards re-entry, and skipping `AProjectile::Destroyed` loses nothing but the duplicate FX (it only calls `AActor::Destroyed` plus cosmetics). No problem found here.
2. `ProjectileRocket.cpp:37-41 (BeginPlay)` — Clients ignore the instigator permanently, while the server re-enables instigator collision after 0.5 s (`AProjectile::ReEnableInstigatorCollision`). If the server rocket hits the shooter later (rocket-jump, self-hit), clients get no hit and no FX. Also, if `GetInstigator()` is not yet resolved on a client at BeginPlay, the rocket can hit the shooter's own capsule at the muzzle and explode locally. Unverified. Suggest mirroring the 0.5 s re-enable on clients, or accept it as a known limitation.
3. `ProjectileRocket.cpp:104-108 (Destroyed)` — A client that never runs the local hit (rocket destroyed by the server first, replicated late, or position corrected) gets no explosion at all. Previously `Destroyed` was also empty, so this isn't a regression. Suggest a fallback: if `!bExploded && GetNetMode() != NM_DedicatedServer`, play `ImpactParticles`/`ImpactSound` there.
4. `ProjectileWeapon.cpp:70-83 (Fire, SSR client-local)` — If the rocket launcher uses SSR, the shooter's client runs a locally spawned predicted rocket (client-spawned actors have `HasAuthority()` true, so `OnHit` is bound by `AProjectile::BeginPlay`). The server's replicated rocket now also explodes on that client, which could mean a double explosion for the shooter. Unverified: it depends on the rocket BP's `bUseServerSideRewind` and `ServerSideRewindProjectileClass`. Check in playtest step 1.
5. `ProjectileBullet.cpp:77` and `ProjectileWeapon.cpp` (all four branches) — BUG-007 is correctly fixed. `SourceWeapon` is set in every branch, `TraceStart` and `InitialVelocity` are now set everywhere, and the owner-chain fallback is kept. The weak pointer is non-UPROPERTY, so there are no Blueprint or replication changes. `Weapon.h` is included in both .cpp files. Fine.
6. `shotgun.cpp:110,148` — BUG-029 and BUG-006 are correct: `PlayerCharacter` is assigned before the first hit-marker use, and the pitch is positive. The second lazy-init at line 148 is now redundant but harmless.
7. Docs: `docs/KNOWN_ISSUES.md` — the diff also flips BUG-002 and BUG-003 to "fixed (T-0002)". This is outside T-0003's lock but belongs to T-0002's uncommitted work, so don't attribute it to this task. `docs/systems/weapons.md` is accurate. Its `Destroyed (:142)` line still correctly describes the base `AProjectile`.
8. Scope: only write-set files changed. No new logs, Tick work, UPROPERTY renames, or Blueprint-visible changes. Grenade and smoke behaviour is unchanged (they only receive the unused `SourceWeapon`/`TraceStart` fields).
