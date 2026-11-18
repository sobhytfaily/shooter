# Task board

Owned by the orchestrator. One row per task. Before spawning a worker, check that its **write-set** doesn't overlap
any `in-progress` row. Status values: proposed / todo / in-progress / blocked / review / done.

## Active

| ID | Title | Status | Assignee | Write-set | Issues |
|---|---|---|---|---|---|
| — | — | — | — | — | — |

## Proposed backlog (from the 2026-11-18 code read — not started, needs user approval)
Grouped so that wave 1 tasks have **disjoint write-sets** and can run in parallel.

| ID | Title | Wave | Write-set | Issues |
|---|---|---|---|---|
| T-0004 | Weapon lifecycle (dropped despawn, ping delegate per weapon) | 1 | `Weapon.cpp` | BUG-023, BUG-009 |
| T-0005 | Match flow & timers | 1 | `ShooterGameMode.cpp`, `PlayerController/ShooterPlayerController.*`, `GameState/ShooterGameState.cpp` | BUG-008, BUG-010, BUG-011, BUG-012, BUG-013, BUG-031 |
| T-0006 | Sessions plugin robustness | 1 | `Plugins/MultiplayerSessions/Source/**` | BUG-021, BUG-022 |
| T-0007 | Character per-frame cost | 2 (after T-0001) | `ShooterCharacter.*` | PERF-001, PERF-002, PERF-003 |
| T-0008 | Server-side validation of client RPCs | 2 (after T-0001/T-0002) | `CombatComponent.*`, `LagCompensationComponent.*`, `ShooterCharacter.cpp` | NET-001, NET-002, NET-003 |
| T-0009 | Combat HUD/null fixes + tick trims | 2 | `CombatComponent.cpp`, `ShooterHud.cpp`, empty-tick classes | BUG-005, BUG-020, BUG-028, BUG-030, PERF-005, PERF-007 |

## Done

| ID | Title | Closed | Notes |
|---|---|---|---|
| T-0001 | Character & Teams crash/null fixes | 2026-11-18 | review approved, build passed; unverified in editor (playtest checklist in task file) |
| T-0003 | Projectile fixes (rocket FX on clients, bullet fall-off, shotgun pitch/markers) | 2026-11-19 | review approved; review findings 2–3 (client instigator re-enable, Destroyed FX fallback) applied by orchestrator; build passed; unverified in editor — watch for double rocket explosion if the launcher uses SSR |
| (direct) | FPS→TPS head/legs visible instantly; grenades hold early wind-up pose, throw at 1.75× on release | 2026-11-19 | orchestrator edit (CombatComponent.*, ShooterCharacter.*), build passed; tune `GrenadeHoldPoseTime` / `GrenadeReleasePlayRate` in BP |
| T-0002 | Shotgun server-side-rewind damage | 2026-11-18 | review approved (2 minor findings applied by orchestrator), build passed; unverified in editor |
