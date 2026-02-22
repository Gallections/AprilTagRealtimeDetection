#pragma once

#include "common/Types.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace kt {

class CsvWriter;
class JsonWriter;

/// Async time-series logger. Records are pushed from any thread and
/// flushed to disk by a dedicated writer thread (MPSC pattern).
class TimeSeriesLogger {
public:
    struct Settings {
        std::string format    = "csv";    // "csv" or "json"
        std::string directory = "./data";
        int flushIntervalMs   = 100;
        int flushBatchSize    = 1000;
    };

    TimeSeriesLogger();
    ~TimeSeriesLogger();

    bool start(const Settings& settings);
    void stop();

    /// Thread-safe: enqueue a record for async writing.
    void log(TimeSeriesRecord record);

    bool isRunning() const { return m_running.load(std::memory_order_acquire); }
    uint64_t recordsWritten() const { return m_written.load(std::memory_order_acquire); }

private:
    void writerLoop();
    std::string generateFilePath(const std::string& directory, const std::string& format) const;

    Settings                        m_settings;
    std::thread                     m_thread;
    std::atomic<bool>               m_running{false};

    std::mutex                      m_queueMutex;
    std::condition_variable         m_queueCV;
    std::queue<TimeSeriesRecord>    m_queue;

    std::atomic<uint64_t>           m_written{0};
};

} // namespace kt
