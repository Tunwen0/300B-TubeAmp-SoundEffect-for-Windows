#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>
#include <QComboBox>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QTime>

#include "SpectrumWidget.h"

// Forward declarations
class AudioEngine;
class DSPProcessor;
class RainbowLine;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

private slots:
    void onStartButtonClicked();
    void onExitButtonClicked();
    void onTabButtonClicked(int id);
    void updateTimer();
    void onAudioDataReady(const QVector<float>& data);
    void onSpectrumDataReady(const QVector<float>& dryData, const QVector<float>& wetData);
    void onLevelChanged(float left, float right);
    void onAudioError(const QString& error);
    void onBypassToggled(bool checked);
    void toggleMainWindow();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void setupUI();
    void setupStatusPage();
    void setupMonitorPage();
    void setupConnections();
    void setupSystemTray();
    void loadStyleSheet();
    void refreshOutputDevices();
    void autoConfigureAudio();
    void enableFrostedGlassEffect();
    void saveSettings();
    void loadSettings();

    // Core components
    AudioEngine* m_audioEngine;
    DSPProcessor* m_dspProcessor;

    // Main UI containers
    QWidget* m_centralWidget;
    QStackedWidget* m_stackedWidget;
    QLabel* m_titleLabel;
    RainbowLine* m_rainbowLine;

    // Navigation
    QWidget* m_navBar;
    QButtonGroup* m_tabGroup;
    QPushButton* m_btnTabStatus;
    QPushButton* m_btnTabMonitor;

    // Status Page widgets (simplified)
    QWidget* m_pageStatus;
    QComboBox* m_outputDeviceCombo;
    QPushButton* m_startButton;
    QLabel* m_statusLabel;
    QLabel* m_inputInfoLabel;  // Shows auto-selected input

    // Monitor Page widgets (simplified)
    QWidget* m_pageMonitor;
    QLabel* m_processingStatusLabel;
    QLabel* m_latencyLabel;
    SpectrumWidget* m_spectrumWidget;
    QPushButton* m_bypassButton;
    QPushButton* m_minimizeButton;
    QLabel* m_timerLabel;
    QPushButton* m_exitButton;

    // State
    bool m_isRunning;
    QTimer* m_runTimer;
    QTime m_elapsedTime;
    QPoint m_dragPosition;

    // System tray
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;

    // Auto-configured input device index
    int m_autoInputDeviceIndex = -1;
};

#endif // MAINWINDOW_H
