# System: Online sessions, maps, input & config

## MultiplayerSessions plugin — `Plugins/MultiplayerSessions/`
Separate module (`MULTIPLAYERSESSIONS_API`), a dependency of the game module.

| File | Class | Role |
|---|---|---|
| `Source/MultiplayerSessions/Public/MultiplayerSessionsSubsystem.h`, `Private/...cpp` | `UMultiplayerSessionsSubsystem : UGameInstanceSubsystem` | wraps `IOnlineSession`: Create / Find / Join / Destroy (Start is a stub); broadcasts `MultiplayerOn*Complete` delegates; stores `DesiredNumPublicConnections`, `DesiredMatchType` |
| `Public/Menu.h`, `Private/Menu.cpp` | `UMenu : UUserWidget` | Host / Join buttons. `MenuSetup(NumConnections, MatchType, LobbyPath)` is called from the start map BP |
| `MultiplayerSessions.Build.cs` | — | defines `MPSESSIONS_WITH_STEAM` for the worldwide lobby distance filter |

Flow: **Host** → `CreateSession` (lobbies, presence, `MatchType` setting) → `OnCreateSession` → `ServerTravel(Lobby?listen)`.
**Join** → `FindSessions(10000)` (Steam worldwide filter) → `OnFindSessions` picks the lowest-ping session with the
same `MatchType` → `JoinSession` → `OnJoinSession` → `ClientTravel(resolved address)`.
Lobby → match travel: `ALobbyGameMode` (game-flow.md). Leave → `UReturnToMainMenu` (controller-and-hud.md).
Note: `UMenu::MenuSetup`'s default `LobbyPath` is `/Game/ThirdPersonCPP/Maps/Lobby`; the BP must pass `/Game/Maps/Lobby`.

## Online config (`Config/DefaultEngine.ini`)
- `DefaultPlatformService=Steam`, `SteamDevAppId=4797420`, net driver `SteamSocketsNetDriver` with `IpNetDriver` fallback.
- Plugins (`Shooter.uproject`): `OnlineSubsystemSteam`, `SteamSockets`, `HDRIBackdrop`, `ModelingToolsEditorMode` (editor).
- Game module deps (`Source/Shooter/Shooter.Build.cs`): Core, CoreUObject, Engine, InputCore, Niagara, MultiplayerSessions,
  OnlineSubsystem, OnlineSubsystemSteam. (UMG comes via `.uproject` AdditionalDependencies.)
- `DefaultGame.ini`: `MaxPlayers=100`.

## Maps — `Content/Maps/`
`StartGameMap` (default + editor startup; main menu), `Lobby`, `ShooterMap` (FFA), `Teams`, `CaptureTheFlag`,
`TransitionMap` (seamless travel). `GlobalDefaultGameMode` still points at the template `BP_ThirdPersonGameMode`;
each map's World Settings overrides it.

## Input — `Config/DefaultInput.ini` (legacy Action/Axis mappings, not Enhanced Input)

| Action | Keyboard/Mouse | Gamepad |
|---|---|---|
| Jump | Space | Face Bottom |
| Equip | E | Face Left |
| Crouch | Left Ctrl | Face Right |
| Aim / Fire | RMB / LMB | LT / RT |
| Reload | R | Face Left (**same as Equip**, BUG-026) |
| Gernade / SmokeGernade | G / H | RB / LB |
| ChangeFOV (TPS↔FPS) | V | D-pad Down |
| SwapToPrimary / SwapToSecondary | 1 / 2 | Face Top / — |
| Sprint (hold) / SprintToggle | Left Shift / — | — / Left Stick click |
| Quit (menu) | P, Esc | Special Right |
| Scoreboard | Tab | — |
Axes: MoveForward (W/S, LeftY), MoveRight (D/A, LeftX), Turn (MouseX, RightX), LookUp (MouseY inverted, RightY).

## Content layout (`Content/`)
Game-owned: `BluePrints/` (all gameplay BPs, see system docs), `Maps/`, `Characters/`, `FX/`, `Sound/`, `Materials/`,
`Curves/`, `Images/`, `Fonts/`. Marketplace / third-party packs (don't edit, large): `Customizable Weapon Pack 5.1`,
`FPS_Weapon_Bundle`, `MilitaryPolygon`, `MilitaryWeapSilver`, `ParagonDekker`, `Quixel`, `WeaponsPack`, `Weapon_Pack`,
`LearningKit_*`, `LevelPrototyping`, `ThirdPerson`, `BlockGame`, `HDRI`.

## Build
Engine: UE **5.7** at `C:\Program Files\Epic Games\UE_5.7`. Targets: `Shooter` (Game), `ShooterEditor`.
```
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" ShooterEditor Win64 Development -Project="C:\Unreal Projects\Shooter - Copy 5.7\Shooter.uproject" -WaitMutex
```
Close the editor first or use Live Coding (Ctrl+Alt+F11) from inside the editor; UBT refuses to build a module the
running editor has loaded.

## Open issues
BUG-021 (CreateSession continues after destroy), BUG-022 (join ignores result), BUG-026 (gamepad binding clash).
