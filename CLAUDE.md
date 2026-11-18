# Shooter (UE 5.7) — agent entry point

Multiplayer third/first-person shooter in Unreal Engine **5.7** C++ (listen server over Steam). Modes: Free-for-all,
Teams, Capture-the-Flag. Game module `Source/Shooter`, plugin `Plugins/MultiplayerSessions`.

**Active project:** `C:\Unreal Projects\Shooter - Copy 5.7` — this folder. `C:\Unreal Projects\Shooter` is a stale copy;
never edit it.

## Read next
1. `docs/README.md` — index + "where is…?" lookup (read this before opening source files)
2. `docs/GLOSSARY.md` — **identifiers are misspelled** (`Gernade`, `GetEqiuppedWeapon`, `Luncher`…); grep by code spelling
3. `docs/ARCHITECTURE.md`, then the `docs/systems/*.md` you need
4. `docs/KNOWN_ISSUES.md` — bug/perf/net ledger with IDs
5. `docs/agents/PROTOCOL.md` — how the orchestrator and workers coordinate

## Working model
- The main session (Opus) is the **orchestrator**: plans, writes task files in `docs/agents/tasks/`, tracks
  `docs/agents/BOARD.md`, spawns `ue-worker` / `ue-reviewer` subagents (Sonnet, defined in `.claude/agents/`), merges,
  builds, and talks to the user.
- Workers execute one task file each and edit only that task's write-set. Hub-and-spoke: agents communicate through the
  task files, never directly.
- Small one-file fixes can be done directly by the orchestrator; use workers for parallel or mechanical work.

## Rules for every agent
- Server-authoritative gameplay. Know which machine code runs on (`HasAuthority`, `IsLocallyControlled`, OnRep vs RPC).
- Don't rename/remove Blueprint-facing symbols (BlueprintCallable, BlueprintImplementableEvent, BP-visible UPROPERTYs,
  BindWidget names, replicated properties, enum order) without a redirect plan — it breaks `.uasset`s.
- Most tuning values live in Blueprints under `Content/BluePrints/`; a C++ default change may be overridden there.
- No new per-frame Tick work; prefer timers / OnRep / delegates. No `UE_LOG(Warning)` in hot paths.
- Minimal, focused diffs in the existing style. Don't touch `Content/` marketplace packs, `Binaries/`, `Intermediate/`,
  `Saved/`, `DerivedDataCache/`.
- Docs are part of done: update the system doc + `KNOWN_ISSUES.md` when behavior changes. Line numbers in docs are hints — grep.
- Agents can't playtest. Say "unverified in editor" and give the user a short PIE checklist.
- Commit/push only when the user asks.

## Build
```
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" ShooterEditor Win64 Development -Project="C:\Unreal Projects\Shooter - Copy 5.7\Shooter.uproject" -WaitMutex
```
Close the Unreal Editor first (or use Live Coding inside the editor). One build at a time.

## Legacy files
Root `BEFORE_AFTER_CODE_COMPARISON.md`, `HEALING_TIME_OPTIMIZATION.md`, `WEAPON_AMMO_*.md` are notes from earlier
edits with stale line numbers/values — `docs/` supersedes them. `.continue/` and `.localpilot/` belong to other tools.
