# Multithreaded Sensor Data Processing System

A high-performance, multithreaded sensor data processing system built in **C++17**.

Simulates multiple sensors running on independent threads, producing data into a thread-safe queue. A consumer thread processes the data, performs validation, detects faults and timeouts, and logs results to both console and CSV.

---

## Features

- **5 Independent Sensor Threads** — Temperature, Pressure, GPS, IMU, Radar
- **Thread-Safe Queue** — Lock-free producer-consumer communication with `std::mutex` and `std::condition_variable`
- **Real-time Data Validation** — Configurable normal/valid ranges per sensor type
- **Fault Simulation** — Random sensor malfunctions, disconnections, and out-of-range values
- **Timeout Detection** — Automatic detection and reporting of sensor communication failures
- **Recovery Detection** — Automatic detection when a timed-out sensor comes back online
- **Thread-Safe Logging** — Concurrent console (colored) and CSV file output
- **Graceful Shutdown** — Clean `Ctrl+C` handling with proper thread joining
- **Performance Metrics** — Real-time status reports and final statistics summary
- **Unit Tests** — Comprehensive GoogleTest suite for queue, sensors, and processor

---

## Architecture

```
                         MAIN
                          │
          ┌───────────────┼────────────────┐
          │               │                │
          ▼               ▼                ▼
   Temperature        Pressure           GPS        IMU        Radar
   Thread (500ms)     Thread (1s)        Thread     Thread     Thread
          │               │              (2s)       (100ms)    (250ms)
          └───────────────┼────────────────┘
                          │
                          ▼
                 Thread-Safe Queue
                 (mutex + condition_variable)
                          │
                          ▼
                 Data Processor
                 Consumer Thread
                          │
              ┌───────────┴───────────┐
              ▼                       ▼
        Data Validation             Logger
              │                       │
              ▼                       ▼
        Fault Detection          sensor_log.csv
        Timeout Detection        Console (colored)
```

### Data Flow

1. **Sensors (Producers)** generate simulated data on independent threads
2. Data is pushed into a shared **ThreadSafeQueue** protected by mutex
3. **DataProcessor (Consumer)** pops data from the queue using condition variables
4. Each data point is **validated** against configurable ranges
5. Results are **logged** to console (warnings/errors) and CSV (all data)
6. **Timeouts** are detected when sensors stop producing data
7. **Statistics** are collected and reported at shutdown

---

## Technologies

| Technology | Purpose |
|-----------|---------|
| C++17 | Core language (threads, optional, filesystem, chrono) |
| CMake 3.16+ | Cross-platform build system |
| GoogleTest 1.14 | Unit testing framework |
| std::thread | Multithreading |
| std::mutex | Synchronization |
| std::condition_variable | Efficient waiting |
| std::atomic | Lock-free shutdown flags |
| MSVC / GCC / Clang | Compiler support |

---

## Requirements

- **CMake** 3.16 or higher
- **C++17** compatible compiler (MSVC 2019+, GCC 7+, Clang 5+)
- **Git** (for cloning and version control)

### Windows
- Visual Studio 2019 or later (with C++ Desktop Development workload)

### Linux
```bash
sudo apt install cmake g++ git
```

---

## Installation

```bash
git clone https://github.com/yourusername/multithreaded-sensor-system.git
cd multithreaded-sensor-system
```

---

## Usage

### Build & Run (Windows)

```cmd
build.bat          # Build and run
build.bat test     # Run unit tests
build.bat clean    # Clean build directory
build.bat all      # Clean + Build + Test + Run
```

### Build & Run (Linux / Manual)

```bash
cmake -S . -B build
cmake --build build
./build/sensor_system        # Run for 30 seconds (default)
./build/sensor_system 60     # Run for 60 seconds
```

Press `Ctrl+C` to stop early with graceful shutdown.

### Run Tests

```bash
ctest --test-dir build --output-on-failure
```

---

## Example Output

```
  ╔══════════════════════════════════════════════╗
  ║  Multithreaded Sensor Processing System     ║
  ║  Version 1.0.0                              ║
  ╚══════════════════════════════════════════════╝

[INFO] CSV log file opened: logs/sensor_log.csv
[INFO] Created 5 sensors
[INFO] Temperature sensor started (ID:1, interval:500ms)
[INFO] Pressure sensor started (ID:2, interval:1000ms)
[INFO] GPS sensor started (ID:3, interval:2000ms)
[INFO] IMU sensor started (ID:4, interval:100ms)
[INFO] Radar sensor started (ID:5, interval:250ms)

[WARNING] Temperature (ID:1) value=42.15 C -> WARNING
[ERROR] Radar (ID:5) value=-850.32 m -> ERROR
[WARNING] IMU (ID:4) timeout - no data for 6s
[INFO] IMU (ID:4) recovered
[INFO] Status: Processed=523 | Queue=3 | OK=498 | WARN=18 | ERR=7

[INFO] Shutting down...
[INFO] Temperature sensor stopped (60 messages sent)
[INFO] Pressure sensor stopped (30 messages sent)
...

========== SYSTEM STATISTICS ==========

  Runtime:              30 seconds
  Processed messages:   1543
  Messages/sec:         51.4
  Avg processing time:  2.150 us
  Max queue size:       12
  OK:                   1472
  Warnings:             48
  Errors:               23

  -- Per Sensor --

  Temperature     60 messages
  Pressure        30 messages
  GPS             15 messages
  IMU             300 messages
  Radar           120 messages

=======================================
```

---

## Testing

The project includes comprehensive unit tests using GoogleTest:

| Test Suite | Tests | Coverage |
|-----------|-------|---------|
| QueueTests | 9 | push, pop, empty, size, FIFO, tryPop, shutdown, concurrent push, concurrent push+pop |
| SensorTests | 10 | creation (all 5 types), start/stop, double-start, data enqueue, field validation, IMU frequency, concurrent sensors |
| ProcessorTests | 9 | valid data, warning range, error range, negative values, multi-data, per-sensor count, empty queue, GPS validation |

```bash
ctest --test-dir build --output-on-failure
```

---

## Project Structure

```
multithreaded-sensor-system/
├── CMakeLists.txt              # Build configuration
├── build.bat                   # Windows build script
├── README.md                   # This file
├── .gitignore                  # Git ignore rules
│
├── include/                    # Header files
│   ├── SensorData.h            # Data struct + SensorStatus enum
│   ├── Sensor.h                # Base sensor + 5 concrete sensors
│   ├── SensorManager.h         # Sensor lifecycle management
│   ├── ThreadSafeQueue.h       # Thread-safe queue template
│   ├── DataProcessor.h         # Consumer + validation + stats
│   └── Logger.h                # Thread-safe logging
│
├── src/                        # Source files
│   ├── main.cpp                # Entry point + signal handling
│   ├── Sensor.cpp              # All sensor implementations
│   ├── SensorManager.cpp       # Sensor management
│   ├── DataProcessor.cpp       # Data processing + validation
│   └── Logger.cpp              # Logging implementation
│
├── tests/                      # Unit tests (GoogleTest)
│   ├── test_queue.cpp          # ThreadSafeQueue tests
│   ├── test_sensor.cpp         # Sensor tests
│   └── test_processor.cpp      # DataProcessor tests
│
├── logs/                       # Runtime log output (gitignored)
└── docs/
    └── architecture.md         # Technical documentation
```

---

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| **Separate thread per sensor** | Simulates real hardware where each sensor operates independently at different frequencies |
| **Producer-Consumer pattern** | Decouples data production from processing, allows independent scaling |
| **std::mutex + condition_variable** | Prevents race conditions while allowing efficient CPU-idle waiting |
| **std::atomic for shutdown** | Simple boolean flags don't need full mutex protection |
| **enum class for status** | Type-safe enums prevent accidental integer conversion bugs |
| **unique_ptr for sensors** | RAII ownership ensures sensors are properly destroyed |
| **Template queue** | Reusable for any data type, demonstrates generic programming |
| **Validation ranges** | Separates "normal operating" from "physically possible" from "error" |

---

## Key C++ Concepts Used

- `std::thread` — Independent execution threads for each sensor
- `std::mutex` / `std::lock_guard` / `std::unique_lock` — Mutual exclusion
- `std::condition_variable` — Efficient blocking wait with notification
- `std::atomic` — Lock-free thread-safe flags
- `std::optional` — Safe nullable return values
- `std::chrono` — High-resolution timing and timestamps
- `std::random` — Mersenne Twister random number generation
- `std::unique_ptr` — RAII smart pointer ownership
- **Polymorphism** — Virtual functions for sensor type abstraction
- **Templates** — Generic thread-safe queue
- **RAII** — Resource management through object lifetime
- **enum class** — Type-safe enumerations

---

## Future Improvements

- [ ] Thread pool for sensor management
- [ ] Priority queue for critical sensor data
- [ ] JSON/YAML configuration file
- [ ] SQLite database storage
- [ ] Network sensor data (TCP/UDP)
- [ ] REST API for real-time monitoring
- [ ] Web dashboard (WebSocket)
- [ ] Docker containerization
- [ ] Prometheus metrics export
- [ ] Grafana dashboard

---

## License

This project is for educational purposes.
