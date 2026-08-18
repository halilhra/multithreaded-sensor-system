# Multithreaded Sensor Data Processing System

A high-performance, multithreaded sensor data processing system built in C++17.

## Project Overview

This system simulates multiple sensors (Temperature, Pressure, GPS, IMU, Radar) running on independent threads, producing data into a thread-safe queue. A consumer thread processes the data, performs validation, detects faults, and logs results.

## Status

🚧 **Under Development** — Phase 1: Project structure complete.

## Building

```bash
cmake -S . -B build
cmake --build build
./build/sensor_system
```

## Testing

```bash
ctest --test-dir build
```

## Project Structure

```
multithreaded-sensor-system/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── .gitignore              # Git ignore rules
├── include/                # Header files
│   ├── Sensor.h
│   ├── SensorData.h
│   ├── SensorManager.h
│   ├── ThreadSafeQueue.h
│   ├── DataProcessor.h
│   └── Logger.h
├── src/                    # Source files
│   ├── main.cpp
│   ├── Sensor.cpp
│   ├── SensorManager.cpp
│   ├── DataProcessor.cpp
│   └── Logger.cpp
├── tests/                  # Unit tests (GoogleTest)
│   ├── test_queue.cpp
│   ├── test_sensor.cpp
│   └── test_processor.cpp
├── logs/                   # Runtime log output
└── docs/                   # Technical documentation
    └── architecture.md
```

## Technologies

- C++17
- CMake 3.16+
- GoogleTest
- POSIX Threads
