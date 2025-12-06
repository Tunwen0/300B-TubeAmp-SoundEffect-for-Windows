#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H

#include <QWidget>
#include <QVector>
#include <QTimer>
#include <complex>

/**
 * SpectrumWidget - ECG-style scrolling waveform display
 *
 * Displays scrolling waveforms comparing dry/wet signals:
 * - Blue: Original (dry) signal
 * - Green: Processed (wet) signal
 *
 * The waveforms scroll from right to left like a heart monitor,
 * allowing audiophiles to see the real-time difference between
 * the original and tube-processed audio.
 */
class SpectrumWidget : public QWidget {
    Q_OBJECT

public:
    explicit SpectrumWidget(QWidget* parent = nullptr);

    // Update with both dry and wet audio data
    void updateSpectrum(const QVector<float>& dryData, const QVector<float>& wetData);

    // For backwards compatibility - updates wet only
    void updateFromAudioData(const QVector<float>& data);

    // Simulation mode when not processing
    void setSimulationMode(bool enabled);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // Push a new sample to the history (scrolls left)
    void pushSample(float drySample, float wetSample);

    // Legacy FFT methods (kept for API compatibility, not used)
    void computeFFT(const QVector<float>& input, QVector<float>& magnitudes);
    void fft(std::vector<std::complex<float>>& data);
    void applyWindow(QVector<float>& data);
    void smoothSpectrum(QVector<float>& current, const QVector<float>& target);
    float toDecibels(float magnitude) const;

    // Waveform history buffers
    QVector<float> m_dryHistory;
    QVector<float> m_wetHistory;

    // Display parameters
    static constexpr int HISTORY_SIZE = 300;  // Number of samples to display

    // Smoothing state
    float m_lastDry = 0.0f;
    float m_lastWet = 0.0f;

    // Animation
    QTimer* m_animationTimer = nullptr;

    // Simulation mode
    bool m_simulationMode = true;
    float m_simPhase = 0.0f;

    // Colors
    QColor m_dryColor{100, 150, 255, 200};   // Blue
    QColor m_wetColor{100, 255, 150, 220};   // Green
    QColor m_gridColor{50, 50, 55};
    QColor m_bgColor{20, 20, 25};
};

#endif // SPECTRUMWIDGET_H
