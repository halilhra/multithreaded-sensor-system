#include "SensorManager.h"
#include <sstream>

// ══════════════════════════════════════════════════════════════
// Constructor / Destructor
// ══════════════════════════════════════════════════════════════

SensorManager::SensorManager(ThreadSafeQueue<SensorData>* queue, Logger* logger)
    : queue_(queue), logger_(logger)
{
}

SensorManager::~SensorManager() {
    stopAll();
}

// ══════════════════════════════════════════════════════════════
// Sensör Yönetimi
// ══════════════════════════════════════════════════════════════

void SensorManager::addSensor(std::unique_ptr<Sensor> sensor) {
    sensors_.push_back(std::move(sensor));
}

void SensorManager::createDefaultSensors() {
    // 5 farklı sensör oluştur, her biri farklı frekansta çalışacak
    //                                    ID  Queue
    addSensor(std::make_unique<TemperatureSensor>(1, queue_));   // 500ms
    addSensor(std::make_unique<PressureSensor>(2, queue_));      // 1000ms
    addSensor(std::make_unique<GPSSensor>(3, queue_));           // 2000ms
    addSensor(std::make_unique<IMUSensor>(4, queue_));           // 100ms
    addSensor(std::make_unique<RadarSensor>(5, queue_));         // 250ms

    logger_->log(LogLevel::INFO, "Created " + std::to_string(sensors_.size()) + " sensors");
}

void SensorManager::startAll() {
    for (auto& sensor : sensors_) {
        sensor->start();
        std::ostringstream oss;
        oss << sensor->getName() << " sensor started (ID:"
            << sensor->getId() << ", interval:" << sensor->getIntervalMs() << "ms)";
        logger_->log(LogLevel::INFO, oss.str());
    }
}

void SensorManager::stopAll() {
    for (auto& sensor : sensors_) {
        if (sensor->isRunning()) {
            sensor->stop();
            std::ostringstream oss;
            oss << sensor->getName() << " sensor stopped ("
                << sensor->getMessageCount() << " messages sent)";
            logger_->log(LogLevel::INFO, oss.str());
        }
    }
}
