#include "io/TimeSeriesLogger.h"
#include "io/CsvWriter.h"
#include "io/JsonWriter.h"

#include <spdlog/spdlog.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace kt {

TimeSeriesLogger::TimeSeriesLogger() = default;

TimeSeriesLogger::~TimeSeriesLogger() {
    stop();
}

bool TimeSeriesLogger::start(const Settings& settings) {
    if (m_running.load())
        return false;

    m_settings = settings;
    std::filesystem::create_directories(settings.directory);

    m_running.store(true, std::memory_order_release);
    m_written.store(0, std::memory_order_release);
    m_thread = std::thread(&TimeSeriesLogger::writerLoop, this);

    spdlog::info("TimeSeriesLogger started: format={}, dir='{}'", settings.format, settings.directory);
    return true;
}

void TimeSeriesLogger::stop() {
    m_running.store(false, std::memory_order_release);
    m_queueCV.notify_one();
    if (m_thread.joinable())
        m_thread.join();
    spdlog::info("TimeSeriesLogger stopped: {} records written", m_written.load());
}

void TimeSeriesLogger::log(TimeSeriesRecord record) {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queue.push(std::move(record));
    }
    m_queueCV.notify_one();
}

void TimeSeriesLogger::writerLoop() {
    std::string filePath = generateFilePath(m_settings.directory, m_settings.format);

    std::unique_ptr<CsvWriter>  csvWriter;
    std::unique_ptr<JsonWriter> jsonWriter;

    if (m_settings.format == "json") {
        jsonWriter = std::make_unique<JsonWriter>();
        if (!jsonWriter->open(filePath)) {
            spdlog::error("TimeSeriesLogger: failed to open '{}'", filePath);
            return;
        }
    } else {
        csvWriter = std::make_unique<CsvWriter>();
        if (!csvWriter->open(filePath)) {
            spdlog::error("TimeSeriesLogger: failed to open '{}'", filePath);
            return;
        }
    }

    spdlog::info("TimeSeriesLogger: writing to '{}'", filePath);

    while (m_running.load(std::memory_order_acquire)) {
        std::vector<TimeSeriesRecord> batch;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait_for(lock,
                std::chrono::milliseconds(m_settings.flushIntervalMs),
                [&] { return !m_queue.empty() || !m_running.load(); });

            int count = std::min(static_cast<int>(m_queue.size()), m_settings.flushBatchSize);
            batch.reserve(static_cast<size_t>(count));
            for (int i = 0; i < count; ++i) {
                batch.push_back(std::move(m_queue.front()));
                m_queue.pop();
            }
        }

        for (const auto& rec : batch) {
            if (csvWriter)  csvWriter->write(rec);
            if (jsonWriter) jsonWriter->write(rec);
        }

        if (csvWriter)  csvWriter->flush();
        if (jsonWriter) jsonWriter->flush();

        m_written.fetch_add(batch.size(), std::memory_order_relaxed);
    }

    // Drain remaining
    std::lock_guard<std::mutex> lock(m_queueMutex);
    while (!m_queue.empty()) {
        auto& rec = m_queue.front();
        if (csvWriter)  csvWriter->write(rec);
        if (jsonWriter) jsonWriter->write(rec);
        m_queue.pop();
        m_written.fetch_add(1, std::memory_order_relaxed);
    }
    if (csvWriter)  csvWriter->flush();
    if (jsonWriter) jsonWriter->flush();
}

std::string TimeSeriesLogger::generateFilePath(const std::string& directory, const std::string& format) const {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss << directory << "/capture_"
        << std::put_time(&tm, "%Y%m%d_%H%M%S")
        << "." << (format == "json" ? "ndjson" : "csv");
    return oss.str();
}

} // namespace kt
