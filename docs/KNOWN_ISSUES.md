# Known issues ledger

Found by a full read of `Source/` and `Plugins/MultiplayerSessions/Source/` on 2026-11-18. Line numbers are as of
that read — **re-check with grep before editing** (they drift).

**How to use:** a task file (`agents/tasks/`) references issue IDs. When an issue is fixed, change its Status to
`fixed (T-xxxx)` — don't delete rows. New findings get the next free ID in their section.
Severity: **S1** crash / data corruption / game-breaking · **S2** wrong gameplay or visible bug · **S3** minor / polish.
"Verify" = plausible from code, needs in-editor confirmation.

## Bugs

| ID | Sev | Location | Problem | Suggested fix | Status |
|---|---|---|---|---|---|
| BUG-001 | S1 | `ShooterCharacter.cpp:1399` | `Destroyed()` calls `Super::Destroy()` instead of `Super::Destroyed()` — re-enters destroy, `AActor::Destroyed` (and `OnDestroyed` broadcast) never runs for characters | call `Super::Destroyed()` | fixed, unverified in editor (T-0001) |
| BUG-015 | S1 | `ShooterCharacter.cpp:876` (`Landed`) → `TeamsGameMode.cpp:56` | fall damage uses `nullptr` instigator; `ATeamsGameMode::CalculateDamage` dereferences `Attacker` → **crash in Teams/CTF on any damaging fall** | null-check `Attacker`/`Victim` in `CalculateDamage` (return `BaseDamage`) | fixed, unverified in editor (T-0001) |
| BUG-002 | S1 | `LagCompensationComponent.cpp:220,247,253,261` | shotgun SSR damage reads the **victim's** equipped weapon (and skips victims without one); DamageCauser is the victim's weapon | use `Character->GetEqiuppedWeapon()` (shooter) | fixed, unverified in editor (T-0002) |
| BUG-004 | S2 | `Weapon/ProjectileRocket.cpp:85` | `Destroyed()` is empty — no `Super::Destroyed()`; `OnHit` runs only on server so **clients never see rocket impact FX/sound** | call `Super::Destroyed()` (or multicast impact cosmetics) and avoid double FX on server | fixed, unverified in editor (T-0003) |
| BUG-007 | S2 | `Weapon/ProjectileWeapon.cpp:28` vs `Weapon/ProjectileBullet.cpp:60,77` | projectile Owner is the character, but `OnHit` casts `GetOwner()` to `AWeapon` → damage fall-off never applied; server-local SSR branch never sets `TraceStart` | pass weapon via a member/instigator chain; set `TraceStart` in all spawn branches | fixed, unverified in editor (T-0003) |
| BUG-008 | S2 | `PlayerController/ShooterPlayerController.cpp:61` | `GetPingInMilliseconds() * 4` — UE5 already returns ms, so threshold is effectively 30 ms → SSR disabled for most players | drop the `* 4` | fixed, unverified in editor (T-0005) |
| BUG-009 | S2 | `Weapon.cpp:436,501,532` | `!HighPingDelegate.IsBound()` guard → only the first weapon ever binds; swapped/picked-up weapons never get ping updates | use `IsAlreadyBound(this, &AWeapon::OnPingTooHigh)` / `AddUniqueDynamic` | fixed, unverified in editor (T-0004) |
| BUG-010 | S2 | `ShooterPlayerController.cpp:132-170` | `HandleCooldown` runs on every server-side PC and each loops all PCs → N² RPCs; winner text computed from the sending PC's perspective (wrong "You Are The Winner") | send from GameMode once per PC, compute text per recipient (or on client) | fixed, unverified in editor (T-0005) |
| BUG-013 | S2 | `ShooterGameMode.cpp:69,77` | warm-up uses `GetTimeSeconds`, in-progress/cooldown use `GetRealTimeSeconds`; clients use `GetTimeSeconds` → timer drift/desync | use `GetTimeSeconds` everywhere | fixed, unverified in editor (T-0005) |
| BUG-014 | S2 | `TeamsGameMode.cpp:39` | `Logout` override never calls `Super::Logout` | add `Super::Logout(Exiting)` | fixed, unverified in editor (T-0001) |
| BUG-021 | S2 | `Plugins/.../MultiplayerSessionsSubsystem.cpp:39-46` | when a session exists, `DestroySession()` is called but the function **continues** to `CreateSession` → failed/double create | `return;` after `DestroySession()` | fixed, unverified in editor (T-0006) |
| BUG-023 | S2 | `Weapon.cpp:543` | `DestroyWeapon` body commented out → dropped weapons never despawn (actor count grows over a match) | restore destroy-if-still-dropped | fixed, unverified in editor (T-0004) |
| BUG-032 | S2 verify | `CombatComponent.cpp:446,455` vs `ShooterCharacter.cpp:209,1735` | two walk-speed sources: Combat `BaseWalkSpeed/AimWalkSpeed` (BP) vs Character `WalkSpeed/SprintSpeed`; releasing aim restores `BaseWalkSpeed`, not `WalkSpeed` | single source of truth for speed | open |
| BUG-003 | S3 | `LagCompensationComponent.cpp:239` | shotgun SSR distance averages `HitLocations` (endpoints at `TRACE_LENGHT`) → always max fall-off | use confirmed impact points | fixed, unverified in editor (T-0002) |
| BUG-005 | S3 | `CombatComponent.cpp:756,765,831` | inverted lazy-init `Controller = Controller ? Cast(...) : Controller` → grenade/shotgun-shell HUD not updated while `Controller` is null | `Controller == nullptr ? Cast(...) : Controller` | open |
| BUG-006 | S3 | `Weapon/shotgun.cpp:104` | pellet hit-sound pitch `FRandRange(-.5f,.5f)` (≤0 pitch) | e.g. `FRandRange(0.8f,1.2f)` | fixed, unverified in editor (T-0003) |
| BUG-011 | S3 | `ShooterPlayerController.cpp:654` | `ShooterGameMode->LevelStartingTime` dereferenced before null check (crash if PC is used under a non-Shooter GameMode) | null-check | fixed, unverified in editor (T-0005) |
| BUG-012 | S3 | `ShooterGameMode.cpp:143` | `RequestRespawn` indexes `StartActors[RandRange(0,-1)]` when map has no PlayerStart | guard `Num()>0` | fixed, unverified in editor (T-0005) |
| BUG-016 | S3 | `ShooterCharacter.cpp:1376` | `SetTeamColor` guard uses `&&` (should be `||`) | fix condition | fixed, unverified in editor (T-0001) |
| BUG-017 | S3 | `ShooterCharacter.cpp:~248-252` | `BeginPlay` checks `ElimBotSound` but plays `ElimBotSoundSpawn` | check the sound it plays | fixed, unverified in editor (T-0001) |
| BUG-018 | S3 | `ShooterCharacter.cpp:333-335` | Tick fallback `if (!Combat) { Combat = ...; Combat->Character = this; }` dereferences null | remove (Combat is a default subobject) | fixed, unverified in editor (T-0001) |
| BUG-019 | S3 | `ShooterCharacter.cpp:1147` | `SpawnDefaultWeapon` doesn't null-check the spawned weapon | guard | fixed, unverified in editor (T-0001) |
| BUG-020 | S3 | `ShooterHud.cpp:69,83` | kill-feed `ElimMessages` never pruned → grows, holds removed widgets | `ElimMessages.Remove(Msg)` in timer callback | open |
| BUG-022 | S3 | `Plugins/.../Menu.cpp:130` | `OnJoinSession` ignores `Result`, travels even on failure; Join button not re-enabled | handle non-Success | fixed, unverified in editor (T-0006) |
| BUG-024 | S3 | `CaptureTheFlagGameMode.cpp:17`, `CaptureTheFlag/FlagZone.cpp:21` | `bValidCapture` unused; no null checks; overlap not authority-gated | gate on `HasAuthority()`, use the flag | open |
| BUG-026 | S3 | `Config/DefaultInput.ini` | `Gamepad_FaceButton_Left` bound to both Equip and Reload | rebind one | open |
| BUG-027 | S3 | `HUD/ReturnToMainMenu.cpp:222` | `OnLeftGame.AddDynamic` after `ServerLeaveGame` and on every click (dup-bind ensure) | bind once, before the RPC | open |
| BUG-028 | S3 | `CombatComponent.cpp:419` (`LocalFire`) | AR shows a hit marker when crosshair is red **and** `AHitScanWeapon::Fire` shows one → double marker | keep one | open |
| BUG-029 | S3 | `Weapon/shotgun.cpp` (`FireShotgun`) | hit markers use `PlayerCharacter` before it is assigned | assign first | fixed, unverified in editor (T-0003) |
| BUG-030 | S3 | `CombatComponent.cpp:155,1064` | `InterpFov` / `OnRep_CarriedAmmo` dereference `Character` without null check | guard | open |
| BUG-031 | S3 | `GameState/ShooterGameState.cpp:17` | `TopScore` not recomputed when a top player leaves | recompute from `PlayerArray` | fixed, unverified in editor (T-0005) |

## Networking / anti-cheat (server trusts the client)

| ID | Sev | Location | Problem | Suggested fix | Status |
|---|---|---|---|---|---|
| NET-001 | S2 | `CombatComponent.cpp:401,410` | `ServerFire`/`ServerShotgunFire` don't check fire rate (`FireDelay` param unused), ammo, combat state, or target sanity | validate on server; consider `WithValidation` | open |
| NET-002 | S2 | `LagCompensationComponent.cpp:105,196,214` | score requests not validated (HitTime window vs `MaxRecordTime`, TraceStart near shooter, rate) | add checks | open |
| NET-003 | S3 | `ShooterCharacter.cpp:1759` | `ServerSetSprinting` trusts client | check `CanSprint()` on server | open |
| NET-004 | S3 | `ShooterPlayerController.cpp:121` | client decides its own high-ping status (toggles SSR) | measure ping server-side | open |
| NET-005 | S3 | `ShooterCharacter.h:77`, `.cpp:1625-1645` | damage indicator is a **reliable multicast to everyone** on every hit; only the victim uses it | Client RPC on the victim's controller, unreliable | open |
| NET-006 | S3 | `CombatComponent.h` (`MulticastFire`, `MulticastShotgunFire`) | high-frequency cosmetic multicasts are Reliable | Unreliable | open |
| NET-007 | S3 verify | `ShooterCharacter.cpp:1646` | `SetIsOnLadder` changes movement mode from BP without server/client agreement → CMC corrections | verify BP_Ladder calls it on both sides; else RPC | open |

## Performance

| ID | Location | Problem | Suggested fix | Status |
|---|---|---|---|---|
| PERF-001 | `ShooterCharacter.cpp:578,594` | `GConfig->GetFloat` for sensitivity on every Turn/LookUp axis call (2×/frame) | cache; refresh when the menu changes it | open |
| PERF-002 | `ShooterCharacter.cpp:357` (`HideCameraWhenCharacterClose`) | every Tick: `GetComponents` arrays + visibility/owner-no-see churn | run only on mode/threshold change | open |
| PERF-003 | `ShooterCharacter.cpp:331` (`UpdateHudAmmo`) | sets two HUD texts (string+FText alloc) every frame | event-driven (already pushed by weapon/combat) | open |
| PERF-004 | `ShooterPlayerController.cpp:42` | `HideTeamScores()` every Tick in FFA | call once | open |
| PERF-005 | `CombatComponent.cpp:88` | `ReloadEmptyWeapon` every Tick | trigger on ammo change | open |
| PERF-006 | `LagCompensationComponent.cpp:26-77` | ticks on clients too; allocates a TMap + list node per character per server tick | disable tick off-server; ring buffer / fixed array | open |
| PERF-007 | `AWeapon`, `AProjectile`, `APickUp`, `APickUpSpawnPoint`, `UBuffComponent` | `bCanEverTick = true` with empty Tick | disable tick | open |
| PERF-008 | `Weapon.cpp` (ammo logs), `ShooterCharacter.cpp` (`RecieveDamage`, fall damage), `LagCompensationComponent.cpp` ("got in function"), `HitScanWeapon.cpp` ("Hit Rewing") | `UE_LOG Warning` in hot paths | remove or `Verbose` under a project log category | open |

## Tech debt / dead code

| ID | Item |
|---|---|
| DEBT-001 | Legacy input (`BindAction/BindAxis`) — UE5 standard is Enhanced Input. Migration touches BP + ini. |
| DEBT-002 | Raw `UObject*` UPROPERTYs everywhere; UE5 prefers `TObjectPtr<>`. `FHUDPackage` texture pointers are not UPROPERTY. |
| DEBT-003 | `UBuffComponent` empty; `UCombatComponent::ShowGrenadeTrajectory` empty and trajectory spline/mesh/decal properties unused; `UMultiplayerSessionsSubsystem::StartSession` empty; `UOverheadWidget::ShowPlayerNetRole` stub. |
| DEBT-004 | Duplicate getters `GetCombatComponent`/`getCombatComponent`; `SwapToPrimaryWeapon` and `SwapToSecondaryWeapon` are identical toggles. |
| DEBT-005 | Duplicated `ZoomedFov`/`ZoomInterpSpeed` on both `UCombatComponent` and `AWeapon`. |
| DEBT-006 | Misspelled identifiers (see GLOSSARY) — renames require Blueprint redirects (`CoreRedirects`) and must be a dedicated task. |
| DEBT-007 | `AShooterCharacter` is ~1800 lines doing camera, input, health, sprint, ladder, elim, team color — candidate to split into components. |
| DEBT-008 | Root-level `BEFORE_AFTER_CODE_COMPARISON.md`, `HEALING_TIME_OPTIMIZATION.md`, `WEAPON_AMMO_*.md` describe past edits with stale numbers/lines; `Source/Shooter/README.md` only says "UrbanAssualt". |
| DEBT-009 | `GlobalDefaultGameMode` still the ThirdPerson template BP. |
