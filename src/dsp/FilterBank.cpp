#include "FilterBank.h"
#include <cmath>

FilterBank::FilterBank() {
}

void FilterBank::setCoefficients(const QVector<Parameters::FilterCoeffs>& coeffs) {
    m_stages.clear();
    m_stages.reserve(coeffs.size());

    for (const auto& fc : coeffs) {
        BiquadState state;
        state.b0 = fc.b0;
        state.b1 = fc.b1;
        state.b2 = fc.b2;
        state.a1 = fc.a1;
        state.a2 = fc.a2;
        state.reset();
        m_stages.append(state);
    }
}

void FilterBank::process(float* left, float* right, int numSamples) {
    if (m_stages.isEmpty()) return;

    for (int i = 0; i < numSamples; ++i) {
        double sampleL = static_cast<double>(left[i]);
        double sampleR = static_cast<double>(right[i]);

        // Process through all biquad stages
        for (auto& stage : m_stages) {
            sampleL = processBiquadSample(stage, sampleL, stage.z1L, stage.z2L);
            sampleR = processBiquadSample(stage, sampleR, stage.z1R, stage.z2R);
        }

        left[i] = static_cast<float>(sampleL);
        right[i] = static_cast<float>(sampleR);
    }
}

void FilterBank::processMono(float* buffer, int numSamples) {
    if (m_stages.isEmpty()) return;

    for (int i = 0; i < numSamples; ++i) {
        double sample = static_cast<double>(buffer[i]);

        // Process through all biquad stages (using left channel state)
        for (auto& stage : m_stages) {
            sample = processBiquadSample(stage, sample, stage.z1L, stage.z2L);
        }

        buffer[i] = static_cast<float>(sample);
    }
}

void FilterBank::reset() {
    for (auto& stage : m_stages) {
        stage.reset();
    }
}
