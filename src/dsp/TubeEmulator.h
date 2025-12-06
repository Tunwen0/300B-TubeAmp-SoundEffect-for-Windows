#ifndef TUBEEMULATOR_H
#define TUBEEMULATOR_H

class TubeEmulator {
public:
    TubeEmulator();

    // Processing
    void process(float* left, float* right, int numSamples);
    void processMono(float* buffer, int numSamples);
    void reset();
    void setSampleRate(int sampleRate);

private:
    struct Coefficients {
        double b[7] = {0};
        double a[6] = {0}; // a0 assumed to be 1
    };

    struct FilterState {
        double z[6] = {0};
    };

    float shapeSample(float x) const;
    double processFilter(double x, FilterState& state) const;

    void loadCoefficients(int sampleRate);

    Coefficients m_coeffs;
    FilterState m_stateL;
    FilterState m_stateR;

    float m_outputGainDb = 0.0f;
    static constexpr float kOutputScale = 1.33f;

    int m_sampleRate = 48000;
};

#endif // TUBEEMULATOR_H
