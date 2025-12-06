#include "WaveformWidget.h"
#include <QPainter>
#include <QRandomGenerator>
#include <cmath>

WaveformWidget::WaveformWidget(QWidget* parent)
    : QWidget(parent)
    , m_animationTimer(new QTimer(this))
    , m_useRealData(false)
    , m_barCount(50)
    , m_topColor(0, 255, 255)      // Cyan
    , m_bottomColor(0, 120, 255)   // Blue
    , m_backgroundColor(30, 40, 50)
{
    setFixedHeight(40);

    // Initialize with random heights
    m_bars.resize(m_barCount);
    for (int i = 0; i < m_barCount; ++i) {
        m_bars[i] = QRandomGenerator::global()->bounded(100);
    }

    // Animation timer for simulation mode
    connect(m_animationTimer, &QTimer::timeout, this, &WaveformWidget::onAnimationTick);
    m_animationTimer->start(50);  // 20 FPS for simulation
}

WaveformWidget::~WaveformWidget() {
    if (m_animationTimer->isActive()) {
        m_animationTimer->stop();
    }
}

void WaveformWidget::updateFromAudioData(const QVector<float>& audioSamples) {
    m_useRealData = true;
    convertAudioToLevels(audioSamples);
    update();
}

void WaveformWidget::updateFromLevels(const QVector<int>& levels) {
    m_useRealData = true;
    m_bars = levels;

    // Resize if needed
    if (m_bars.size() != m_barCount) {
        if (m_bars.size() > m_barCount) {
            // Downsample
            QVector<int> resampled(m_barCount);
            float ratio = static_cast<float>(m_bars.size()) / m_barCount;
            for (int i = 0; i < m_barCount; ++i) {
                int srcIdx = static_cast<int>(i * ratio);
                resampled[i] = m_bars[qMin(srcIdx, m_bars.size() - 1)];
            }
            m_bars = resampled;
        } else {
            m_bars.resize(m_barCount);
        }
    }

    update();
}

void WaveformWidget::setSimulationMode(bool simulate) {
    m_useRealData = !simulate;
}

void WaveformWidget::setBarCount(int count) {
    m_barCount = qMax(10, qMin(count, 200));
    m_bars.resize(m_barCount);
}

void WaveformWidget::setBarColor(const QColor& topColor, const QColor& bottomColor) {
    m_topColor = topColor;
    m_bottomColor = bottomColor;
    update();
}

void WaveformWidget::setBackgroundColor(const QColor& color) {
    m_backgroundColor = color;
    update();
}

void WaveformWidget::onAnimationTick() {
    if (!m_useRealData) {
        // Simulate waveform animation
        for (int& h : m_bars) {
            int noise = QRandomGenerator::global()->bounded(-80, 81);
            h = qBound(5, h + noise, 100);
        }
        update();
    }
}

void WaveformWidget::convertAudioToLevels(const QVector<float>& samples) {
    if (samples.isEmpty()) return;

    // Calculate how many samples per bar
    int samplesPerBar = samples.size() / m_barCount;
    if (samplesPerBar < 1) samplesPerBar = 1;

    m_bars.resize(m_barCount);

    for (int i = 0; i < m_barCount; ++i) {
        int startIdx = i * samplesPerBar;
        int endIdx = qMin(startIdx + samplesPerBar, samples.size());

        // Calculate RMS for this segment
        float sum = 0.0f;
        for (int j = startIdx; j < endIdx; ++j) {
            sum += samples[j] * samples[j];
        }
        float rms = std::sqrt(sum / (endIdx - startIdx));

        // Convert to 0-100 scale with some gain
        int level = static_cast<int>(rms * 300.0f);
        m_bars[i] = qBound(5, level, 100);
    }
}

void WaveformWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(rect(), m_backgroundColor);

    if (m_bars.isEmpty()) return;

    int count = m_bars.size();
    double unitWidth = static_cast<double>(width()) / count;

    // Ensure minimum bar width
    double barWidth = unitWidth * 0.9;
    if (barWidth < 1.5) barWidth = 1.5;

    for (int i = 0; i < count; ++i) {
        int h = m_bars[i] * height() / 120;
        int x = static_cast<int>(i * unitWidth);
        int y = height() - h;

        // Gradient for bar
        QLinearGradient gradient(x, y, x, y + h);
        gradient.setColorAt(0, m_topColor);
        gradient.setColorAt(1, m_bottomColor);

        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);
        painter.drawRect(x, y, static_cast<int>(barWidth), h);
    }
}
