#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <QString>
#include <QVector>
#include <QMap>
#include <array>

class Parameters {
public:
    struct FilterCoeffs {
        double b0 = 0.0, b1 = 0.0, b2 = 0.0;  // Feedforward coefficients
        double a1 = 0.0, a2 = 0.0;            // Feedback coefficients (a0 normalized to 1)
    };

    struct FilterStage {
        QVector<FilterCoeffs> coeffs;
    };

    Parameters();

    bool loadFromFile(const QString& path);
    bool loadFromResource();

    // Get filter coefficients for a specific sample rate
    QVector<FilterCoeffs> getPreFilterCoeffs(int sampleRate) const;
    QVector<FilterCoeffs> getPostFilterCoeffs(int sampleRate) const;

    // Tube emulation parameters
    double getTubeBias() const { return m_tubeBias; }
    double getTubeDrive() const { return m_tubeDrive; }
    double getTubeAsymmetry() const { return m_tubeAsymmetry; }

    // Supported sample rates
    QVector<int> getSupportedSampleRates() const;

    bool isLoaded() const { return m_loaded; }

private:
    bool parseIDAFormat(const QString& content);
    double bytesToDouble(const QVector<uint8_t>& bytes) const;
    QVector<uint8_t> parseHexLine(const QString& line) const;

    // Filter coefficients organized by sample rate
    QMap<int, QVector<FilterCoeffs>> m_preFilterCoeffs;
    QMap<int, QVector<FilterCoeffs>> m_postFilterCoeffs;

    // Raw coefficient data extracted from IDA format
    QVector<double> m_rawCoeffs;

    // Tube parameters
    double m_tubeBias = 0.0;
    double m_tubeDrive = 0.5;
    double m_tubeAsymmetry = 0.1;

    bool m_loaded = false;

    // Default coefficients if file parsing fails
    void loadDefaultCoefficients();
};

#endif // PARAMETERS_H
