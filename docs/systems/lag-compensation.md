# System: Lag compensation (server-side rewind, "SSR")

**File:** `Source/Shooter/ShooterComponent/LagCompensationComponent.h/.cpp` — `ULagCompensationComponent`
on every `AShooterCharacter`.

## Idea
The server records each character's 16 hit boxes every tick. A client that hits someone with SSR enabled sends the
hit + the **server time at which it fired**; the server rewinds the victim's boxes to that time, re-traces on
`ECC_HitBox`, and applies damage only if the trace confirms.

## Data
- `FBoxInformation { Location, Rotation, BoxExtent }`
- `FFramePackage { Time, TMap<FName,FBoxInformation> HitBoxInfo, Character }`
- `FServerSideRewindResult { bHitConfirmed, bHeadShot, HitDistance }`
- `FShotgunServerSideRewindResult { TMap<Char*,uint32> HeadShots, BodyShots }`
- `FrameHistory`: `TDoubleLinkedList<FFramePackage>`, head = newest, trimmed to `MaxRecordTime` (4 s).

## Flow

```
TickComponent → SaveFramePackage()  (server only; early-out elsewhere)       :26/:33
Client weapon hit (HitScanWeapon / shotgun / ProjectileBullet) with bUseServerSideRewind
   HitTime = PC->GetServerTime() - PC->SingleTripTime
   ├─ ServerScoreRequest(HitChar, Start, HitLocation, HitTime)                :196
   ├─ ShotgunServerScoreRequest(HitChars, Start, HitLocations[], HitTime)     :214
   └─ ProjectileServerScoreRequest(HitChar, Start, InitialVelocity, HitTime)  :105
Server: GetFrameToCheck(HitChar, HitTime) (:139) – find bracketing frames, InterpBetweenFrames (:266)
        ConfirmHit / ShotgunConfirmHit / ProjectileConfirmHit (:345/:501/:429):
          CacheBoxPosition → MoveBoxes → disable mesh collision
          trace with only head box enabled → headshot? else enable all boxes → body?
          ResetHitBoxes + re-enable mesh collision
        ApplyDamage with the shooter's equipped weapon damage + fall-off
```
`ProjectileConfirmHit` uses `PredictProjectilePath` from `TraceStart`/`InitialVelocity` instead of a line trace.

## When SSR is on/off
`AWeapon::bUseServerSideRewind` (BP default per weapon) is flipped off by `AWeapon::OnPingTooHigh` when the
controller's `HighPingDelegate` fires (`AShooterPlayerController::CheckPing`, every `CheckPingFrequency` 20 s,
threshold `HighPingThreshold` 120 ms). With SSR off, the server's own trace/projectile decides damage.

## Clock
`AShooterPlayerController::GetServerTime()` = local time + `ServerClientDelta`, synced every `TimeSyncFreq` (5 s) via
`ServerRequestServerTime` / `ClientReportServerTime`; `SingleTripTime` = RTT/2.

## Debugging
`ShowFramePackage(Package, Color)` draws boxes for 4 s. Commented `DrawDebugBox` blocks exist in each confirm function.

## Open issues
BUG-002 / BUG-003 (shotgun SSR damage source and distance) fixed, unverified in editor (T-0002); BUG-008
(ping ×4 → SSR disabled far too often), NET-002 (no validation of HitTime/TraceStart/rate), NET-004 (client controls
its own SSR flag), PERF-006 (per-tick TMap allocations, ticks on clients). `TMap::operator[]` on `"head"` asserts if a
hit box is missing — keep the 16 boxes in sync with the constructor.
