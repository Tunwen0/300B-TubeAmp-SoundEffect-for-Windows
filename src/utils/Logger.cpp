#include "Logger.h"
#include <QTextStream>
#include <iostream>

QFile Logger::s_logFile;
QMutex Logger::s_mutex;
Logger::Level Logger::s_minLevel = Logger::Debug;
bool Logger::s_consoleOutput = true;

void Logger::log(Level level, const QString& message) {
    if (level < s_minLevel) {
        return;
    }

    QMutexLocker locker(&s_mutex);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = levelToString(level);
    QString formattedMessage = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);

    if (s_consoleOutput) {
        switch (level) {
            case Debug:
                qDebug().noquote() << formattedMessage;
                break;
            case Info:
                qInfo().noquote() << formattedMessage;
                break;
            case Warning:
                qWarning().noquote() << formattedMessage;
                break;
            case Error:
                qCritical().noquote() << formattedMessage;
                break;
        }
    }

    if (s_logFile.isOpen()) {
        QTextStream stream(&s_logFile);
        stream << formattedMessage << "\n";
        stream.flush();
    }
}

void Logger::setLogFile(const QString& path) {
    QMutexLocker locker(&s_mutex);

    if (s_logFile.isOpen()) {
        s_logFile.close();
    }

    s_logFile.setFileName(path);
    if (!s_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file:" << path;
    }
}

void Logger::setLogLevel(Level minLevel) {
    QMutexLocker locker(&s_mutex);
    s_minLevel = minLevel;
}

void Logger::enableConsoleOutput(bool enable) {
    QMutexLocker locker(&s_mutex);
    s_consoleOutput = enable;
}

QString Logger::levelToString(Level level) {
    switch (level) {
        case Debug:   return "DEBUG";
        case Info:    return "INFO";
        case Warning: return "WARN";
        case Error:   return "ERROR";
        default:      return "UNKNOWN";
    }
}
