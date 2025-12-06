#ifndef DSPPROCESSOR_H
#define DSPPROCESSOR_H

#include "TubeEmulator.h"
#include <atomic>

/**
 * DSPProcessor - Audiophile-grade audio processing
 *
 * Pre-tuned for optimal sound quality with no user adjustment needed.
 * Provides subtle 300B tube warmth enhancement.
 */
class DSPProcessor {
public:
    DSPProcessor();
    ~DSPProcessor();

    // Configuration
    void setSampleRate(int rate);
    int getSampleRate() const { return m_sampleRate; }

    // Real-time processing (called from audio thread)
    void process(float* buffer, int numFrames, int numChannels);

    // Bypass control
    void setBypass(bool bypass);
    bool isBypassed() const { return m_bypass.load(); }

    // Reset state
    void reset();

private:
    void processInterleaved(float* buffer, int numFrames);

    // DSP component - optimized tube emulation
    TubeEmulator m_tubeEmulator;

    std::atomic<bool> m_bypass{false};
    int m_sampleRate = 48000;
};

#endif // DSPPROCESSOR_H
