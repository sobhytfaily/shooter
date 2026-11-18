# T-0004: Weapon lifecycle (dropped despawn, ping delegate per weapon)

| Field | Value |
|---|---|
| Status | done (build passed 2026-11-19; review follow-ups applied by orchestrator; unverified in editor) |
| Assignee | ue-worker (sonnet) |
| Reviewer | ue-reviewer (sonnet) |
| Issues | BUG-023, BUG-009 |
| Build | orchestrator |
| Created | 2026-11-19 by orchestrator |
| Branch | `fix/wave1-T0004-T0006` — runs **in parallel** with T-0005 and T-0006 |

## Goal
Dropped weapons despawn after `DestroyWeaponTime`; every weapon a player holds reacts to the high-ping delegate.

## Read first
`CLAUDE.md`, `docs/GLOSSARY.md`, `docs/systems/weapons.md` (AWeapon state machine), `docs/KNOWN_ISSUES.md` rows BUG-009, BUG-023.

## Files (write) — binding lock
- `Source/Shooter/Weapon.cpp`
- `docs/systems/weapons.md` (only lines that become untrue)
- **Not** `docs/KNOWN_ISSUES.md` / `docs/agents/BOARD.md` — parallel tasks share them; list status changes in your report, the orchestrator applies them.

## Required changes
1. **BUG-023** `AWeapon::DestroyWeapon`: on authority, destroy the weapon only if it is still `EWS_Dropped`.
   Clear `DestroyWeaponTimer` when the weapon is equipped (primary or secondary) so a picked-up weapon can't be destroyed
   by an old timer. `AFlag` overrides `OnWeaponDropped` and must keep never despawning (verify, don't change Flag.cpp).
2. **BUG-009** `OnWeaponEquipped`, `OnEquippedSecondary`, `OnWeaponDropped`: replace the `HighPingDelegate.IsBound()`
   guards with `AddUniqueDynamic` (bind) and an unconditional `RemoveDynamic` (unbind), so every held weapon gets
   `OnPingTooHigh`. Keep the `HasAuthority()` + `bUseServerSideRewind` conditions.

## Constraints
No header changes, no renames, no new logs. Don't touch other files.

## Acceptance criteria
- [ ] Dropped weapon destroys after `DestroyWeaponTime` unless picked up again; flags never despawn.
- [ ] Each weapon binds/unbinds the ping delegate independently.
- [ ] Report lists KNOWN_ISSUES rows to flip.

## Playtest checklist (for the user)
1. Drop a weapon (die or swap-pickup), wait `DestroyWeaponTime` (BP, default 50 s) → it disappears. Pick one up before
   then → it stays in your hands.
2. CTF: drop the flag → it never disappears.

---
## Worker report
Status: review. Build: not run.

Changes (`Source/Shooter/Weapon.cpp`):
- `DestroyWeapon`: `if (HasAuthority() && WeaponState == EWS_Dropped) Destroy();` (replaced commented-out body).
- `OnWeaponEquipped`, `OnEquippedSecondary`: `GetWorldTimerManager().ClearTimer(DestroyWeaponTimer)` at the top (before the owner early-return).
- `OnWeaponEquipped`, `OnEquippedSecondary`: `IsBound()` guard dropped, `AddDynamic` -> `AddUniqueDynamic` (still `bUseServerSideRewind` + `HasAuthority()`).
- `OnWeaponDropped`: `IsBound()` guard dropped; `RemoveDynamic` now unconditional (still authority + SSR).
- Flag.cpp verified unchanged: `AFlag::OnWeaponDropped` does not call Super, so no timer starts; flags never despawn.
- `docs/systems/weapons.md`: state table rows updated.

Unverified: all in-editor behavior. Note: `OnWeaponDropped` also runs on clients via OnRep and sets the timer there; `DestroyWeapon` is authority-gated so this is harmless.

KNOWN_ISSUES to flip: BUG-023, BUG-009 -> fixed, unverified in editor (T-0004).

### Orchestrator addendum (user request, 2026-11-19): respawn at origin
Because dropped weapons now despawn, map-placed weapons respawn: `AWeapon::BeginPlay` (server) records
`SpawnTransform` and `bPlacedInWorld` (state `EWS_intial`). `ScheduleRespawn()` — called from `OnWeaponEquipped` /
`OnEquippedSecondary` — sets a **world** timer (`RespawnDelay`, default 60 s, EditAnywhere; `bRespawnAfterPickup` toggle)
that spawns a fresh actor of the same class at the original transform. Once per actor; skipped for starting weapons
(`bDestroyWeapon`) and flags. Files: `Weapon.h`, `Weapon.cpp`.
Playtest: pick up a map weapon → after 60 s a new one appears at the spot; pick that one up → again after 60 s.
Starting weapon dropped on death → no respawn of it anywhere.

---
## Review

**Verdict: approve** (with minor follow-ups; nothing blocks the build or the playtest)

Findings
1. Weapon.cpp:565 (OnWeaponDropped) - the `&& bUseServerSideRewind` guard was kept around the unbind. If a ping spike flipped the flag to false (OnPingTooHigh) while held, the dropped weapon stays bound to the old controller's `HighPingDelegate`. A later pickup by another player then has the previous owner's ping toggling its SSR flag. Same asymmetry on bind (Weapon.cpp:467/534): a weapon equipped while the flag is false never binds, so it never recovers. Pre-existing and the spec said to keep the condition, but BUG-009 is not fully closed. Suggest: unbind unconditionally (drop the SSR check in OnWeaponDropped) and bind regardless of the flag.
2. Weapon.cpp:567 (OnWeaponDropped) - unbind depends on `PlayerCharacter->Controller` being valid when `Dropped()` runs (cached `PlayerController` only helps if previously set). If a weapon is dropped after the controller is gone, the entry stays; harmless on destroy (dynamic delegates skip dead objects) but see item 1. Unverified for the Elim path (ShooterCharacter.cpp:1403).
3. Weapon.cpp:96-103 (ScheduleRespawn) - respawn spawns from `GetClass()` with class defaults, so per-instance level overrides on the placed weapon (Ammo, SSR, DestroyWeaponTime, any EditAnywhere tweak) are lost. Acceptable, but note it in docs, or capture the properties.
4. Weapon.cpp:96-104 - safety checks passed: lambda is weak on the UWorld (the timer manager is owned by that world, so it never fires after teardown), the capture holds no weapon pointer, and the timer is not tied to the weapon so it survives that weapon being destroyed. Seamless travel and ServerTravel/RestartGame create a new world, so pending respawns are dropped with the old one; no crash path found. Caveat (unverified): any in-place level reset (e.g. AGameMode::ResetLevel without travel) would not cancel the timer and would add an extra weapon. Cheap hardening: keep the handle in a member and clear it in EndPlay is NOT appropriate (it must outlive the actor), so just document it.
5. Duplicate / wrong-source checks passed: `bRespawnScheduled` makes it once per actor, and repeated server SetWeaponState(Equipped) calls (CombatComponent 482/549/577, swaps) only hit the guard. Client OnRep calls are blocked by `HasAuthority()`. Starting weapons are safe: BeginPlay runs inside SpawnActor (`bPlacedInWorld` becomes true) but `bDestroyWeapon = true` is set before `EquipWeapon` (ShooterCharacter.cpp:1145-1150), so the `bDestroyWeapon` guard in ScheduleRespawn wins; those also never reach Dropped() on death (DropOrDestroyWeapon destroys them). A starting weapon dropped by swap (CombatComponent.cpp:673) gets only the despawn timer, no respawn. Flags: AFlag::OnWeaponEquipped does not call Super, so ScheduleRespawn is never reached, and the WeaponType check is a second guard. Respawned copies are themselves "placed" (state EWS_intial) so the chain repeats as intended. `WeaponState` is an uninitialised UPROPERTY but zero-inits to EWS_intial (first enumerator).
6. BUG-023 trace OK: server timer set in OnWeaponDropped, cleared at the top of both equip handlers (before the owner early-return), `DestroyWeapon` re-checks state and authority. Client copies of the timer are inert. Actor timers are cleared on EndPlay. Flag.cpp unchanged and never starts the timer.
7. Docs - docs/systems/weapons.md has no mention of the respawn feature (`bRespawnAfterPickup`, `RespawnDelay`, SpawnTransform/bPlacedInWorld/ScheduleRespawn); add a row/paragraph. The `(:444)/(:402)/(:509)` line refs in the table are already stale. Weapon.h was changed (new UPROPERTYs, new private members), which is allowed by the addendum; no Blueprint-visible members were removed or renamed.
8. Minor: `UPROPERTY` ClampMin on RespawnDelay is editor-only; the code already clamps with `FMath::Max(...,0.1f)`, fine. The SetWeaponState UE_LOG (Weapon.cpp:361) is pre-existing and runs per state change, not per frame.
