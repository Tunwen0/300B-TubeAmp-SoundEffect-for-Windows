#include "AudioBuffer.h"
#include <algorithm>
#include <cstring>

AudioBuffer::AudioBuffer(size_t capacity)
    : m_capacity(capacity)
    , m_buffer(capacity, 0.0f)
{
}

bool AudioBuffer::write(const float* data, size_t frames, int channels) {
    size_t samplesToWrite = frames * channels;

    if (availableWrite() < samplesToWrite) {
        return false;
    }

    size_t writePos = m_writePos.load(std::memory_order_relaxed);

    for (size_t i = 0; i < samplesToWrite; ++i) {
        m_buffer[(writePos + i) % m_capacity] = data[i];
    }

    m_writePos.store((writePos + samplesToWrite) % m_capacity, std::memory_order_release);
    return true;
}

bool AudioBuffer::read(float* data, size_t frames, int channels) {
    size_t samplesToRead = frames * channels;

    if (availableRead() < samplesToRead) {
        return false;
    }

    size_t readPos = m_readPos.load(std::memory_order_relaxed);

    for (size_t i = 0; i < samplesToRead; ++i) {
        data[i] = m_buffer[(readPos + i) % m_capacity];
    }

    m_readPos.store((readPos + samplesToRead) % m_capacity, std::memory_order_release);
    return true;
}

size_t AudioBuffer::availableRead() const {
    size_t writePos = m_writePos.load(std::memory_order_acquire);
    size_t readPos = m_readPos.load(std::memory_order_relaxed);

    if (writePos >= readPos) {
        return writePos - readPos;
    } else {
        return m_capacity - readPos + writePos;
    }
}

size_t AudioBuffer::availableWrite() const {
    return m_capacity - availableRead() - 1;
}

void AudioBuffer::clear() {
    m_readPos.store(0, std::memory_order_relaxed);
    m_writePos.store(0, std::memory_order_release);
}

void AudioBuffer::resize(size_t newCapacity) {
    m_buffer.resize(newCapacity, 0.0f);
    m_capacity = newCapacity;
    clear();
}
