#ifndef WAVEFORMWIDGET_H
#define WAVEFORMWIDGET_H

#include <QTimer>
#include <QVector>
#include <QWidget>

class WaveformWidget : public QWidget {
    Q_OBJECT

public:
    explicit WaveformWidget(QWidget* parent = nullptr);
    ~WaveformWidget();

    // Update waveform from real audio data (float samples)
    void updateFromAudioData(const QVector<float>& audioSamples);

    // Update from integer levels (0-100)
    void updateFromLevels(const QVector<int>& levels);

    // Mode control
    void setSimulationMode(bool simulate);
    bool isSimulationMode() const { return !m_useRealData; }

    // Visual settings
    void setBarCount(int count);
    void setBarColor(const QColor& topColor, const QColor& bottomColor);
    void setBackgroundColor(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onAnimationTick();

private:
    void convertAudioToLevels(const QVector<float>& samples);

    QVector<int> m_bars;
    QTimer* m_animationTimer;
    bool m_useRealData;

    // Visual settings
    int m_barCount;
    QColor m_topColor;
    QColor m_bottomColor;
    QColor m_backgroundColor;
};

#endif // WAVEFORMWIDGET_H
