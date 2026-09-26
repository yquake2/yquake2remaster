# Technical Specification: Multithreading in the `ref_gl4` Renderer

## 1. Overview & Architecture
The `ref_gl4` renderer introduces a lightweight, zero-overhead **fork-join parallel task model** (`gl4_threads.c`) designed specifically to offload CPU-bound data parallelization in Quake II. Unlike traditional game engines with permanent background task scheduler threads, `ref_gl4` employs synchronous worker pool execution per frame/task to avoid lock contention and thread synchronization overhead while eliminating micro-stuttering on multi-core processors.

---

## 2. Core Components & Implementation Details

### A. Hardware Thread Detection (`GL4_NumHWThreads`)
* **POSIX / Unix:** Queries online processors using `sysconf(_SC_NPROCESSORS_ONLN)`.
* **Windows:** Queries system processor info using Win32 API `GetSystemInfo`.
* **Capping:** Bounded by `#define GL4_MAX_THREADS 8` to prevent over-subscription on high-core systems.

### B. Platform Abstraction Layer (`GL4_ParallelTasks`)
* **POSIX:** Uses standard `pthread_create` and `pthread_join`.
* **Windows:** Uses Win32 `CreateThread` and `WaitForMultipleObjects`.
* **Fallback:** If hardware threads $\le 1$ or workload size is below `min_rows_per_task`, execution automatically falls back to single-threaded synchronous processing on the calling thread, eliminating threading overhead when unnecessary.

### C. Task Workloads Paralleled
1. **Particle Vertex Processing (`gl4_main.c` $\to$ `GL4_DrawParticles`):**
   * **Workload:** Iterates over active game particles (`numParticles`), transforming their origins, calculating view distances, mapping point sizes, and doing 8-to-24-bit color table lookups (`d_8to24table`).
   * **Chunking:** Dynamically splits particles into balanced chunks (`min_rows_per_task = 1024`).
2. **Palette Expansion for Raw Video/Cinematics (`gl4_draw.c` $\to$ `GL4_Draw_StretchRaw`):**
   * **Workload:** Expands 8-bit paletted pixel rows into 32-bit RGBA pixel buffers via `GL4_PaletteExpandWorker`.
   * **Chunking:** Dynamically splits rows into balanced chunks (`min_rows_per_task = 32`).

---

## 3. Performance Benefits

* **Elimination of Micro-Stuttering:** Heavy CPU-bound vertex transformations (such as massive particle explosions or spells) and raw video frame decoding are distributed across multiple CPU cores rather than stalling the main render thread.
* **Predictable Frame Times:** By breaking large arrays into parallel chunks, frame preparation time remains stable and below the vsync threshold.
* **Low Overhead / Zero Deadlock Design:** The fork-join pattern ensures workers finish and join before rendering commands are dispatched to OpenGL, avoiding complex mutex locking, queue contention, or race conditions.
