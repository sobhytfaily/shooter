# T-0005: Match flow & timers

| Field | Value |
|---|---|
| Status | done (build passed 2026-11-19; review follow-ups applied by orchestrator; unverified in editor) |
| Assignee | ue-worker (sonnet) |
| Reviewer | ue-reviewer (sonnet) |
| Issues | BUG-008, BUG-010, BUG-011, BUG-012, BUG-013, BUG-031 |
| Build | orchestrator |
| Created | 2026-11-19 by orchestrator |
| Branch | `fix/wave1-T0004-T0006` — runs **in parallel** with T-0004 and T-0006 |

## Goal
Correct high-ping detection, a correct and single winner announcement per player, a consistent match clock, and no
crashes in respawn / HUD time when assumptions fail.

## Read first
`CLAUDE.md`, `docs/GLOSSARY.md`, `docs/systems/game-flow.md`, `docs/systems/controller-and-hud.md`,
`docs/KNOWN_ISSUES.md` rows BUG-008, BUG-010, BUG-011, BUG-012, BUG-013, BUG-031.

## Files (write) — binding lock
- `Source/Shooter/ShooterGameMode.cpp`
- `Source/Shooter/PlayerController/ShooterPlayerController.cpp` (and `.h` only if strictly needed)
- `Source/Shooter/GameState/ShooterGameState.h/.cpp`
- `docs/systems/game-flow.md`, `docs/systems/controller-and-hud.md` (only lines that become untrue)
- **Not** `docs/KNOWN_ISSUES.md` / `docs/agents/BOARD.md` — list status changes in your report.

## Required changes
1. **BUG-008** `AShooterPlayerController::CheckPing`: compare `GetPingInMilliseconds()` (no `* 4`) to `HighPingThreshold`.
2. **BUG-010** `HandleCooldown`: `AShooterGameMode::OnMatchStateSet` already calls it on every server-side PC, so the
   authority block must send `ClientShowCooldownAnnouncement` **only to this PC** (no loop over all PCs), with the info
   text computed for this PC (`GetInfoText` uses this PC's PlayerState — correct once it's per-recipient).
   `GetTeamsInfoText` must not depend on `ShooterGameMode` being cached (use the passed GameState; null-check it).
3. **BUG-013** `AShooterGameMode::Tick`: use `GetWorld()->GetTimeSeconds()` in all three states (clients use it too).
4. **BUG-011** `SetHudTime`: null-check the GameMode before reading `LevelStartingTime`.
5. **BUG-012** `AShooterGameMode::RequestRespawn`: if there are no `APlayerStart`s, fall back to `RestartPlayer(Controller)`
   instead of indexing an empty array.
6. **BUG-031** `AShooterGameState`: add a server function that recomputes `TopScore` and `TopScoringPlayers` from
   `PlayerArray`, excluding a given leaving player; call it from `AShooterGameMode::PlayerLeftGame` (replace the manual
   `Remove`). Keep `UpdateTopScore` behavior for kills.

## Constraints
- RPC signatures unchanged. Don't rename BP-visible things. No new logs; no new Tick work.
- Teams arrays (`RedTeam/BlueTeam`) untouched.

## Acceptance criteria
- [ ] Each player gets exactly one cooldown announcement, with "You Are The Winner" only for the winner.
- [ ] Ping warning only above `HighPingThreshold` ms.
- [ ] Countdown on host and client agree.
- [ ] No crash with zero PlayerStarts; top score correct after the leader leaves.
- [ ] Report lists KNOWN_ISSUES rows to flip.

## Playtest checklist (for the user)
1. FFA, 2–3 players, short `MatchTime` in BP_ShooterGameMode. Let one player win: only they see "You Are The Winner";
   the others see "Winner: <name>". Teams map: team result text correct.
2. Compare the countdown on host and client — same seconds.
3. With `Net PktLag=50` (≈100 ms RTT) no high-ping icon; with `Net PktLag=150` it appears after ≤20 s.
4. Leader quits mid-match (Esc → leave) → at cooldown the next-best player is the winner.

---
## Worker report
Build: not run.

Changes:
- `ShooterPlayerController.cpp`: `CheckPing` (removed `* 4`); `HandleCooldown` (sends `ClientShowCooldownAnnouncement` only to self, no PC loop); `GetTeamsInfoText` (null-checks passed GameState instead of cached GameMode); `SetHudTime` (null-check GameMode before reading `LevelStartingTime`).
- `ShooterGameMode.cpp`: `Tick` (InProgress/Cooldown use `GetTimeSeconds()`); `RequestRespawn` (falls back to `RestartPlayer` when no PlayerStarts); `PlayerLeftGame` (calls `RecomputeTopScore(LeavingPlayer)`).
- `ShooterGameState.h/.cpp`: new `RecomputeTopScore(AShooterPlayerState* ExcludedPlayer = nullptr)` (authority only; rebuilds from `PlayerArray`; empty list if top score <= 0, matching kill-driven behavior).
- Docs: `game-flow.md`, `controller-and-hud.md` updated.

KNOWN_ISSUES rows to flip to `fixed, unverified in editor (T-0005)`: BUG-008, BUG-010, BUG-011, BUG-012, BUG-013, BUG-031.

Unverified: everything (no build, no PIE). Note: PlayerLeftGame runs before the leaver's PlayerState is removed from PlayerArray, hence the exclusion parameter.

---
## Review

**Verdict: changes requested** (reviewed by ue-reviewer; read-only, no build/PIE)

All six listed edits are present and only write-set files changed. BUG-008, BUG-011, BUG-012, BUG-013 look correct. Findings:

1. ShooterPlayerController.cpp:140-154 (HandleCooldown) / :739 (HandleMatchHasStarted) — team text path: `bShowTeamScores` is set only in `HandleMatchHasStarted` on the server PC, which `OnMatchStateSet(InProgress)` reaches only for PCs that exist when InProgress starts. A player who joins mid-InProgress of a team match has `bShowTeamScores == false` server-side, so the now per-PC `HandleCooldown` sends the FFA winner text (`GetInfoText(TopScoringPlayers)`) instead of the team result. Before the change the last PC in the loop decided the text for everybody, so this is a behaviour change. Suggested change: choose the branch from the game mode (`ShooterGameMode->bTeamMatch`, or the `bTeamsMatch` arg that `OnMatchStateSet` already receives and could pass to `HandleCooldown`) rather than the PC flag.
2. ShooterPlayerController.cpp:140 / :796-806 (HandleCooldown, ClientJoinMidGame, OnRep_MatchState) — a player who joins during Cooldown gets no announcement. `HandleCooldown` only sends when `HasAuthority()`. The mid-join paths (`ClientJoinMidGame` → `OnMatchStateSet`, and `OnRep_MatchState`) run on the client, so they send nothing. The server-side `OnMatchStateSet` has already fired for the existing PCs. The old code behaved the same, so this is not a regression, but the acceptance criterion "each player gets exactly one" is not met for this case. Suggested change: in `ServerCheckMatchState_Implementation`, if the state is `Cooldown`, run the authority part of `HandleCooldown` for that PC (it has the correct PlayerState by then). Keep the client-side `OnMatchStateSet` call from sending anything, so there is no double send.
3. ShooterPlayerController.cpp:655 (SetHudTime, authority branch) — timer consistency: `GetCountdownTime()` already includes `+ LevelStartingTime` (GameMode Tick), and `SecondsLeft` adds `LevelStartingTime` again. The host's redraw gate `CountDownInt != SecondsLeft` is therefore offset by `LevelStartingTime` seconds from the displayed `TimeLeft` and from the client gate. This is harmless when `LevelStartingTime` is ~0, but the host's HUD can update up to one second out of phase with the clients if it is larger. Suggested change: use `FMath::CeilToInt(ShooterGameMode->GetCountdownTime())`, or drop the special case so host and client both use `TimeLeft`. Unverified in PIE.
4. ShooterGameState.cpp:38-65 / ShooterGameMode.cpp:154 (RecomputeTopScore, PlayerLeftGame) — correct for the explicit-leave path: authority-guarded, excludes the leaver, ties and zero-score cases are handled, and it matches `UpdateTopScore`. Remaining gap: a player who disconnects without `ServerLeaveGame` (crash, alt-F4) is never removed from `TopScoringPlayers`. The array entry goes null after GC, and `GetInfoText` dereferences `Players[0]->GetPlayerName()` / `TiedPlayer->...` without a null check, which could crash at cooldown. Suggested change: also call `RecomputeTopScore(Exiting PS)` from `AShooterGameMode::Logout`, and/or skip null entries in `GetInfoText`. Not strictly required by BUG-031; unverified.
5. ShooterGameMode.cpp:139-151 (RequestRespawn) — the zero-PlayerStart fallback is correct; `RestartPlayer` picks a start via `FindPlayerStart` as normal. No change needed. Note: the `ElimmedController` null case is still unguarded, as before. Only matters if `RequestRespawn` can be called with a null controller.

Confirmed OK: CheckPing (no `* 4`); `GetTeamsInfoText` null-checks the passed GameState; `HandleCooldown` sends exactly once to every PC that exists at the transition. The listen-server host is covered by the same `OnMatchStateSet` loop, and the `OnRep_MatchState` path on remote clients does not send. Tick uses `GetTimeSeconds()` consistently with the client's `GetServerTime()`. RPC signatures and Blueprint-visible members are unchanged. The docs updates match the code. KNOWN_ISSUES flips are acceptable only once findings 1-2 are fixed or deliberately deferred; BUG-010 is the one at risk.
