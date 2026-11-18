# T-0002: Shotgun server-side-rewind damage

| Field | Value |
|---|---|
| Status | done (build passed 2026-11-18; review findings 1–2 applied; unverified in editor) |
| Assignee | ue-worker (sonnet) |
| Reviewer | ue-reviewer (sonnet) |
| Issues | BUG-002, BUG-003 |
| Build | orchestrator |
| Created | 2026-11-18 by orchestrator |
| Branch | `fix/T-0002-shotgun-ssr` (stacked on `fix/T-0001-crash-fixes`) |

## Goal
When a remote client's shotgun hit is confirmed by server-side rewind, damage must come from the **shooter's**
shotgun, and distance fall-off must use the **confirmed impact distance** of the pellets that hit each character.

## Read first
- `CLAUDE.md`, `docs/GLOSSARY.md`
- `docs/systems/lag-compensation.md` (whole file — short)
- `docs/systems/weapons.md` (sections "Damage fall-off", "Ashotgun")
- `docs/KNOWN_ISSUES.md` rows BUG-002, BUG-003

## Files (read)
- `Source/Shooter/Weapon.h` (`CalculateDamageWithFalloff`, `GetDamage`, `GetHeadShotDamage`)
- `Source/Shooter/Weapon/shotgun.cpp` (`FireShotgun` — the non-rewind reference implementation)
- `Source/Shooter/ShooterCharacter.h`

## Files (write) — binding lock
- `Source/Shooter/ShooterComponent/LagCompensationComponent.cpp`
- `Source/Shooter/ShooterComponent/LagCompensationComponent.h` (only the `FShotgunServerSideRewindResult` struct)
- `docs/KNOWN_ISSUES.md` (Status column of BUG-002, BUG-003 only)
- `docs/systems/lag-compensation.md` (only lines that become untrue)

## Required changes
1. **BUG-002** `ShotgunServerScoreRequest_Implementation`: take the shooter's weapon once
   (`Character->GetEqiuppedWeapon()`), return early if `Character` or that weapon is null, and use it for
   `GetHeadShotDamage()`, `GetDamage()`, `CalculateDamageWithFalloff`, and as the DamageCauser in `ApplyDamage`.
   Stop skipping victims that have no weapon. Skip calling `ApplyDamage` when a character's total damage is 0.
2. **BUG-003** `ShotgunConfirmHit` + `FShotgunServerSideRewindResult`: record the distance from `TraceStart` to
   `ConfirmHitResult.ImpactPoint` for each confirmed pellet (e.g. a `TMap<AShooterCharacter*, float>` sum of distances,
   as a `UPROPERTY()` in the struct). In `ShotgunServerScoreRequest_Implementation`, average distance per character =
   that sum / (head hits + body hits). Remove the loop over `HitLocations`.
3. In `ShotgunConfirmHit`, a pellet already confirmed as a headshot in the head pass must not also be counted in the
   body pass (track confirmed pellet indices and skip them in the second loop).

## Constraints
- Keep all RPC signatures, struct names and existing field names unchanged (adding a field is OK).
- Don't change the non-shotgun paths (`ServerScoreRequest`, `ProjectileServerScoreRequest`) or frame history.
- No new logs, no refactor of the unrelated "return empty result if any frame has no character" behavior.

## Acceptance criteria
- [ ] Damage, headshot damage, fall-off and DamageCauser all come from the shooter's equipped weapon.
- [ ] Fall-off distance is the mean confirmed impact distance per character.
- [ ] A pellet counts at most once (head or body).
- [ ] Compiles (`ShooterEditor Win64 Development`) — orchestrator builds.
- [ ] KNOWN_ISSUES rows set to `fixed, unverified in editor (T-0002)`; lag-compensation.md open-issues line updated.

## Playtest checklist (for the user)
1. PIE, 2 players, Net Mode "Play As Listen Server"; in the **client** window console run `Net PktLag=100`.
2. Client equips the shotgun; the host has a different weapon (or none). Shoot the host at close range: damage should
   match the shotgun's per-pellet damage × pellets that hit (compare with the host shooting the client).
3. If the shotgun BP has `bUseDamageFalloff` on: hits at long range do less damage than point-blank, but not always
   the minimum.

---
## Worker report
<!-- appended by ue-worker -->

**Status:** review. **Build:** not run.

Changes (`Source/Shooter/ShooterComponent/`):
- `LagCompensationComponent.h`: `FShotgunServerSideRewindResult` gained `TMap<AShooterCharacter*, float> HitDistanceSums` (UPROPERTY).
- `LagCompensationComponent.cpp`:
  - `ShotgunServerScoreRequest_Implementation`: shooter weapon fetched once, early return if `Character`/weapon null; used for headshot damage, damage, fall-off and DamageCauser; victims without a weapon no longer skipped; `ApplyDamage` skipped when total damage <= 0; average distance = `HitDistanceSums / (head+body hits)`; `HitLocations` loop removed.
  - `ShotgunConfirmHit`: both loops now index-based; each confirmed pellet adds `|ImpactPoint - TraceStart|` to `HitDistanceSums`; `TSet<int32> ConfirmedPellets` makes head-confirmed pellets skipped in the body pass.
- Docs: KNOWN_ISSUES BUG-002/003 -> `fixed, unverified in editor (T-0002)`; lag-compensation.md open-issues line updated.

Unverified: compile and PIE (see playtest checklist). Questions: none.

---
## Review
<!-- appended by ue-reviewer -->

**Verdict:** approve (compile/PIE unverified; orchestrator builds)

Acceptance: damage/headshot/fall-off/DamageCauser all from `ShooterWeapon` (traced); mean confirmed impact distance per character via `HitDistanceSums / (head+body)`; head-confirmed pellets skipped in body pass via `ConfirmedPellets`; KNOWN_ISSUES and lag-compensation.md updated. Write-set respected (BOARD.md change is the orchestrator's). No RPC/BP-visible signature changes, no new logs/Tick work; server-only path, no new client trust.

Findings (all minor, non-blocking):
1. LagCompensationComponent.cpp:~216-219 (ShotgunServerScoreRequest_Implementation) — the null check on `Character`/shooter weapon runs after `ShotgunServerSideRewind`, so the rewind (box moves, traces) is done and discarded when the shooter has no weapon — move the early return above the rewind call.
2. LagCompensationComponent.h:~64-66 (FShotgunServerSideRewindResult) — the new comment/UPROPERTY/member are indented one tab deeper than the neighbouring members — align indentation.
3. LagCompensationComponent.cpp ShotgunConfirmHit (pre-existing, unchanged) — `TMap::operator[]` on `FName("head")` and the early "any frame has no character" return are untouched per constraints; noted only, unverified at runtime.
