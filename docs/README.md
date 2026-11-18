# Docs index — start here

These docs exist so any model (Opus orchestrator or a Sonnet/Haiku worker) can find the right code fast,
without re-reading the whole project. **Read only what your task needs**, in this order:

| Step | File | Size | Read when |
|---|---|---|---|
| 1 | `../CLAUDE.md` | small | always (project rules, build, paths) |
| 2 | `GLOSSARY.md` | small | always before grepping — identifiers are misspelled |
| 3 | `ARCHITECTURE.md` | medium | first time in the codebase, or any cross-system task |
| 4 | `systems/<area>.md` | medium | the area(s) your task touches |
| 5 | `KNOWN_ISSUES.md` | medium | bug-fix / perf / review tasks |
| 6 | `agents/PROTOCOL.md` | medium | you are orchestrating or executing a task file |

## System docs

| Doc | Covers | Main files |
|---|---|---|
| `systems/character.md` | pawn, input, camera TPS/FPS, health/regen/fall damage, sprint, ladder, elim | `ShooterCharacter.*` |
| `systems/combat.md` | fire pipeline, reload, equip/swap, grenades, carried ammo, crosshair data | `ShooterComponent/CombatComponent.*` |
| `systems/weapons.md` | weapon state/ammo, hitscan, shotgun, projectile weapons, projectiles, fall-off, how to add a weapon | `Weapon.*`, `Weapon/*`, `Flag.*` |
| `systems/lag-compensation.md` | server-side rewind, frame history, time sync | `ShooterComponent/LagCompensationComponent.*` |
| `systems/game-flow.md` | match states, scoring, respawn, teams, CTF, lobby | `*GameMode.*`, `GameState/*`, `PlayerState/*`, `CaptureTheFlag/*` |
| `systems/controller-and-hud.md` | HUD setters, widgets & BindWidget names, kill feed, menus, ping | `PlayerController/*`, `ShooterHud.*`, `HUD/*` |
| `systems/pickups-animation-misc.md` | pickups, anim instance fields, buff component, small classes | `PickUps/*`, `ShooterAnimInstance.*` |
| `systems/online-and-config.md` | Steam sessions plugin, maps, input bindings, content layout, build command | `Plugins/MultiplayerSessions/*`, `Config/*` |

## Quick lookup: "where is…?"

| I need… | Go to |
|---|---|
| damage applied | `AShooterCharacter::RecieveDamage`, `AShooterGameMode::CalculateDamage` (+ Teams override) |
| a shot fired | `UCombatComponent::Fire` → `LocalFire` → `AWeapon::Fire` subclass |
| ammo numbers | `AWeapon::Ammo` (magazine), `UCombatComponent::CarriedAmmoMap` (reserve) |
| the HUD text for X | `AShooterPlayerController::SetHud<X>` |
| kill / death / respawn | `AShooterGameMode::PlayerEliminated`, `AShooterCharacter::Elim`, `RequestRespawn` |
| team logic | `ATeamsGameMode`, `AShooterPlayerState::Team`, `AShooterCharacter::SetTeamColor/SetSpawnPoint` |
| match timers | `AShooterGameMode::Tick`, `AShooterPlayerController::SetHudTime` |
| animation inputs | `UShooterAnimInstance::NativeUpdateAnimation` |
| session/host/join | `UMultiplayerSessionsSubsystem`, `UMenu`, `ALobbyGameMode` |
| key bindings | `Config/DefaultInput.ini`, `AShooterCharacter::SetupPlayerInputComponent`, `AShooterPlayerController::SetupInputComponent` |

## Keeping docs true
Docs are part of "done". Any task that changes behavior, adds a class/RPC/replicated property, or fixes an issue must
update the matching system doc and `KNOWN_ISSUES.md` in the same task. Line numbers in docs are hints — always grep.
Last full sync with code: **2026-11-18**.
