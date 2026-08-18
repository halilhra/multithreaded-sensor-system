# System Architecture

## Overview

The Multithreaded Sensor Data Processing System is a producer-consumer architecture where multiple sensor threads produce data independently, and a single processor thread consumes and validates the data.

## Thread Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                           MAIN THREAD                               │
│                                                                     │
│  1. Initialize components                                          │
│  2. Start DataProcessor thread                                     │
│  3. Start 5 Sensor threads                                        │
│  4. Wait for shutdown signal (Ctrl+C or timer)                     │
│  5. Graceful shutdown sequence                                     │
└─────────────────────────────────────────────────────────────────────┘
         │
         │ spawns 6 threads
         ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│ Temperature  │ │   Pressure   │ │     GPS      │ │     IMU      │
│ Thread       │ │   Thread     │ │   Thread     │ │   Thread     │
│ (500ms)      │ │   (1000ms)   │ │   (2000ms)   │ │   (100ms)    │
└──────┬───────┘ └──────┬───────┘ └──────┬───────┘ └──────┬───────┘
       │                │                │                │
       └────────────────┼────────────────┼────────────────┘
                        │                │
                ┌───────┴────────┐       │
                │  ThreadSafe    │◄──────┘
┌──────────┐    │    Queue       │
│  Radar   │───►│  (shared)      │
│  Thread  │    └───────┬────────┘
│  (250ms) │            │
└──────────┘            ▼
                ┌──────────────┐
                │DataProcessor │
                │   Thread     │
                │  (Consumer)  │
                └──────┬───────┘
                       │
               ┌───────┴───────┐
               ▼               ▼
          Validation       Logger
               │               │
               ▼               ├──► Console (colored)
         Fault/Timeout         └──► CSV File
          Detection
```

### Thread Count: 7 Total
| Thread | Role | Frequency |
|--------|------|-----------|
| Main | Orchestration, signal handling | - |
| Temperature | Data production | 500ms |
| Pressure | Data production | 1000ms |
| GPS | Data production | 2000ms |
| IMU | Data production | 100ms |
| Radar | Data production | 250ms |
| DataProcessor | Data consumption + validation | Continuous |

## Producer-Consumer Architecture

### Why Producer-Consumer?

In real systems, sensors produce data at different rates. Processing all data in the same thread that generates it would mean:
1. Fast sensors block waiting for slow processing
2. No separation between data acquisition and data processing
3. Cannot scale processing independently of data production

The producer-consumer pattern solves this by using a **shared queue** as a buffer:
- **Producers** (sensors) only care about generating data and pushing it to the queue
- **Consumer** (processor) only cares about pulling data and validating it
- The queue absorbs rate differences between producers and consumer

### Data Flow

```
Sensor::run()                    DataProcessor::run()
┌─────────────────┐             ┌─────────────────────┐
│ while(running) { │             │ while(running) {    │
│   data = generate│             │   data = queue.pop()│
│   queue.push(data)──────────►  │   validate(data)    │
│   sleep(interval)│             │   log(data)         │
│ }                │             │   updateStats()     │
└─────────────────┘             │ }                   │
                                └─────────────────────┘
```

## Queue Design

The `ThreadSafeQueue<T>` is a template class using:

```
┌──────────────────────────────────────────────────────┐
│                  ThreadSafeQueue<T>                    │
│                                                       │
│  ┌─────────────┐                                     │
│  │ std::queue   │  ← Underlying FIFO data storage    │
│  └─────────────┘                                     │
│                                                       │
│  ┌─────────────┐                                     │
│  │ std::mutex   │  ← Prevents concurrent access      │
│  └─────────────┘                                     │
│                                                       │
│  ┌─────────────────────┐                             │
│  │ condition_variable   │  ← Efficient waiting       │
│  └─────────────────────┘     (no busy-wait/polling)  │
│                                                       │
│  ┌─────────────────┐                                 │
│  │ atomic<bool>     │  ← Shutdown signal             │
│  │ shutdown_        │     (lock-free)                 │
│  └─────────────────┘                                 │
│                                                       │
│  Methods:                                            │
│    push(item)     → lock_guard + notify_one          │
│    pop(timeout)   → unique_lock + wait_for           │
│    tryPop()       → lock_guard (non-blocking)        │
│    shutdown()     → atomic + notify_all              │
└──────────────────────────────────────────────────────┘
```

### Why lock_guard for push?
`lock_guard` is simpler and automatically releases the lock when it goes out of scope (RAII). It cannot be manually locked/unlocked, which prevents misuse.

### Why unique_lock for pop?
`condition_variable::wait_for()` requires a lock that can be temporarily released while waiting. `unique_lock` supports this; `lock_guard` does not.

## Synchronization

### Race Condition Prevention

Without mutex protection, two threads pushing data simultaneously could corrupt the queue's internal state:

```
Thread 1: queue.push()  →  reads size=5, writes to position 5
Thread 2: queue.push()  →  reads size=5, writes to position 5  ← OVERWRITE!
```

With mutex:
```
Thread 1: lock → queue.push() → unlock
Thread 2: waits... → lock → queue.push() → unlock
```

### Deadlock Prevention

This system avoids deadlock by:
1. **Single mutex per resource** — Queue has one mutex, Logger has two separate mutexes (console, file)
2. **No nested locking** — No thread holds two locks simultaneously
3. **RAII locks** — `lock_guard` and `unique_lock` automatically release on scope exit (even on exceptions)
4. **Timeout on pop** — Consumer never blocks indefinitely

## Error Handling

### Validation Ranges

Each sensor type has two ranges:
```
         ERROR           WARNING         OK          WARNING          ERROR
  ◄──────────────┤──────────────┤───────────────┤──────────────┤──────────────►
              validMin       normalMin       normalMax       validMax
```

| Sensor | Normal Range | Valid Range |
|--------|-------------|-------------|
| Temperature | 20 — 40 °C | -50 — 100 °C |
| Pressure | 950 — 1050 hPa | 800 — 1200 hPa |
| GPS | 39 — 41 ° | -90 — 90 ° |
| IMU | -2 — 2 g | -20 — 20 g |
| Radar | 0.5 — 1000 m | 0 — 5000 m |

### Fault Simulation

Each sensor cycle has:
- **3% probability** of generating out-of-range data (sensor malfunction)
- **1% probability** of simulated disconnection (2-6 seconds)

### Timeout Detection

The DataProcessor tracks the last data timestamp for each sensor. If no data arrives for **5 seconds**, a timeout warning is issued. When data resumes, a recovery message is logged.

## Shutdown Mechanism

```
  Ctrl+C or Timer
       │
       ▼
  signalHandler()
  g_running = false ──── atomic<bool>, no lock needed
       │
       ▼
  Main thread wakes up
       │
       ▼
  manager.stopAll()
  ├── sensor.running_ = false
  ├── sensor.thread_.join() ── waits for sensor loop to finish
  └── logs "sensor stopped"
       │
       ▼
  queue.shutdown()
  ├── shutdown_ = true
  └── condVar.notify_all() ── wakes consumer if waiting
       │
       ▼
  processor.stop()
  ├── running_ = false
  ├── drains remaining queue items
  └── thread_.join()
       │
       ▼
  processor.printStatistics()
       │
       ▼
  logger.closeCSV()
       │
       ▼
  main() returns 0
  └── destructors clean up resources (RAII)
```

### Why This Order Matters

1. **Sensors stop first** — No new data enters the queue
2. **Queue shutdown** — Wakes consumer if it's waiting for data
3. **Processor stops** — Processes remaining items, then stops
4. **Logger closes** — Ensures all data is flushed to CSV

If we stopped the processor first, data in the queue would be lost.
If we shut down the queue first, sensors might crash trying to push data.
