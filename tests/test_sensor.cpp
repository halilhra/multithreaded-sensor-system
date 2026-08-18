#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "Sensor.h"
#include "ThreadSafeQueue.h"

// ══════════════════════════════════════════════════════════════
// Sensor Testleri
// ══════════════════════════════════════════════════════════════

class SensorTest : public ::testing::Test {
protected:
    ThreadSafeQueue<SensorData> queue;
};

// ── Sensör oluşturma ──
TEST_F(SensorTest, CreateTemperatureSensor) {
    TemperatureSensor sensor(1, &queue);
    EXPECT_EQ(sensor.getId(), 1);
    EXPECT_EQ(sensor.getName(), "Temperature");
    EXPECT_FALSE(sensor.isRunning());
}

TEST_F(SensorTest, CreatePressureSensor) {
    PressureSensor sensor(2, &queue);
    EXPECT_EQ(sensor.getId(), 2);
    EXPECT_EQ(sensor.getName(), "Pressure");
}

TEST_F(SensorTest, CreateGPSSensor) {
    GPSSensor sensor(3, &queue);
    EXPECT_EQ(sensor.getId(), 3);
    EXPECT_EQ(sensor.getName(), "GPS");
}

TEST_F(SensorTest, CreateIMUSensor) {
    IMUSensor sensor(4, &queue);
    EXPECT_EQ(sensor.getId(), 4);
    EXPECT_EQ(sensor.getName(), "IMU");
}

TEST_F(SensorTest, CreateRadarSensor) {
    RadarSensor sensor(5, &queue);
    EXPECT_EQ(sensor.getId(), 5);
    EXPECT_EQ(sensor.getName(), "Radar");
}

// ── Start/Stop lifecycle ──
TEST_F(SensorTest, StartAndStop) {
    TemperatureSensor sensor(1, &queue);
    EXPECT_FALSE(sensor.isRunning());

    sensor.start();
    EXPECT_TRUE(sensor.isRunning());

    // Biraz veri üretmesini bekle
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    sensor.stop();
    EXPECT_FALSE(sensor.isRunning());

    // En az 1 mesaj üretilmiş olmalı
    EXPECT_GT(sensor.getMessageCount(), 0);
}

// ── Çift start güvenliği ──
TEST_F(SensorTest, DoubleStartSafe) {
    TemperatureSensor sensor(1, &queue);
    sensor.start();
    sensor.start(); // İkinci start bir şey yapmamalı
    EXPECT_TRUE(sensor.isRunning());
    sensor.stop();
}

// ── Veri kuyruğa ekleniyor mu? ──
TEST_F(SensorTest, DataEnqueued) {
    TemperatureSensor sensor(1, &queue);
    sensor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    sensor.stop();

    EXPECT_FALSE(queue.empty());
    auto data = queue.pop(std::chrono::milliseconds(50));
    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(data->sensorId, 1);
    EXPECT_EQ(data->sensorName, "Temperature");
}

// ── Üretilen verinin alanları doğru mu? ──
TEST_F(SensorTest, DataFieldsValid) {
    RadarSensor sensor(5, &queue);
    sensor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    sensor.stop();

    auto data = queue.pop(std::chrono::milliseconds(50));
    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(data->sensorId, 5);
    EXPECT_EQ(data->sensorName, "Radar");
    EXPECT_EQ(data->unit, "m");
    // Timestamp geçerli olmalı (bugünün tarihi)
    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - data->timestamp);
    EXPECT_LT(diff.count(), 10);
}

// ── IMU yüksek frekansta çalışır ──
TEST_F(SensorTest, IMUHighFrequency) {
    IMUSensor sensor(4, &queue);
    sensor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(550));
    sensor.stop();

    // 100ms aralıkla 550ms'de en az 4 mesaj beklenir
    EXPECT_GE(sensor.getMessageCount(), 4);
}

// ── Birden fazla sensör aynı anda ──
TEST_F(SensorTest, MultipleSensorsConcurrent) {
    TemperatureSensor temp(1, &queue);
    PressureSensor press(2, &queue);
    RadarSensor radar(3, &queue);

    temp.start();
    press.start();
    radar.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    temp.stop();
    press.stop();
    radar.stop();

    // Üçü de veri üretmiş olmalı
    EXPECT_GT(temp.getMessageCount(), 0);
    EXPECT_GT(press.getMessageCount(), 0);
    EXPECT_GT(radar.getMessageCount(), 0);

    // Kuyrukta toplam veri olmalı
    int total = temp.getMessageCount() + press.getMessageCount() + radar.getMessageCount();
    EXPECT_GT(static_cast<int>(queue.size()), 0);
}
