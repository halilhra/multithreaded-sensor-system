#include "Sensor.h"
#include <iostream>

// ══════════════════════════════════════════════════════════════
// Sensor Base Class Implementation
// ══════════════════════════════════════════════════════════════

Sensor::Sensor(int id, const std::string& name, const std::string& unit,
               int intervalMs, double normalMin, double normalMax,
               ThreadSafeQueue<SensorData>* queue,
               double faultProb, double disconnectProb)
    : id_(id), name_(name), unit_(unit),
      intervalMs_(intervalMs), normalMin_(normalMin), normalMax_(normalMax),
      queue_(queue), faultProbability_(faultProb), disconnectProbability_(disconnectProb),
      rng_(std::random_device{}())    // Her sensör için farklı seed
{
}

Sensor::~Sensor() {
    stop();
}

void Sensor::start() {
    if (running_.load()) return;   // Zaten çalışıyorsa tekrar başlatma
    running_.store(true);
    thread_ = std::thread(&Sensor::run, this);  // Yeni thread başlat
}

void Sensor::stop() {
    running_.store(false);             // Döngüyü durdur sinyali
    if (thread_.joinable()) {
        thread_.join();                // Thread'in bitmesini bekle
    }
}

SensorData Sensor::generateData() {
    SensorData data;
    data.sensorId = id_;
    data.sensorName = name_;
    data.timestamp = std::chrono::system_clock::now();
    data.unit = unit_;

    // Arıza simülasyonu: %3 olasılıkla hatalı veri üret
    std::uniform_real_distribution<double> chanceDist(0.0, 1.0);

    if (chanceDist(rng_) < faultProbability_) {
        // Aralık dışı (çok büyük veya negatif) değer üret
        std::uniform_real_distribution<double> faultDist(normalMax_ * 5, normalMax_ * 20);
        data.value = (chanceDist(rng_) < 0.5) ? faultDist(rng_) : -faultDist(rng_);
        data.status = SensorStatus::ERROR_STATE;
    } else {
        data.value = generateValue();  // Normal veri üret (polymorphic çağrı)
        data.status = SensorStatus::OK;
    }

    return data;
}

void Sensor::run() {
    std::uniform_real_distribution<double> chanceDist(0.0, 1.0);
    std::uniform_int_distribution<int> disconnectDuration(2, 6);  // 2-6 saniye

    auto disconnectUntil = std::chrono::steady_clock::now();  // Başlangıçta bağlı

    while (running_.load()) {
        auto now = std::chrono::steady_clock::now();

        // Bağlantı kopmuşsa, yeniden bağlanana kadar bekle
        if (now < disconnectUntil) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Rastgele bağlantı kopması simülasyonu (%1 olasılık)
        if (chanceDist(rng_) < disconnectProbability_) {
            int duration = disconnectDuration(rng_);
            disconnectUntil = now + std::chrono::seconds(duration);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Veri üret ve kuyruğa ekle
        SensorData data = generateData();
        queue_->push(data);
        messageCount_.fetch_add(1);

        // Belirlenen aralık kadar bekle
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs_));
    }
}

// ══════════════════════════════════════════════════════════════
// Temperature Sensor: 20-40 °C
// ══════════════════════════════════════════════════════════════

TemperatureSensor::TemperatureSensor(int id, ThreadSafeQueue<SensorData>* queue)
    : Sensor(id, "Temperature", "C", 500, 20.0, 40.0, queue)
{
}

double TemperatureSensor::generateValue() {
    // Gaussian dağılım: ortalama 30°C, standart sapma 5°C
    std::normal_distribution<double> dist(30.0, 5.0);
    return dist(rng_);
}

// ══════════════════════════════════════════════════════════════
// Pressure Sensor: 950-1050 hPa
// ══════════════════════════════════════════════════════════════

PressureSensor::PressureSensor(int id, ThreadSafeQueue<SensorData>* queue)
    : Sensor(id, "Pressure", "hPa", 1000, 950.0, 1050.0, queue)
{
}

double PressureSensor::generateValue() {
    std::normal_distribution<double> dist(1013.0, 20.0);
    return dist(rng_);
}

// ══════════════════════════════════════════════════════════════
// GPS Sensor: Latitude 39-41° (Ankara civarı)
// ══════════════════════════════════════════════════════════════

GPSSensor::GPSSensor(int id, ThreadSafeQueue<SensorData>* queue)
    : Sensor(id, "GPS", "deg", 2000, 39.0, 41.0, queue)
{
}

double GPSSensor::generateValue() {
    // Ankara civarı koordinatlar: 39.92° ± küçük değişim
    std::normal_distribution<double> dist(39.92, 0.05);
    return dist(rng_);
}

// ══════════════════════════════════════════════════════════════
// IMU Sensor: Acceleration -2 to 2 g
// ══════════════════════════════════════════════════════════════

IMUSensor::IMUSensor(int id, ThreadSafeQueue<SensorData>* queue)
    : Sensor(id, "IMU", "g", 100, -2.0, 2.0, queue)
{
}

double IMUSensor::generateValue() {
    // Sabit duruşta ~0g, küçük titreşimler
    std::normal_distribution<double> dist(0.0, 0.5);
    return dist(rng_);
}

// ══════════════════════════════════════════════════════════════
// Radar Sensor: Distance 0.5-1000 m
// ══════════════════════════════════════════════════════════════

RadarSensor::RadarSensor(int id, ThreadSafeQueue<SensorData>* queue)
    : Sensor(id, "Radar", "m", 250, 0.5, 1000.0, queue)
{
}

double RadarSensor::generateValue() {
    std::uniform_real_distribution<double> dist(10.0, 500.0);
    return dist(rng_);
}
