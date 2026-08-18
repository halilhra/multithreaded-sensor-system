#include "DataProcessor.h"
#include <iostream>
#include <sstream>
#include <iomanip>

// ══════════════════════════════════════════════════════════════
// Constructor / Destructor
// ══════════════════════════════════════════════════════════════

DataProcessor::DataProcessor(ThreadSafeQueue<SensorData>* queue, Logger* logger)
    : queue_(queue), logger_(logger)
{
    setupValidationRanges();
}

DataProcessor::~DataProcessor() {
    stop();
}

// ══════════════════════════════════════════════════════════════
// Doğrulama Aralıklarını Ayarla
// ══════════════════════════════════════════════════════════════

void DataProcessor::setupValidationRanges() {
    //                          normalMin  normalMax  validMin  validMax
    validationRanges_["Temperature"] = { 20.0,    40.0,    -50.0,   100.0  };
    validationRanges_["Pressure"]    = { 950.0,   1050.0,  800.0,   1200.0 };
    validationRanges_["GPS"]         = { 39.0,    41.0,    -90.0,   90.0   };
    validationRanges_["IMU"]         = { -2.0,    2.0,     -20.0,   20.0   };
    validationRanges_["Radar"]       = { 0.5,     1000.0,  0.0,     5000.0 };
}

// ══════════════════════════════════════════════════════════════
// Thread Kontrol
// ══════════════════════════════════════════════════════════════

void DataProcessor::start() {
    if (running_.load()) return;
    running_.store(true);
    stats_.startTime = std::chrono::steady_clock::now();
    thread_ = std::thread(&DataProcessor::run, this);
}

void DataProcessor::stop() {
    running_.store(false);
    if (thread_.joinable()) {
        thread_.join();
    }
    stats_.endTime = std::chrono::steady_clock::now();
}

// ══════════════════════════════════════════════════════════════
// Ana İşleme Döngüsü (Consumer Thread)
// ══════════════════════════════════════════════════════════════

void DataProcessor::run() {
    auto lastStatusReport = std::chrono::steady_clock::now();

    while (running_.load() || !queue_->empty()) {
        // Kuyruktan veri al (500ms timeout ile bekle)
        auto data = queue_->pop(std::chrono::milliseconds(500));

        if (data.has_value()) {
            processData(data.value());
        }

        // Sensör timeout kontrolü
        checkTimeouts();

        // Her 5 saniyede durum raporu
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastStatusReport);
        if (elapsed.count() >= 5) {
            std::ostringstream oss;
            oss << "Status: Processed=" << stats_.totalProcessed
                << " | Queue=" << queue_->size()
                << " | OK=" << stats_.okCount
                << " | WARN=" << stats_.warningCount
                << " | ERR=" << stats_.errorCount;
            logger_->log(LogLevel::INFO, oss.str());
            lastStatusReport = now;
        }
    }
}

// ══════════════════════════════════════════════════════════════
// Tek Veri İşleme
// ══════════════════════════════════════════════════════════════

void DataProcessor::processData(SensorData& data) {
    auto start = std::chrono::high_resolution_clock::now();

    int sensorId = data.sensorId;

    // Timeout'tan kurtulma kontrolü
    if (timedOut_.count(sensorId) && timedOut_[sensorId]) {
        timedOut_[sensorId] = false;
        logger_->log(LogLevel::INFO, data.sensorName + " (ID:" + std::to_string(sensorId) + ") recovered");
    }
    lastDataTime_[sensorId] = std::chrono::steady_clock::now();
    stats_.sensorNames[sensorId] = data.sensorName;

    // Veri doğrulama
    SensorStatus validatedStatus = validate(data);
    data.status = validatedStatus;

    // CSV'ye her zaman yaz
    logger_->logSensorData(data);

    // Konsol'a sadece WARNING/ERROR yaz
    if (data.status == SensorStatus::WARNING || data.status == SensorStatus::ERROR_STATE) {
        LogLevel level = (data.status == SensorStatus::WARNING) ? LogLevel::WARNING : LogLevel::ERROR_LVL;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2)
            << data.sensorName << " (ID:" << data.sensorId << ") "
            << "value=" << data.value << " " << data.unit
            << " -> " << statusToString(data.status);
        logger_->log(level, oss.str());
    }

    // İstatistikleri güncelle
    stats_.totalProcessed++;
    switch (data.status) {
        case SensorStatus::OK:          stats_.okCount++;      break;
        case SensorStatus::WARNING:     stats_.warningCount++; break;
        default:                        stats_.errorCount++;   break;
    }
    stats_.perSensorCount[sensorId]++;

    auto end = std::chrono::high_resolution_clock::now();
    stats_.totalProcessingTimeMs +=
        std::chrono::duration<double, std::milli>(end - start).count();
}

// ══════════════════════════════════════════════════════════════
// Veri Doğrulama
// ══════════════════════════════════════════════════════════════

SensorStatus DataProcessor::validate(const SensorData& data) {
    auto it = validationRanges_.find(data.sensorName);
    if (it == validationRanges_.end()) {
        return SensorStatus::WARNING;  // Bilinmeyen sensör
    }

    const auto& range = it->second;

    // Normal aralıkta → OK
    if (data.value >= range.normalMin && data.value <= range.normalMax) {
        return SensorStatus::OK;
    }
    // Geçerli aralıkta ama normal dışında → WARNING
    if (data.value >= range.validMin && data.value <= range.validMax) {
        return SensorStatus::WARNING;
    }
    // Tamamen dışında → ERROR
    return SensorStatus::ERROR_STATE;
}

// ══════════════════════════════════════════════════════════════
// Sensör Timeout Kontrolü
// ══════════════════════════════════════════════════════════════

void DataProcessor::checkTimeouts() {
    auto now = std::chrono::steady_clock::now();

    for (auto& [id, lastTime] : lastDataTime_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);

        if (elapsed.count() > timeoutSeconds_ && !timedOut_[id]) {
            timedOut_[id] = true;
            std::string name = stats_.sensorNames.count(id) ? stats_.sensorNames[id] : "Unknown";
            logger_->log(LogLevel::WARNING,
                name + " (ID:" + std::to_string(id) + ") timeout - no data for "
                + std::to_string(elapsed.count()) + "s");
        }
    }
}

// ══════════════════════════════════════════════════════════════
// Son İstatistik Raporu
// ══════════════════════════════════════════════════════════════

void DataProcessor::printStatistics() {
    auto runtime = std::chrono::duration_cast<std::chrono::seconds>(
        stats_.endTime - stats_.startTime);
    double seconds = static_cast<double>(runtime.count());
    double msgsPerSec = (seconds > 0) ? stats_.totalProcessed / seconds : 0;
    double avgProcessingUs = (stats_.totalProcessed > 0)
        ? (stats_.totalProcessingTimeMs * 1000.0) / stats_.totalProcessed : 0;

    std::cout << "\n";
    std::cout << "\033[1;36m"; // Bold cyan
    std::cout << "========== SYSTEM STATISTICS ==========\n\n";
    std::cout << "\033[0m";

    std::cout << "  Runtime:              " << seconds << " seconds\n";
    std::cout << "  Processed messages:   " << stats_.totalProcessed << "\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  Messages/sec:         " << msgsPerSec << "\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "  Avg processing time:  " << avgProcessingUs << " us\n";
    std::cout << "  Max queue size:       " << queue_->getMaxSizeReached() << "\n";
    std::cout << "  OK:                   " << stats_.okCount << "\n";
    std::cout << "  Warnings:             " << stats_.warningCount << "\n";
    std::cout << "  Errors:               " << stats_.errorCount << "\n";

    std::cout << "\n  \033[1m-- Per Sensor --\033[0m\n\n";
    for (auto& [id, count] : stats_.perSensorCount) {
        std::string name = stats_.sensorNames.count(id) ? stats_.sensorNames[id] : "Unknown";
        std::cout << "  " << std::left << std::setw(16) << name
                  << count << " messages\n";
    }

    std::cout << "\n\033[1;36m=======================================\033[0m\n\n";
}
