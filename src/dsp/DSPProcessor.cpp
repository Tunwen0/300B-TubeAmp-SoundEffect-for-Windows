#include "DSPProcessor.h"
#include "../utils/Logger.h"

DSPProcessor::DSPProcessor() {
    LOG_INFO("DSPProcessor initialized - Audiophile mode");
}

DSPProcessor::~DSPProcessor() {
}

void DSPProcessor::setSampleRate(int rate) {
    if (rate != m_sampleRate) {
        m_sampleRate = rate;
        m_tubeEmulator.setSampleRate(rate);
        reset();
        LOG_INFO(QString("DSPProcessor sample rate: %1 Hz").arg(rate));
    }
}

void DSPProcessor::process(float* buffer, int numFrames, int numChannels) {
    if (m_bypass.load(std::memory_order_relaxed)) {
        return;  // Pass through unchanged
    }

    if (numChannels == 2) {
        processInterleaved(buffer, numFrames);
    } else if (numChannels == 1) {
        m_tubeEmulator.processMono(buffer, numFrames);
    }
}

void DSPProcessor::processInterleaved(float* buffer, int numFrames) {
    // Process interleaved stereo efficiently
    // De-interleave to separate channels for processing

    constexpr int STACK_THRESHOLD = 1024;
    float stackBufferL[STACK_THRESHOLD];
    float stackBufferR[STACK_THRESHOLD];

    float* left;
    float* right;
    bool useHeap = numFrames > STACK_THRESHOLD;

    if (useHeap) {
        left = new float[numFrames];
        right = new float[numFrames];
    } else {
        left = stackBufferL;
        right = stackBufferR;
    }

    // De-interleave
    for (int i = 0; i < numFrames; ++i) {
        left[i] = buffer[i * 2];
        right[i] = buffer[i * 2 + 1];
    }

    // Apply tube emulation
    m_tubeEmulator.process(left, right, numFrames);

    // Re-interleave
    for (int i = 0; i < numFrames; ++i) {
        buffer[i * 2] = left[i];
        buffer[i * 2 + 1] = right[i];
    }

    if (useHeap) {
        delete[] left;
        delete[] right;
    }
}

void DSPProcessor::setBypass(bool bypass) {
    m_bypass.store(bypass, std::memory_order_relaxed);
}

void DSPProcessor::reset() {
    m_tubeEmulator.reset();
}
