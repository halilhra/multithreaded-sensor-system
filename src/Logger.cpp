#include "Logger.h"
#include <filesystem>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#endif

// ══════════════════════════════════════════════════════════════
// Constructor / Destructor
// ══════════════════════════════════════════════════════════════

Logger::Logger() {
    enableAnsiColors();
}

Logger::~Logger() {
    closeCSV();
}

// ══════════════════════════════════════════════════════════════
// CSV Dosyası İşlemleri
// ══════════════════════════════════════════════════════════════

bool Logger::openCSV(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(fileMutex_);

    // logs/ klasörünü oluştur (yoksa)
    std::filesystem::path p(filepath);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    csvFile_.open(filepath, std::ios::out | std::ios::trunc);
    if (csvFile_.is_open()) {
        csvFile_ << "timestamp,sensor_id,sensor_name,value,unit,status\n";
        csvFile_.flush();
        csvOpen_ = true;
        return true;
    }
    return false;
}

void Logger::closeCSV() {
    std::lock_guard<std::mutex> lock(fileMutex_);
    if (csvFile_.is_open()) {
        csvFile_.flush();
        csvFile_.close();
        csvOpen_ = false;
    }
}

// ══════════════════════════════════════════════════════════════
// Konsol Loglama
// ══════════════════════════════════════════════════════════════

void Logger::log(LogLevel level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    std::string timestamp = formatTimestamp(now);
    std::string levelStr = levelToString(level);

    std::lock_guard<std::mutex> lock(consoleMutex_);
    std::cout << colorForLevel(level)
              << "[" << timestamp << "] "
              << "[" << levelStr << "] "
              << message
              << resetColor()
              << "\n";
}

// ══════════════════════════════════════════════════════════════
// Sensör Verisi Loglama (CSV + Konsol)
// ══════════════════════════════════════════════════════════════

void Logger::logSensorData(const SensorData& data) {
    // CSV'ye her zaman yaz
    if (csvOpen_) {
        std::lock_guard<std::mutex> lock(fileMutex_);
        csvFile_ << formatTimestamp(data.timestamp) << ","
                 << data.sensorId << ","
                 << data.sensorName << ","
                 << std::fixed << std::setprecision(4) << data.value << ","
                 << data.unit << ","
                 << statusToString(data.status) << "\n";
        csvFile_.flush();
    }
}

// ══════════════════════════════════════════════════════════════
// Yardımcı Fonksiyonlar
// ══════════════════════════════════════════════════════════════

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:      return "INFO";
        case LogLevel::WARNING:   return "WARNING";
        case LogLevel::ERROR_LVL: return "ERROR";
        default:                  return "UNKNOWN";
    }
}

std::string Logger::colorForLevel(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:      return "\033[36m";   // Cyan
        case LogLevel::WARNING:   return "\033[33m";   // Yellow
        case LogLevel::ERROR_LVL: return "\033[31m";   // Red
        default:                  return "";
    }
}

std::string Logger::resetColor() {
    return "\033[0m";
}

void Logger::enableAnsiColors() {
#ifdef _WIN32
    // Windows 10+ ANSI renk desteğini etkinleştir
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}
