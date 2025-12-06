#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMutex>
#include <portaudio.h>

class DSPProcessor;

struct AudioDeviceInfo {
    int index;
    QString name;
    int maxInputChannels;
    int maxOutputChannels;
    double defaultSampleRate;
    bool isDefaultInput;
    bool isDefaultOutput;
    QString hostApi;
};

class AudioEngine : public QObject {
    Q_OBJECT

public:
    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine();

    // Initialization
    bool initialize();
    void terminate();
    bool isInitialized() const { return m_initialized; }

    // Device management
    QVector<AudioDeviceInfo> getInputDevices() const;
    QVector<AudioDeviceInfo> getOutputDevices() const;
    bool setInputDevice(int index);
    bool setOutputDevice(int index);
    int getInputDeviceIndex() const { return m_inputDeviceIndex; }
    int getOutputDeviceIndex() const { return m_outputDeviceIndex; }

    // Find VB-CABLE device
    int findVBCableDevice() const;

    // Stream control
    bool start();
    void stop();
    bool isRunning() const { return m_running; }

    // Configuration
    void setSampleRate(int rate);
    void setBufferSize(int frames);
    void setChannels(int channels);
    int getSampleRate() const { return m_sampleRate; }
    int getBufferSize() const { return m_bufferSize; }
    int getChannels() const { return m_channels; }

    // DSP processor
    void setDSPProcessor(DSPProcessor* processor);
    DSPProcessor* getDSPProcessor() const { return m_dspProcessor; }

    // Latency info
    double getInputLatency() const;
    double getOutputLatency() const;
    double getTotalLatency() const;

    // Debug: list all devices
    void logAllDevices() const;

signals:
    void audioDataReady(const QVector<float>& data);
    void spectrumDataReady(const QVector<float>& dryData, const QVector<float>& wetData);
    void errorOccurred(const QString& error);
    void latencyChanged(double inputMs, double outputMs);
    void levelChanged(float leftLevel, float rightLevel);

private:
    // PortAudio callback
    static int audioCallback(const void* inputBuffer, void* outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void* userData);

    // Instance callback handler
    int processAudio(const float* input, float* output, unsigned long frames,
                    const PaStreamCallbackTimeInfo* timeInfo,
                    PaStreamCallbackFlags statusFlags);

    // Calculate RMS level
    float calculateRMS(const float* buffer, int frames, int channel, int totalChannels);

    // PortAudio stream
    PaStream* m_stream = nullptr;

    // DSP processor (not owned)
    DSPProcessor* m_dspProcessor = nullptr;

    // Configuration
    int m_inputDeviceIndex = -1;
    int m_outputDeviceIndex = -1;
    int m_sampleRate = 48000;
    int m_bufferSize = 512;  // Increased default for stability
    int m_channels = 2;

    // Actual channel counts used in the stream (may differ from m_channels)
    int m_actualInputChannels = 2;
    int m_actualOutputChannels = 2;

    // State
    bool m_initialized = false;
    bool m_running = false;

    // Latency
    double m_inputLatency = 0.0;
    double m_outputLatency = 0.0;

    // Level metering
    float m_levelL = 0.0f;
    float m_levelR = 0.0f;
    int m_levelUpdateCounter = 0;
    static constexpr int LEVEL_UPDATE_INTERVAL = 4;

    // Visualization buffers (dry = before DSP, wet = after DSP)
    QVector<float> m_visualizationBuffer;
    QVector<float> m_dryVisualizationBuffer;
    QVector<float> m_wetVisualizationBuffer;
    QMutex m_visualizationMutex;
    int m_visualizationCounter = 0;
    static constexpr int VISUALIZATION_INTERVAL = 2;
};

#endif // AUDIOENGINE_H
