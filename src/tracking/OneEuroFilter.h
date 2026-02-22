#pragma once

#include <cmath>

namespace kt {

constexpr double kPi = 3.14159265358979323846;

/// 1€ Filter for a single scalar value.
/// Adaptive low-pass filter: low jitter at rest, low lag during fast motion.
/// Reference: Casiez et al., "1€ Filter: A Simple Speed-based Low-pass Filter
///            for Noisy Input in Interactive Systems", CHI 2012.
class OneEuroFilter {
public:
    struct Settings {
        double minCutoff = 1.0;     // Hz — minimum cutoff frequency
        double beta      = 0.007;   // speed coefficient
        double dCutoff   = 1.0;     // Hz — cutoff for derivative estimation
    };

    explicit OneEuroFilter(const Settings& settings = {})
        : m_settings(settings) {}

    double filter(double value, double timestamp) {
        if (!m_initialized) {
            m_xPrev   = value;
            m_dxPrev  = 0.0;
            m_tPrev   = timestamp;
            m_initialized = true;
            return value;
        }

        double dt = timestamp - m_tPrev;
        if (dt <= 0.0)
            dt = 1e-6;  // avoid division by zero

        // Estimate derivative
        double dx     = (value - m_xPrev) / dt;
        double alphaD = smoothingFactor(dt, m_settings.dCutoff);
        double dxHat  = exponentialSmooth(alphaD, dx, m_dxPrev);

        // Adaptive cutoff
        double cutoff = m_settings.minCutoff + m_settings.beta * std::abs(dxHat);
        double alpha  = smoothingFactor(dt, cutoff);
        double xHat   = exponentialSmooth(alpha, value, m_xPrev);

        m_xPrev  = xHat;
        m_dxPrev = dxHat;
        m_tPrev  = timestamp;

        return xHat;
    }

    void reset() { m_initialized = false; }

    Settings& settings() { return m_settings; }
    const Settings& settings() const { return m_settings; }

private:
    static double smoothingFactor(double dt, double cutoff) {
        double tau = 1.0 / (2.0 * kPi * cutoff);
        return 1.0 / (1.0 + tau / dt);
    }

    static double exponentialSmooth(double alpha, double x, double xPrev) {
        return alpha * x + (1.0 - alpha) * xPrev;
    }

    Settings m_settings;
    bool     m_initialized = false;
    double   m_xPrev  = 0.0;
    double   m_dxPrev = 0.0;
    double   m_tPrev  = 0.0;
};

} // namespace kt
