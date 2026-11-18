# System: Pickups, animation, buffs & small classes

## Pickups — `Source/Shooter/PickUps/`

| Class | File | Notes |
|---|---|---|
| `APickUp` | `PickUp.h/.cpp` | replicated actor; `OverlapSphere` (r=150, Pawn overlap) bound **0.25 s after spawn, server only** (`BindOverlapTimerFinished`); `PickUpMesh` purple outline; plays `PickUpSound` in `Destroyed` |
| `AAmmoPickUp` | `AmmoPickUp.h/.cpp` | on overlap → `Combat->PickUpAmmo(weaponType, AmmoAmount)` → `Destroy()` |
| `APickUpSpawnPoint` | `PickUpSpawnPoint.h/.cpp` | spawns a random class from `PickUpClasses` after `FRandRange(SpawnPickUpTimeMin, Max)`; restarts timer on the spawned pickup's `OnDestroyed` |

BPs: `Content/BluePrints/PickUps/BP_*Ammo`, `BP_PickUp*` (weapon pickups placed in maps), `BP_PickUpSpawnPoint`,
`BP_PickUpSpawnShotgun`. **Weapon pickups are just dropped `AWeapon` actors**, not `APickUp`.

To add a pickup type (health, shield, speed): subclass `APickUp`, override `OnSphereOverlap`, act on the server.
`UBuffComponent` is the intended home for timed effects but is currently an **empty ticking component**.

## Animation — `Source/Shooter/ShooterAnimInstance.h/.cpp`
`UShooterAnimInstance::NativeUpdateAnimation` (:17) copies from the character every frame into BlueprintReadOnly
fields used by `BP_AnimBp`: `speed`, `IsInAir`, `isAccerlating`, `bWeaponEquipped`, `bIsCrouched`, `bAiming`,
`YawOffset`, `Lean`, `AO_Yaw`, `AO_Pitch`, `TurningInPlace`, `bRotateRootBone`, `bElimed`, `bHoldingTheFlag`,
`bIsSprinting`, `SprintAnimPlayRate`, plus IK data:
- `LeftHandTransform` — weapon `LeftHandSocket` converted into `hand_r` bone space (FABRIK target).
- `RightHandRotation` (local player only) — rotates right hand so the muzzle points at `HitTarget`.
- `bUseFabrick` / `bUseAimOffsets` / `bTransformRightHand` — gated on `CombatState == Unoccupied`
  (local player also respects `bLocallyReloading` and `bFinishedSwaping`).
`AnimNotify_PlaySound` plays 2D for the local player, 3D for others.
Montages (on `BP_ShooterCharacter`): Fire, Reload, HitReact, Elim, ThrowGrenade, Swap — sections listed in GLOSSARY.
Combat anim-notify callbacks are listed in combat.md.

## Small classes
- `UOverheadWidget` (`OverheadWidget.*`): shows the player name; removes itself when its level unloads.
  `ShowPlayerNetRole` is a debug stub that prints "fd".
- `IInteractWithCrosshairInterface` (`Interfaces/`): empty marker interface (crosshair turns red).
- `URocketMovementComponent` (`Weapon/RocketMovementComponent.*`): see weapons.md.
- `ATeamPlayerStart` (`PlayerStart/`): see game-flow.md.
- `Shooter.h/.cpp`: module + collision channel macros.

## Open issues
PERF-007 (empty ticks on `APickUp`, `APickUpSpawnPoint`, `UBuffComponent`), DEBT items in KNOWN_ISSUES.md.
