#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <vector>
#include <memory>
#include "Sensor.h"
#include "ThreadSafeQueue.h"
#include "Logger.h"

// ══════════════════════════════════════════════════════════════
// SensorManager — Tüm sensörleri yöneten sınıf
// ══════════════════════════════════════════════════════════════
//
// Smart pointer (unique_ptr) kullanarak sensörlerin sahipliğini yönetir.
// RAII prensibi: SensorManager yok edildiğinde tüm sensörler otomatik durur.

class SensorManager {
private:
    std::vector<std::unique_ptr<Sensor>> sensors_;
    ThreadSafeQueue<SensorData>* queue_;
    Logger* logger_;

public:
    SensorManager(ThreadSafeQueue<SensorData>* queue, Logger* logger);
    ~SensorManager();

    // Sensör ekle (ownership transfer ile)
    void addSensor(std::unique_ptr<Sensor> sensor);

    // Varsayılan 5 sensörü oluştur
    void createDefaultSensors();

    // Tüm sensörleri başlat
    void startAll();

    // Tüm sensörleri durdur
    void stopAll();

    const std::vector<std::unique_ptr<Sensor>>& getSensors() const { return sensors_; }
};

#endif // SENSOR_MANAGER_H
