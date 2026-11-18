# System: Shooter character

**Files:** `Source/Shooter/ShooterCharacter.h/.cpp` — `AShooterCharacter : ACharacter, IInteractWithCrosshairInterface`
(the interface is an empty marker that turns the crosshair red). Largest file in the project (~1800 lines).
**Blueprint:** `Content/BluePrints/Characters/BP_ShooterCharacter` (anim BP: `Characters/Animation/BP_AnimBp`).

## Components (constructor, ShooterCharacter.cpp:31)
`CameraBoom` (spring arm on mesh) → `FollowCamera`; `OverheadWidget` (player name); `Combat` (`UCombatComponent`);
`Buff` (`UBuffComponent`, empty); `LagCompensation` (`ULagCompensationComponent`); `DesolveTimeline`;
`AttachedGernade` (static mesh on `RightHandSocket`); `MaskMesh` (on `headSocket`); 16 `UBoxComponent` hit boxes
registered in `HitCollisionBoxies` (object type `ECC_HitBox`, used only by server-side rewind).
Net: `NetUpdateFrequency 66`, `MinNetUpdateFrequency 33`, linear smoothing with enlarged smoothing distances.

## Replicated
`OverlappingWeapon` (owner only, `OnRep_OverlappingWeapon` toggles pickup widget), `Health` (`OnRep_Health` → HUD +
hit-react), `bDisableGameplay`, `bIsSprinting` (`OnRep_IsSprinting` → MaxWalkSpeed).

## Input (legacy input, `SetupPlayerInputComponent` :504)
Axis: `MoveForward`, `MoveRight`, `Turn`, `LookUp`. Actions: `Jump`, `Equip`, `Crouch`, `Aim`, `Fire`, `Reload`,
`Gernade`, `SmokeGernade`, `ChangeFOV`, `SwapToPrimary`, `SwapToSecondary`, `Sprint`, `SprintToggle`.
`Quit` and `Scoreboard` are bound in the PlayerController. Mappings: `Config/DefaultInput.ini` (see input-config-content.md).
Most handlers early-out on `bDisableGameplay` and on `Combat->bHoldingTheFlag`.

## Tick (:320) — does a lot every frame
One-time HUD init → `UpdateHudAmmo` → `RegenerateHealth` (server) → `UpdateSprint` (local) → `RotateInPlace`
(aim offset / turn-in-place for local, sim-proxy turn for remote) → first-person crouch camera offset →
`HideCameraWhenCharacterClose` → `PollInit` (waits for PlayerState) → FPS/TPS camera transition when `Changefov`.

## Health, damage, death
- `RecieveDamage` (:830, bound to `OnTakeAnyDamage` on server): `GameMode->CalculateDamage`, clamp health, reset regen
  timer, hit-react, on 0 → kill marker + `GameMode->PlayerEliminated`; always `ServerShowDamageIndicator`.
- `RegenerateHealth` (:287): after `HealthRegenDelay` (1.5 s) heals `HealthRegenRate` (25/s). (Older root md files
  document different numbers — the code is authoritative.)
- `Landed` (:876): fall damage from impact speed, `FallDamageMinHeight 300` → 0, `FallDamageLethalHeight 700` → 100%.
  Applied with a **null instigator controller** (see BUG-015).
- `Elim` (:1369) → `DropOrDestroyWeapons` + `MulticaseElim` (:1300): elim montage, dissolve material + timeline,
  disable movement/collision, elim-bot FX, hide sniper scope, `ElimTimer` → `ElimTimerFinished` (:1411) → respawn or
  broadcast `OnLeftGame`.

## Movement extras
- **Sprint:** `bSprintButtonHeld` (hold or toggle) + `CanSprint()` (:1702) → `UpdateSprint` (:1735) sets `MaxWalkSpeed`
  and calls `ServerSetSprinting`. `SprintSpeed 750`, `WalkSpeed 600`, `SprintAnimationPlayRate 2`.
- **Ladder:** `SetIsOnLadder` (:1646, BlueprintCallable — called from `BP_Ladder`) switches to `MOVE_Flying`, forward
  input becomes vertical.
- **Crouch** toggles; `Jump` while crouched uncrouches.

## Camera: third ↔ first person
`ChangeFOV` (key `V`) sets `Changefov`; Tick then either snaps to FPS (`SocketOffset (25,10,30)`, arm 0, FOV 90, owner-
no-see on body parts, pitch clamp −60/70) or interpolates back to TPS (`(0,100,50)`, arm 300). `inFirst` is the mode flag
(read by `UCombatComponent::InterpFov` and `AimOffset`). `HideCameraWhenCharacterClose` (:955) manages visibility in both modes.

## Aim offset / turning
`AimOffset` (:667): standing still → `AO_Yaw` relative to `StartingAimRotation`, `TurnInPlace` at ±90°; moving → reset.
`CalculateYO_Ptich` remaps pitch >90 for remote pawns. Sim proxies use `SimProxiesTurn` (:760) from replicated rotation.
Values are read by `UShooterAnimInstance` (animation.md).

## BlueprintImplementableEvents (implemented in BP_ShooterCharacter)
`ShowSnipeScopeWidget`, `ShowHitMarker`, `ShowHeadShotHitMarker`, `ShowKillMarker`, `ShowHitIndicator` (uses `Angle`).

## Teams / spawn
`OnPlayerStateIntialized` (:1122, via `PollInit`) refreshes score/deaths HUD, `SetTeamColor`, `SetSpawnPoint` (team
start teleport), overhead name. `SpawnDefaultWeapon` (:1140, server) spawns `DefaultWeaponClass` with `bDestroyWeapon=true`.

## Open issues
**BUG-001 (`Destroyed` calls `Super::Destroy`)**, **BUG-015 (fall damage null instigator crashes team modes)**,
BUG-016, BUG-017, BUG-018, BUG-019, BUG-032, NET-003 (sprint trusted), NET-005 (damage indicator multicast),
NET-007 (ladder authority), PERF-001/002/003 (per-frame config reads, component scans, HUD text).
