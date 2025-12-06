#include "AudioEngine.h"
#include "../dsp/DSPProcessor.h"
#include "../utils/Logger.h"
#include <cmath>
#include <cstring>
#include <algorithm>

// Find WASAPI host API index (preferred for Windows low-latency audio)
static int getWasapiHostApiIndex() {
    int numHostApis = Pa_GetHostApiCount();
    for (int i = 0; i < numHostApis; ++i) {
        const PaHostApiInfo* hostApi = Pa_GetHostApiInfo(i);
        if (hostApi && QString::fromUtf8(hostApi->name).contains("WASAPI", Qt::CaseInsensitive)) {
            return i;
        }
    }
    return -1;  // Not found, will use all devices
}

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
{
}

AudioEngine::~AudioEngine() {
    stop();
    terminate();
}

bool AudioEngine::initialize() {
    if (m_initialized) {
        return true;
    }

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        LOG_ERROR(QString("PortAudio initialization failed: %1").arg(Pa_GetErrorText(err)));
        emit errorOccurred(QString("PortAudio initialization failed: %1").arg(Pa_GetErrorText(err)));
        return false;
    }

    m_initialized = true;
    LOG_INFO(QString("PortAudio initialized. Version: %1").arg(Pa_GetVersionText()));

    // Log all devices for debugging
    logAllDevices();

    // Find WASAPI host API for better default device selection
    int wasapiIndex = getWasapiHostApiIndex();
    const PaHostApiInfo* wasapiInfo = wasapiIndex >= 0 ? Pa_GetHostApiInfo(wasapiIndex) : nullptr;

    // Try to find VB-CABLE device (prefers WASAPI)
    int vbCableIndex = findVBCableDevice();
    if (vbCableIndex >= 0) {
        m_inputDeviceIndex = vbCableIndex;
        const PaDeviceInfo* info = Pa_GetDeviceInfo(vbCableIndex);
        LOG_INFO(QString("Found VB-CABLE at device index %1: %2").arg(vbCableIndex).arg(info ? info->name : "unknown"));
    } else {
        // Use WASAPI default input if available, otherwise global default
        if (wasapiInfo && wasapiInfo->defaultInputDevice >= 0) {
            m_inputDeviceIndex = wasapiInfo->defaultInputDevice;
            LOG_INFO("VB-CABLE not found, using WASAPI default input device");
        } else {
            m_inputDeviceIndex = Pa_GetDefaultInputDevice();
            LOG_WARNING("VB-CABLE not found, using global default input device");
        }
    }

    // Use WASAPI default output if available, otherwise global default
    if (wasapiInfo && wasapiInfo->defaultOutputDevice >= 0) {
        m_outputDeviceIndex = wasapiInfo->defaultOutputDevice;
    } else {
        m_outputDeviceIndex = Pa_GetDefaultOutputDevice();
    }

    const PaDeviceInfo* outInfo = Pa_GetDeviceInfo(m_outputDeviceIndex);
    if (outInfo) {
        LOG_INFO(QString("Default output device: %1").arg(outInfo->name));
    }

    return true;
}

void AudioEngine::terminate() {
    if (!m_initialized) {
        return;
    }

    stop();
    Pa_Terminate();
    m_initialized = false;
    LOG_INFO("PortAudio terminated");
}

void AudioEngine::logAllDevices() const {
    if (!m_initialized) return;

    int numDevices = Pa_GetDeviceCount();
    int numHostApis = Pa_GetHostApiCount();
    int wasapiIndex = getWasapiHostApiIndex();

    LOG_INFO(QString("===== Audio Devices (%1 total, %2 host APIs) =====").arg(numDevices).arg(numHostApis));
    LOG_INFO(QString("WASAPI Host API index: %1").arg(wasapiIndex));

    // Log Host APIs
    for (int i = 0; i < numHostApis; ++i) {
        const PaHostApiInfo* hostApi = Pa_GetHostApiInfo(i);
        if (hostApi) {
            LOG_INFO(QString("  Host API [%1]: %2 (devices: %3)")
                .arg(i)
                .arg(QString::fromUtf8(hostApi->name))
                .arg(hostApi->deviceCount));
        }
    }

    // Log only WASAPI devices (what will be shown in UI)
    LOG_INFO("===== WASAPI Devices (shown in UI) =====");
    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info) continue;

        // Only log WASAPI devices
        if (wasapiIndex >= 0 && info->hostApi != wasapiIndex) continue;

        LOG_INFO(QString("[%1] %2")
            .arg(i)
            .arg(QString::fromUtf8(info->name)));
        LOG_INFO(QString("    In: %1ch, Out: %2ch, Rate: %3")
            .arg(info->maxInputChannels)
            .arg(info->maxOutputChannels)
            .arg(info->defaultSampleRate));
    }

    LOG_INFO(QString("Default Input: %1, Default Output: %2")
        .arg(Pa_GetDefaultInputDevice())
        .arg(Pa_GetDefaultOutputDevice()));
    LOG_INFO("=====================================");
}

QVector<AudioDeviceInfo> AudioEngine::getInputDevices() const {
    QVector<AudioDeviceInfo> devices;

    if (!m_initialized) {
        return devices;
    }

    int numDevices = Pa_GetDeviceCount();
    int defaultInput = Pa_GetDefaultInputDevice();
    int wasapiIndex = getWasapiHostApiIndex();

    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxInputChannels > 0) {
            // Filter to only WASAPI devices if available
            if (wasapiIndex >= 0 && info->hostApi != wasapiIndex) {
                continue;
            }

            const PaHostApiInfo* hostApi = Pa_GetHostApiInfo(info->hostApi);

            AudioDeviceInfo device;
            device.index = i;
            device.name = QString::fromUtf8(info->name);
            device.maxInputChannels = info->maxInputChannels;
            device.maxOutputChannels = info->maxOutputChannels;
            device.defaultSampleRate = info->defaultSampleRate;
            device.isDefaultInput = (i == defaultInput);
            device.isDefaultOutput = false;
            device.hostApi = hostApi ? QString::fromUtf8(hostApi->name) : "Unknown";
            devices.append(device);
        }
    }

    return devices;
}

QVector<AudioDeviceInfo> AudioEngine::getOutputDevices() const {
    QVector<AudioDeviceInfo> devices;

    if (!m_initialized) {
        return devices;
    }

    int numDevices = Pa_GetDeviceCount();
    int defaultOutput = Pa_GetDefaultOutputDevice();
    int wasapiIndex = getWasapiHostApiIndex();

    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxOutputChannels > 0) {
            // Filter to only WASAPI devices if available
            if (wasapiIndex >= 0 && info->hostApi != wasapiIndex) {
                continue;
            }

            const PaHostApiInfo* hostApi = Pa_GetHostApiInfo(info->hostApi);

            AudioDeviceInfo device;
            device.index = i;
            device.name = QString::fromUtf8(info->name);
            device.maxInputChannels = info->maxInputChannels;
            device.maxOutputChannels = info->maxOutputChannels;
            device.defaultSampleRate = info->defaultSampleRate;
            device.isDefaultInput = false;
            device.isDefaultOutput = (i == defaultOutput);
            device.hostApi = hostApi ? QString::fromUtf8(hostApi->name) : "Unknown";
            devices.append(device);
        }
    }

    return devices;
}

bool AudioEngine::setInputDevice(int index) {
    if (m_running) {
        LOG_WARNING("Cannot change input device while stream is running");
        return false;
    }

    const PaDeviceInfo* info = Pa_GetDeviceInfo(index);
    if (!info || info->maxInputChannels < 1) {
        LOG_ERROR(QString("Invalid input device index: %1").arg(index));
        return false;
    }

    m_inputDeviceIndex = index;
    LOG_INFO(QString("Input device set to [%1]: %2 (%3 channels)")
        .arg(index).arg(info->name).arg(info->maxInputChannels));
    return true;
}

bool AudioEngine::setOutputDevice(int index) {
    if (m_running) {
        LOG_WARNING("Cannot change output device while stream is running");
        return false;
    }

    const PaDeviceInfo* info = Pa_GetDeviceInfo(index);
    if (!info || info->maxOutputChannels < 1) {
        LOG_ERROR(QString("Invalid output device index: %1").arg(index));
        return false;
    }

    m_outputDeviceIndex = index;
    LOG_INFO(QString("Output device set to [%1]: %2 (%3 channels)")
        .arg(index).arg(info->name).arg(info->maxOutputChannels));
    return true;
}

int AudioEngine::findVBCableDevice() const {
    if (!m_initialized) {
        return -1;
    }

    int numDevices = Pa_GetDeviceCount();
    int wasapiIndex = getWasapiHostApiIndex();
    int fallbackDevice = -1;

    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxInputChannels > 0) {
            QString name = QString::fromUtf8(info->name).toLower();
            // VB-CABLE Output is what we capture FROM (it receives from CABLE Input)
            if (name.contains("cable output") || name.contains("vb-audio virtual cable")) {
                // Prefer WASAPI device
                if (wasapiIndex >= 0 && info->hostApi == wasapiIndex) {
                    LOG_INFO(QString("Found VB-CABLE (WASAPI) device: %1").arg(info->name));
                    return i;
                }
                // Keep as fallback if no WASAPI version found
                if (fallbackDevice < 0) {
                    fallbackDevice = i;
                }
            }
        }
    }

    if (fallbackDevice >= 0) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(fallbackDevice);
        LOG_INFO(QString("Found VB-CABLE device (non-WASAPI): %1").arg(info ? info->name : "unknown"));
    }

    return fallbackDevice;
}

bool AudioEngine::start() {
    if (m_running) {
        LOG_WARNING("Audio stream already running");
        return true;
    }

    if (!m_initialized) {
        LOG_ERROR("AudioEngine not initialized");
        return false;
    }

    if (m_inputDeviceIndex < 0 || m_outputDeviceIndex < 0) {
        LOG_ERROR("Input or output device not set");
        return false;
    }

    // Get device info
    const PaDeviceInfo* inputInfo = Pa_GetDeviceInfo(m_inputDeviceIndex);
    const PaDeviceInfo* outputInfo = Pa_GetDeviceInfo(m_outputDeviceIndex);

    if (!inputInfo || !outputInfo) {
        LOG_ERROR("Failed to get device info");
        return false;
    }

    LOG_INFO(QString("Opening stream: Input='%1' (%2ch), Output='%3' (%4ch)")
        .arg(inputInfo->name).arg(inputInfo->maxInputChannels)
        .arg(outputInfo->name).arg(outputInfo->maxOutputChannels));

    // Determine actual channel counts (use 2 if available, otherwise 1)
    m_actualInputChannels = std::min(m_channels, inputInfo->maxInputChannels);
    m_actualOutputChannels = std::min(m_channels, outputInfo->maxOutputChannels);

    // Ensure at least 1 channel
    m_actualInputChannels = std::max(1, m_actualInputChannels);
    m_actualOutputChannels = std::max(1, m_actualOutputChannels);

    LOG_INFO(QString("Using %1 input channels, %2 output channels")
        .arg(m_actualInputChannels).arg(m_actualOutputChannels));

    // Configure input parameters
    PaStreamParameters inputParams;
    inputParams.device = m_inputDeviceIndex;
    inputParams.channelCount = m_actualInputChannels;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = inputInfo->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    // Configure output parameters
    PaStreamParameters outputParams;
    outputParams.device = m_outputDeviceIndex;
    outputParams.channelCount = m_actualOutputChannels;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = outputInfo->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    // Check if format is supported
    PaError err = Pa_IsFormatSupported(&inputParams, &outputParams, m_sampleRate);
    if (err != paFormatIsSupported) {
        LOG_WARNING(QString("Format not directly supported: %1. Trying anyway...")
            .arg(Pa_GetErrorText(err)));
    }

    // Open stream
    err = Pa_OpenStream(
        &m_stream,
        &inputParams,
        &outputParams,
        m_sampleRate,
        m_bufferSize,
        paClipOff | paDitherOff,
        audioCallback,
        this
    );

    if (err != paNoError) {
        LOG_ERROR(QString("Failed to open stream: %1").arg(Pa_GetErrorText(err)));
        emit errorOccurred(QString("Failed to open audio stream: %1").arg(Pa_GetErrorText(err)));
        return false;
    }

    // Get actual stream info
    const PaStreamInfo* streamInfo = Pa_GetStreamInfo(m_stream);
    if (streamInfo) {
        m_inputLatency = streamInfo->inputLatency;
        m_outputLatency = streamInfo->outputLatency;
        LOG_INFO(QString("Stream opened. Actual sample rate: %1, Input latency: %2ms, Output latency: %3ms")
            .arg(streamInfo->sampleRate)
            .arg(m_inputLatency * 1000.0, 0, 'f', 1)
            .arg(m_outputLatency * 1000.0, 0, 'f', 1));
        emit latencyChanged(m_inputLatency * 1000.0, m_outputLatency * 1000.0);
    }

    // Start stream
    err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        LOG_ERROR(QString("Failed to start stream: %1").arg(Pa_GetErrorText(err)));
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
        emit errorOccurred(QString("Failed to start audio stream: %1").arg(Pa_GetErrorText(err)));
        return false;
    }

    m_running = true;
    LOG_INFO(QString("Audio stream STARTED successfully! Sample rate: %1, Buffer: %2")
             .arg(m_sampleRate).arg(m_bufferSize));

    return true;
}

void AudioEngine::stop() {
    if (!m_running || !m_stream) {
        return;
    }

    LOG_INFO("Stopping audio stream...");

    PaError err = Pa_StopStream(m_stream);
    if (err != paNoError) {
        LOG_WARNING(QString("Error stopping stream: %1").arg(Pa_GetErrorText(err)));
    }

    err = Pa_CloseStream(m_stream);
    if (err != paNoError) {
        LOG_WARNING(QString("Error closing stream: %1").arg(Pa_GetErrorText(err)));
    }

    m_stream = nullptr;
    m_running = false;
    LOG_INFO("Audio stream stopped");
}

void AudioEngine::setSampleRate(int rate) {
    if (m_running) {
        LOG_WARNING("Cannot change sample rate while stream is running");
        return;
    }
    m_sampleRate = rate;
    LOG_INFO(QString("Sample rate set to: %1").arg(rate));

    if (m_dspProcessor) {
        m_dspProcessor->setSampleRate(rate);
    }
}

void AudioEngine::setBufferSize(int frames) {
    if (m_running) {
        LOG_WARNING("Cannot change buffer size while stream is running");
        return;
    }
    m_bufferSize = frames;
    LOG_INFO(QString("Buffer size set to: %1 frames").arg(frames));
}

void AudioEngine::setChannels(int channels) {
    if (m_running) {
        LOG_WARNING("Cannot change channel count while stream is running");
        return;
    }
    m_channels = std::clamp(channels, 1, 2);
}

void AudioEngine::setDSPProcessor(DSPProcessor* processor) {
    m_dspProcessor = processor;
    if (m_dspProcessor) {
        m_dspProcessor->setSampleRate(m_sampleRate);
    }
}

double AudioEngine::getInputLatency() const {
    return m_inputLatency * 1000.0;
}

double AudioEngine::getOutputLatency() const {
    return m_outputLatency * 1000.0;
}

double AudioEngine::getTotalLatency() const {
    return (m_inputLatency + m_outputLatency) * 1000.0;
}

int AudioEngine::audioCallback(const void* inputBuffer, void* outputBuffer,
                               unsigned long framesPerBuffer,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void* userData) {
    AudioEngine* engine = static_cast<AudioEngine*>(userData);
    return engine->processAudio(
        static_cast<const float*>(inputBuffer),
        static_cast<float*>(outputBuffer),
        framesPerBuffer,
        timeInfo,
        statusFlags
    );
}

int AudioEngine::processAudio(const float* input, float* output, unsigned long frames,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags) {
    (void)timeInfo;

    // Log status flags for debugging (only occasionally)
    static int callCount = 0;
    if (++callCount == 1) {
        LOG_INFO(QString("First audio callback! Frames: %1, InputCh: %2, OutputCh: %3")
            .arg(frames).arg(m_actualInputChannels).arg(m_actualOutputChannels));
    }

    if (statusFlags) {
        if (statusFlags & paInputUnderflow) LOG_DEBUG("Input underflow");
        if (statusFlags & paInputOverflow) LOG_DEBUG("Input overflow");
        if (statusFlags & paOutputUnderflow) LOG_DEBUG("Output underflow");
        if (statusFlags & paOutputOverflow) LOG_DEBUG("Output overflow");
    }

    // Handle the case where input or output is null
    if (!output) {
        return paContinue;
    }

    if (!input) {
        // No input available, output silence
        std::memset(output, 0, frames * m_actualOutputChannels * sizeof(float));
        return paContinue;
    }

    // Copy input to output, handling channel count differences
    if (m_actualInputChannels == m_actualOutputChannels) {
        // Same channel count - direct copy
        std::memcpy(output, input, frames * m_actualOutputChannels * sizeof(float));
    } else if (m_actualInputChannels == 1 && m_actualOutputChannels == 2) {
        // Mono to stereo
        for (unsigned long i = 0; i < frames; ++i) {
            output[i * 2] = input[i];
            output[i * 2 + 1] = input[i];
        }
    } else if (m_actualInputChannels == 2 && m_actualOutputChannels == 1) {
        // Stereo to mono
        for (unsigned long i = 0; i < frames; ++i) {
            output[i] = (input[i * 2] + input[i * 2 + 1]) * 0.5f;
        }
    } else {
        // Fallback: just copy what we can
        int minChannels = std::min(m_actualInputChannels, m_actualOutputChannels);
        for (unsigned long i = 0; i < frames; ++i) {
            for (int ch = 0; ch < minChannels; ++ch) {
                output[i * m_actualOutputChannels + ch] = input[i * m_actualInputChannels + ch];
            }
            // Zero any extra output channels
            for (int ch = minChannels; ch < m_actualOutputChannels; ++ch) {
                output[i * m_actualOutputChannels + ch] = 0.0f;
            }
        }
    }

    // Capture dry (pre-DSP) signal for spectrum visualization
    bool shouldCaptureSpectrum = (m_visualizationCounter + 1 >= VISUALIZATION_INTERVAL);
    QVector<float> dryCapture;
    if (shouldCaptureSpectrum) {
        dryCapture.resize(frames);
        for (unsigned long i = 0; i < frames; ++i) {
            if (m_actualOutputChannels >= 2) {
                dryCapture[i] = (output[i * m_actualOutputChannels] +
                                output[i * m_actualOutputChannels + 1]) * 0.5f;
            } else {
                dryCapture[i] = output[i];
            }
        }
    }

    // Process through DSP (using output channel count)
    if (m_dspProcessor && !m_dspProcessor->isBypassed()) {
        m_dspProcessor->process(output, frames, m_actualOutputChannels);
    }

    // Level metering
    ++m_levelUpdateCounter;
    if (m_levelUpdateCounter >= LEVEL_UPDATE_INTERVAL) {
        m_levelUpdateCounter = 0;

        if (m_actualOutputChannels >= 2) {
            m_levelL = calculateRMS(output, frames, 0, m_actualOutputChannels);
            m_levelR = calculateRMS(output, frames, 1, m_actualOutputChannels);
        } else {
            m_levelL = m_levelR = calculateRMS(output, frames, 0, m_actualOutputChannels);
        }

        QMetaObject::invokeMethod(this, [this]() {
            emit levelChanged(m_levelL, m_levelR);
        }, Qt::QueuedConnection);
    }

    // Visualization data
    ++m_visualizationCounter;
    if (m_visualizationCounter >= VISUALIZATION_INTERVAL) {
        m_visualizationCounter = 0;

        QMutexLocker locker(&m_visualizationMutex);

        // Capture wet (post-DSP) signal
        m_wetVisualizationBuffer.resize(frames);
        for (unsigned long i = 0; i < frames; ++i) {
            if (m_actualOutputChannels >= 2) {
                m_wetVisualizationBuffer[i] = (output[i * m_actualOutputChannels] +
                                               output[i * m_actualOutputChannels + 1]) * 0.5f;
            } else {
                m_wetVisualizationBuffer[i] = output[i];
            }
        }

        // Store the dry signal captured earlier
        m_dryVisualizationBuffer = dryCapture;

        // Keep legacy buffer for compatibility
        m_visualizationBuffer = m_wetVisualizationBuffer;

        QMetaObject::invokeMethod(this, [this]() {
            QMutexLocker locker(&m_visualizationMutex);
            emit audioDataReady(m_visualizationBuffer);
            emit spectrumDataReady(m_dryVisualizationBuffer, m_wetVisualizationBuffer);
        }, Qt::QueuedConnection);
    }

    return paContinue;
}

float AudioEngine::calculateRMS(const float* buffer, int frames, int channel, int totalChannels) {
    float sum = 0.0f;
    for (int i = 0; i < frames; ++i) {
        float sample = buffer[i * totalChannels + channel];
        sum += sample * sample;
    }
    return std::sqrt(sum / frames);
}
