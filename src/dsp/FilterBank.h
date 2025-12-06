#ifndef FILTERBANK_H
#define FILTERBANK_H

#include "Parameters.h"
#include <QVector>

class FilterBank {
public:
    FilterBank();

    void setCoefficients(const QVector<Parameters::FilterCoeffs>& coeffs);
    void process(float* left, float* right, int numSamples);
    void processMono(float* buffer, int numSamples);
    void reset();

    int getStageCount() const { return m_stages.size(); }

private:
    struct BiquadState {
        // Coefficients
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;

        // State variables (Direct Form II Transposed)
        double z1L = 0.0, z2L = 0.0;  // Left channel
        double z1R = 0.0, z2R = 0.0;  // Right channel

        void reset() {
            z1L = z2L = z1R = z2R = 0.0;
        }
    };

    // Process single sample through one biquad stage
    inline double processBiquadSample(BiquadState& state, double input, double& z1, double& z2) {
        double output = state.b0 * input + z1;
        z1 = state.b1 * input - state.a1 * output + z2;
        z2 = state.b2 * input - state.a2 * output;
        return output;
    }

    QVector<BiquadState> m_stages;
};

#endif // FILTERBANK_H
