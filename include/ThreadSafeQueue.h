#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <optional>
#include <chrono>

// ══════════════════════════════════════════════════════════════
// ThreadSafeQueue<T> — Thread-safe producer-consumer kuyruğu
// ══════════════════════════════════════════════════════════════
//
// Template class: herhangi bir tip ile kullanılabilir.
// Header-only: template olduğu için tüm kod header'da olmalı.
//
// Senkronizasyon mekanizması:
//   std::mutex         → Eşzamanlı erişimi engeller
//   std::lock_guard    → push/tryPop/empty/size için (basit kilitleme)
//   std::unique_lock   → pop() için (condition_variable ile kullanım gerektirir)
//   condition_variable → Consumer'ın veri beklemesini sağlar (CPU kullanmadan)

template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;           // mutable: const fonksiyonlarda da kilitlenebilir
    std::condition_variable condVar_;
    std::atomic<bool> shutdown_{false};  // atomic: lock olmadan thread-safe okuma/yazma
    size_t maxSizeReached_{0};           // İstatistik: en yüksek kuyruk boyutu

public:
    // ── Producer tarafı: Kuyruğa veri ekle ──
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
            if (queue_.size() > maxSizeReached_) {
                maxSizeReached_ = queue_.size();
            }
        } // lock_guard burada otomatik serbest bırakılır (RAII)
        condVar_.notify_one(); // Bekleyen consumer'a "veri var" sinyali gönder
    }

    // ── Consumer tarafı: Kuyruktan veri al (beklemeli) ──
    // Timeout süresi kadar bekler. Veri gelirse döner, gelmezse nullopt döner.
    std::optional<T> pop(std::chrono::milliseconds timeout = std::chrono::milliseconds(500)) {
        std::unique_lock<std::mutex> lock(mutex_);

        // wait_for: condition sağlanana veya timeout dolana kadar bekle
        // Predicate (lambda): spurious wakeup koruması — koşul sağlanmadan uyanırsa
        // tekrar bekler
        bool ready = condVar_.wait_for(lock, timeout, [this] {
            return !queue_.empty() || shutdown_.load();
        });

        if (ready && !queue_.empty()) {
            T item = std::move(queue_.front());
            queue_.pop();
            return item;
        }
        return std::nullopt;
    }

    // ── Kuyruktan veri al (beklemesiz) ──
    std::optional<T> tryPop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return std::nullopt;
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    // ── Kuyruk boş mu? ──
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    // ── Kuyruk boyutu ──
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    // ── Kuyruğu kapat (tüm bekleyen thread'lere sinyal gönder) ──
    void shutdown() {
        shutdown_.store(true);
        condVar_.notify_all();
    }

    bool isShutdown() const {
        return shutdown_.load();
    }

    size_t getMaxSizeReached() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return maxSizeReached_;
    }
};

#endif // THREAD_SAFE_QUEUE_H
