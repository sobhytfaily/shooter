# System: Player controller, HUD & widgets

## Files

| File | Class | Role |
|---|---|---|
| `Source/Shooter/PlayerController/ShooterPlayerController.h/.cpp` | `AShooterPlayerController` | HUD setters, match-state client side, time sync, ping, menus, scoreboard, kill feed |
| `Source/Shooter/ShooterHud.h/.cpp` | `AShooterHud : AHUD` + `FHUDPackage` | crosshair drawing, owns overlay/announcement widgets, kill-feed stack |
| `Source/Shooter/HUD/CharacterOverlay.h` | `UCharacterOverlay` | main in-match widget (BindWidget fields) |
| `Source/Shooter/HUD/Announcement.h` | `UAnnouncement` | warm-up / cooldown screen |
| `Source/Shooter/HUD/ElimAnnouncement.h/.cpp` | `UElimAnnouncement` | one kill-feed line |
| `Source/Shooter/HUD/ScoreboardWidget.h/.cpp`, `ScoreboardRow.*` | `UScoreboardWidget`, `UScoreboardRow` | Tab scoreboard (sorted by score, K/D) |
| `Source/Shooter/HUD/ReturnToMainMenu.h/.cpp` | `UReturnToMainMenu` | Esc/P menu: leave game, sensitivity, resolution, quality |
| `Source/Shooter/OverheadWidget.h/.cpp` | `UOverheadWidget` | player name over head |

Widget BPs: `Content/BluePrints/HUD/` — `WPB_CharacterOverlay`, `WBP_Announcement`, `WBP_ElimAnnouncement`,
`ScoreboardWidget`, `WBP_ScoreboardRow`, `WBP_ReturnToMainMenu`, `WBP_OverheadWidget`, `WPB_PickUpWidget`,
`WPB_SniperScope`, `WBP_HitMarker`, `WBP_HitMarkerHeadShot`, `WBP_Kill`, `WBP_HitIndicator`. `BP_ShooterHud`,
`PlayerController/BP_ShooterPlayerController`.

**BindWidget contract:** every `UPROPERTY(meta=(BindWidget))` name in `UCharacterOverlay`, `UAnnouncement`,
`UElimAnnouncement`, `UScoreboard*`, `UReturnToMainMenu` must exist with that exact name in the widget BP or the BP
fails to compile. Overlay fields: `LowHealth`, `ScoreAmount`, `GreenTeamScore`, `GreyTeamScore`, `DeathAmount`,
`ScoreSpacerText`, `WeaponAmmoAmount`, `CarriedWeaponAmmount`, `MatchCountDownText`, `GernadeText`,
`SmokeGernadeText`, `HighPingImage`, `DamageIndicator`, anim `HighPingAnimation`.

## HUD setter pattern
Every `SetHud*` on the controller (`SetHudScore`, `SetHudDeaths`, `SetHudWeaponAmmo`, `SetHudCarriedAmmo`,
`SetHudDamageOverlay`, `SetHudGernades`, `SetHudSmokeGernades`, team scores, countdowns) does:
lazy-cast `ShooterHUD` → if overlay + target widget valid, `SetText`; else cache the value and set a
`bIntialize*` flag. `PollInit` (:677) applies cached values once the overlay exists.
Health is shown only as a red `LowHealth` image opacity (`SetHudDamageOverlay` :411) — there is no health bar
(`SetHudHealth` is declared but unused).

## Match state on the client
`BeginPlay` → `ServerCheckMatchState` (:814) → `ClientJoinMidGame` (:800) with warm-up/match/cooldown lengths and
`LevelStartingTime`. `MatchState` also replicates (`OnRep_MatchState` :727).
- `InProgress` → `HandleMatchHasStarted` (:739): create overlay, hide announcement, show/hide team scores.
- `Cooldown` → `HandleCooldown` (:132): overlay removed, announcement shows winner text (`GetInfoText`,
  `GetTeamsInfoText`), gameplay disabled.
`SetHudTime` (:642) recomputes the countdown each tick from `GetServerTime()` (server uses the GameMode's value).

## Time sync & ping
`ReceivedPlayer` and every `TimeSyncFreq` (5 s): `ServerRequestServerTime` → `ClientReportServerTime` →
`SingleTripTime`, `ServerClientDelta`. `CheckPing` (:50) every 20 s: high ping → HUD warning animation +
`ServerReportPingStatus(true)` → server broadcasts `HighPingDelegate` → weapons disable SSR.

## Kill feed / announcements
Server `AShooterGameMode::PlayerEliminated` → each PC `BroadcastElim` → `ClientElimAnnoucement` (:256) chooses
"You"/"you"/"yourself"/"themselves" phrasing → `AShooterHud::AddElimAnnouncement` (stacks lines upward, removes each
after `ElimAnnouncementTime` 2.5 s, triggers kill marker when "You").

## Crosshair
`UCombatComponent::SetHudCrosshairs` fills `FHUDPackage` every tick → `AShooterHud::DrawHUD` (:111) draws 5 textures
offset by `CrosshairSpreadMax * CrosshairSpread`, red when the trace hits an `IInteractWithCrosshairInterface`.

## Return-to-main-menu widget
`ShowReturnToMainMenu` toggles `UReturnToMainMenu::MenuSetup/MenuTearDown`. Leave: `ReturnButtonClicked` →
`ServerLeaveGame` → (server elim) → client `OnLeftGame` → `MultiplayerSessionsSubsystem->DestroySession()` →
`OnDestroySession`: host `ReturnToMainMenuHost()`, client `OpenLevel("StartGameMap")`.
Settings: mouse sensitivity saved in `[Shooter.Settings] MouseSensitivity`; resolution 1080p/1440p/4K and
scalability Low/Medium/High via `UGameUserSettings`.

## Open issues
BUG-008 (ping ×4), BUG-010 (cooldown announcement N² RPCs + wrong "you won" text), BUG-011 (`SetHudTime` null deref),
BUG-020 (kill-feed array never pruned), BUG-027, NET-004, PERF-004 (team-score hide each tick).
