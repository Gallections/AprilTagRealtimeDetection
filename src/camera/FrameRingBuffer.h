#pragma once

#include "common/Types.h"

#include <atomic>
#include <optional>
#include <vector>

namespace kt {

/// Lock-free Single-Producer Single-Consumer (SPSC) ring buffer.
/// One instance per camera → one producer thread, one consumer thread.
class FrameRingBuffer {
public:
    explicit FrameRingBuffer(size_t capacity = 8)
        : m_capacity(capacity + 1)   // +1 so we can distinguish full from empty
        , m_buffer(m_capacity)
        , m_head(0)
        , m_tail(0) {}

    /// Producer: try to push; returns false if full.
    bool tryPush(Frame frame) {
        const size_t head     = m_head.load(std::memory_order_relaxed);
        const size_t nextHead = next(head);
        if (nextHead == m_tail.load(std::memory_order_acquire))
            return false;

        m_buffer[head] = std::move(frame);
        m_head.store(nextHead, std::memory_order_release);
        return true;
    }

    /// Producer: push, overwriting the oldest frame if full.
    void pushOverwrite(Frame frame) {
        const size_t head     = m_head.load(std::memory_order_relaxed);
        const size_t nextHead = next(head);
        if (nextHead == m_tail.load(std::memory_order_acquire)) {
            // Drop oldest by advancing tail
            size_t oldTail = m_tail.load(std::memory_order_relaxed);
            m_tail.store(next(oldTail), std::memory_order_release);
        }
        m_buffer[head] = std::move(frame);
        m_head.store(nextHead, std::memory_order_release);
    }

    /// Consumer: pop the oldest frame if available.
    std::optional<Frame> tryPop() {
        const size_t tail = m_tail.load(std::memory_order_relaxed);
        if (tail == m_head.load(std::memory_order_acquire))
            return std::nullopt;

        Frame frame = std::move(m_buffer[tail]);
        m_tail.store(next(tail), std::memory_order_release);
        return frame;
    }

    /// Consumer: peek at the newest frame without removing.
    /// Useful for the UI to grab the latest frame without consuming it.
    std::optional<Frame> peekNewest() const {
        const size_t head = m_head.load(std::memory_order_acquire);
        const size_t tail = m_tail.load(std::memory_order_acquire);
        if (head == tail)
            return std::nullopt;

        size_t newest = (head == 0) ? (m_capacity - 1) : (head - 1);
        // Return a copy (does not advance tail)
        return m_buffer[newest];
    }

    bool   empty()    const { return m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire); }
    size_t capacity() const { return m_capacity - 1; }

    size_t size() const {
        const size_t h = m_head.load(std::memory_order_acquire);
        const size_t t = m_tail.load(std::memory_order_acquire);
        return (h >= t) ? (h - t) : (m_capacity - t + h);
    }

private:
    size_t next(size_t idx) const { return (idx + 1) % m_capacity; }

    size_t             m_capacity;
    std::vector<Frame> m_buffer;

    // Separate cache lines to avoid false sharing
    alignas(64) std::atomic<size_t> m_head;
    alignas(64) std::atomic<size_t> m_tail;
};

} // namespace kt
