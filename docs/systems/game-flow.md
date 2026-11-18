# System: Game flow — GameModes, GameState, PlayerState, CTF

**Owns:** match lifecycle, scoring, teams, respawn, flag capture.
**Authority:** everything here runs on the server except `OnRep_*` handlers.

## Files

| File | Class | Base |
|---|---|---|
| `Source/Shooter/ShooterGameMode.h/.cpp` | `AShooterGameMode` (free-for-all) | `AGameMode` |
| `Source/Shooter/TeamsGameMode.h/.cpp` | `ATeamsGameMode` | `AShooterGameMode` |
| `Source/Shooter/CaptureTheFlagGameMode.h/.cpp` | `ACaptureTheFlagGameMode` | `ATeamsGameMode` |
| `Source/Shooter/LobbyGameMode.h/.cpp` | `ALobbyGameMode` | `AGameMode` |
| `Source/Shooter/GameState/ShooterGameState.h/.cpp` | `AShooterGameState` | `AGameState` |
| `Source/Shooter/PlayerState/ShooterPlayerState.h/.cpp` | `AShooterPlayerState` | `APlayerState` |
| `Source/Shooter/PlayerStart/TeamPlayerStart.h` | `ATeamPlayerStart` (adds `ETeam Team`) | `APlayerStart` |
| `Source/Shooter/Flag.h/.cpp` | `AFlag` | `AWeapon` (see weapons.md) |
| `Source/Shooter/CaptureTheFlag/FlagZone.h/.cpp` | `AFlagZone` (sphere + `ETeam Team`) | `AActor` |
| `Source/Shooter/Announcement.h` | namespace `Announcement` (end-of-match strings) | — |

Blueprints: `Content/BluePrints/GameModes/BP_{Shooter,Teams,CTF,Lobby}GameMode`, `GameState/BP_ShooterGameState`,
`PlayerState/BP_ShooterPlayerState`, `PlayerStart/BP_Green/GreyTeamPlayerStart`, `CaptureTheFlag/BP_Green/GreyFlagZone`,
`Weapon/BP_Flag`, `BP_GreenFlag`, `BP_GreyFlag`.

## Match lifecycle (`AShooterGameMode`)

`bDelayedStart = true`, so the match sits in `WaitingToStart` until the warm-up ends.

```
BeginPlay ── LevelStartingTime = now; first-launch sets scalability to Medium (GConfig flag)
Tick (ShooterGameMode.cpp:55)
  WaitingToStart: CountDown = WarmUpTime - now + LevelStartingTime → StartMatch()
  InProgress:     CountDown = WarmUp + MatchTime - now + Start      → SetMatchState(Cooldown)
  Cooldown:       CountDown = Cooldown + WarmUp + Match - now + Start → RestartGame()
OnMatchStateSet (:84) → every AShooterPlayerController::OnMatchStateSet(MatchState, bTeamMatch)
```
Defaults (EditDefaultsOnly, overridden in BP): `WarmUpTime 10`, `MatchTime 120`, `CooldownTime 10`.
Clients compute their own countdown from values sent by `ServerCheckMatchState` (see controller-and-hud.md).

## Elimination & respawn
1. `AShooterCharacter::RecieveDamage` (server) → `GameMode->CalculateDamage(Attacker, Victim, Dmg)` → health hits 0.
2. `AShooterGameMode::PlayerEliminated` (:101): attacker `AddToScore(1)`, `GameState->UpdateTopScore`, victim `AddToDeaths(1)`,
   `Character->Elim(false)`, then `BroadcastElim` to **every** controller (kill-feed).
3. `AShooterCharacter::Elim` → `DropOrDestroyWeapons` + `MulticaseElim` → after `ElimDelay` (3 s) `ElimTimerFinished`
   → `GameMode->RequestRespawn` (:131): `Reset()+Destroy()` the pawn, `RestartPlayerAtPlayerStart` at a random `APlayerStart`.
4. Team modes: the new character's `SetSpawnPoint()` then teleports to a matching `ATeamPlayerStart`.

Leaving: `ServerLeaveGame` → `PlayerLeftGame` (:147) removes from `TopScoringPlayers` and calls `Elim(true)`;
the client's `OnLeftGame` delegate then destroys the session (see online-sessions.md).

## Teams (`ATeamsGameMode`, `bTeamMatch = true`)
- `PostLogin` and `HandleMatchHasStarted` assign `ET_NoTeam` players to the smaller team
  (`RedTeam`⇔Green, `BlueTeam`⇔Grey — see GLOSSARY).
- `CalculateDamage` returns 0 for friendly fire (self-damage still allowed).
- `PlayerEliminated` adds 1 to the attacker's team score via `GreenTeamScores()/GreyTeamScores()`.

## Capture the flag (`ACaptureTheFlagGameMode`)
- `PlayerEliminated` skips the team-kill score (calls `AShooterGameMode::PlayerEliminated` directly).
- `AFlagZone::OnSphereOverlap`: an `AFlag` whose team ≠ zone team → `FlagCaptured(Flag, Zone)` (zone's team scores) → `Flag->ResetFlag()`.
- Flag pickup is a normal weapon equip with `EWT_Flag`: carrier is forced to crouch, can't jump/aim/fire/swap/grenade
  (checks of `Combat->bHoldingTheFlag` in `AShooterCharacter` input handlers). Own-team flag can't be picked up
  (`AWeapon::OnSphereOverlap`).

## Lobby (`ALobbyGameMode`)
`BeginPlay` starts a `LobbyWaitTime` (30 s) timer; `PostLogin` starts early when
`PlayerArray.Num() >= Subsystem->DesiredNumPublicConnections`. `StartMatch` does seamless `ServerTravel` to
`/Game/Maps/ShooterMap`, `/Game/Maps/Teams` or `/Game/Maps/CaptureTheFlag` based on `DesiredMatchType`
(`"FreeForAll" | "Teams" | "CaptureTheFlag"`).

## Replicated state

| Class | Property | Notes |
|---|---|---|
| `AShooterGameState` | `TopScoringPlayers` | used for winner text |
| `AShooterGameState` | `GreenTeamScore`, `GreyTeamScore` (`OnRep_*` → HUD of first local PC) | floats |
| `AShooterPlayerState` | `Score` (engine, `OnRep_Score` → HUD), `Deaths` (`OnRep_Deaths`), `Team` (`OnRep_Team` → `SetTeamColor`) | |

`RedTeam` / `BlueTeam` arrays are **server-only** (not replicated).

## Gotchas / open issues
See `docs/KNOWN_ISSUES.md`: BUG-012 (respawn with no PlayerStart), BUG-013 (mixed clocks in Tick),
BUG-024 (CTF capture checks), BUG-031 (TopScore stale).

## How to extend
- **New game mode:** subclass `AShooterGameMode` (or `ATeamsGameMode`), override `PlayerEliminated` / `CalculateDamage`,
  create a BP, add a map, and add a branch in `ALobbyGameMode::StartMatch` + a match-type string in the menu.
- **New score type:** put replicated state on `AShooterGameState`/`AShooterPlayerState`, push to HUD via a new
  `AShooterPlayerController::SetHud*` setter (follow the existing "cache if overlay not ready" pattern).
