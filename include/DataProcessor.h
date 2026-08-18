#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <thread>
#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include "SensorData.h"
#include "ThreadSafeQueue.h"
#include "Logger.h"

// ══════════════════════════════════════════════════════════════
// ProcessingStats — İşleme istatistikleri
// ══════════════════════════════════════════════════════════════
struct ProcessingStats {
    int totalProcessed = 0;
    int okCount = 0;
    int warningCount = 0;
    int errorCount = 0;
    std::map<int, int> perSensorCount;        // Sensör ID → mesaj sayısı
    std::map<int, std::string> sensorNames;   // Sensör ID → isim
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    double totalProcessingTimeMs = 0;
};

// ══════════════════════════════════════════════════════════════
// ValidationRange — Sensör doğrulama aralıkları
// ══════════════════════════════════════════════════════════════
struct ValidationRange {
    double normalMin;   // Bu aralıktaysa → OK
    double normalMax;
    double validMin;    // Bu aralıktaysa → WARNING
    double validMax;    // Bunun dışındaysa → ERROR
};

// ══════════════════════════════════════════════════════════════
// DataProcessor — Consumer thread
// ══════════════════════════════════════════════════════════════
//
// Kuyruktan veri alır, doğrular, loglar.
// Producer-Consumer mimarisinin Consumer kısmı.

class DataProcessor {
private:
    ThreadSafeQueue<SensorData>* queue_;
    Logger* logger_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    ProcessingStats stats_;

    // Doğrulama aralıkları (sensör adına göre)
    std::map<std::string, ValidationRange> validationRanges_;

    // Timeout takibi
    std::map<int, std::chrono::steady_clock::time_point> lastDataTime_;
    std::map<int, bool> timedOut_;
    int timeoutSeconds_ = 5;

    void run();
    void processData(SensorData& data);
    SensorStatus validate(const SensorData& data);
    void checkTimeouts();
    void setupValidationRanges();

public:
    DataProcessor(ThreadSafeQueue<SensorData>* queue, Logger* logger);
    ~DataProcessor();

    void start();
    void stop();

    const ProcessingStats& getStats() const { return stats_; }
    void printStatistics();
};

#endif // DATA_PROCESSOR_H
