#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QStyleFactory>
#include <QMessageBox>
#include <QDateTime>
#include <QTimeZone>
#include "../ui/MainWindow.h"
#include "utils/Logger.h"


int main(int argc, char* argv[]) {

    // Enable high DPI scaling
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);

    // Application info
    app.setApplicationName("AmpTube300B");
    app.setApplicationVersion("0.1");
    app.setOrganizationName("AmpTube300B");
    app.setOrganizationDomain("amptube300b.local");

    // Use Fusion style for consistent look
    app.setStyle(QStyleFactory::create("Fusion"));

    // Set application icon
    app.setWindowIcon(QIcon(":/icons/app.png"));

    // Setup logging
    QString logPath = QDir::currentPath() + "/amptube300b.log";
    Logger::setLogFile(logPath);
    Logger::setLogLevel(Logger::Info);

    LOG_INFO("=================================");
    LOG_INFO("AmpTube300B Starting...");
    LOG_INFO(QString("Version: %1").arg(app.applicationVersion()));
    LOG_INFO(QString("Working directory: %1").arg(QDir::currentPath()));

    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();

    LOG_INFO("Main window displayed");

    int result = app.exec();

    LOG_INFO("AmpTube300B Exiting...");
    LOG_INFO("=================================");

    return result;
}
