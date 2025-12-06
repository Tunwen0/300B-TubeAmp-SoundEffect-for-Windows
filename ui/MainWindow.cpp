#include "MainWindow.h"
#include "RainbowLine.h"
#include "../src/core/AudioEngine.h"
#include "../src/dsp/DSPProcessor.h"
#include "../src/utils/Logger.h"

#include <QApplication>
#include <QGuiApplication>
#include <QFile>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QSettings>
#include <QMessageBox>
#include <QScreen>

#ifdef Q_OS_WIN
#include <dwmapi.h>
#include <windows.h>
#pragma comment(lib, "dwmapi.lib")
#endif

// Optimized audio settings for low latency
static constexpr int OPTIMAL_SAMPLE_RATE = 48000;
static constexpr int OPTIMAL_BUFFER_SIZE = 128;  // ~2.7ms latency at 48kHz

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_audioEngine(new AudioEngine(this))
    , m_dspProcessor(new DSPProcessor())
    , m_isRunning(false)
    , m_runTimer(new QTimer(this))
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
{
    // Window setup
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(400, 280);  // Slightly smaller for simplified UI
    setWindowTitle("AmpTube300B");

    // Initialize audio engine
    if (!m_audioEngine->initialize()) {
        LOG_ERROR("Failed to initialize audio engine");
    }
    m_audioEngine->setDSPProcessor(m_dspProcessor);

    setupUI();
    setupConnections();
    setupSystemTray();
    loadStyleSheet();
    enableFrostedGlassEffect();
    autoConfigureAudio();
    refreshOutputDevices();
    loadSettings();

    // Timer setup
    connect(m_runTimer, &QTimer::timeout, this, &MainWindow::updateTimer);
}

MainWindow::~MainWindow() {
    saveSettings();
    if (m_isRunning) {
        m_audioEngine->stop();
    }
    delete m_dspProcessor;
}

void MainWindow::setupUI() {
    m_centralWidget = new QWidget(this);
    m_centralWidget->setObjectName("centralWidget");
    setCentralWidget(m_centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(15, 10, 15, 15);
    mainLayout->setSpacing(8);

    // Title bar
    QHBoxLayout* titleLayout = new QHBoxLayout();
    m_titleLabel = new QLabel("AmpTube", this);
    m_titleLabel->setObjectName("mainTitle");

    QLabel* titleGreen = new QLabel("300B", this);
    titleGreen->setObjectName("mainTitleGreen");

    titleLayout->addWidget(m_titleLabel);
    titleLayout->addWidget(titleGreen);
    titleLayout->addStretch();

    // Tab buttons
    m_btnTabStatus = new QPushButton(QString::fromUtf8("设置"), this);
    m_btnTabStatus->setObjectName("tabButton");
    m_btnTabStatus->setCheckable(true);
    m_btnTabStatus->setChecked(true);
    m_btnTabStatus->setFixedSize(55, 22);

    m_btnTabMonitor = new QPushButton(QString::fromUtf8("监听"), this);
    m_btnTabMonitor->setObjectName("tabButton");
    m_btnTabMonitor->setCheckable(true);
    m_btnTabMonitor->setFixedSize(60, 22);

    m_tabGroup = new QButtonGroup(this);
    m_tabGroup->addButton(m_btnTabStatus, 0);
    m_tabGroup->addButton(m_btnTabMonitor, 1);

    titleLayout->addWidget(m_btnTabStatus);
    titleLayout->addWidget(m_btnTabMonitor);

    mainLayout->addLayout(titleLayout);

    // Rainbow line
    m_rainbowLine = new RainbowLine(this);
    m_rainbowLine->setObjectName("rainbowLine");
    mainLayout->addWidget(m_rainbowLine);

    // Stacked widget for pages
    m_stackedWidget = new QStackedWidget(this);
    setupStatusPage();
    setupMonitorPage();
    m_stackedWidget->addWidget(m_pageStatus);
    m_stackedWidget->addWidget(m_pageMonitor);
    mainLayout->addWidget(m_stackedWidget);
}

void MainWindow::setupStatusPage() {
    m_pageStatus = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(m_pageStatus);
    layout->setContentsMargins(5, 15, 5, 5);
    layout->setSpacing(15);

    // Input info (auto-configured, display only)
    QHBoxLayout* inputLayout = new QHBoxLayout();
    QLabel* inputLabel = new QLabel(QString::fromUtf8("音频输入:"), this);
    inputLabel->setObjectName("fieldLabel");
    inputLabel->setFixedWidth(90);
    m_inputInfoLabel = new QLabel("CABLE Output (VB-Audio)", this);
    m_inputInfoLabel->setObjectName("infoLabel");
    m_inputInfoLabel->setStyleSheet("color: #888888; font-style: italic;");
    inputLayout->addWidget(inputLabel);
    inputLayout->addWidget(m_inputInfoLabel);
    inputLayout->addStretch();
    layout->addLayout(inputLayout);

    // Output device selection
    QHBoxLayout* outputLayout = new QHBoxLayout();
    QLabel* outputLabel = new QLabel(QString::fromUtf8("输出设备:"), this);
    outputLabel->setObjectName("fieldLabel");
    outputLabel->setFixedWidth(90);
    m_outputDeviceCombo = new QComboBox(this);
    m_outputDeviceCombo->setMinimumWidth(250);
    outputLayout->addWidget(outputLabel);
    outputLayout->addWidget(m_outputDeviceCombo);
    outputLayout->addStretch();
    layout->addLayout(outputLayout);

    layout->addStretch();

    // Status and Start button
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    m_statusLabel = new QLabel(QString::fromUtf8("准备就绪"), this);
    m_statusLabel->setObjectName("statusValueRed");

    m_startButton = new QPushButton(QString::fromUtf8("开始"), this);
    m_startButton->setObjectName("startButton");
    m_startButton->setFixedSize(90, 32);

    bottomLayout->addWidget(m_statusLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_startButton);
    layout->addLayout(bottomLayout);
}

void MainWindow::setupMonitorPage() {
    m_pageMonitor = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(m_pageMonitor);
    layout->setContentsMargins(5, 10, 5, 5);
    layout->setSpacing(10);

    // Status row
    QHBoxLayout* statusLayout = new QHBoxLayout();
    QLabel* procLabel = new QLabel(QString::fromUtf8("状态:"), this);
    procLabel->setObjectName("monitorLabel");
    m_processingStatusLabel = new QLabel(QString::fromUtf8("空闲"), this);
    m_processingStatusLabel->setObjectName("statusValueRed");

    QLabel* latLabel = new QLabel(QString::fromUtf8("延迟:"), this);
    latLabel->setObjectName("monitorLabel");
    latLabel->setVisible(false);
    m_latencyLabel = new QLabel("-- ms", this);
    m_latencyLabel->setObjectName("monitorLabel");
    m_latencyLabel->setVisible(false);

    statusLayout->addWidget(procLabel);
    statusLayout->addWidget(m_processingStatusLabel);
    statusLayout->addSpacing(30);
    statusLayout->addWidget(latLabel);
    statusLayout->addWidget(m_latencyLabel);
    statusLayout->addStretch();
    layout->addLayout(statusLayout);

    // Spectrum visualization (dry vs wet comparison)
    m_spectrumWidget = new SpectrumWidget(this);
    m_spectrumWidget->setMinimumHeight(100);
    layout->addWidget(m_spectrumWidget);

    // Bottom row
    QHBoxLayout* bottomLayout = new QHBoxLayout();

    m_timerLabel = new QLabel("00:00:00", this);
    m_timerLabel->setObjectName("timerLabel");

    m_bypassButton = new QPushButton(QString::fromUtf8("直通"), this);
    m_bypassButton->setObjectName("tabButton");
    m_bypassButton->setCheckable(true);
    m_bypassButton->setFixedSize(55, 26);

    m_minimizeButton = new QPushButton(QString::fromUtf8("最小化"), this);
    m_minimizeButton->setObjectName("tabButton");
    m_minimizeButton->setFixedSize(55, 26);

    m_exitButton = new QPushButton(QString::fromUtf8("退出"), this);
    m_exitButton->setObjectName("exitButton");
    m_exitButton->setFixedSize(55, 26);

    bottomLayout->addWidget(m_timerLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_bypassButton);
    bottomLayout->addWidget(m_minimizeButton);
    bottomLayout->addWidget(m_exitButton);
    layout->addLayout(bottomLayout);
}

void MainWindow::setupConnections() {
    // Tab navigation
    connect(m_tabGroup, &QButtonGroup::idClicked, this, &MainWindow::onTabButtonClicked);

    // Start/Stop
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(m_exitButton, &QPushButton::clicked, this, &MainWindow::onExitButtonClicked);

    // Minimize to tray
    connect(m_minimizeButton, &QPushButton::clicked, this, [this]() {
        hide();
    });

    // Bypass
    connect(m_bypassButton, &QPushButton::toggled, this, &MainWindow::onBypassToggled);

    // Audio engine signals
    connect(m_audioEngine, &AudioEngine::audioDataReady, this, &MainWindow::onAudioDataReady);
    connect(m_audioEngine, &AudioEngine::spectrumDataReady, this, &MainWindow::onSpectrumDataReady);
    connect(m_audioEngine, &AudioEngine::levelChanged, this, &MainWindow::onLevelChanged);
    connect(m_audioEngine, &AudioEngine::errorOccurred, this, &MainWindow::onAudioError);
    connect(m_audioEngine, &AudioEngine::latencyChanged, this, [this](double input, double output) {
        m_latencyLabel->setText(QString("%1 ms").arg(input + output, 0, 'f', 1));
    });

    // Device selection
    connect(m_outputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0 && !m_isRunning) {
            int deviceIdx = m_outputDeviceCombo->currentData().toInt();
            m_audioEngine->setOutputDevice(deviceIdx);
        }
    });
}

void MainWindow::setupSystemTray() {
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setToolTip(QString::fromUtf8("AmpTube300B - 发烧级电子管音效"));

    // Set tray icon from resource
    m_trayIcon->setIcon(QIcon(":/icons/app.png"));

    m_trayMenu = new QMenu(this);
    QAction* showAction = m_trayMenu->addAction(QString::fromUtf8("显示/隐藏"));
    QAction* bypassAction = m_trayMenu->addAction(QString::fromUtf8("直通"));
    bypassAction->setCheckable(true);
    m_trayMenu->addSeparator();
    QAction* exitAction = m_trayMenu->addAction(QString::fromUtf8("退出"));

    connect(showAction, &QAction::triggered, this, &MainWindow::toggleMainWindow);
    connect(bypassAction, &QAction::toggled, m_bypassButton, &QPushButton::setChecked);
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExitButtonClicked);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);

    m_trayIcon->show();
}

void MainWindow::loadStyleSheet() {
    QFile file(":/styles/styles.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString stylesheet = file.readAll();
        qApp->setStyleSheet(stylesheet);
        file.close();
    } else {
        LOG_WARNING("Could not load stylesheet");
    }
}

void MainWindow::autoConfigureAudio() {
    // Find CABLE Output for input (auto-configure)
    m_autoInputDeviceIndex = m_audioEngine->findVBCableDevice();

    if (m_autoInputDeviceIndex >= 0) {
        m_audioEngine->setInputDevice(m_autoInputDeviceIndex);
        m_inputInfoLabel->setText("CABLE Output (VB-Audio)");
        LOG_INFO("Auto-configured input: CABLE Output");
    } else {
        m_inputInfoLabel->setText(QString::fromUtf8("未找到VB-CABLE!"));
        m_inputInfoLabel->setStyleSheet("color: #ff5555; font-style: italic;");
        LOG_WARNING("VB-CABLE not found - please install VB-Audio Virtual Cable");
    }

    // Set optimal audio settings for low latency
    m_audioEngine->setSampleRate(OPTIMAL_SAMPLE_RATE);
    m_audioEngine->setBufferSize(OPTIMAL_BUFFER_SIZE);
    m_dspProcessor->setSampleRate(OPTIMAL_SAMPLE_RATE);

    LOG_INFO(QString("Auto-configured: %1 Hz, %2 samples buffer (~%3 ms)")
             .arg(OPTIMAL_SAMPLE_RATE)
             .arg(OPTIMAL_BUFFER_SIZE)
             .arg(OPTIMAL_BUFFER_SIZE * 1000.0 / OPTIMAL_SAMPLE_RATE, 0, 'f', 1));
}

void MainWindow::refreshOutputDevices() {
    m_outputDeviceCombo->clear();

    auto outputDevices = m_audioEngine->getOutputDevices();
    int defaultIndex = -1;

    for (const auto& device : outputDevices) {
        // Skip CABLE Input (that's for playback to virtual cable, not for output)
        if (device.name.toLower().contains("cable input")) {
            continue;
        }

        m_outputDeviceCombo->addItem(device.name, device.index);

        if (device.isDefaultOutput) {
            defaultIndex = m_outputDeviceCombo->count() - 1;
        }
    }

    // Select default output device
    if (defaultIndex >= 0) {
        m_outputDeviceCombo->setCurrentIndex(defaultIndex);
    }
}

void MainWindow::enableFrostedGlassEffect() {
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());

    DWM_BLURBEHIND bb = {0};
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.fEnable = TRUE;
    bb.hRgnBlur = CreateRectRgn(0, 0, width(), height());
    DwmEnableBlurBehindWindow(hwnd, &bb);
    DeleteObject(bb.hRgnBlur);

    BOOL value = TRUE;
    DwmSetWindowAttribute(hwnd, 19, &value, sizeof(value));
#endif
}

void MainWindow::onStartButtonClicked() {
    if (!m_isRunning) {
        // Check if VB-CABLE is available
        if (m_autoInputDeviceIndex < 0) {
            QMessageBox::warning(this, QString::fromUtf8("配置错误"),
                QString::fromUtf8("未找到VB-CABLE！\n\n"
                "请安装VB-Audio Virtual Cable，\n"
                "并将'CABLE Input'设为Windows默认播放设备。"));
            return;
        }

        // Get selected output device
        int outputIdx = m_outputDeviceCombo->currentData().toInt();
        m_audioEngine->setOutputDevice(outputIdx);

        if (m_audioEngine->start()) {
            m_isRunning = true;
            m_startButton->setText(QString::fromUtf8("停止"));
            m_statusLabel->setText(QString::fromUtf8("处理中"));
            m_statusLabel->setObjectName("statusValueGreen");
            m_statusLabel->style()->unpolish(m_statusLabel);
            m_statusLabel->style()->polish(m_statusLabel);

            m_processingStatusLabel->setText(QString::fromUtf8("运行中"));
            m_processingStatusLabel->setObjectName("statusValueGreen");
            m_processingStatusLabel->style()->unpolish(m_processingStatusLabel);
            m_processingStatusLabel->style()->polish(m_processingStatusLabel);

            m_spectrumWidget->setSimulationMode(false);

            // Start timer
            m_elapsedTime = QTime(0, 0, 0);
            m_runTimer->start(1000);

            // Disable output selection while running
            m_outputDeviceCombo->setEnabled(false);

            // Auto switch to Monitor page
            m_stackedWidget->setCurrentIndex(1);
            m_btnTabMonitor->setChecked(true);

            LOG_INFO("Audio processing started");
        } else {
            QMessageBox::warning(this, QString::fromUtf8("错误"),
                QString::fromUtf8("无法启动音频处理。\n请检查音频设备。"));
        }
    } else {
        m_audioEngine->stop();
        m_isRunning = false;
        m_startButton->setText(QString::fromUtf8("开始"));
        m_statusLabel->setText(QString::fromUtf8("准备就绪"));
        m_statusLabel->setObjectName("statusValueRed");
        m_statusLabel->style()->unpolish(m_statusLabel);
        m_statusLabel->style()->polish(m_statusLabel);

        m_processingStatusLabel->setText(QString::fromUtf8("空闲"));
        m_processingStatusLabel->setObjectName("statusValueRed");
        m_processingStatusLabel->style()->unpolish(m_processingStatusLabel);
        m_processingStatusLabel->style()->polish(m_processingStatusLabel);

        m_spectrumWidget->setSimulationMode(true);
        m_runTimer->stop();

        // Re-enable output selection
        m_outputDeviceCombo->setEnabled(true);

        LOG_INFO("Audio processing stopped");
    }
}

void MainWindow::onExitButtonClicked() {
    if (m_isRunning) {
        m_audioEngine->stop();
    }
    saveSettings();
    qApp->quit();
}

void MainWindow::onTabButtonClicked(int id) {
    m_stackedWidget->setCurrentIndex(id);
}

void MainWindow::updateTimer() {
    m_elapsedTime = m_elapsedTime.addSecs(1);
    m_timerLabel->setText(m_elapsedTime.toString("hh:mm:ss"));
}

void MainWindow::onAudioDataReady(const QVector<float>& data) {
    // Legacy compatibility - now using spectrum instead
    Q_UNUSED(data);
}

void MainWindow::onSpectrumDataReady(const QVector<float>& dryData, const QVector<float>& wetData) {
    m_spectrumWidget->updateSpectrum(dryData, wetData);
}

void MainWindow::onLevelChanged(float left, float right) {
    Q_UNUSED(left);
    Q_UNUSED(right);
}

void MainWindow::onAudioError(const QString& error) {
    QMessageBox::warning(this, QString::fromUtf8("音频错误"), error);
    if (m_isRunning) {
        onStartButtonClicked();  // Stop
    }
}

void MainWindow::onBypassToggled(bool checked) {
    m_dspProcessor->setBypass(checked);
    if (checked) {
        m_processingStatusLabel->setText(QString::fromUtf8("已直通"));
        m_processingStatusLabel->setObjectName("statusValueYellow");
    } else if (m_isRunning) {
        m_processingStatusLabel->setText(QString::fromUtf8("运行中"));
        m_processingStatusLabel->setObjectName("statusValueGreen");
    }
    m_processingStatusLabel->style()->unpolish(m_processingStatusLabel);
    m_processingStatusLabel->style()->polish(m_processingStatusLabel);
}

void MainWindow::toggleMainWindow() {
    if (isVisible() && !isMinimized()) {
        hide();
    } else {
        showNormal();
        raise();
        activateWindow();

#ifdef Q_OS_WIN
        HWND hwnd = reinterpret_cast<HWND>(winId());
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(hwnd);
#endif
    }
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick ||
        reason == QSystemTrayIcon::Trigger) {
        toggleMainWindow();
    }
}

void MainWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    hide();
    event->ignore();
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (windowState() == Qt::WindowNoState || windowState() == Qt::WindowActive) {
            raise();
            activateWindow();
        }
    }
    QMainWindow::changeEvent(event);
}

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);

        if (msg->message == WM_SYSCOMMAND) {
            int cmd = msg->wParam & 0xFFF0;
            if (cmd == SC_RESTORE) {
                QTimer::singleShot(0, this, [this]() {
                    showNormal();
                    raise();
                    activateWindow();
                    HWND hwnd = reinterpret_cast<HWND>(winId());
                    SetForegroundWindow(hwnd);
                });
            }
        }

        if (msg->message == WM_ACTIVATE) {
            if (LOWORD(msg->wParam) != WA_INACTIVE) {
                raise();
                activateWindow();
            }
        }
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
#endif
    Q_UNUSED(result);
    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::saveSettings() {
    QSettings settings("AmpTube300B", "AmpTube300B");
    settings.setValue("outputDevice", m_outputDeviceCombo->currentIndex());
    settings.setValue("windowPos", pos());
}

void MainWindow::loadSettings() {
    QSettings settings("AmpTube300B", "AmpTube300B");

    int outputIdx = settings.value("outputDevice", -1).toInt();
    if (outputIdx >= 0 && outputIdx < m_outputDeviceCombo->count()) {
        m_outputDeviceCombo->setCurrentIndex(outputIdx);
    }

    QPoint pos = settings.value("windowPos", QPoint(100, 100)).toPoint();

    // Validate position
    bool positionValid = false;
    for (QScreen* screen : QGuiApplication::screens()) {
        if (screen->availableGeometry().contains(pos)) {
            positionValid = true;
            break;
        }
    }

    if (!positionValid) {
        QScreen* primary = QGuiApplication::primaryScreen();
        if (primary) {
            QRect screenGeom = primary->availableGeometry();
            pos = QPoint(screenGeom.center().x() - width() / 2,
                        screenGeom.center().y() - height() / 2);
        }
    }

    move(pos);
}
