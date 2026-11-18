# Glossary, naming quirks & shared types

> Read this before grepping. Many identifiers are **misspelled in the code** — search for the
> code spelling, not the English one. Do not "fix" a spelling in passing: renames touch
> Blueprints/UHT and must be their own task (see `agents/PROTOCOL.md`).

## Misspellings you must search by

| English | Code spelling (use this in grep) | Where |
|---|---|---|
| Grenade | `Gernade`, `Gerande`, `Granade` (all three exist) | CombatComponent, ShooterCharacter, Projectile*Granade |
| Launch grenade | `LunchGernade`, `ServerLunchGernade`, `ServerLunchSmokeGernade` | CombatComponent |
| Launcher | `Luncher` (`EWT_RocketLuncher`, `EWT_GranadeLuncher`) | WeaponTypes.h |
| Assault | `Assualt` (`EWT_AssualtRifle`) | WeaponTypes.h |
| Equipped weapon (getter) | `GetEqiuppedWeapon()` | ShooterCharacter |
| OnRep equipped | `OnRep_EuippedWeapon` | CombatComponent |
| Equip secondary | `EquipSecondayWeapon` | CombatComponent |
| Server set aiming | `SeverSetAiming` | CombatComponent |
| Lag compensation getter | `GetLagCompansationComponent()` | ShooterCharacter |
| Hit boxes map | `HitCollisionBoxies` | ShooterCharacter |
| Receive damage | `RecieveDamage` | ShooterCharacter |
| Multicast elim | `MulticaseElim` | ShooterCharacter |
| Dissolve | `Desolve` / `Disolve` | ShooterCharacter |
| Initialize | `Intialize*`, `IntialTransform`, `EWS_intial` | many |
| Trace length | `TRACE_LENGHT` | WeaponTypes.h |
| Character overlay (HUD ptr) | `CharaterOverlay`, `AddCaracterOverlay()` | ShooterHud |
| Announcement class | `AnnouncmentClass`, `ElimAnnoucementClass`, `ClientElimAnnoucement` | ShooterHud / PlayerController |
| Victim | `Victem` | GameModes |
| Default | `Defualt` (`DefualtFov`, enum DisplayNames) | CombatComponent, enums |
| Destroy timer finished | `DestroyTimerFinshed` | Projectile |
| Pitch calc | `CalculateYO_Ptich` | ShooterCharacter |
| Shotgun class | `Ashotgun` (lower-case s, file `Weapon/shotgun.h`) | Weapon/ |
| Carried ammo text | `CarriedWeaponAmmount` | CharacterOverlay |
| Swap | `SwapingWeapons` | CombatState |
| HUD input actions | `"Gernade"`, `"SmokeGernade"` | DefaultInput.ini |

Both `GetCombatComponent()` and `getCombatComponent()` exist on `AShooterCharacter` and return the same pointer.

## Team naming mismatch
`AShooterGameState` stores teams in arrays named **`RedTeam`** and **`BlueTeam`**, but the
`ETeam` values and all UI say **Green** and **Grey**. Mapping used by `ATeamsGameMode`:
`RedTeam` ⇔ `ET_GreenTeam`, `BlueTeam` ⇔ `ET_GreyTeam`.

## Enums (all `UENUM(BlueprintType)`)

| Enum | File | Values |
|---|---|---|
| `ECombatState` | `Source/Shooter/CombatState.h` | `ECS_Unoccupied`, `ECS_Reloading`, `ECS_ThrowingGernade`, `ECS_SwapingWeapons` |
| `ETeam` | `Source/Shooter/Team.h` | `ET_GreenTeam`, `ET_GreyTeam`, `ET_NoTeam` |
| `ETurningInPlace` | `Source/Shooter/TurningInPlace.h` | `ETIP_Left`, `ETIP_Right`, `ETIP_NotTurning` |
| `EWeaponType` | `Source/Shooter/Weapon/WeaponTypes.h` | `EWT_AssualtRifle`, `EWT_RocketLuncher`, `EWT_Pistol`, `EWT_SMG`, `EWT_ShotGun`, `EWT_SniperRifle`, `EWT_GranadeLuncher`, `EWT_Flag` |
| `EWeaponState` | `Source/Shooter/Weapon.h` | `EWS_intial`, `EWS_Equipped`, `EWS_Dropped`, `EWS_EquippedSecondary` |
| `EFireType` | `Source/Shooter/Weapon.h` | `EFT_HitScan`, `EFT_Projectile`, `EFT_Shotgun` |

`MatchState::Cooldown` is a custom `FName` declared in `ShooterGameMode.h` (extends the engine `MatchState` namespace).

## Constants / macros

| Name | Value | File |
|---|---|---|
| `TRACE_LENGHT` | `1000000.f` | `Weapon/WeaponTypes.h` |
| `CUSTOM_DEPTH_PURPLE/BLUE/TAN` | 250 / 251 / 252 (outline stencil) | `Weapon/WeaponTypes.h` |
| `ECC_SkeletalMesh` | `ECC_GameTraceChannel1` | `Shooter.h` |
| `ECC_HitBox` | `ECC_GameTraceChannel2` (server-side-rewind boxes) | `Shooter.h` |

## UI strings
`Source/Shooter/Announcement.h` (namespace `Announcement`) holds the end-of-match strings
(`NewMatchStartsIn`, `YouAreTheWinner`, `GreenTeamWins`, ...). Not to be confused with the widget
class `UAnnouncement` in `HUD/Announcement.h`.

## Bone / socket names used from C++
- Character mesh sockets: `RightHandSocket`, `LeftHandSocket`, `PistolSocket`, `BackSocket`, `FlagSocket`, `headSocket`.
- Weapon mesh sockets: `MuzzleFlash`, `AmmoEject`, `LeftHandSocket`, `Hand_R`.
- Headshot check: bone `head` **or** `neck_01`.
- 16 hit boxes keyed by bone name: `head pelvis spine_02 spine_03 upperarm_l/r lowerarm_l/r hand_l/r thigh_l/r calf_l/r foot_l/r`.
- Montage sections: Fire `AimRifle`/`RifleHip`; Reload `Rifle RocketLuncher Pistol SMG Shotgun Sniper GranadeLuncher` + `ShotgunEnd`; HitReact `FromFront`.

## Config keys written by code
`[Shooter.Settings]` in `GGameUserSettingsIni`: `MouseSensitivity` (float), `HasInitializedGraphics` (bool).
