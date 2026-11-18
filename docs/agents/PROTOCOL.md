# Multi-agent protocol (Opus orchestrator ⇄ Sonnet workers)

Agents never talk to each other directly. **All coordination goes through files in `docs/agents/`**, and the
orchestrator is the hub. This keeps every decision on disk, reviewable by the user, and resumable after a context reset.

```
            user
             │  goals, approvals, playtests
             ▼
   ┌────────────────────┐   writes task files, BOARD.md, merges, builds, updates docs index
   │ ORCHESTRATOR (Opus)│
   └─────────┬──────────┘
     spawns  │  (Agent tool, subagent_type = ue-worker / ue-reviewer)
   ┌─────────┼─────────────┐
   ▼         ▼             ▼
 ue-worker  ue-worker   ue-reviewer        (Sonnet; defined in .claude/agents/)
   │ reads task file → edits code in its write-set → appends "Worker report"
   └──────────────► ue-reviewer appends "Review" → orchestrator decides
```

## Roles

| Role | Model | Defined in | Does | Never does |
|---|---|---|---|---|
| Orchestrator | Opus (the main Claude Code session) | `CLAUDE.md` | breaks goals into tasks, writes task files, assigns write-sets, spawns workers, reviews/merges, runs the build, keeps `BOARD.md` + docs index current, talks to the user | large mechanical edits it could delegate |
| Worker | Sonnet | `.claude/agents/ue-worker.md` | executes exactly one task file; edits only files in its write-set; reports | edit outside write-set, commit, start other tasks, rename BP-exposed symbols unless the task says so |
| Reviewer | Sonnet (Opus for networking/SSR) | `.claude/agents/ue-reviewer.md` | checks a finished task against acceptance criteria and UE networking rules; writes verdict | edit code |

## Files

| Path | Owner | Purpose |
|---|---|---|
| `docs/agents/BOARD.md` | orchestrator | single source of truth: every task, status, owner, write-set |
| `docs/agents/tasks/T-NNNN-slug.md` | orchestrator writes spec; worker appends report; reviewer appends review | one unit of work |
| `docs/agents/tasks/_TEMPLATE.md` | — | copy this to start a task |
| `docs/KNOWN_ISSUES.md` | worker updates rows for issues its task fixes | issue ledger |
| `docs/systems/*.md` | worker updates only if listed in its write-set; else lists changes in report | system knowledge |

## Task lifecycle

```
proposed ──(user/orchestrator approve)──► todo ──(spawned)──► in-progress ──► review ──► done
                                                     │                           │
                                                     └──► blocked (question) ◄───┘ (changes requested → in-progress)
```
- Status lives in **both** the task file header and `BOARD.md`; the orchestrator keeps them in sync.
- `blocked` means the worker wrote a question under **Questions for orchestrator** and stopped.

## Write-set locking (prevents agents clobbering each other)
1. Each task lists **Files (write)**. Two tasks that are `in-progress` at the same time must have **disjoint** write-sets
   (compare against BOARD.md before spawning).
2. A worker that discovers it needs another file **does not edit it** — it adds it under *Questions* and stops, or
   finishes the rest and lists the follow-up.
3. `.h` files are high-contention (UHT, includes). Prefer one task per header at a time.
4. For larger parallel waves, spawn workers with `isolation: "worktree"` and merge their branches one by one.

## Build & verification rules (Unreal-specific)
- **Only one build at a time** (UBT holds a mutex; the editor locks loaded modules). Workers do *not* build in parallel —
  the orchestrator builds after merging a wave, or a worker builds only if the task says `Build: worker`.
- Build command and Live Coding note: `docs/systems/online-and-config.md#build`.
- Agents cannot play the game. Every task ends with a **Playtest checklist** for the user (PIE with 2+ players,
  "Play As Listen Server", "Net PktLag=150" for SSR work). A task is `done` only after build passes; behavior claims stay
  "unverified in editor" until the user confirms.
- Blueprint-facing changes (renaming/removing `UFUNCTION(BlueprintCallable)`, `BlueprintImplementableEvent`,
  `UPROPERTY` used in BPs, `BindWidget` names, enum order) need `CoreRedirects` or a BP fix-up step the user performs —
  call this out in the task spec and the report.

## Spawning a worker (orchestrator)
Prompt template for the Agent tool (`subagent_type: "ue-worker"`):
```
Execute task docs/agents/tasks/T-NNNN-slug.md.
Project root: C:\Unreal Projects\Shooter - Copy 5.7
Follow .claude/agents/ue-worker.md and docs/agents/PROTOCOL.md. Write-set is binding.
When finished, append your Worker report to the task file and set Status: review.
Reply with: status, files changed, build result (if built), questions.
```
Then spawn `ue-reviewer` with the same task path. The orchestrator reads the review, updates `BOARD.md`, and either
closes the task or reopens it with the requested changes.

## Communication etiquette
- Write facts with locations: `File.cpp:123 (FunctionName)`. Line numbers drift — always include the function name.
- Keep reports short: what changed, why, what's unverified. No pasted full files.
- Anything another agent should know long-term goes into a system doc (or a "Doc updates needed" list), not chat.
- Never mark an issue fixed without the build passing; mark "fixed, unverified in editor" until playtested.
- Commits/PRs are made by the orchestrator only, and only when the user asks.
