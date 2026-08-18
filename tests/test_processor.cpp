#include <gtest/gtest.h>
#include "DataProcessor.h"
#include "ThreadSafeQueue.h"
#include "Logger.h"

// ══════════════════════════════════════════════════════════════
// DataProcessor Testleri
// ══════════════════════════════════════════════════════════════

class ProcessorTest : public ::testing::Test {
protected:
    ThreadSafeQueue<SensorData> queue;
    Logger logger;

    // Yardımcı: SensorData oluştur
    SensorData makeData(const std::string& name, double value, int id = 1) {
        SensorData data;
        data.sensorId = id;
        data.sensorName = name;
        data.timestamp = std::chrono::system_clock::now();
        data.value = value;
        data.status = SensorStatus::OK;
        data.unit = "test";
        return data;
    }
};

// ── Normal değer → OK ──
TEST_F(ProcessorTest, ValidTemperature) {
    DataProcessor processor(&queue, &logger);

    queue.push(makeData("Temperature", 25.0));
    processor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    processor.stop();

    EXPECT_EQ(processor.getStats().totalProcessed, 1);
    EXPECT_EQ(processor.getStats().okCount, 1);
}

// ── Aralık dışı değer → WARNING ──
TEST_F(ProcessorTest, WarningTemperature) {
    DataProcessor processor(&queue, &logger);

    // 45°C → normal aralık dışı (20-40) ama geçerli aralıkta (-50 - 100)
    queue.push(makeData("Temperature", 45.0));
    processor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    processor.stop();

    EXPECT_EQ(processor.getStats().warningCount, 1);
}

// ── Tamamen geçersiz değer → ERROR ──
TEST_F(ProcessorTest, ErrorTemperature) {
    DataProcessor processor(&queue, &logger);

    // 9999°C → geçerli aralığın da dışında
    queue.push(makeData("Temperature", 9999.0));
    processor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    processor.stop();

    EXPECT_EQ(processor.getStats().errorCount, 1);
}

// ── Negatif radar mesafesi → ERROR ──
TEST_F(ProcessorTest, NegativeRadarDistance) {
    DataProcessor processor(&queue, &logger);

    queue.push(makeData("Radar", -5.0, 5));
    processor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    processor.stop();

    EXPECT_EQ(processor.getStats().errorCount, 1);
}

// ── Normal basınç → OK ──
TEST_F(ProcessorTest, ValidPressure) {
    DataProcessor processor(&queue, &logger);

    queue.push(makeData("Pressure", 1013.0, 2));
    processor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    processor.stop();

    EXPECT_EQ(processor.getStats().okCount, 1);
}

// ── Birden fazla veri işleme ──
TEST_F(ProcessorTest, MultipleDataProcessing) {
    DataProcessor processor(&queue, &logger);

    queue.push(makeData("Temperature", 25.0, 1));   // OK
    queue.push(makeData("Pressure", 1013.0, 2));     // OK
    queue.push(makeData("Radar", 100.0, 5));         // OK
    queue.push(makeData("Temperature", 150.0, 1));   // ERROR (>100)
    queue.push(makeData("IMU", 5.0, 4));             // WARNING (-2 to 2 normal, -20 to 20 valid)

    processor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    processor.stop();

    EXPECT_EQ(processor.getStats().totalProcessed, 5);
    EXPECT_EQ(processor.getStats().okCount, 3);
    EXPECT_EQ(processor.getStats().warningCount, 1);
    EXPECT_EQ(processor.getStats().errorCount, 1);
}

// ── Per-sensor count doğru mu? ──
TEST_F(ProcessorTest, PerSensorCount) {
    DataProcessor processor(&queue, &logger);

    queue.push(makeData("Temperature", 25.0, 1));
    queue.push(makeData("Temperature", 26.0, 1));
    queue.push(makeData("Pressure", 1013.0, 2));

    processor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    processor.stop();

    EXPECT_EQ(processor.getStats().perSensorCount.at(1), 2);
    EXPECT_EQ(processor.getStats().perSensorCount.at(2), 1);
}

// ── Boş kuyrukla processor çalışır ve durur ──
TEST_F(ProcessorTest, EmptyQueueStopsGracefully) {
    DataProcessor processor(&queue, &logger);
    processor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    queue.shutdown();
    processor.stop();

    EXPECT_EQ(processor.getStats().totalProcessed, 0);
}

// ── GPS geçerli aralık testi ──
TEST_F(ProcessorTest, GPSValidRange) {
    DataProcessor processor(&queue, &logger);

    queue.push(makeData("GPS", 39.92, 3));    // OK (39-41)
    queue.push(makeData("GPS", 50.0, 3));     // WARNING (valid: -90 to 90)
    queue.push(makeData("GPS", 100.0, 3));    // ERROR (>90)

    processor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    processor.stop();

    EXPECT_EQ(processor.getStats().okCount, 1);
    EXPECT_EQ(processor.getStats().warningCount, 1);
    EXPECT_EQ(processor.getStats().errorCount, 1);
}
