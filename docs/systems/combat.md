# System: Combat component

**File:** `Source/Shooter/ShooterComponent/CombatComponent.h/.cpp` — `UCombatComponent` (replicated, ticks).
**Owner:** `AShooterCharacter` (created in its constructor, `Character` back-pointer set in `PostInitializeComponents`;
`AShooterCharacter` is a `friend` and reads the private fields directly).
**Owns:** equipped/secondary weapon, aiming, firing cadence, reload, carried ammo, grenades, swap, flag-holding flag,
crosshair data for the HUD, camera FOV zoom.

## Replicated state

| Property | Condition | OnRep |
|---|---|---|
| `EquippedWeapon` | all | `OnRep_EuippedWeapon` (:206) attach to right hand, state Equipped |
| `SecondaryWeapon` | all | `OnRep_SecondaryWeapon` (:230) attach to back |
| `bAiming` | all | `OnRep_Aiming` (:593) local player keeps its own button state |
| `CarriedAmmo` | owner only | `OnRep_CarriedAmmo` (:1064) HUD + shotgun-end |
| `CombatState` (`ECombatState`) | all | `OnRep_CombatState` (:772) plays montages on remote machines |
| `Gerandes`, `SmokeGerandes` | all | HUD |
| `bHoldingTheFlag` | all | crouch locally |

`CarriedAmmoMap` (`TMap<EWeaponType,int32>`) is **server only**; `CarriedAmmo` is the replicated view of the current weapon's entry.

## Per-frame (locally controlled only) — `TickComponent` (:73)
`TraceUnderCrosshairs` → `HitTarget`; `SetHudCrosshairs` (spread = velocity + in-air − aim + shooting factors);
`InterpFov` (camera zoom; TPS any weapon, FPS sniper only); `UpdateGrenadeTrajectory` (debug-line arc while
holding grenade); `ReloadEmptyWeapon` (auto-reload).

## Fire pipeline

```
Input Fire → AShooterCharacter::FireButtonPressed → Combat::FireButtonPressed(true) (:312)
  Fire() (:250)  if CanFire(): bCanFire=false; switch FireType:
     FireHitScanWeapon / FireProjectileWeapon (:289/:277): apply scatter → HitTarget
         client: LocalFire(HitTarget) immediately (prediction)
         ServerFire(HitTarget, FireDelay) ──► MulticastFire ──► LocalFire on everyone
                                               except the predicting client
     FireShotgun (:301): pellets array → LocalShotgunFire / ServerShotgunFire → MulticastShotgunFire
  StartFireTimer() → FireDelay → FireTimerFinished (:182): bCanFire=true; re-fire if automatic & held
LocalFire (:419): only when CombatState == Unoccupied → PlayFireMontage + EquippedWeapon->Fire(target)
```
`CanFire` (:1050): not empty, `bCanFire`, `Unoccupied`, not `bLocallyReloading` — **except** shotgun may fire
while `Reloading` (interrupts shell reload).

## Reload
`Reload` (:676) (client or server) → `ServerReload` sets `CombatState=Reloading` + `HandleReload` (montage).
Montage anim-notify (Blueprint) calls `FinishReloading` (:861) → server `UpdateAmmoValues` (:806) moves
`AmountToReload()` from `CarriedAmmoMap` into the weapon.
Shotgun: notify `ShotgunShellReload` (:321) → `UpdateShotgunAmmoValues` (:822) +1 shell; `JumpToShotgunEnd`
when full/empty (montage section `ShotgunEnd`).

## Equip / swap
- `EquipWeapon` (:473, server via `AShooterCharacter::ServerEquipButtonPressed`):
  flag → `AttachFlagToLeftHand`, crouch, `bHoldingTheFlag`; else if a primary exists and no secondary →
  `EquipSecondayWeapon`, else `EquipPrimaryWeapon` (drops current primary).
- Swap: `AShooterCharacter::SwapToPrimary/SecondaryWeapon` → `SwapWeapon` (:507) (server; client predicts montage +
  `ServerSwapWeapon`). Swap montage notifies `FinishSwapWeapons` (re-attach) and `FinishSwap` (state → Unoccupied).

## Grenades (hold to aim, release to throw — changed 2026-11-19)
- **Press** (`GrenadeButtonPressed`) → `ThrowGernade` (owner) + `ServerThrowGernade` (server): state `ThrowingGernade`,
  montage, show `AttachedGernade`, decrement count (server), `bGrenadeHeld = true` on owner **and** server.
- **Hold:** owner starts `GrenadeHoldTimer` (`GrenadeHoldPoseTime`, default 0.35 s, EditAnywhere) →
  `PauseGrenadeMontage()` freezes the wind-up while `bGrenadeHeld`.
- **Release** (`GrenadeButtonReleased`) → `ReleaseGrenade()`: clear timer, `ResumeGrenadeMontage(GrenadeReleasePlayRate)`
  (default 1.75×). Tapping before the hold point just speeds the throw up.
- **Montage notify `LunchGernade`** (hand-release frame): on the owner, if released → `LaunchGrenadeNow()`: owner predicts
  `FinishGrenadeThrow()` (hide grenade, `StopGrenadeMontage`, `ThrowGernadeFinish` → `Unoccupied`) and sends
  `ServerReleaseGrenade(HitTarget)`; the server spawns via `SpawnGrenade(Target, server's bThrowingSmokeGernade)` and
  finishes. Host does both locally. If still held when the notify fires (hold point set too late) it pauses there.
  Remote views just hide the attached grenade.
- `ThrowGernadeFinish` now no-ops unless state is `ThrowingGernade` (protects against blend-out notifies).
- Remote views: `OnRep_CombatState(Unoccupied)` stops the throw montage, hides the grenade, re-attaches the weapon.
  Simulated proxies don't see the held pose (the hold flag isn't replicated) — known cosmetic limitation.
Smoke grenades use the same path (`SmokeGrenadeButtonPressed/Released`, `bThrowingSmokeGernade` selects the class).
Requires the `LunchGernade` notify to sit at the hand-release frame of `ThrowGrenadeMontage`.

Anim-notify entry points (`UFUNCTION(BlueprintCallable)`, called from montages in `BP_AnimBp`):
`FinishReloading`, `ShotgunShellReload`, `FinishSwap`, `FinishSwapWeapons`, `ThrowGernadeFinish`, `LunchGernade`.
**Renaming any of these breaks Blueprints.**

## Ammo pickup
`PickUpAmmo(type, amount)` (:340) clamps into `CarriedAmmoMap` (`MaxCarriedAmmo` 300) and auto-reloads if the
equipped weapon of that type is empty.

## Tunables (EditAnywhere, set in BP_ShooterCharacter's component)
`BaseWalkSpeed`, `AimWalkSpeed`, `ZoomedFov`, `ZoomInterpSpeed`, `Starting*Ammo`, `MaxCarriedAmmo`, `Gerandes/MaxGerandes`,
`SmokeGerandes/MaxSmokeGerandes`, `GernadeClass`, `SmokeGernadeClass`, `Grenade*ThrowSpeed/GravityScale`, `TrajectorySegments`.
Unused: `TrajectorySpline`, `TrajectoryMeshes`, `TrajectoryMesh/Material`, `LandingDecal*` (`ShowGrenadeTrajectory` is empty).

## Open issues
BUG-005 (inverted `Controller ?` ternary → HUD not updated), BUG-028 (double hit marker), BUG-030 (null derefs),
BUG-032 (two walk-speed sources), NET-001 (server trusts fire RPCs), NET-006 (reliable multicasts), PERF-005.
