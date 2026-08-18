// ══════════════════════════════════════════════════════════════
// Multithreaded Sensor Data Processing System
// ══════════════════════════════════════════════════════════════
//
// Mimari:
//   5 Sensor Thread (Producer) → ThreadSafeQueue → DataProcessor Thread (Consumer) → Logger
//
// Graceful Shutdown:
//   Ctrl+C → Signal Handler → atomic flag → Sensors stop → Queue drain → Processor stop

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

#include "SensorData.h"
#include "ThreadSafeQueue.h"
#include "Sensor.h"
#include "SensorManager.h"
#include "DataProcessor.h"
#include "Logger.h"

// ══════════════════════════════════════════════════════════════
// Global shutdown flag (atomic — mutex'siz thread-safe)
// ══════════════════════════════════════════════════════════════
std::atomic<bool> g_running{true};

void signalHandler(int signum) {
    (void)signum;  // Kullanılmayan parametre uyarısını engelle
    g_running.store(false);
}

// ══════════════════════════════════════════════════════════════
// Ana Program
// ══════════════════════════════════════════════════════════════
int main(int argc, char* argv[]) {
    // Çalışma süresi (saniye), komut satırından değiştirilebilir
    int runtimeSeconds = 30;
    if (argc > 1) {
        runtimeSeconds = std::atoi(argv[1]);
        if (runtimeSeconds <= 0) runtimeSeconds = 30;
    }

    // Sinyal yakalayıcıyı kaydet (Ctrl+C)
    std::signal(SIGINT, signalHandler);
#ifndef _WIN32
    std::signal(SIGTERM, signalHandler);
#endif

    // ── Başlangıç Banner'ı ──
    Logger::enableAnsiColors();
    std::cout << "\033[1;36m";
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════╗\n";
    std::cout << "  ║  Multithreaded Sensor Processing System     ║\n";
    std::cout << "  ║  Version 1.0.0                              ║\n";
    std::cout << "  ╚══════════════════════════════════════════════╝\n";
    std::cout << "\033[0m\n";

    // ── Bileşenleri oluştur ──
    Logger logger;
    if (logger.openCSV("logs/sensor_log.csv")) {
        logger.log(LogLevel::INFO, "CSV log file opened: logs/sensor_log.csv");
    } else {
        logger.log(LogLevel::ERROR_LVL, "Failed to open CSV log file!");
    }

    ThreadSafeQueue<SensorData> queue;
    DataProcessor processor(&queue, &logger);
    SensorManager manager(&queue, &logger);

    // ── Sensörleri oluştur ve başlat ──
    manager.createDefaultSensors();
    logger.log(LogLevel::INFO, "Starting system (runtime: " + std::to_string(runtimeSeconds) + "s, press Ctrl+C to stop)...");
    std::cout << "\n";

    processor.start();
    manager.startAll();
    std::cout << "\n";

    // ── Ana döngü: Çalışma süresi veya Ctrl+C bekle ──
    auto startTime = std::chrono::steady_clock::now();
    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime);
        if (elapsed.count() >= runtimeSeconds) {
            break;
        }
    }

    // ══════════════════════════════════════════════════════════
    // Graceful Shutdown Sırası
    // ══════════════════════════════════════════════════════════
    std::cout << "\n";
    logger.log(LogLevel::INFO, "Shutting down...");

    // 1. Sensörleri durdur (artık yeni veri üretilmez)
    manager.stopAll();

    // 2. Kuyruğu kapat (consumer'a sinyal gönder)
    queue.shutdown();

    // 3. Kuyruktaki kalan verileri işle ve processor'ı durdur
    processor.stop();

    // 4. İstatistikleri yazdır
    processor.printStatistics();

    // 5. Logger'ı kapat
    logger.log(LogLevel::INFO, "System shut down successfully.");
    logger.closeCSV();

    return 0;
}
