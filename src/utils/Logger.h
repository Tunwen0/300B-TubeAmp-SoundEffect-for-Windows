#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QMutex>
#include <QDateTime>
#include <QDebug>

class Logger {
public:
    enum Level {
        Debug,
        Info,
        Warning,
        Error
    };

    static void log(Level level, const QString& message);
    static void setLogFile(const QString& path);
    static void setLogLevel(Level minLevel);
    static void enableConsoleOutput(bool enable);

private:
    static QString levelToString(Level level);
    static QFile s_logFile;
    static QMutex s_mutex;
    static Level s_minLevel;
    static bool s_consoleOutput;
};

#define LOG_DEBUG(msg) Logger::log(Logger::Debug, msg)
#define LOG_INFO(msg) Logger::log(Logger::Info, msg)
#define LOG_WARNING(msg) Logger::log(Logger::Warning, msg)
#define LOG_ERROR(msg) Logger::log(Logger::Error, msg)

#endif // LOGGER_H
