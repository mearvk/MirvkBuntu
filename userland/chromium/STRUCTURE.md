# Chromium Source — Formal Structure Document

**Version:** 153.0.7982.0 (tip-of-tree, dev channel)  
**Commit:** 5006852cd6  
**Source:** github.com/chromium/chromium  
**License:** BSD-3-Clause  
**Document Date:** 2026-08-13  
**Distribution:** Galactic Cherry Marvell Edition 98

---

## Table of Contents

1. [Process Architecture](#process-architecture)
2. [Threading Model](#threading-model)
3. [Memory Management](#memory-management)
4. [GUI Compositor Pipeline](#gui-compositor-pipeline)
5. [Source Tree Map](#source-tree-map)
6. [Thread–Memory Interaction Points](#thread-memory-interaction-points)
7. [Key Files Reference](#key-files-reference)

---

## Process Architecture

Chromium is a multi-process system. Each process class has distinct memory ownership and threading responsibilities.

| Process | Role | Main Thread | Key Threads |
|---------|------|-------------|-------------|
| **Browser** | UI, navigation, coordination | UI thread (BrowserThread::UI) | IO, GPU host, compositor |
| **Renderer** | Page rendering, JavaScript, Blink | Blink main thread | Compositor, workers, audio |
| **GPU** | Graphics command execution, display | GPU main thread | Command buffer decoder, Viz display |
| **Utility** | Isolated tasks (network, audio, print) | Utility main thread | IO |
| **Zygote** (Linux) | Pre-fork template for renderer/utility | — | — |

```
┌────────────────────────────────────────────────────────────────────┐
│  BROWSER PROCESS                                                    │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────────────┐ │
│  │  UI Thread   │  │  IO Thread   │  │  ThreadPool (general)     │ │
│  │  (main)      │  │  (IPC/Mojo)  │  │  BEST_EFFORT / DEFAULT /  │ │
│  │              │  │              │  │  USER_VISIBLE / USER_BLOCKING│
│  └──────┬───────┘  └──────┬───────┘  └───────────────────────────┘ │
│         │                  │                                        │
│         │  Mojo IPC        │                                        │
└─────────┼──────────────────┼────────────────────────────────────────┘
          │                  │
┌─────────┼──────────────────┼────────────────────────────────────────┐
│  RENDERER PROCESS          │                                        │
│  ┌─────────────┐  ┌───────┴──────┐  ┌───────────────────────────┐ │
│  │ Blink Main   │  │ Compositor   │  │  Workers (Web Workers,    │ │
│  │ Thread       │  │ Thread       │  │  Service Workers)         │ │
│  │ (DOM, JS,    │  │ (impl-side   │  │                           │ │
│  │  layout,     │  │  commit,     │  │                           │ │
│  │  style)      │  │  tiles,      │  │                           │ │
│  │              │  │  animation)  │  │                           │ │
│  └──────────────┘  └──────────────┘  └───────────────────────────┘ │
└────────────────────────────────────────────────────────────────────┘
          │
┌─────────┼──────────────────────────────────────────────────────────┐
│  GPU PROCESS                                                        │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────────────┐ │
│  │ GPU Main     │  │ Display      │  │  Command Buffer           │ │
│  │ Thread       │  │ Compositor   │  │  Decoder Threads          │ │
│  │              │  │ (Viz)        │  │                           │ │
│  └──────────────┘  └──────────────┘  └───────────────────────────┘ │
└────────────────────────────────────────────────────────────────────┘
```

### Process Isolation & Memory Boundaries

- Each process has its own address space — no shared heap
- Cross-process communication via Mojo IPC (message passing, not shared memory by default)
- Shared memory regions used explicitly for: GPU command buffers, compositor surfaces, shared bitmaps
- The browser process is the privileged coordinator; renderers are sandboxed

---

## Threading Model

### Core Concepts

| Concept | Description |
|---------|-------------|
| **Task** | A `base::OnceClosure` — a bound function pointer with arguments, posted for async execution |
| **Sequence** | A virtual thread. Tasks run sequentially but may hop physical threads between tasks |
| **Thread Pool** | A pool of physical threads with shared task queues (`base::ThreadPoolInstance`) |
| **Task Runner** | Interface for posting tasks: `TaskRunner`, `SequencedTaskRunner`, `SingleThreadTaskRunner` |
| **Physical Thread** | OS thread (`base::PlatformThread`). Rarely created directly. |

### Thread Hierarchy

```
base::PlatformThread          ← OS thread (pthread/CreateThread). Almost never used directly.
  └── base::Thread            ← Named thread with dedicated task queue (IO, UI)
  └── base::ThreadPool       ← Pool of worker threads for general tasks
        ├── BEST_EFFORT      ← Low priority, may be deferred (e.g. metrics, cleanup)
        ├── USER_VISIBLE     ← Affects what user sees but not immediately blocking
        └── USER_BLOCKING    ← Blocks user interaction (e.g. file picker result)
```

### Sequences vs. Physical Threads

**Chromium strongly prefers virtual sequences over physical threads.**

- A sequence guarantees serial execution but may move between physical threads
- Allows the thread pool to manage physical thread count efficiently
- Most objects are **thread-unsafe by design** — they live on one sequence and use `SEQUENCE_CHECKER` to enforce
- `base::SequenceBound<T>` wraps objects that must live on a specific sequence

### Key Named Threads

| Thread | Process | Responsibility |
|--------|---------|----------------|
| UI (BrowserThread::UI) | Browser | User interface updates, navigation decisions |
| IO (BrowserThread::IO) | Browser | IPC message dispatch, async I/O, network events |
| Blink Main | Renderer | DOM, JavaScript, style, layout |
| Compositor (impl) | Renderer | Tile management, animation ticking, frame production |
| GPU Main | GPU | Graphics state management |
| Viz Display | GPU | Display compositor, frame aggregation, output to screen |
| DedicatedWorker | Renderer | Web Worker execution (one thread per worker) |

### Threading Rules

1. **Never block the UI thread** — expensive work goes to the thread pool
2. **Never block the IO thread** — it dispatches all IPC
3. **Prefer sequences to locks** — message passing over shared state
4. **No raw shared memory between threads** — pass ownership via `std::unique_ptr` or use `base::SequenceBound`
5. **Use weak pointers for prevent-dangling** — `base::WeakPtr` invalidates on sequence destruction

### Task Posting Patterns

```cpp
// Parallel task (any thread, no ordering guarantee)
base::ThreadPool::PostTask(FROM_HERE, base::BindOnce(&DoWork));

// Sequenced task (serial, but on any physical thread)
scoped_refptr<base::SequencedTaskRunner> runner =
    base::ThreadPool::CreateSequencedTaskRunner({base::TaskPriority::USER_VISIBLE});
runner->PostTask(FROM_HERE, base::BindOnce(&DoSequentialWork));

// Same physical thread (thread-affine, rare)
scoped_refptr<base::SingleThreadTaskRunner> runner =
    base::ThreadPool::CreateSingleThreadTaskRunner({base::MayBlock()});

// Cross-thread reply
base::ThreadPool::PostTaskAndReplyWithResult(
    FROM_HERE, {base::MayBlock()},
    base::BindOnce(&LoadFile, path),
    base::BindOnce(&OnFileLoaded));
```

### Synchronization Primitives (Used Sparingly)

| Primitive | Location | Use Case |
|-----------|----------|----------|
| `base::Lock` | `base/synchronization/lock.h` | Rare. Short critical sections only. |
| `base::WaitableEvent` | `base/synchronization/` | Thread signaling (startup/shutdown) |
| `base::AtomicFlag` | `base/synchronization/` | One-shot flags |
| `SEQUENCE_CHECKER` | `base/sequence_checker.h` | Debug-mode enforcement of single-sequence access |
| `THREAD_CHECKER` | `base/threading/thread_checker.h` | Debug-mode enforcement of single-thread access |

---

## Memory Management

### Allocator Architecture (3 Stages)

```
┌───────────────────────────┐     ┌──────────────────────────┐     ┌─────────────────────┐
│  malloc() / operator new  │ ──► │  Unified Allocator Shim  │ ──► │  PartitionAlloc      │
│  (symbol override)        │     │  (dispatch + hooks)      │     │  (actual allocator)  │
└───────────────────────────┘     └──────────────────────────┘     └─────────────────────┘
```

**Stage 1 — Symbol Override:** Chrome overrides `malloc`/`free`/`new`/`delete` at link time on all platforms (except iOS).

**Stage 2 — Allocator Shim:** Central dispatch layer. Enables:
- Memory-infra heap profiling hooks
- Security checks
- Routing to the correct allocator backend

**Stage 3 — PartitionAlloc:** Chrome's production allocator.

### PartitionAlloc

The primary allocator for the majority of Chrome's heap. Key design principles:

| Property | Description |
|----------|-------------|
| **Partitioning** | Separate heaps (partitions) for different object types — prevents type confusion exploits |
| **Slot spans** | Fixed-size slots within pages. Reduces fragmentation. |
| **MTE tagging** | ARM Memory Tagging Extension support. Objects are tagged; slots use raw addresses. |
| **Thread caching** | Per-thread free lists for hot allocation paths |
| **Quarantine** | Freed memory held temporarily before reuse (use-after-free mitigation) |
| **BackupRefPtr (BRP)** | Reference count in slot header. Detects dangling pointers at runtime. |

**Slot vs. Object distinction:**
- **Slot** = indivisible allocation unit internal to PartitionAlloc (represented as `uintptr_t`)
- **Object** = the memory handed to the application (represented as `void*`, MTE-tagged)
- Transition between worlds via `SlotStart::Checked()` and `SlotStart::ToObject()`

### Other Memory Subsystems

| Subsystem | Owner | Purpose |
|-----------|-------|---------|
| **Oilpan (BlinkGC)** | Blink/Renderer | Garbage-collected heap for DOM objects |
| **V8 Heap** | V8 engine | JavaScript object memory (generational GC) |
| **SharedMemory** | Cross-process | Explicit shared regions (GPU buffers, compositor surfaces) |
| **DiscardableMemory** | Various | Memory that the OS can reclaim under pressure (works differently on Windows vs. Unix) |
| **GPU Memory** | GPU process | GPU buffers, textures. Managed via command buffer or Vulkan Memory Allocator (VMA). |

### Smart Pointer Types (`base/memory/`)

| Type | File | Semantics |
|------|------|-----------|
| `raw_ptr<T>` | `raw_ptr.h` | Safer raw pointer (BackupRefPtr or MTE annotation). Default for class members. |
| `raw_ref<T>` | `raw_ref.h` | Non-null `raw_ptr` variant |
| `scoped_refptr<T>` | `scoped_refptr.h` | Reference-counted shared ownership (discouraged for new code) |
| `base::WeakPtr<T>` | `weak_ptr.h` | Non-owning pointer that auto-nulls when target destroyed |
| `std::unique_ptr<T>` | (stdlib) | Exclusive ownership. Preferred ownership model. |
| `base::SafeRef<T>` | `safe_ref.h` | Non-owning reference that CHECKs if target is destroyed |

### Memory Pressure System

```
OS memory pressure signal
        │
        ▼
┌─────────────────────────────────┐
│  base::MemoryPressureListener   │
│  (registered by subsystems)     │
└─────────────────────┬───────────┘
                      │
       ┌──────────────┼──────────────┐
       ▼              ▼              ▼
  MODERATE        CRITICAL       (platform-specific)
  (trim caches)   (release all   
                   reclaimable)   
```

Subsystems register for pressure callbacks and respond by:
- Dropping decoded image caches
- Trimming font caches
- Compacting V8 heap
- Discarding compositor tile memory
- Shrinking PartitionAlloc free lists

### Platform-Specific Memory Differences

| Behavior | Windows | Linux/CrOS/Android |
|----------|---------|-------------------|
| `malloc(N)` | Immediately commits N bytes (consumes swap) | Commits only on first touch (overcommit) |
| `free()` → OS return | Via `VirtualFree` only | `madvise(MADV_DONTNEED)` returns pages immediately |
| Discardable memory | `DiscardVirtualMemory()` — frees physical RAM but NOT swap | `madvise(MADV_FREE)` — truly returns all resources |
| OOM behavior | Other apps fail to allocate when commit limit reached | OOM killer selects process to terminate |
| Shared library dedup | Copy-on-write per process | Copy-on-write per process (similar) |

### Memory Safety Mechanisms

| Mechanism | Location | What It Does |
|-----------|----------|--------------|
| BackupRefPtr (BRP) | PartitionAlloc | Reference count detects dangling `raw_ptr<T>` at runtime |
| MTE (Memory Tagging) | PartitionAlloc (ARM) | Hardware tag mismatch traps on use-after-free/overflow |
| ASAN integration | `raw_ptr_asan_service.h` | AddressSanitizer hooks for debug builds |
| Quarantine | PartitionAlloc | Delays memory reuse to catch temporal bugs |
| `SEQUENCE_CHECKER` | Everywhere | Catches cross-sequence access (data races) in debug |
| PartitionAlloc hardening | Slots, cookies, canaries | Detects heap corruption, overflow into adjacent slots |

---

## GUI Compositor Pipeline

Chromium uses a multi-stage compositing architecture split across processes and threads. This is the critical path for getting pixels to the screen.

### The Three Compositors

| Compositor | Location | Thread | Responsibility |
|------------|----------|--------|----------------|
| **Blink Paint** | Renderer (Blink) | Main thread | Generates paint ops (display items) from DOM/style/layout |
| **cc (Chromium Compositor)** | Renderer | Compositor thread (impl-side) | Tiles, rasterizes, animates, produces compositor frames |
| **Viz (Display Compositor)** | GPU process | Viz thread | Aggregates frames from all surfaces, outputs to display |

### Frame Production Pipeline

```
┌─────────────────────────────────────────────────────────────────────┐
│  RENDERER PROCESS — Main Thread                                      │
│                                                                      │
│  DOM → Style → Layout → Paint (display items) → Commit to impl      │
│                                                                      │
│  [Blink owns: DOM tree, style, layout tree, paint artifacts]         │
└────────────────────────────────┬────────────────────────────────────┘
                                 │ BeginMainFrame / Commit
                                 ▼
┌─────────────────────────────────────────────────────────────────────┐
│  RENDERER PROCESS — Compositor Thread (impl-side)                    │
│                                                                      │
│  Layer Tree → Tile Manager → Raster (on worker threads) →            │
│  Draw → Produce CompositorFrame → Submit to Viz                      │
│                                                                      │
│  [cc owns: layer tree copy, tiles, raster scheduling, animations]    │
└────────────────────────────────┬────────────────────────────────────┘
                                 │ SubmitCompositorFrame (Mojo)
                                 ▼
┌─────────────────────────────────────────────────────────────────────┐
│  GPU PROCESS — Viz Display Compositor                                │
│                                                                      │
│  Surface Aggregation → Draw (Skia/GL/Vulkan) → Swap to Display       │
│                                                                      │
│  [Viz owns: surfaces, aggregated frame, output device, vsync]        │
└─────────────────────────────────────────────────────────────────────┘
```

### Compositor Scheduling

The compositor operates on a **BeginFrame** cadence (typically vsync-aligned):

```
Vsync Signal (from display)
    │
    ├──► BeginFrame → Compositor thread: tick animations, produce frame
    │
    ├──► BeginMainFrame → Main thread: run requestAnimationFrame,
    │                      style/layout/paint if dirty, commit to impl
    │
    └──► Deadline → Compositor must submit frame or skip
```

Key scheduler states (`cc/scheduler/`):
- `WAITING_FOR_BEGINFRAME` — idle, awaiting next vsync
- `BEGIN_IMPL_FRAME` — compositor thread producing frame
- `BEGIN_MAIN_FRAME` — main thread updating (may run in parallel with impl work)
- `COMMIT` — main thread state transferred to impl thread
- `DRAW` — compositor produces final frame for submission

### Layer Tree Architecture (`cc/trees/`, `cc/layers/`)

```
LayerTreeHost (main thread)          LayerTreeHostImpl (compositor thread)
    │                                        │
    ├── Layer (conceptual tree)              ├── LayerImpl (committed snapshot)
    │    ├── PictureLayer                    │    ├── PictureLayerImpl + tiles
    │    ├── SurfaceLayer                    │    ├── SurfaceLayerImpl
    │    ├── SolidColorLayer                 │    └── ...
    │    └── ...                             │
    │                                        ├── PropertyTrees (transform, clip,
    ├── PropertyTrees (main copy)            │    effect, scroll)
    │                                        │
    └── CommitState                          └── FrameData (output)
```

### Rasterization (Tile-Based)

| Concept | Description |
|---------|-------------|
| **Tile** | A fixed-size raster unit (typically 256×256 or 512×512 pixels) |
| **TileManager** | Prioritizes which tiles to raster based on viewport proximity |
| **RasterBufferProvider** | Abstracts raster backends (GPU raster, software raster, OOP raster) |
| **TaskGraphRunner** | Work-stealing thread pool for raster tasks (`cc/raster/`) |
| **GPU Raster** | Rasterizes paint ops directly into GPU textures (preferred) |
| **OOP Raster** | Out-of-process raster — paint ops sent to GPU process for rasterization |

### Viz (Visual Compositor Service)

| Directory | Role |
|-----------|------|
| `components/viz/client/` | Client-side frame submission API |
| `components/viz/common/` | Shared types (quads, surfaces, resource IDs) |
| `components/viz/host/` | Browser-side host for Viz service |
| `components/viz/service/display/` | Display compositor (aggregation, draw) |
| `components/viz/service/frame_sinks/` | Frame sink management |
| `components/viz/service/surfaces/` | Surface allocation and lifetime |
| `components/viz/service/gl/` | GL context management |
| `services/viz/` | Mojo service interfaces |

### GPU Command Buffer

The GPU process exposes a command buffer interface — a ring buffer of serialized GL/Vulkan commands:

```
Renderer Process                       GPU Process
┌──────────────┐                      ┌──────────────┐
│ GL commands  │  ──shared memory──►  │ Command      │
│ serialized   │                      │ Buffer       │
│ into ring    │                      │ Decoder      │
│ buffer       │                      │ (executes    │
│              │                      │  real GL)    │
└──────────────┘                      └──────────────┘
```

This provides:
- Security isolation (renderer never touches real GPU driver)
- Batching (many commands per IPC round-trip)
- Validation (malformed commands rejected before GPU driver)

### GUI Thread Interaction Matrix

| Operation | Threads Involved | Synchronization |
|-----------|-----------------|-----------------|
| DOM mutation → paint | Main only | Synchronous within main thread |
| Paint → commit | Main → Compositor | `Commit` synchronization point |
| Compositor animation tick | Compositor only | Runs at vsync independently |
| Raster tile | Compositor → Worker pool | TaskGraphRunner completion |
| Submit frame | Compositor → GPU (Viz) | Mojo async message |
| Display draw | Viz thread only | Waits for surface aggregation |
| Input event dispatch | IO → UI → Renderer Main | Mojo async + compositor can handle scroll |
| JavaScript animation | Main thread | `requestAnimationFrame` aligned to BeginMainFrame |

### Compositor-Driven (Off-Main-Thread) Operations

These run on the compositor thread WITHOUT blocking the main thread:

- CSS transforms/opacity animations
- Scroll (compositor-initiated)
- Pinch zoom
- Tile upload after raster completes
- Frame submission to Viz

This is why scroll remains smooth even when JavaScript is running heavy computation on the main thread.

---

## Source Tree Map

### Memory Management Files

```
base/allocator/                          — Allocator shim, routing logic
base/allocator/partition_allocator/      — PartitionAlloc source (the main allocator)
  src/partition_alloc/
    partition_alloc.h                    — Top-level include
    partition_root.h/.cc                 — Root of a partition (bucket configuration)
    partition_bucket.h/.cc               — Bucket (size class) management
    thread_cache.h/.cc                   — Per-thread allocation cache
    partition_page.h                     — Slot span / page management
    starscan/                            — *Scan: conservative GC scan for dangling ptrs
base/memory/
    raw_ptr.h                            — BackupRefPtr-based safe raw pointer
    raw_ref.h                            — Non-null raw_ptr variant
    scoped_refptr.h                      — Reference-counted pointer
    weak_ptr.h/.cc                       — Pointers that auto-null
    shared_memory_mapping.h/.cc          — Cross-process shared memory
    discardable_memory.h/.cc             — Reclaimable memory abstraction
    memory_pressure_listener.h/.cc       — OS memory pressure callbacks
    protected_memory.h/.cc               — Write-protected memory regions
    aligned_memory.h/.cc                 — Aligned allocation utilities
    page_size.h                          — System page size queries
    singleton.h                          — Singleton with leak-on-exit semantics
base/memory_coordinator/                 — Coordinated memory reduction
third_party/blink/renderer/platform/heap/ — Oilpan (Blink GC)
v8/src/heap/                             — V8 JavaScript GC heap
```

### Threading & Task Files

```
base/task/
    thread_pool.h/.cc                    — Public API for posting to the pool
    task_runner.h                         — Base task runner interface
    sequenced_task_runner.h              — Sequenced (virtual thread) runner
    single_thread_task_runner.h          — Physical thread runner
    task_traits.h                         — Priority, MayBlock, etc.
    post_job.h                            — Parallel job API
    thread_pool/
        thread_pool_impl.cc              — Pool implementation
        worker_thread.cc                 — Physical worker thread
        task_tracker.cc                  — Task lifecycle tracking
        job_task_source.cc              — Job (parallel work) source
    sequence_manager/
        sequence_manager_impl.cc         — Sequence scheduling
        task_queue.cc                    — Priority queue per sequence
        thread_controller.cc             — Physical thread lifecycle
base/threading/
    platform_thread.h/.cc               — OS thread abstraction
    thread.h/.cc                         — Named thread with message loop
    thread_local.h                       — Thread-local storage
    sequence_bound.h                    — Type-safe sequence-bound objects
    hang_watcher.h/.cc                  — Watchdog for hung threads
    thread_restrictions.h/.cc           — ScopedAllowBlocking etc.
base/synchronization/
    lock.h/.cc                           — Mutex
    waitable_event.h/.cc                — Cross-thread signaling
    atomic_flag.h                        — One-shot atomic flag
```

### Compositor & GUI Files

```
cc/                                      — Chromium Compositor (renderer-side)
  layers/                                — Layer type implementations
  trees/
    layer_tree_host.h/.cc               — Main-thread layer tree owner
    layer_tree_host_impl.h/.cc          — Compositor-thread impl
    property_tree.h/.cc                 — Transform/clip/effect/scroll trees
    proxy_main.h/.cc                    — Main→impl thread communication
    proxy_impl.h/.cc                    — Impl-side proxy
  scheduler/
    scheduler.h/.cc                     — BeginFrame / draw scheduling
    scheduler_state_machine.h/.cc       — State machine for frame production
  raster/
    task_graph_runner.h/.cc             — Work-stealing raster pool
    raster_buffer_provider.h/.cc        — Raster backend abstraction
  tiles/
    tile_manager.h/.cc                  — Tile prioritization and lifecycle
    picture_layer_tiling.h/.cc          — Tiling grid management
  paint/
    paint_op.h/.cc                      — Recorded paint operations
    display_item_list.h/.cc             — Paint artifact (list of ops)

ui/compositor/
    compositor.h/.cc                     — Browser-side compositor (Aura)
    layer.h/.cc                          — UI layer
    layer_animation_element.h/.cc        — UI animation

ui/aura/                                — Window system abstraction (Linux/CrOS/Windows)
ui/views/                               — Cross-platform widget toolkit
ui/gfx/                                 — Geometry, color, image utilities
ui/gl/                                  — OpenGL/EGL context management
ui/ozone/                               — Platform abstraction (Wayland, X11, DRM)

components/viz/
  service/display/                       — Display compositor
  service/frame_sinks/                   — Frame sink routing
  service/surfaces/                      — Surface lifetime management
  service/gl/                            — GL context for output
  client/                                — Client-side frame submission

gpu/command_buffer/
  client/                                — Client-side GL command serialization
  service/                               — GPU-side command execution
  common/                                — Shared types

content/browser/renderer_host/           — Browser↔Renderer coordination
content/browser/gpu/                     — Browser↔GPU coordination
content/renderer/                        — Renderer process bootstrap
content/gpu/                             — GPU process bootstrap
```

---

## Thread–Memory Interaction Points

These are the critical junctions where threading and memory management intersect. Mistakes here cause crashes, jank, or security vulnerabilities.

### 1. Compositor Commit (Main → Impl Transfer)

The main thread's layer tree state is copied to the compositor thread during **Commit**. This is a synchronization point — the main thread is blocked.

- **Memory concern:** Deep-copy of property trees, layer data. Must be fast.
- **Threading concern:** Main thread blocked until commit completes.
- **Files:** `cc/trees/proxy_main.cc`, `cc/trees/commit_state.cc`

### 2. Tile Raster (Worker Threads)

Rasterization runs on thread pool workers. Each tile produces a GPU texture or software bitmap.

- **Memory concern:** Tile memory is the largest single consumer in the renderer. TileManager applies budgets.
- **Threading concern:** Worker threads write raster results; compositor thread reads. Completion signaled via TaskGraphRunner.
- **Files:** `cc/tiles/tile_manager.cc`, `cc/raster/task_graph_runner.cc`

### 3. Shared Memory for GPU Command Buffers

GL commands are serialized into shared memory regions that the GPU process reads.

- **Memory concern:** Ring buffer sized for throughput. Overflow = stall.
- **Threading concern:** Producer (renderer compositor thread) and consumer (GPU decoder thread) coordinate via atomic put/get pointers.
- **Files:** `gpu/command_buffer/common/cmd_buffer_common.h`, `gpu/command_buffer/service/`

### 4. Cross-Process Surface Submission

CompositorFrames are submitted from renderer → GPU (Viz) via Mojo IPC.

- **Memory concern:** Frames reference GPU resources by ID. Resources must stay alive until Viz acks.
- **Threading concern:** Async Mojo message; Viz aggregates on its own thread.
- **Files:** `components/viz/service/frame_sinks/`, `cc/trees/layer_tree_host_impl.cc`

### 5. Memory Pressure → Cache Eviction

When the OS signals memory pressure, multiple threads respond simultaneously.

- **Memory concern:** All caches drop (tiles, decoded images, font, V8 heap). Risk of jank on re-creation.
- **Threading concern:** Pressure callbacks fire on the thread that registered them. Must be thread-safe.
- **Files:** `base/memory/memory_pressure_listener.cc`, `cc/tiles/tile_manager.cc`

### 6. PartitionAlloc Thread Cache

Each thread has a local free-list cache to avoid lock contention on hot paths.

- **Memory concern:** Thread caches consume per-thread memory. Periodic purging required.
- **Threading concern:** Only the owning thread touches its cache (no lock needed). Purge is cooperative.
- **Files:** `base/allocator/partition_allocator/src/partition_alloc/thread_cache.h`

### 7. Oilpan (Blink GC) and Main Thread

Blink's garbage collector runs on the main thread during idle time or under pressure.

- **Memory concern:** Pauses main thread. Incremental/concurrent marking mitigates.
- **Threading concern:** GC must not run while JS/DOM is mutating. Coordinates with V8's GC.
- **Files:** `third_party/blink/renderer/platform/heap/`

---

## Key Files Reference

### Memory Management — Critical Path

| File | Purpose |
|------|---------|
| `base/allocator/partition_allocator/src/partition_alloc/partition_root.h` | Top-level allocator config |
| `base/allocator/partition_alloc_features.h` | Feature flags for allocator |
| `base/memory/raw_ptr.h` | Safe pointer type (BackupRefPtr) |
| `base/memory/weak_ptr.h` | Prevent-dangling weak reference |
| `base/memory/memory_pressure_listener.h` | OS pressure signal handling |
| `base/memory/discardable_memory.h` | Platform-aware reclaimable memory |
| `base/memory/shared_memory_mapping.h` | Cross-process shared regions |

### Threading — Critical Path

| File | Purpose |
|------|---------|
| `base/task/thread_pool.h` | Public task posting API |
| `base/task/sequenced_task_runner.h` | Virtual thread interface |
| `base/threading/sequence_bound.h` | Sequence-owned objects |
| `base/sequence_checker.h` | Debug single-sequence enforcement |
| `base/threading/thread.h` | Named physical thread |
| `base/threading/hang_watcher.h` | Hang detection for all threads |
| `base/synchronization/lock.h` | Mutex (use sparingly) |

### GUI Compositor — Critical Path

| File | Purpose |
|------|---------|
| `cc/trees/layer_tree_host.h` | Main-thread tree owner |
| `cc/trees/layer_tree_host_impl.h` | Compositor-thread tree |
| `cc/scheduler/scheduler.h` | Frame production scheduling |
| `cc/tiles/tile_manager.h` | Tile memory budgeting |
| `cc/raster/task_graph_runner.h` | Parallel raster dispatch |
| `ui/compositor/compositor.h` | Browser-side compositor |
| `components/viz/service/display/display.h` | Output to screen |
| `gpu/command_buffer/service/gles2_cmd_decoder.h` | GL command execution |

---

## Summary

Chromium's architecture enforces memory safety and UI responsiveness through structural separation:

1. **Processes isolate security domains** — renderer can't touch browser heap
2. **Sequences isolate data ownership** — most objects live on exactly one sequence
3. **The compositor thread runs independently of JavaScript** — scroll/animation never jank from JS
4. **PartitionAlloc partitions isolate type domains** — prevents type confusion exploits
5. **Memory pressure is cooperative** — each subsystem registered for its own cleanup responsibility

The threading model's preference for sequences over locks, and the compositor's ability to produce frames independently of the main thread, are the two design decisions most responsible for Chrome's UI responsiveness despite the complexity of the platform.

---

## HAZARD: GUI Threading Lock via Git I/O During Commits

**Severity: HIGH — System freeze / multi-second UI hang**  
**Observed evidence: Stale lock files, 13GB orphan temp pack, fragmented object store**

### The Problem

The Chromium source (505,317 files, 5.5 GB) is **tracked in the outer git repository** and NOT excluded by `.gitignore`. This creates catastrophic I/O pressure during any git operation that touches the index or object store.

### Repository Metrics (Observed)

| Metric | Value | Problem Threshold |
|--------|-------|-------------------|
| Tracked files (total repo) | **1,060,023** | > 100,000 degrades git |
| Of which, Chromium source | **505,317** | 48% of entire index |
| `.git/index` file size | **179 MB** | > 10 MB is problematic |
| `.git/objects/pack/` | **31 GB** across 15 packfiles | > 5 GB needs maintenance |
| Loose objects | **236,747** (8.5 GB) | > 10,000 triggers auto-gc |
| Largest single packfile | **13 GB** | Exceeds memory-map limits |
| Orphan temp packfile | **13 GB** (`tmp_pack_BUHnF2`) | Failed gc — never cleaned |
| `core.compression` | **9** (maximum zlib) | Maximizes CPU time per object |
| Stale `shallow.lock` | Since **Aug 3** (10 days) | Blocks shallow operations |
| Stale `gc.log.lock` | Since **Aug 12** | Blocks garbage collection |
| `[submodule] active = .` | All submodules active | Multiplies operations |

### How This Causes GUI Thread Lock

**Scenario 1: `git status` / `git diff` (IDE, commit prep)**

```
1. git must stat() all 1,060,023 tracked files against the 179 MB index
2. On a filesystem with 505K+ files, this generates massive inode lookups
3. If the disk is HDD or USB-backed (this distro supports USB swap):
   → Inode table seeks dominate → 30-120 second wall time
4. During this time:
   → IDE UI thread blocked waiting for git subprocess
   → Terminal session blocked
   → If an inotify watcher is recursively watching the tree:
     → The inotify reader thread holds its Lock while processing events
     → Other threads waiting on that Lock stall
```

**Scenario 2: `git add` / `git commit`**

```
1. git add must hash every modified file (SHA-1, reading full content)
2. git commit writes tree objects for all 1,060,023 entries
3. With core.compression=9:
   → Each blob is deflated at maximum zlib level
   → A single 1 MB file takes ~100ms CPU vs ~10ms at compression=1
   → Across 505K files of Chromium source: hours of CPU time for initial commit
4. Writing objects fills the loose object store:
   → 236K loose objects already present → filesystem directory contention
   → ext4 htree lookups degrade past ~32K entries per directory
5. Auto-gc triggers when loose objects > 6700 (default):
   → gc.log.lock is STALE → gc cannot acquire lock → retries in loop
   → OR gc starts repacking 31 GB of packs → multi-GB memory allocation
   → System enters swap → USB swap (if active) → 35 MB/s throughput
   → Entire system becomes unresponsive for minutes
```

**Scenario 3: `git push` to remote**

```
1. git must compute delta compression across all objects
2. Pack negotiation with remote involves enumerating all refs
3. With 15 existing packs (largest 13 GB) and core.compression=9:
   → Delta computation is O(n²) within pack window
   → CPU pegged at 100% for the compression pass
   → Memory usage: git thin-pack can consume 2-8 GB RAM
4. If system RAM is limited → hits USB swap → I/O wait compounds
5. Network I/O of multi-GB push is BLOCKING (no async) in git
```

**Scenario 4: `git gc` (manual or auto)**

```
1. gc must repack all objects into a single packfile
2. Current state: 31 GB in packs + 8.5 GB loose = ~40 GB to process
3. The 13 GB orphan temp pack indicates a PREVIOUS gc that:
   → Ran out of memory, OR
   → Was killed by OOM, OR
   → The system froze and the user force-rebooted
4. gc.log.lock is stale → indicates gc crashed without cleanup
5. Next gc attempt will either:
   → Fail immediately (stale lock) — keeps retrying
   → OR (if lock removed) attempt to repack 40 GB → system freeze
```

### Compositor Impact Chain

When git I/O saturates the disk, the compositor pipeline is affected:

```
┌──────────────────────────────────────────────────────────────────┐
│  Git commit/push/gc (heavy disk I/O)                              │
│  → Saturates I/O scheduler                                        │
│  → Disk queue depth reaches device limit                          │
└──────────────────────────┬───────────────────────────────────────┘
                           │
┌──────────────────────────┴───────────────────────────────────────┐
│  Renderer Compositor: tile raster writes to /tmp or shared memory │
│  → mmap() page faults stall on I/O queue                         │
│  → Tile raster completion delayed                                 │
│  → TileManager has no tiles ready for next frame                  │
└──────────────────────────┬───────────────────────────────────────┘
                           │
┌──────────────────────────┴───────────────────────────────────────┐
│  Viz Display Compositor: waiting for CompositorFrame               │
│  → No frame submitted in time for vsync deadline                  │
│  → Frame dropped                                                  │
│  → Multiple frames dropped → visible freeze                       │
└──────────────────────────┬───────────────────────────────────────┘
                           │
┌──────────────────────────┴───────────────────────────────────────┐
│  Memory Pressure cascade:                                         │
│  → git gc allocates multi-GB → triggers memory pressure signal   │
│  → Compositor discards tile cache → MUST re-raster                │
│  → Re-raster needs disk I/O → disk still saturated               │
│  → System enters swap death spiral                                │
└──────────────────────────────────────────────────────────────────┘
```

### Blocking Commit Amplification

The cc compositor has a **blocking commit** mechanism (`WaitForCommitCompletion()` in `cc/trees/layer_tree_host.cc`). During a blocking commit:

1. The main thread calls `commit_completion_event_->Wait()`
2. This **blocks the main thread entirely** until the impl thread finishes
3. If the impl thread is waiting on tile raster completion
4. And tile raster is stalled on I/O (because git saturated the disk)
5. Then the **main thread is frozen** — no DOM updates, no JS, no UI interaction

The `block_on_next_commit_` flag in `proxy_main.cc` (line 829) forces this blocking mode. It's set during navigation, resize, and other critical operations. If disk I/O is saturated by git at that moment, the entire tab freezes.

### Inotify Watch Exhaustion

On Linux, `inotify` has a system-wide limit (default: 8192 watches, configurable via `/proc/sys/fs/inotify/max_user_watches`).

The Chromium source alone has:
- 505,317 files across ~4,000+ directories
- A recursive `FilePathWatcher` on a parent directory would need one watch per subdirectory
- IDEs (IntelliJ, VSCode) that recursively watch the project directory will consume thousands of watches

When watches are exhausted:
- `inotify_add_watch()` returns `ENOSPC`
- The `FilePathWatcherImpl` fires an **error callback** and cancels all watches
- File change detection stops silently
- IDE features dependent on file watching degrade

### Stale Lock Files (Current State)

| Lock File | Created | Impact |
|-----------|---------|--------|
| `.git/shallow.lock` | Aug 3 (10 days stale) | Blocks `git fetch --depth`, `git fetch --unshallow` |
| `.git/gc.log.lock` | Aug 12 (1 day stale) | Blocks all garbage collection attempts |

These locks indicate **previous operations that crashed or were killed**. They will not self-heal and will cause subsequent git operations to fail or retry in loops.

### Recommended Mitigations

#### Immediate (resolve current state):

```bash
# 1. Remove stale lock files
rm -f .git/shallow.lock .git/gc.log.lock

# 2. Remove orphan temp pack (13 GB wasted disk)
rm -f .git/objects/pack/tmp_pack_BUHnF2

# 3. Add Chromium source to .gitignore (CRITICAL)
echo "userland/chromium/chromium-src/" >> .gitignore

# 4. Remove Chromium from git tracking (keeps files, removes from index)
git rm -r --cached userland/chromium/chromium-src/
# WARNING: This will take significant time with 505K files.
# Expect 10-30 minutes. Do NOT interrupt.

# 5. Commit the .gitignore change
git add .gitignore
git commit -m "Exclude Chromium source from git tracking (505K files)"
```

#### Git Configuration (reduce I/O impact):

```bash
# Reduce compression (9 is overkill, 1 is much faster with minimal size increase)
git config core.compression 1

# Enable filesystem monitor (reduces stat() calls dramatically)
git config core.fsmonitor true

# Increase pack size limit to avoid auto-gc during operations
git config gc.auto 0           # Disable auto-gc entirely
git config pack.packSizeLimit 0 # No pack size limit

# Enable split index (faster index operations for large repos)
git config core.splitIndex true

# Reduce pack delta window for faster push/fetch
git config pack.window 5
git config pack.depth 25

# After removing Chromium from tracking, repack everything
git gc --aggressive --prune=now
# OR safer: git repack -a -d --depth=50 --window=50
```

#### System-Level (prevent future freezes):

```bash
# Increase inotify watch limit (for IDE + file watchers)
echo 524288 | sudo tee /proc/sys/fs/inotify/max_user_watches
# Make permanent:
echo "fs.inotify.max_user_watches=524288" | sudo tee -a /etc/sysctl.d/99-inotify.conf

# Increase vm.dirty_ratio to batch more writes before flush
echo 40 | sudo tee /proc/sys/vm/dirty_ratio
echo 10 | sudo tee /proc/sys/vm/dirty_background_ratio
```

#### Architecture (long-term):

The correct approach for Chromium source in this distribution:

1. **Git-ignore the source tree** — it's a fetch-on-demand artifact, not authored code
2. **Track only** `fetch-chromium.sh`, `Makefile`, `README.md`, and `STRUCTURE.md`
3. **Use `GALACTIC_CHERRY_SOURCE_INFO`** as the provenance record instead of git history
4. The fetch script already records commit hash — no need for git to track 505K files

### Root Cause Summary

| Factor | Contribution to Freeze |
|--------|----------------------|
| 505K Chromium files tracked in git | Index operations take minutes, not seconds |
| `core.compression=9` | 10x CPU overhead per object vs. default |
| 179 MB git index | Every `git status` must parse/mmap this entirely |
| 15 fragmented packfiles (31 GB) | Pack lookups hit multiple files; no single-pack optimization |
| 236K loose objects | Auto-gc pressure; directory hash-table contention |
| 13 GB orphan temp pack | Wasted I/O bandwidth; evidence of past OOM |
| Stale `.lock` files | Operations retry/fail indefinitely |
| `[submodule] active = .` | Git scans all submodule paths on every operation |
| No `core.fsmonitor` | Every operation does full 1M-file stat() scan |

---

## Applied Fixes — Galactic Cherry Marvell Edition 98

The following source modifications address the GUI threading lock and system freeze issues at the Chromium code level. These changes ensure that I/O pressure from external processes (git, file indexing, swap thrash) cannot cause unbounded blocking of the GUI thread.

### Fix 1: CompletionEvent Timeout (`cc/base/completion_event.h`)

**Problem:** `CompletionEvent::Wait()` blocked the main thread indefinitely. If the impl thread stalled on I/O, the entire GUI froze.

**Fix:** `Wait()` now uses a timed wait with a 4-second default ceiling. When I/O pressure is detected (via PSI), the timeout shortens to 500ms. If the timeout fires, the main thread is released and a warning is logged.

| Condition | Timeout |
|-----------|---------|
| Normal | 4000ms (more than enough for any frame commit) |
| I/O pressure detected | 500ms (system is already saturated) |
| Custom (per-event) | Configurable via `SetTimeout()` |

**File:** `cc/base/completion_event.h`

### Fix 2: I/O Pressure Monitor (`cc/base/io_pressure_monitor.{h,cc}`)

**Problem:** No mechanism to detect system-wide I/O saturation before deciding to block.

**Fix:** New singleton reads `/proc/pressure/io` (Linux PSI) every 500ms. Reports whether the system is under I/O pressure. Thread-safe, zero-allocation, minimal overhead (one cached file read per 500ms).

| Threshold | Trigger |
|-----------|---------|
| `some avg10 > 25.0` | 25% of last 10s had at least one I/O-stalled task |
| `full avg10 > 5.0` | 5% of last 10s was a complete I/O stall |

**Files:** `cc/base/io_pressure_monitor.h`, `cc/base/io_pressure_monitor.cc`, `cc/base/BUILD.gn`

### Fix 3: WaitForCommitCompletion Timeout (`cc/trees/layer_tree_host.cc`)

**Problem:** `LayerTreeHost::WaitForCommitCompletion()` blocked forever waiting for the impl thread. If tile raster was stalled on I/O, the main thread (and all UI) froze.

**Fix:** After `Wait()` returns (with or without timeout), the code checks `DidTimeOut()`. On timeout, the completion event pointer is NOT nulled — the impl thread will still signal when it finishes. The main thread continues, and the next frame will see the signal.

**File:** `cc/trees/layer_tree_host.cc`

### Fix 4: Inotify Lock Inversion (`base/files/file_path_watcher_inotify.cc`)

**Problem:** `InotifyReader::AddWatch()` held a global `Lock` while calling `inotify_add_watch()` — a blocking syscall. Under I/O pressure, this syscall blocks for seconds. During that time, the inotify event reader thread cannot dispatch events (it also needs the lock), causing a cascade where GUI update callbacks stall.

**Fix:** The blocking `inotify_add_watch()` syscall is now performed OUTSIDE the lock. The lock is held only for the brief map update. Similarly, `RemoveWatch()` removes from the map first, then calls `inotify_rm_watch()` outside the lock.

**Before:**
```
lock → inotify_add_watch(BLOCKS) → update map → unlock
```

**After:**
```
inotify_add_watch(BLOCKS) → lock → update map → unlock
```

**File:** `base/files/file_path_watcher_inotify.cc`

### Fix 5: Skip .git in Recursive Watchers (`base/files/file_path_watcher_inotify.cc`)

**Problem:** Recursive inotify watchers would add watches to `.git/objects/` subdirectories. A large git repo can have hundreds of thousands of these. During git gc/repack, massive inotify event floods are generated, saturating the reader thread.

**Fix:** `UpdateRecursiveWatchesForPath()` now skips any directory named `.git` and all paths containing `/.git/`. These are internal git structures and should never trigger application-level file notifications.

**File:** `base/files/file_path_watcher_inotify.cc`

### Fix 6: Skip .git in DevTools Indexer (`chrome/browser/devtools/devtools_file_system_indexer.cc`)

**Problem:** DevTools file system indexing would traverse into `.git/objects/` when indexing a workspace. On large repos, this meant stat()ing and reading hundreds of thousands of binary git objects — saturating the disk and triggering memory pressure.

**Fix:** `CollectFilesToIndex()` now excludes directories named `.git`, `node_modules`, and `.hg` by default, in addition to user-specified exclusions.

**File:** `chrome/browser/devtools/devtools_file_system_indexer.cc`

### Fix 7: DetachInputDelegate Timeout (`cc/trees/proxy_main.cc`)

**Problem:** `DetachInputDelegateAndRenderFrameObserver()` used an unbounded `Wait()`. Called during navigation, if the impl thread was I/O-blocked, the browser would freeze during page transitions.

**Fix:** Uses a 2-second timeout. On timeout, logs a warning and continues. The detach will still complete asynchronously.

**File:** `cc/trees/proxy_main.cc`

### Summary of Changed Files

| File | Change |
|------|--------|
| `cc/base/completion_event.h` | Timed wait + I/O pressure awareness |
| `cc/base/io_pressure_monitor.h` | NEW: I/O pressure monitor header |
| `cc/base/io_pressure_monitor.cc` | NEW: I/O pressure monitor (Linux PSI) |
| `cc/base/BUILD.gn` | Added new source files |
| `cc/trees/layer_tree_host.cc` | Timeout-aware commit completion |
| `cc/trees/proxy_main.cc` | Timeout documentation + DetachInputDelegate fix |
| `base/files/file_path_watcher_inotify.cc` | Lock inversion fix + .git skip |
| `chrome/browser/devtools/devtools_file_system_indexer.cc` | .git exclusion |

---

*Document generated for Galactic Cherry Marvell Edition 98. Source commit 5006852cd6.*
