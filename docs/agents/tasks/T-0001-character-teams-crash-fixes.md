# T-0001: Character & Teams crash/null fixes

| Field | Value |
|---|---|
| Status | done (build passed 2026-11-18; unverified in editor) |
| Assignee | ue-worker (sonnet) |
| Reviewer | ue-reviewer (sonnet) |
| Issues | BUG-001, BUG-015, BUG-014, BUG-016, BUG-017, BUG-018, BUG-019 |
| Build | orchestrator |
| Created | 2026-11-18 by orchestrator |

## Goal
Remove the crash-class and null-safety bugs in `AShooterCharacter` and `ATeamsGameMode`. Teams/CTF matches must
survive damaging falls, characters must run the engine's normal `Destroyed` path, and team logout must clean up properly.

## Read first
- `CLAUDE.md`, `docs/GLOSSARY.md`
- `docs/systems/character.md` (Health, damage, death; Teams / spawn)
- `docs/systems/game-flow.md` (Elimination & respawn; Teams)
- `docs/KNOWN_ISSUES.md` rows BUG-001, BUG-014, BUG-015, BUG-016, BUG-017, BUG-018, BUG-019

## Files (read)
- `Source/Shooter/ShooterCharacter.h`, `Source/Shooter/TeamsGameMode.h`, `Source/Shooter/ShooterGameMode.h/.cpp`

## Files (write) — binding lock
- `Source/Shooter/ShooterCharacter.cpp`
- `Source/Shooter/TeamsGameMode.cpp`
- `docs/KNOWN_ISSUES.md` (Status column of the 7 listed issues only)
- `docs/systems/character.md`, `docs/systems/game-flow.md` (only lines that become untrue)

## Required changes
1. **BUG-001** `AShooterCharacter::Destroyed()`: `Super::Destroy()` → `Super::Destroyed()`. Keep the rest of the
   function's behavior.
2. **BUG-015** `ATeamsGameMode::CalculateDamage`: null-check `Attacker` and `Victim` before calling `GetPlayerState`;
   return `BaseDamage` if either is null (fall damage has no instigator and must still hurt). Keep friendly-fire = 0 and
   self-damage allowed.
3. **BUG-014** `ATeamsGameMode::Logout`: call `Super::Logout(Exiting)` (after removing the player from the team arrays);
   null-check `Exiting`.
4. **BUG-016** `AShooterCharacter::SetTeamColor`: early return if `GetMesh() == nullptr`; materials may be null
   individually — only call `SetMaterial` with a non-null material (don't skip the whole function just because one is null).
5. **BUG-017** `AShooterCharacter::BeginPlay`: the spawn sound block checks `ElimBotSound` but plays
   `ElimBotSoundSpawn` — check `ElimBotSoundSpawn`.
6. **BUG-018** `AShooterCharacter::Tick`: remove the `if (!Combat) { Combat = ...; Combat->Character = this; }` block
   (Combat is a default subobject; this block can only dereference null).
7. **BUG-019** `AShooterCharacter::SpawnDefaultWeapon`: null-check the spawned weapon before using it; also null-check
   `GetPickUpWidget()` before `DestroyComponent()`.

## Constraints
- Don't rename anything; don't touch headers.
- No behavior changes beyond the fixes above; no refactors, no new logs.

## Acceptance criteria
- [ ] All 7 changes made exactly in the listed functions; no other code edits.
- [ ] Compiles (`ShooterEditor Win64 Development`) — orchestrator builds.
- [ ] KNOWN_ISSUES rows set to `fixed, unverified in editor (T-0001)`.
- [ ] character.md / game-flow.md "Open issues" lines updated to drop the fixed IDs.

## Playtest checklist (for the user)
1. Teams map, PIE 2 players, Listen Server: jump off something tall (>700 units) → you die, **no crash**, death counts.
2. Teams: shoot a teammate → 0 damage; shoot an enemy → damage; grenade yourself → damage.
3. Die and respawn several times; quit one client mid-match → no crash / ensure in Output Log.
4. FFA map: die/respawn; elim-bot spawn sound still plays on spawn (if assigned in BP).

---
## Worker report
All 7 changes made as specified (ShooterCharacter.cpp: BeginPlay, Tick, SpawnDefaultWeapon, SetTeamColor, Destroyed;
TeamsGameMode.cpp: Logout, CalculateDamage). Logout null-checks `Exiting` (skips team removal) and always calls
`Super::Logout(Exiting)` last. KNOWN_ISSUES rows set to `fixed, unverified in editor (T-0001)`; character.md and
game-flow.md open-issue lines updated.
Build: not run. Doc updates needed: none beyond those done.

---
## Review
<!-- appended by ue-reviewer -->
**Verdict: approve**

Findings (none blocking; all 7 changes traced and correct):
1. ShooterCharacter.cpp (BeginPlay/Tick/SpawnDefaultWeapon/SetTeamColor/Destroyed) and TeamsGameMode.cpp (Logout/CalculateDamage) — all 7 required changes match the spec; no header or rename changes; only write-set files plus BOARD.md (orchestrator's) changed.
2. ShooterCharacter.cpp Tick (BUG-018) — removal is safe: Combat is created in the constructor (:48) and `Combat->Character` is set in PostInitializeComponents (:185).
3. TeamsGameMode.cpp Logout — Super::Logout is called last on all paths, incl. null Exiting; good. Note: Super::Logout now runs the base-class logic (previously skipped) — verify in the 'quit client mid-match' playtest.
4. ShooterCharacter.cpp Destroyed — now runs AActor::Destroyed (broadcasts OnDestroyed); rest of the function unchanged. Unverified in editor.
5. ShooterCharacter.cpp SpawnDefaultWeapon (minor, pre-existing, out of scope) — if `Combat` were null the spawned weapon would be left unowned; not reachable in practice, no change requested.
6. Docs — KNOWN_ISSUES statuses and character.md / game-flow.md open-issue lines are updated correctly. (KNOWN_ISSUES BUG-016 "Problem" text still describes the old `&&` guard; acceptable for a fixed row.)
