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
| T-0001 | Character & Teams crash/null fixes | 1 | `ShooterCharacter.cpp`, `TeamsGameMode.cpp` | BUG-001, BUG-015, BUG-014, BUG-016, BUG-017, BUG-018, BUG-019 |
| T-0002 | Shotgun server-side-rewind damage | 1 | `ShooterComponent/LagCompensationComponent.cpp` | BUG-002, BUG-003 |
| T-0003 | Projectile fixes (rocket FX on clients, bullet fall-off, shotgun pitch) | 1 | `Weapon/Projectile*.cpp`, `Weapon/ProjectileWeapon.cpp`, `Weapon/shotgun.cpp` | BUG-004, BUG-006, BUG-007, BUG-029 |
| T-0004 | Weapon lifecycle (dropped despawn, ping delegate per weapon) | 1 | `Weapon.cpp` | BUG-023, BUG-009 |
| T-0005 | Match flow & timers | 1 | `ShooterGameMode.cpp`, `PlayerController/ShooterPlayerController.*`, `GameState/ShooterGameState.cpp` | BUG-008, BUG-010, BUG-011, BUG-012, BUG-013, BUG-031 |
| T-0006 | Sessions plugin robustness | 1 | `Plugins/MultiplayerSessions/Source/**` | BUG-021, BUG-022 |
| T-0007 | Character per-frame cost | 2 (after T-0001) | `ShooterCharacter.*` | PERF-001, PERF-002, PERF-003 |
| T-0008 | Server-side validation of client RPCs | 2 (after T-0001/T-0002) | `CombatComponent.*`, `LagCompensationComponent.*`, `ShooterCharacter.cpp` | NET-001, NET-002, NET-003 |
| T-0009 | Combat HUD/null fixes + tick trims | 2 | `CombatComponent.cpp`, `ShooterHud.cpp`, empty-tick classes | BUG-005, BUG-020, BUG-028, BUG-030, PERF-005, PERF-007 |

## Done

| ID | Title | Closed | Notes |
|---|---|---|---|
| — | — | — | — |
