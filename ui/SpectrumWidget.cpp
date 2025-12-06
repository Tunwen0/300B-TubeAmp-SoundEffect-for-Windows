#include "SpectrumWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>
#include <algorithm>

SpectrumWidget::SpectrumWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(100);
    setAttribute(Qt::WA_OpaquePaintEvent, false);

    // Initialize waveform history buffers
    m_dryHistory.resize(HISTORY_SIZE, 0.0f);
    m_wetHistory.resize(HISTORY_SIZE, 0.0f);

    // Start animation timer for smooth scrolling
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, [this]() {
        if (!m_simulationMode) {
            update();
        }
    });
    m_animationTimer->start(16);  // ~60 FPS
}

void SpectrumWidget::updateSpectrum(const QVector<float>& dryData, const QVector<float>& wetData) {
    m_simulationMode = false;

    if (dryData.isEmpty() || wetData.isEmpty()) return;

    // Calculate RMS or peak for visualization
    float dryLevel = 0.0f;
    float wetLevel = 0.0f;

    // Use peak detection for more responsive visualization
    int drySize = static_cast<int>(dryData.size());
    int wetSize = static_cast<int>(wetData.size());

    for (int i = 0; i < drySize; ++i) {
        dryLevel = std::max(dryLevel, std::abs(dryData[i]));
    }
    for (int i = 0; i < wetSize; ++i) {
        wetLevel = std::max(wetLevel, std::abs(wetData[i]));
    }

    // Push multiple samples for smoother waveform
    int samplesToAdd = std::max(1, drySize / 64);
    for (int i = 0; i < samplesToAdd; ++i) {
        int idx = (i * 64) % drySize;
        pushSample(dryData[idx], wetData[idx]);
    }
}

void SpectrumWidget::updateFromAudioData(const QVector<float>& data) {
    m_simulationMode = false;

    if (data.isEmpty()) return;

    int dataSize = static_cast<int>(data.size());

    // Just use for wet signal (legacy compatibility)
    float level = 0.0f;
    for (int i = 0; i < dataSize; ++i) {
        level = std::max(level, std::abs(data[i]));
    }

    int samplesToAdd = std::max(1, dataSize / 64);
    for (int i = 0; i < samplesToAdd; ++i) {
        int idx = (i * 64) % dataSize;
        pushSample(data[idx] * 0.95f, data[idx]);  // Simulate slight difference
    }
}

void SpectrumWidget::setSimulationMode(bool enabled) {
    m_simulationMode = enabled;
    if (enabled) {
        // Clear history - flat line at zero
        std::fill(m_dryHistory.begin(), m_dryHistory.end(), 0.0f);
        std::fill(m_wetHistory.begin(), m_wetHistory.end(), 0.0f);
        m_lastDry = 0.0f;
        m_lastWet = 0.0f;
        update();  // Repaint to show flat line
    }
}

void SpectrumWidget::pushSample(float drySample, float wetSample) {
    // Shift history left and add new sample at the end
    for (int i = 0; i < HISTORY_SIZE - 1; ++i) {
        m_dryHistory[i] = m_dryHistory[i + 1];
        m_wetHistory[i] = m_wetHistory[i + 1];
    }

    // Smooth the input
    float smoothedDry = m_lastDry * 0.7f + drySample * 0.3f;
    float smoothedWet = m_lastWet * 0.7f + wetSample * 0.3f;

    m_dryHistory[HISTORY_SIZE - 1] = smoothedDry;
    m_wetHistory[HISTORY_SIZE - 1] = smoothedWet;

    m_lastDry = smoothedDry;
    m_lastWet = smoothedWet;
}

void SpectrumWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
}

void SpectrumWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    const int marginLeft = 5;
    const int marginRight = 5;
    const int marginTop = 18;
    const int marginBottom = 5;
    const int graphW = w - marginLeft - marginRight;
    const int graphH = h - marginTop - marginBottom;
    const int centerY = marginTop + graphH / 2;

    // Background
    painter.fillRect(rect(), m_bgColor);

    // Draw grid lines
    painter.setPen(QPen(m_gridColor, 1, Qt::DotLine));

    // Horizontal center line (zero line)
    painter.drawLine(marginLeft, centerY, w - marginRight, centerY);

    // Horizontal amplitude lines (+/- 0.5)
    int quarterH = graphH / 4;
    painter.drawLine(marginLeft, centerY - quarterH, w - marginRight, centerY - quarterH);
    painter.drawLine(marginLeft, centerY + quarterH, w - marginRight, centerY + quarterH);

    // Vertical time markers
    for (int i = 1; i < 4; ++i) {
        int x = marginLeft + (graphW * i) / 4;
        painter.drawLine(x, marginTop, x, h - marginBottom);
    }

    // Helper lambda to draw waveform
    auto drawWaveform = [&](const QVector<float>& history, const QColor& color, float offsetY = 0.0f) {
        if (history.isEmpty()) return;

        QPainterPath path;
        bool started = false;
        int histSize = static_cast<int>(history.size());

        for (int i = 0; i < histSize; ++i) {
            float x = marginLeft + (static_cast<float>(i) / (histSize - 1)) * graphW;

            // Clamp and scale amplitude
            float amp = std::clamp(history[i], -1.0f, 1.0f);
            float y = centerY - (amp * (graphH / 2 - 2)) + offsetY;

            if (!started) {
                path.moveTo(x, y);
                started = true;
            } else {
                path.lineTo(x, y);
            }
        }

        // Draw glow effect
        QColor glowColor = color;
        glowColor.setAlpha(30);
        painter.setPen(QPen(glowColor, 4.0));
        painter.drawPath(path);

        // Draw main line
        painter.setPen(QPen(color, 1.5));
        painter.drawPath(path);
    };

    // Draw dry waveform (blue) - slightly offset up
    drawWaveform(m_dryHistory, m_dryColor, -1.0f);

    // Draw wet waveform (green) - slightly offset down
    drawWaveform(m_wetHistory, m_wetColor, 1.0f);

    // Legend
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    // Draw legend with small line samples
    int legendY = 12;

    // Dry label
    painter.setPen(QPen(m_dryColor, 2));
    painter.drawLine(marginLeft + 5, legendY, marginLeft + 20, legendY);
    painter.setPen(m_dryColor);
    painter.drawText(marginLeft + 25, legendY + 4, QString::fromUtf8("原始"));

    // Wet label
    painter.setPen(QPen(m_wetColor, 2));
    painter.drawLine(marginLeft + 70, legendY, marginLeft + 85, legendY);
    painter.setPen(m_wetColor);
    painter.drawText(marginLeft + 90, legendY + 4, QString::fromUtf8("处理后"));

    // Time indicator
    painter.setPen(QColor(80, 80, 80));
    painter.drawText(w - marginRight - 45, h - 2, QString::fromUtf8("时间 →"));
}

// Keep these methods for API compatibility (they're declared in header)
void SpectrumWidget::computeFFT(const QVector<float>& input, QVector<float>& magnitudes) {
    Q_UNUSED(input);
    Q_UNUSED(magnitudes);
    // Not used in waveform mode
}

void SpectrumWidget::fft(std::vector<std::complex<float>>& data) {
    Q_UNUSED(data);
    // Not used in waveform mode
}

void SpectrumWidget::applyWindow(QVector<float>& data) {
    Q_UNUSED(data);
    // Not used in waveform mode
}

void SpectrumWidget::smoothSpectrum(QVector<float>& current, const QVector<float>& target) {
    Q_UNUSED(current);
    Q_UNUSED(target);
    // Not used in waveform mode
}

float SpectrumWidget::toDecibels(float magnitude) const {
    Q_UNUSED(magnitude);
    return 0.0f;
    // Not used in waveform mode
}
