#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <set>
#include "ThreadSafeQueue.h"

// ══════════════════════════════════════════════════════════════
// ThreadSafeQueue Testleri
// ══════════════════════════════════════════════════════════════

// ── Temel push/pop ──
TEST(QueueTest, PushAndPop) {
    ThreadSafeQueue<int> q;
    q.push(42);
    auto result = q.pop(std::chrono::milliseconds(100));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);
}

// ── Boş kuyruktan pop → nullopt ──
TEST(QueueTest, PopFromEmptyReturnsNullopt) {
    ThreadSafeQueue<int> q;
    auto result = q.pop(std::chrono::milliseconds(50));
    EXPECT_FALSE(result.has_value());
}

// ── empty() kontrolü ──
TEST(QueueTest, EmptyCheck) {
    ThreadSafeQueue<int> q;
    EXPECT_TRUE(q.empty());
    q.push(1);
    EXPECT_FALSE(q.empty());
    q.pop(std::chrono::milliseconds(50));
    EXPECT_TRUE(q.empty());
}

// ── size() kontrolü ──
TEST(QueueTest, SizeCheck) {
    ThreadSafeQueue<int> q;
    EXPECT_EQ(q.size(), 0u);
    q.push(1);
    q.push(2);
    q.push(3);
    EXPECT_EQ(q.size(), 3u);
    q.pop(std::chrono::milliseconds(50));
    EXPECT_EQ(q.size(), 2u);
}

// ── FIFO sırası ──
TEST(QueueTest, FIFOOrder) {
    ThreadSafeQueue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);
    EXPECT_EQ(q.pop(std::chrono::milliseconds(50)).value(), 1);
    EXPECT_EQ(q.pop(std::chrono::milliseconds(50)).value(), 2);
    EXPECT_EQ(q.pop(std::chrono::milliseconds(50)).value(), 3);
}

// ── tryPop ──
TEST(QueueTest, TryPop) {
    ThreadSafeQueue<int> q;
    EXPECT_FALSE(q.tryPop().has_value());
    q.push(99);
    auto result = q.tryPop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 99);
}

// ── maxSizeReached ──
TEST(QueueTest, MaxSizeReached) {
    ThreadSafeQueue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);
    q.pop(std::chrono::milliseconds(50));
    q.pop(std::chrono::milliseconds(50));
    EXPECT_EQ(q.getMaxSizeReached(), 3u);
}

// ── Shutdown tüm bekleyenleri uyandırır ──
TEST(QueueTest, ShutdownWakesWaiters) {
    ThreadSafeQueue<int> q;

    std::thread consumer([&q] {
        auto result = q.pop(std::chrono::milliseconds(5000));
        // Shutdown sonrası nullopt dönmeli
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    q.shutdown();
    consumer.join();
    EXPECT_TRUE(q.isShutdown());
}

// ── Concurrent push (birden fazla producer) ──
TEST(QueueTest, ConcurrentPush) {
    ThreadSafeQueue<int> q;
    const int numThreads = 4;
    const int numPerThread = 250;

    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; t++) {
        threads.emplace_back([&q, t, numPerThread] {
            for (int i = 0; i < numPerThread; i++) {
                q.push(t * 1000 + i);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(q.size(), static_cast<size_t>(numThreads * numPerThread));
}

// ── Concurrent push + pop ──
TEST(QueueTest, ConcurrentPushAndPop) {
    ThreadSafeQueue<int> q;
    const int numItems = 1000;
    std::atomic<int> popCount{0};

    // Producer thread
    std::thread producer([&q, numItems] {
        for (int i = 0; i < numItems; i++) {
            q.push(i);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });

    // Consumer thread
    std::thread consumer([&q, &popCount, numItems] {
        while (popCount.load() < numItems) {
            auto result = q.pop(std::chrono::milliseconds(100));
            if (result.has_value()) {
                popCount.fetch_add(1);
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(popCount.load(), numItems);
    EXPECT_TRUE(q.empty());
}
