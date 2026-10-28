# SMP design and staged implementation

Reviewed against main b6ea1f9a9556963a197c8d446e989164e7f8636d.
Status: stages 1–5. Experimental Lazy SMP is available through Threads (1–4),
defaulting to 1. Parallel strength/scaling acceptance remains stage 6.

## Goals and architecture

Keep macOS ARM64, Linux ARM64/x64 and Windows ARM64/x64. Preserve the existing
single-thread search as the correctness and performance reference. Start with
Lazy SMP: workers search private copies of the root position and exchange useful
bounds/moves through a synchronized transposition table. Do not implement DTS,
NUMA tuning, affinity, shared history heuristics or worker-result voting initially.

Use one coordinator (worker zero) to retain the current iterative deepening,
aspiration, MultiPV, root restrictions, adaptive time management and final result.
Helpers are silent and their completed results do not override worker zero in the
first version. Small deterministic depth/scheduling differences can diversify
helper searches; evaluate the exact policy experimentally after correctness.
MultiPV may initially use one worker explicitly rather than shipping unverified
parallel root exclusions. Book and root DTZ shortcuts run before helper dispatch.

## Ownership audit

| Current resource | Required owner / change |
| --- | --- |
| search.c counterMoves and rootBestMove | Per-search state (stage 1) |
| S_BOARD position, history, PV, killers and history scores | Private to each worker; preserve real repetition history when copying |
| Pawn cache | S_SEARCHINFO.state.pawnCache (stage 2a); zero initialize each instance, retain across searches |
| S_BOARD.HashTable wrapper | CopyBoardForSearch creates a borrowed view; FreeHashTable detaches it without freeing owner storage (stage 2a) |
| TT entries in pvtable.c | Synchronized access to entire entry including key, draw context, clock, score, move, depth and generation |
| TT counters / hashfull / PV lookup | Worker-local statistics or synchronized snapshots; all readers must obey the same protection |
| S_SEARCHINFO limits, nodes, stop and timing | Split immutable request, worker counters/results and shared atomic cancellation |
| EngineOptions | Snapshot per search; changes apply only after workers stop and join |
| misc.c input queue / Windows input handles | Coordinator only; helpers must never call ReadInput or WaitForSearchSignal |
| PrMove / PrSq static buffers and stdout | Coordinator only; use caller-owned formatting buffers if workers later need diagnostics |
| Fathom globals / mappings | Initialize and free only while idle; enable library threading before concurrent WDL probes; root probes serialized |
| Polyglot state and random selection | Coordinator only, before dispatch; book cleanup only while idle |
| Attack masks, Zobrist keys, MVV/LVA and evaluation constants | Initialize once before workers; read-only thereafter |

Stage 1 does not make SearchPosition reentrant: the pawn cache, TT, input and
options remain shared. A separate S_SEARCHINFO is required for each future worker.

Stage 2a removes the global pawn cache. Search uses EvalPositionCached with its
own cache; EvalPosition remains available for uncached diagnostics. Only pawn
bitboards determine cache content, so it survives position resets and evaluation
mode changes. Future workers must heap-allocate S_SEARCHINFO, including this cache,
before thread dispatch. It is still stack allocated by today's single-thread main.

CopyBoardForSearch requires a fresh/detached destination and makes its board,
history and heuristics independent while borrowing TT entries. Borrowed views
cannot resize or clear the owner's table; FreeHashTable only detaches them. This
is a lifetime contract, not reference counting: all borrowers must be detached
before owner resize/free. Raw board assignments used by diagnostic snapshots are
not independent owners and must not be freed. TT reads/writes remain unsynchronized;
this helper must not yet be used to run simultaneous searches.

Stage 2b snapshots EngineOptions in ClearForSearch. Search, cached evaluation and
Syzygy eligibility use that snapshot; diagnostic evaluation still uses the current
configured option. UCI time-budget parsing remains coordinator-only before search.
Depth, node limit, root restrictions and computed budgets already reside in the
per-search S_SEARCHINFO. Ponder/stop fields remain mutable coordinator control
state and must be separated from worker controls when threading is introduced.
Nodes, node limits, tablebase hits, fail-high/null-cut statistics and TT counters
now use uint64_t, with portable UCI formatting. The parser accepts nonnegative
node limits through the existing signed 64-bit input range. Worker-local counters
remain non-atomic; stage 4 adds separate atomic publication. Fathom mappings,
the TT, input and global option updates still require serialized lifecycle use.

## Threading and memory model

Stage 3 implements threads.h/threads.c: opaque heap-owned thread, mutex and
condition handles, pthreads on Unix and _beginthreadex/native synchronization on
Windows. Each worker reserves an explicit 8 MiB stack. Start returns NULL for
allocation/native-start failure; join consumes the handle only on success. No
detached threads, forced cancellation, engine worker pool or parallel search is
introduced. Callers must publish a cooperative stop predicate, wake waiters and
join before freeing any shared state. Unexpected synchronization errors fail fast.

The standalone lifecycle test runs 100 pool lifetimes, including injected start
failure before any worker and after 1/2/3 workers. Complete pools process 8000
tasks across repeated epochs, including notifications with no predicate change.
Tests join every started worker before freeing its pool. Failure injection is
compiled only into the test binary; native allocation exhaustion is not induced.
make smoke runs the bounded native test on all five platforms; make test adds
ASan/UBSan, and macOS CI runs make thread-tsan for race detection. This checks the
primitive layer and a test pool, not the future engine search lifecycle.

Provide a small C wrapper around pthreads on macOS/Linux and native Windows
threads, mutexes and condition variables. Verify it with the existing compilers,
including Windows static packaging. Keep OS types out of search code. Allocate
worker boards and caches on the heap; budget worker stacks for recursive search
and move lists, particularly on Windows.

Start with striped mutexes for TT access. Copy entries into local snapshots while
locked, then release before interpreting them; store/replacement also locks the
same stripe. This is deliberately a correctness-first design to benchmark. Do not
use plain racing C accesses, volatile, or key-last writes as a substitute for
synchronization. A later atomic layout is a separate measured change. Preserve
DrawContext checks and mate-score normalization. Advance generation once per go,
not once per worker. Resize/clear/free only after all workers are quiescent.

Use C11 atomics for cancellation and published node totals, with an explicit
ordering contract. Workers may accumulate local counts and publish periodically;
document and test bounded node-limit overshoot. Avoid concurrent non-atomic reads
of counters. Use fixed-width 64-bit counters across platforms rather than relying
on long (32-bit on Windows). Allocation/thread-creation failure must unwind started
workers and retain a usable one-thread engine, with a protocol-safe diagnostic.

## Lifecycle

Stage 4 adds opt-in EnableHashSharing before board views are created. A shared
table has 256 mutex stripes; all production probe/PV/hashfull readers copy an
entry while locked and stores protect replacement plus the entire write. Serial
tables keep a NULL synchronization pointer and acquire no locks. View counters
and generation snapshots are private; only the owner advances generation before
dispatch. Shared borrower counts prevent owner clear/resize/free until every
view detaches. These lifecycle calls remain coordinator-only and must not race
with creation of views. Enable sharing before the first borrowed view; preexisting
serial views are not tracked. Failed lock allocation leaves a usable serial table.

S_SEARCHCONTROL provides atomic cancellation, node publication and a replaceable
deadline (zero means none, suitable for pondering). Initialize only while idle;
the node limit is immutable. Stop/deadline use release/acquire; node totals use
relaxed atomics and are not a publication mechanism for other state. This layer
does not yet replace SearchPosition's serial control path: stage 5 wires workers
to it, handles final node deltas and specifies batching/overshoot. The stress test
publishes one node per iteration and checks total overshoot <= workers-1.

Fathom's internal threading is enabled. Concurrent real-file WDL tests include
cold initialization of table mappings; root probing and init/free are still idle
coordinator operations. Shared-resource tests deliberately collide keys with
different scores/moves/draw contexts at one TT slot and verify intact snapshots,
draw-sensitive probes and bounded cancellation. Native tests run on all targets;
ASan/UBSan and macOS ThreadSanitizer additionally exercise the shared path.

Idle -> prepare request and root copies -> dispatch helpers -> coordinator search
-> request stop -> join/park all helpers -> publish exactly one bestmove -> Idle.
Persistent workers may wait on a condition-variable predicate with a search epoch;
never rely on a wakeup alone. No worker may access a previous request after join.
The coordinator handles stdin, stop, quit, isready and ponderhit. Queued position,
newgame, Hash, Threads, SyzygyPath and other changes wait for quiescence. Quit and
EOF join workers before releasing TT, tablebase mappings or book storage.

Ponderhit starts the play clock once. Infinite/ponder terminal results retain the
existing wait semantics without leaving helpers spinning. Only worker zero makes
adaptive soft-time decisions; hard deadline and cancellation apply to every
worker. Coordinator output aggregates synchronized counters, while PV and score
come from its latest completed iteration. Default Threads remains 1.

## Stages and acceptance gates

1. **Search-state ownership (this PR).** Move counter-move/root-best globals into
   S_SEARCHINFO. Test independent resets, full regressions, real Syzygy smoke and
   fixed-depth equality of scores, PVs, nodes and best moves against main.
2. **Remaining private state.** Introduce worker-owned pawn cache and a clear
   ownership model for board/TT storage, immutable options and 64-bit counters.
   Keep execution serial. Repeat exact search comparisons and measure memory.
3. **Portable lifecycle.** Add and test thread/lock/condition wrappers independently
   of chess search on all five targets. Test repeated start/stop/join, failed
   creation and teardown. Add race detection on a supported native CI runner.
4. **Safe shared resources.** Implement and stress-test TT snapshot/store access,
   counters, cancellation and Fathom concurrent WDL support. Verify table reload
   and hash resize only while idle. Benchmark one-thread overhead before proceeding.
5. **Experimental two-worker search.** Add Threads (default 1), helper dispatch and
   coordinator-only I/O/results. Test stop/quit/EOF, ponderhit, infinite, repeated
   go, node/time limits, searchmoves, MultiPV policy and terminal/tablebase roots.
6. **Scaling and strength.** Test 1/2/4 workers at fixed wall time on ARM64 and x64.
   Measure nodes/sec, time to depth, memory, stop latency and game results. Keep
   total Hash constant; restrict match concurrency to avoid oversubscription.
   Use paired fresh openings and confirmation matches before promoting parallel
   search. Faster node counting alone is not acceptance evidence.

Each stage is a separate reviewable change with the existing five-platform CI
gate. No automatic merge and no switch away from the single-thread baseline.
Threaded searches need not reproduce identical PVs/nodes between runs; require
legal results, correct lifecycle and no detected races instead. Single-thread
refactors should preserve exact fixed-depth behavior unless a reviewed bug fix
explains a difference. Set performance thresholds before each experiment, based
on repeated measurements rather than one noisy run.

## Open decisions before stage 5

Choose worker count limit and memory cap from measured per-worker requirements;
verify default stack sizes; measure striped TT contention; choose helper depth
diversification; quantify global node-limit overshoot; decide whether MultiPV
stays single-worker initially. Physical-core count is a starting benchmark point,
not a universal optimum, especially on mixed performance/efficiency cores.

## Stage 5 implementation

Threads counts the coordinator plus helpers, with an initial cap of four. Each
helper owns a heap-allocated board, search state and pawn cache, and reserves an
8 MiB stack through the portable wrapper. Helpers use iterative deepening with
alternating initial depths 1/2, aspiration windows, and the synchronized shared
TT; only the coordinator chooses the move. This is an experimental policy.

For this first implementation, each go creates and joins its helper threads.
There is no persistent pool or search epoch to reuse incorrectly. Measure this
startup cost in stage 6 before adding a persistent pool. Startup failure stops,
wakes and joins every already-created helper, detaches borrowed TT views, and
falls back to serial search with an info string. Tests inject failure before
worker 1 and after workers 1/2. Shared TT locks are removed after joining, so a
subsequent Threads=1 search returns to the existing lock-free serial path.

Workers publish node and tablebase-hit deltas every 256 local nodes and at exit.
Node limits apply to the aggregate; batching can overshoot by at most 256 times
the configured thread count. Final counters include every worker's final delta.
Only the coordinator handles input and adaptive soft deadlines; all workers
observe the atomic hard deadline and stop request. All helpers are joined before
bestmove, settings changes, or the terminal ponder/infinite wait. EOF and quit
join without emitting a bestmove. Searchmoves restrictions apply to every worker.

MultiPV greater than 1 and console mode deliberately use serial search. Book,
terminal positions, and root DTZ shortcuts do not dispatch helpers. Fathom WDL
probes inside search may run concurrently. Threads defaults to 1, preserving the
baseline behavior. No parallel Elo or scaling gain is claimed yet.

Validation: portable actual-engine tests cover Threads 1/2/4, restricted roots,
node limits, timed search, readiness, stop, ponderhit, completed pondering,
Hash resize, repeated searches, MultiPV, terminal roots, quit and EOF. They run
on all five CI platforms; macOS additionally runs actual search under TSan.
Partial startup failures run natively, with ASan/UBSan, and with TSan. Real-table
protocol tests run with four threads, including root shortcuts and WDL fallback.
Local one-thread comparison against main 2d42dd3 preserves every iteration's
score/PV/nodes and final move across 28 opening/tactical/endgame positions,
13,235,914 nodes per pass, three passes. Timing was not isolated from other
validation jobs and is not used as performance evidence.
