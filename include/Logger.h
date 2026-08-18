#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include "SensorData.h"

// ══════════════════════════════════════════════════════════════
// LogLevel — Log seviyesi
// ══════════════════════════════════════════════════════════════
enum class LogLevel {
    INFO,
    WARNING,
    ERROR_LVL
};

// ══════════════════════════════════════════════════════════════
// Logger — Thread-safe loglama sınıfı
// ══════════════════════════════════════════════════════════════
//
// İki çıktı kanalı:
//   1. Konsol (renkli) — WARNING/ERROR her zaman, INFO seçmeli
//   2. CSV dosyası    — Tüm sensör verileri
//
// Thread-safe: Birden fazla thread aynı anda log yazabilir.

class Logger {
private:
    std::ofstream csvFile_;
    mutable std::mutex consoleMutex_;   // Konsol çıktısını korur
    mutable std::mutex fileMutex_;      // CSV dosyasını korur
    bool csvOpen_{false};

public:
    Logger();
    ~Logger();

    // CSV dosyasını aç
    bool openCSV(const std::string& filepath);
    void closeCSV();

    // Konsol'a log yaz (her zaman)
    void log(LogLevel level, const std::string& message);

    // Sensör verisini CSV'ye yaz + opsiyonel konsol çıktısı
    void logSensorData(const SensorData& data);

    // Yardımcı fonksiyonlar
    static std::string levelToString(LogLevel level);
    static std::string colorForLevel(LogLevel level);
    static std::string resetColor();
    static void enableAnsiColors();
};

#endif // LOGGER_H
