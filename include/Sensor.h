#ifndef SENSOR_H
#define SENSOR_H

#include <string>
#include <thread>
#include <atomic>
#include <random>
#include <chrono>
#include "SensorData.h"
#include "ThreadSafeQueue.h"

// ══════════════════════════════════════════════════════════════
// Sensor — Temel (abstract) sensör sınıfı
// ══════════════════════════════════════════════════════════════
//
// Polymorphism: Her sensör türü bu sınıftan türer.
// Pure virtual function: generateValue() her alt sınıf tarafından
//   farklı şekilde implement edilmelidir.
// Her sensör kendi thread'inde çalışır.

class Sensor {
protected:
    int id_;
    std::string name_;
    std::string unit_;
    std::atomic<bool> running_{false};
    std::thread thread_;
    int intervalMs_;                         // Veri üretme aralığı (ms)
    ThreadSafeQueue<SensorData>* queue_;      // Paylaşılan kuyruk (sahip değil, pointer)

    // Değer aralıkları (normal çalışma aralığı)
    double normalMin_;
    double normalMax_;

    // Arıza simülasyonu
    double faultProbability_;                // Hatalı veri üretme olasılığı (0.0 - 1.0)
    double disconnectProbability_;           // Bağlantı kopma olasılığı

    // Random engine (her sensörde ayrı, thread-safe)
    std::mt19937 rng_;

    // İstatistik
    std::atomic<int> messageCount_{0};

    // Alt sınıfların implement etmesi gereken saf sanal fonksiyon
    virtual double generateValue() = 0;

    // Veri paketi oluştur (arıza simülasyonu dahil)
    virtual SensorData generateData();

    // Thread ana döngüsü
    void run();

public:
    Sensor(int id, const std::string& name, const std::string& unit,
           int intervalMs, double normalMin, double normalMax,
           ThreadSafeQueue<SensorData>* queue,
           double faultProb = 0.03, double disconnectProb = 0.01);

    virtual ~Sensor();

    // Thread'i başlat
    void start();

    // Thread'i güvenli şekilde durdur
    void stop();

    // Getter'lar
    int getId() const { return id_; }
    std::string getName() const { return name_; }
    bool isRunning() const { return running_.load(); }
    int getMessageCount() const { return messageCount_.load(); }
    int getIntervalMs() const { return intervalMs_; }
};

// ══════════════════════════════════════════════════════════════
// Concrete Sensor Türleri (Polymorphism)
// ══════════════════════════════════════════════════════════════

// ── Sıcaklık Sensörü: 20-40 °C normal, -50 to 100 valid ──
class TemperatureSensor : public Sensor {
protected:
    double generateValue() override;
public:
    TemperatureSensor(int id, ThreadSafeQueue<SensorData>* queue);
};

// ── Basınç Sensörü: 950-1050 hPa normal, 800 to 1200 valid ──
class PressureSensor : public Sensor {
protected:
    double generateValue() override;
public:
    PressureSensor(int id, ThreadSafeQueue<SensorData>* queue);
};

// ── GPS Sensörü: 39-41° latitude normal, -90 to 90 valid ──
class GPSSensor : public Sensor {
protected:
    double generateValue() override;
public:
    GPSSensor(int id, ThreadSafeQueue<SensorData>* queue);
};

// ── IMU Sensörü: -2 to 2 g normal, -20 to 20 valid ──
class IMUSensor : public Sensor {
protected:
    double generateValue() override;
public:
    IMUSensor(int id, ThreadSafeQueue<SensorData>* queue);
};

// ── Radar/Mesafe Sensörü: 0.5-1000 m normal, 0 to 5000 valid ──
class RadarSensor : public Sensor {
protected:
    double generateValue() override;
public:
    RadarSensor(int id, ThreadSafeQueue<SensorData>* queue);
};

#endif // SENSOR_H
