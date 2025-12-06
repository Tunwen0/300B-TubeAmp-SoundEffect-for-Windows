#include "Parameters.h"
#include "../utils/Logger.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <cstring>
#include <cmath>

Parameters::Parameters() {
    loadDefaultCoefficients();
}

bool Parameters::loadFromFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Failed to open parameters file: %1").arg(path));
        return false;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    if (parseIDAFormat(content)) {
        m_loaded = true;
        LOG_INFO(QString("Loaded %1 coefficients from %2").arg(m_rawCoeffs.size()).arg(path));
        return true;
    }

    LOG_WARNING("Failed to parse IDA format, using default coefficients");
    return false;
}

bool Parameters::loadFromResource() {
    return loadFromFile(":/resources/parameters.txt");
}

bool Parameters::parseIDAFormat(const QString& content) {
    m_rawCoeffs.clear();
    QVector<uint8_t> allBytes;

    // Parse DCB lines to extract bytes
    QStringList lines = content.split('\n');
    QRegularExpression dcbRegex(R"(DCB\s+(.+))");

    for (const QString& line : lines) {
        QRegularExpressionMatch match = dcbRegex.match(line);
        if (match.hasMatch()) {
            QString bytesStr = match.captured(1);
            QVector<uint8_t> bytes = parseHexLine(bytesStr);
            allBytes.append(bytes);
        }
    }

    if (allBytes.isEmpty()) {
        LOG_WARNING("No DCB data found in parameter file");
        return false;
    }

    LOG_DEBUG(QString("Parsed %1 bytes from parameter file").arg(allBytes.size()));

    // Convert bytes to doubles (8 bytes per double, little-endian)
    for (int i = 0; i + 7 < allBytes.size(); i += 8) {
        QVector<uint8_t> doubleBytes;
        for (int j = 0; j < 8; ++j) {
            doubleBytes.append(allBytes[i + j]);
        }
        double value = bytesToDouble(doubleBytes);

        // Filter out invalid values (NaN, Inf, or extremely large values)
        if (std::isfinite(value) && std::abs(value) < 1e10) {
            m_rawCoeffs.append(value);
        }
    }

    LOG_DEBUG(QString("Extracted %1 valid coefficients").arg(m_rawCoeffs.size()));

    // Organize coefficients into filter stages
    // Based on the data structure, we have sets of biquad coefficients
    // Each biquad needs 5 coefficients: b0, b1, b2, a1, a2

    // The data appears to contain multiple filter stages for different sample rates
    // For now, organize as a general filter bank

    if (m_rawCoeffs.size() >= 30) {
        // Create filter coefficients from raw data
        // Assuming pairs of doubles form filter parameters

        // Pre-filter: first set of coefficients
        QVector<FilterCoeffs> preCoeffs;
        for (int i = 0; i + 4 < m_rawCoeffs.size() / 2; i += 5) {
            FilterCoeffs fc;
            fc.b0 = m_rawCoeffs[i];
            fc.b1 = m_rawCoeffs[i + 1];
            fc.b2 = m_rawCoeffs[i + 2];
            fc.a1 = m_rawCoeffs[i + 3];
            fc.a2 = m_rawCoeffs[i + 4];

            // Validate coefficient ranges for IIR stability
            if (std::abs(fc.a1) < 2.0 && std::abs(fc.a2) < 1.0) {
                preCoeffs.append(fc);
            }
        }

        // Post-filter: second half of coefficients
        QVector<FilterCoeffs> postCoeffs;
        int offset = m_rawCoeffs.size() / 2;
        for (int i = offset; i + 4 < m_rawCoeffs.size(); i += 5) {
            FilterCoeffs fc;
            fc.b0 = m_rawCoeffs[i];
            fc.b1 = m_rawCoeffs[i + 1];
            fc.b2 = m_rawCoeffs[i + 2];
            fc.a1 = m_rawCoeffs[i + 3];
            fc.a2 = m_rawCoeffs[i + 4];

            if (std::abs(fc.a1) < 2.0 && std::abs(fc.a2) < 1.0) {
                postCoeffs.append(fc);
            }
        }

        // Only use parsed coefficients if we got valid filter stages
        if (!preCoeffs.isEmpty() || !postCoeffs.isEmpty()) {
            // Store for common sample rates
            QVector<int> rates = {44100, 48000, 88200, 96000, 176400, 192000};
            for (int rate : rates) {
                if (!preCoeffs.isEmpty()) {
                    m_preFilterCoeffs[rate] = preCoeffs;
                }
                if (!postCoeffs.isEmpty()) {
                    m_postFilterCoeffs[rate] = postCoeffs;
                }
            }

            LOG_INFO(QString("Loaded %1 pre-filter stages and %2 post-filter stages")
                     .arg(preCoeffs.size()).arg(postCoeffs.size()));
        } else {
            LOG_WARNING("Parsed data but no valid filter coefficients found, keeping defaults");
        }
    } else {
        LOG_WARNING("Not enough coefficients in file, keeping defaults");
    }

    return !m_rawCoeffs.isEmpty();
}

QVector<uint8_t> Parameters::parseHexLine(const QString& line) const {
    QVector<uint8_t> bytes;
    QStringList parts = line.split(',');

    for (const QString& part : parts) {
        QString trimmed = part.trimmed();

        // Remove trailing comments (like "; j" or "; #")
        int semicolonPos = trimmed.indexOf(';');
        if (semicolonPos >= 0) {
            trimmed = trimmed.left(semicolonPos).trimmed();
        }

        if (trimmed.isEmpty()) continue;

        // Handle hex values (0x?? or 0??h)
        bool ok = false;
        uint8_t byte = 0;

        if (trimmed.startsWith("0x", Qt::CaseInsensitive)) {
            byte = trimmed.mid(2).toUInt(&ok, 16);
        } else if (trimmed.endsWith("h", Qt::CaseInsensitive)) {
            byte = trimmed.chopped(1).toUInt(&ok, 16);
        } else {
            // Try decimal
            byte = trimmed.toUInt(&ok, 10);
        }

        if (ok) {
            bytes.append(byte);
        }
    }

    return bytes;
}

double Parameters::bytesToDouble(const QVector<uint8_t>& bytes) const {
    if (bytes.size() != 8) return 0.0;

    // Little-endian byte order
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= static_cast<uint64_t>(bytes[i]) << (i * 8);
    }

    double result;
    std::memcpy(&result, &value, sizeof(double));
    return result;
}

QVector<Parameters::FilterCoeffs> Parameters::getPreFilterCoeffs(int sampleRate) const {
    if (m_preFilterCoeffs.contains(sampleRate)) {
        return m_preFilterCoeffs[sampleRate];
    }

    // Return closest match or 48000 as default
    if (m_preFilterCoeffs.contains(48000)) {
        return m_preFilterCoeffs[48000];
    }

    return m_preFilterCoeffs.isEmpty() ? QVector<FilterCoeffs>() : m_preFilterCoeffs.first();
}

QVector<Parameters::FilterCoeffs> Parameters::getPostFilterCoeffs(int sampleRate) const {
    if (m_postFilterCoeffs.contains(sampleRate)) {
        return m_postFilterCoeffs[sampleRate];
    }

    if (m_postFilterCoeffs.contains(48000)) {
        return m_postFilterCoeffs[48000];
    }

    return m_postFilterCoeffs.isEmpty() ? QVector<FilterCoeffs>() : m_postFilterCoeffs.first();
}

QVector<int> Parameters::getSupportedSampleRates() const {
    return {44100, 48000, 88200, 96000, 176400, 192000};
}

void Parameters::loadDefaultCoefficients() {
    // Default high-quality biquad coefficients for tube amp emulation
    // These provide a warm, musical frequency response

    QVector<int> rates = {44100, 48000, 88200, 96000, 176400, 192000};

    for (int rate : rates) {
        QVector<FilterCoeffs> preCoeffs;
        QVector<FilterCoeffs> postCoeffs;

        // Pre-filter: Gentle high-shelf boost (presence)
        FilterCoeffs pre1;
        pre1.b0 = 1.0306;
        pre1.b1 = -1.9692;
        pre1.b2 = 0.9398;
        pre1.a1 = -1.9692;
        pre1.a2 = 0.9704;
        preCoeffs.append(pre1);

        // Pre-filter: Low-shelf warmth
        FilterCoeffs pre2;
        pre2.b0 = 1.0158;
        pre2.b1 = -1.9839;
        pre2.b2 = 0.9685;
        pre2.a1 = -1.9839;
        pre2.a2 = 0.9843;
        preCoeffs.append(pre2);

        // Post-filter: Speaker cabinet simulation (low-pass)
        FilterCoeffs post1;
        post1.b0 = 0.0675;
        post1.b1 = 0.1349;
        post1.b2 = 0.0675;
        post1.a1 = -1.1430;
        post1.a2 = 0.4128;
        postCoeffs.append(post1);

        // Post-filter: Resonance peak
        FilterCoeffs post2;
        post2.b0 = 0.9826;
        post2.b1 = -1.9321;
        post2.b2 = 0.9507;
        post2.a1 = -1.9321;
        post2.a2 = 0.9333;
        postCoeffs.append(post2);

        m_preFilterCoeffs[rate] = preCoeffs;
        m_postFilterCoeffs[rate] = postCoeffs;
    }

    LOG_DEBUG("Loaded default filter coefficients");
}
