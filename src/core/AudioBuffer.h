#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

#include <vector>
#include <atomic>
#include <cstddef>

class AudioBuffer {
public:
    explicit AudioBuffer(size_t capacity = 65536);

    bool write(const float* data, size_t frames, int channels = 2);
    bool read(float* data, size_t frames, int channels = 2);

    size_t availableRead() const;
    size_t availableWrite() const;
    size_t capacity() const { return m_capacity; }

    void clear();
    void resize(size_t newCapacity);

private:
    std::vector<float> m_buffer;
    size_t m_capacity;
    std::atomic<size_t> m_readPos{0};
    std::atomic<size_t> m_writePos{0};
};

#endif // AUDIOBUFFER_H
