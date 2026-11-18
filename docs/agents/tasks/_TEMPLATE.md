# T-NNNN: <short title>

| Field | Value |
|---|---|
| Status | todo  <!-- proposed / todo / in-progress / blocked / review / done --> |
| Assignee | ue-worker (sonnet) |
| Reviewer | ue-reviewer (sonnet) <!-- use opus for networking / lag-compensation --> |
| Issues | BUG-0xx, PERF-0xx |
| Build | orchestrator <!-- or: worker --> |
| Created | YYYY-MM-DD by orchestrator |

## Goal
One or two sentences: the observable outcome.

## Read first
- `CLAUDE.md`, `docs/GLOSSARY.md`
- `docs/systems/<area>.md` (sections: ...)
- `docs/KNOWN_ISSUES.md` rows: ...

## Files (read)
- `Source/Shooter/...`

## Files (write) — binding lock
- `Source/Shooter/...`
- `docs/KNOWN_ISSUES.md` (status column of the listed issues only)
- `docs/systems/<area>.md` (only if behavior described there changes)

## Constraints
- Do not rename BlueprintCallable / BlueprintImplementableEvent / BindWidget / replicated property names.
- Keep server authority; any new RPC must say Reliable/Unreliable and why.
- No new Tick work; prefer timers/events.
- <task-specific>

## Acceptance criteria
- [ ] ...
- [ ] Compiles (`ShooterEditor Win64 Development`)
- [ ] KNOWN_ISSUES rows updated; system doc updated if behavior changed

## Playtest checklist (for the user)
1. PIE, 2 players, Net Mode "Play As Listen Server" ...
2. ...

---
## Worker report
<!-- appended by ue-worker -->
- **Status:** review | blocked
- **Changed:** `File.cpp` (`Function`) — what & why
- **Build:** not run / passed / failed (paste the first error)
- **Docs updated:** ...
- **Unverified:** ...
- **Doc updates needed (outside my write-set):** ...
- **Questions for orchestrator:** ...

---
## Review
<!-- appended by ue-reviewer -->
- **Verdict:** approve | changes requested
- **Findings:** `File.cpp:123 (Function)` — problem — suggested change
