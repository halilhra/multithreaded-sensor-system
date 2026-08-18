#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>

// ══════════════════════════════════════════════════════════════
// SensorStatus — enum class ile sensör durumu
// ══════════════════════════════════════════════════════════════
enum class SensorStatus {
    OK,
    WARNING,
    ERROR_STATE,    // "ERROR" Windows'ta makro olarak tanımlı, çakışmayı önlüyoruz
    TIMEOUT,
    DISCONNECTED
};

// ══════════════════════════════════════════════════════════════
// SensorData — Sensör verisini temsil eden yapı
// ══════════════════════════════════════════════════════════════
struct SensorData {
    int sensorId;
    std::string sensorName;
    std::chrono::system_clock::time_point timestamp;
    double value;
    SensorStatus status;
    std::string unit;
};

// ── Yardımcı fonksiyonlar (inline = header'da tanımlanabilir) ──

inline std::string statusToString(SensorStatus status) {
    switch (status) {
        case SensorStatus::OK:           return "OK";
        case SensorStatus::WARNING:      return "WARNING";
        case SensorStatus::ERROR_STATE:  return "ERROR";
        case SensorStatus::TIMEOUT:      return "TIMEOUT";
        case SensorStatus::DISCONNECTED: return "DISCONNECTED";
        default:                         return "UNKNOWN";
    }
}

inline std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;

    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t_val);
#else
    localtime_r(&time_t_val, &tm_buf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

#endif // SENSOR_DATA_H
