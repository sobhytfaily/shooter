# Architecture overview

UE **5.7** C++ multiplayer third/first-person shooter (project codename *UrbanAssualt*). Listen-server over Steam
(SteamSockets, IpNetDriver fallback). ~11k lines of C++ in one game module (`Source/Shooter`) plus one plugin
(`Plugins/MultiplayerSessions`). Modes: Free-for-all, Teams, Capture-the-Flag.

## Module map

```
Shooter.uproject
├─ Source/Shooter/                      game module "Shooter" (SHOOTER_API)
│  ├─ ShooterCharacter.*                pawn: input, camera, health, sprint, ladder, elim   → systems/character.md
│  ├─ ShooterComponent/
│  │  ├─ CombatComponent.*              weapons in hand, fire/reload/swap/grenades, ammo     → systems/combat.md
│  │  ├─ LagCompensationComponent.*     server-side rewind                                   → systems/lag-compensation.md
│  │  └─ BuffComponent.*                empty placeholder                                    → systems/pickups-animation-misc.md
│  ├─ Weapon.*, Flag.*, Weapon/*        weapons, projectiles, casings                        → systems/weapons.md
│  ├─ ShooterGameMode.*, TeamsGameMode.*, CaptureTheFlagGameMode.*, LobbyGameMode.*,
│  │  GameState/*, PlayerState/*, PlayerStart/*, CaptureTheFlag/*, Announcement.h          → systems/game-flow.md
│  ├─ PlayerController/*, ShooterHud.*, HUD/*, OverheadWidget.*                           → systems/controller-and-hud.md
│  ├─ PickUps/*, ShooterAnimInstance.*, Interfaces/*                                       → systems/pickups-animation-misc.md
│  └─ CombatState.h, Team.h, TurningInPlace.h, Weapon/WeaponTypes.h, Shooter.h             → GLOSSARY.md
├─ Plugins/MultiplayerSessions/         session subsystem + host/join menu                   → systems/online-and-config.md
├─ Config/                              Steam, input mappings, maps                          → systems/online-and-config.md
└─ Content/BluePrints/                  BP subclasses of every gameplay class (data lives here)
```

## Runtime object graph

```
UGameInstance ── UMultiplayerSessionsSubsystem (session create/find/join/destroy, desired match type)

Server only                      Replicated to all                       Owning client only
─────────────                    ─────────────────                       ──────────────────
AShooterGameMode ───────────────▶ AShooterGameState (top scorers, team scores)
 ├ ATeamsGameMode                 AShooterPlayerState ×N (score, deaths, team)
 │  └ ACaptureTheFlagGameMode     AShooterCharacter ×N ◀── possessed by ── AShooterPlayerController
 └ (ALobbyGameMode in lobby)       ├ UCombatComponent (weapons, state,     ├ AShooterHud ── UCharacterOverlay,
                                   │   grenades; CarriedAmmo owner-only)   │                 UAnnouncement, kill feed
                                   ├ ULagCompensationComponent (server      ├ UScoreboardWidget, UReturnToMainMenu
                                   │   frame history)                       └ time sync, ping, match-state mirror
                                   └ AWeapon ×≤2 (Ammo owner-only)
                                  AProjectile*, APickUp*, AFlag, AFlagZone
```
Ownership chain used everywhere: `AWeapon::Owner = AShooterCharacter`, `Character->Controller = AShooterPlayerController`,
weapons/components push HUD updates through `AShooterPlayerController::SetHud*`.

## Networking model (read before touching gameplay)
- **Server-authoritative** for health, ammo totals, combat state, scores, spawning. Damage is applied only on the
  server via `UGameplayStatics::ApplyDamage` → `AShooterCharacter::RecieveDamage` → `GameMode::CalculateDamage`.
- **Client prediction** for: firing (local `LocalFire` before `ServerFire`), magazine ammo (`Sequence` reconciliation in
  `AWeapon`), reload/swap montages (`bLocallyReloading`, `bFinishedSwaping`), sprint.
- **Server-side rewind** for hit registration when `AWeapon::bUseServerSideRewind` is true and ping is OK
  (lag-compensation.md). Otherwise the server's own trace decides.
- RPC naming: `Server*` (client→server, Reliable), `Multicast*` (server→all), `Client*` (server→owning client),
  `OnRep_*` for RepNotify. Most OnReps re-run the same "apply state" function the server called directly.
- `HasAuthority()` = server copy; `IsLocallyControlled()` = the player's own pawn (true on listen-server host for its pawn).

## Frame / event flows (cross-system)
1. **Shot (hitscan, SSR off):** input → Combat::Fire → client LocalFire (cosmetic) + ServerFire → MulticastFire →
   server LocalFire → `AHitScanWeapon::Fire` trace → ApplyDamage → `RecieveDamage` → health OnRep → HUD.
2. **Shot (SSR on, remote client):** client `AHitScanWeapon::Fire` → `ServerScoreRequest(HitTime)` → server rewind →
   ApplyDamage.
3. **Kill:** `RecieveDamage` → `GameMode::PlayerEliminated` → PlayerState score/deaths (OnRep → HUD) →
   `Character::Elim` → multicast death FX → 3 s → `RequestRespawn` → new pawn → `PollInit` → HUD refresh.
4. **Match:** Lobby (timer/full) → ServerTravel → GameMode warm-up → InProgress → Cooldown (winner screen) → RestartGame.
5. **Menu/Session:** StartGameMap UMenu → Create/Find/Join session → Lobby → match map; leave via Esc menu →
   ServerLeaveGame → DestroySession → main menu.

## Where data lives
Numbers (damage, fire delay, mag size, speeds, montages, sounds, FX) are mostly **set in Blueprints**
(`Content/BluePrints/...`), with C++ defaults as fallbacks. A C++ default change may have **no effect** if the BP
overrides it — check the BP (or tell the user to) before claiming a tuning change worked.

## Conventions observed in the code
- Lazy pointer caching: `X = X == nullptr ? Cast<...>(...) : X;` (watch for the inverted form — BUG-005).
- Unreal prefixes (`A`, `U`, `F`, `E`, `I`), UPROPERTY on most pointers, `FORCEINLINE` getters at class bottom.
- Includes use module-relative paths like `"Shooter/PlayerController/ShooterPlayerController.h"`.
- Comments are sparse; prior AI tools added explanatory comments and verbose `UE_LOG`s in places.

## Health of the codebase
See `KNOWN_ISSUES.md` — 2 crash-class bugs (BUG-001, BUG-015), 1 high-impact damage bug (BUG-002), several
networking trust gaps, and per-frame work in `AShooterCharacter::Tick` that is the main CPU hotspot candidate.
