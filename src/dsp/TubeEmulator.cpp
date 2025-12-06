#include "TubeEmulator.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
struct RateEntry {
    int rate;
    std::array<double, 14> coeffs;
};

// Coefficients extracted from resources/储存的参数.txt (xmmword_42C0..42B0 blocks)
static const RateEntry kRateTable[] = {
    {44100, {0.848837734156434, -2.879886361821670, 2.725585256735570, 0.930839365623406,
             -3.079589350505240, 1.770725087918830, -0.316511731835608, 1.000000000000000,
             -3.483284779450430, 3.477549116048100, 0.933540396458836, -3.832907894267460,
             2.371422474712170, -0.466319313229480}},
    {48000, {0.859613953341805, -2.881262866479970, 2.716760030674950, 0.764865085654241,
             -2.676598511921890, 1.440192350820560, -0.223570041868478, 1.000000000000000,
             -3.441768540626980, 3.415464677676710, 0.767170485973692, -3.376459849090420,
             1.998392624648130, -0.362799398359903}},
    {88200, {0.867388389271491, -3.568372014755150, 5.446137758181810, -3.445401648026590,
             0.332293626439954, 0.524596404405209, -0.156642515506745, 1.000000000000000,
             -4.098256690320620, 6.107478187022560, -3.444212228440640, -0.329642119552943,
             1.053291660384730, -0.288658809083111}},
    {96000, {0.859562598352408, -3.598222595366990, 5.574911775924850, -3.506393779647540,
             0.158511343920126, 0.736248938801894, -0.224618281979237, 1.000000000000000,
             -4.159415663760060, 6.275407359110680, -3.505236268570930, -0.542563394277778,
             1.296284496118340, -0.364476528614760}},
    {176400, {0.898810453231595, -4.361647014994980, 8.575765118892219, -8.662152351049730,
              4.666181822924430, -1.233594712287200, 0.116636683283928, 1.000000000000000,
              -4.766184829159850, 9.081044865157910, -8.661697742636489, 4.160674710632680,
              -0.829511506535580, 0.015674502541581}},
    {192000, {0.887929471244264, -4.271530571680550, 8.229258617877781, -7.959864259669550,
              3.900406089953790, -0.808177841238952, 0.021978493513359, 1.000000000000000,
              -4.719585961357560, 8.788926590382619, -7.959401737733720, 3.340506816466110,
              -0.360584973497765, -0.089860734259541}},
};

const RateEntry& pickRate(int rate) {
    // Exact match first
    for (const auto& entry : kRateTable) {
        if (entry.rate == rate) return entry;
    }

    // Fallback to nearest
    const RateEntry* best = &kRateTable[0];
    int bestDiff = std::abs(rate - kRateTable[0].rate);
    for (const auto& entry : kRateTable) {
        int diff = std::abs(rate - entry.rate);
        if (diff < bestDiff) {
            best = &entry;
            bestDiff = diff;
        }
    }
    return *best;
}
} // namespace

TubeEmulator::TubeEmulator() {
    loadCoefficients(m_sampleRate);
    reset();
}

void TubeEmulator::setSampleRate(int sampleRate) {
    if (sampleRate != m_sampleRate) {
        m_sampleRate = sampleRate;
        loadCoefficients(sampleRate);
        reset();
    }
}

void TubeEmulator::reset() {
    m_stateL = FilterState{};
    m_stateR = FilterState{};
}

void TubeEmulator::loadCoefficients(int sampleRate) {
    const auto& entry = pickRate(sampleRate);
    m_coeffs = {};
    // Feedforward b0..b6
    for (int i = 0; i < 7; ++i) {
        m_coeffs.b[i] = entry.coeffs[i];
    }
    // Feedback a1..a6 (a0 is 1.0)
    for (int i = 0; i < 6; ++i) {
        m_coeffs.a[i] = entry.coeffs[8 + i]; // skip the a0=1.0 at index 7
    }
}

float TubeEmulator::shapeSample(float x) const {
    // Recreate the log/exp soft saturation from resources/原始代码.txt
    float scaled = x * 0.75f;
    float shaped = scaled * 0.85f - std::log(1.0f - scaled) * 0.15f;

    float comp = scaled * 0.9f;
    float absComp = std::fabs(comp);

    if (absComp < shaped) {
        float diff = shaped - absComp;
        float expv = std::exp(-diff);
        shaped = absComp + diff / (expv + 1.0f);
    } else if (-absComp > shaped) {
        float sum = absComp + shaped;
        float expv = std::exp(-sum);
        shaped = (sum / (expv + 1.0f)) - absComp;
    }

    return shaped;
}

double TubeEmulator::processFilter(double x, FilterState& state) const {
    // High-order IIR in transposed form (7 feedforward taps, 6 feedback)
    double y = m_coeffs.b[0] * x + state.z[0];
    state.z[0] = m_coeffs.b[1] * x - m_coeffs.a[0] * y + state.z[1];
    state.z[1] = m_coeffs.b[2] * x - m_coeffs.a[1] * y + state.z[2];
    state.z[2] = m_coeffs.b[3] * x - m_coeffs.a[2] * y + state.z[3];
    state.z[3] = m_coeffs.b[4] * x - m_coeffs.a[3] * y + state.z[4];
    state.z[4] = m_coeffs.b[5] * x - m_coeffs.a[4] * y + state.z[5];
    state.z[5] = m_coeffs.b[6] * x - m_coeffs.a[5] * y;
    return y;
}

void TubeEmulator::process(float* left, float* right, int numSamples) {
    const double gain = std::pow(10.0, static_cast<double>(m_outputGainDb) / 20.0);

    for (int i = 0; i < numSamples; ++i) {
        double shapedL = shapeSample(left[i]);
        double shapedR = shapeSample(right[i]);

        double filteredL = processFilter(shapedL, m_stateL);
        double filteredR = processFilter(shapedR, m_stateR);

        left[i] = static_cast<float>(filteredL * kOutputScale * gain);
        right[i] = static_cast<float>(filteredR * kOutputScale * gain);
    }
}

void TubeEmulator::processMono(float* buffer, int numSamples) {
    const double gain = std::pow(10.0, static_cast<double>(m_outputGainDb) / 20.0);

    for (int i = 0; i < numSamples; ++i) {
        double shaped = shapeSample(buffer[i]);
        double filtered = processFilter(shaped, m_stateL);
        buffer[i] = static_cast<float>(filtered * kOutputScale * gain);
    }
}
