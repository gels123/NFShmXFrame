#include "NFServerControllerQt.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QCheckBox>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtGui/QDesktopServices>
#include <QtCore/QUrl>
#include <QtCore/QDebug>
#include <thread>
#include <chrono>
#include <algorithm>

NFServerControllerQt::NFServerControllerQt(QWidget* parent)
    : QMainWindow(parent)
      , m_controller(std::make_unique<NFServerController>())
      , m_centralWidget(nullptr)
      , m_mainSplitter(nullptr)
      , m_topSplitter(nullptr)
      , m_serverGroup(nullptr)
      , m_serverTable(nullptr)
      , m_controlGroup(nullptr)
      , m_logGroup(nullptr)
      , m_logEdit(nullptr)
      , m_toolBar(nullptr)
      , m_configPathEdit(nullptr)
      , m_autoRefreshTimer(new QTimer(this))
      , m_statusUpdateTimer(new QTimer(this))
#if NF_PLATFORM == NF_PLATFORM_WIN
      , m_currentConfigFile("win_servers.conf")
#else
      , m_currentConfigFile("linux_servers.conf")
#endif
      , m_autoRefreshEnabled(true)
      , m_refreshInterval(5)
      , m_isInitialized(false)
      , m_currentOperation(nullptr)
      , m_operationInProgress(false)
{
    setWindowTitle("NFServerController - NFrame Server Management Tool v2.0");
    setWindowIcon(QIcon(":/icons/server.png"));
    resize(1400, 900);

    // Set window to center
    QDesktopWidget desktop;
    QRect screenGeometry = desktop.screenGeometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);

    // Set application style
    setStyleSheet(R"(
        QMainWindow {
            background-color: #f5f5f5;
        }
        QGroupBox {
            font-weight: bold;
            border: 2px solid #cccccc;
            border-radius: 8px;
            margin-top: 1ex;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
        QPushButton {
            background-color: #4CAF50;
            border: none;
            color: white;
            padding: 8px 16px;
            text-align: center;
            text-decoration: none;
            font-size: 14px;
            margin: 2px;
            border-radius: 4px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
        QPushButton:pressed {
            background-color: #3e8e41;
        }
        QPushButton:disabled {
            background-color: #cccccc;
            color: #666666;
        }
        QPushButton[class="danger"] {
            background-color: #f44336;
        }
        QPushButton[class="danger"]:hover {
            background-color: #da190b;
        }
        QPushButton[class="warning"] {
            background-color: #ff9800;
        }
        QPushButton[class="warning"]:hover {
            background-color: #f57c00;
        }
        QPushButton[class="info"] {
            background-color: #2196f3;
        }
        QPushButton[class="info"]:hover {
            background-color: #0b7dda;
        }
        QTableWidget {
            gridline-color: #d0d0d0;
            background-color: white;
            alternate-background-color: #f9f9f9;
        }
        QTableWidget::item:selected {
            background-color: #3498db;
            color: white;
        }
        QTextEdit {
            background-color: #2b2b2b;
            color: #ffffff;
            font-family: 'Consolas', 'Monaco', 'Courier New', monospace;
            font-size: 9pt;
        }
        QLineEdit {
            padding: 4px;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        QComboBox {
            padding: 4px;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
    )");

    // Initialize UI
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupConnections();

    // Load settings
    loadSettings();

    // Initialize controller
    if (!m_currentConfigFile.isEmpty() && m_controller)
    {
        try
        {
            // Safe string conversion
            std::string configPath = safeQStringToStdString(m_currentConfigFile);
            if (m_controller->Initialize(configPath))
            {
                m_isInitialized = true;
                addLogMessage("Server controller initialized successfully", "INFO");
                updateServerTable();
                updateStatusBar();
            }
            else
            {
                addLogMessage("Server controller initialization failed: " + m_currentConfigFile, "ERROR");
            }
        }
        catch (const std::exception& e)
        {
            addLogMessage(QString("Controller initialization exception: %1").arg(e.what()), "ERROR");
        } catch (...)
        {
            addLogMessage("Controller initialization failed with unknown exception", "ERROR");
        }
    }

    // Start timers
    if (m_autoRefreshEnabled)
    {
        m_autoRefreshTimer->start(m_refreshInterval * 1000);
    }
    m_statusUpdateTimer->start(1000); // Update status bar every second
}

NFServerControllerQt::~NFServerControllerQt()
{
    // Stop timers
    if (m_autoRefreshTimer)
    {
        m_autoRefreshTimer->stop();
    }
    if (m_statusUpdateTimer)
    {
        m_statusUpdateTimer->stop();
    }

    // Safely clean up controller
    try
    {
        if (m_controller)
        {
            m_controller->StopMonitoring();
            // Let smart pointer automatically manage memory, no need to manually reset
        }
    }
    catch (...)
    {
        // Ignore exceptions in destructor
    }

    // Save settings
    try
    {
        saveSettingsToFile();
    }
    catch (...)
    {
        // Ignore exceptions in settings save
    }
}

void NFServerControllerQt::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    // Main splitter - horizontal split
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);

    // Top splitter - vertical split (server list and control panel)
    m_topSplitter = new QSplitter(Qt::Vertical, this);

    // Create components
    createServerListWidget();
    createControlPanelWidget();
    createLogWidget();

    // Setup splitters
    m_topSplitter->addWidget(m_serverGroup);
    m_topSplitter->addWidget(m_controlGroup);
    // Increase server list proportion: 800:200 (was 600:300)
    m_topSplitter->setSizes({800, 200});
    // Set stretch factor, server list gets more space
    m_topSplitter->setStretchFactor(0, 4); // Server list
    m_topSplitter->setStretchFactor(1, 1); // Control panel

    m_mainSplitter->addWidget(m_topSplitter);
    m_mainSplitter->addWidget(m_logGroup);
    // Increase left panel proportion: 1200:350 (was 1000:400)
    m_mainSplitter->setSizes({1200, 350});
    // Set stretch factor, left panel (including server list) gets more space
    m_mainSplitter->setStretchFactor(0, 3); // Left panel
    m_mainSplitter->setStretchFactor(1, 1); // Log panel

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->addWidget(m_mainSplitter);
    mainLayout->setContentsMargins(5, 5, 5, 5);
}

void NFServerControllerQt::createServerListWidget()
{
    m_serverGroup = new QGroupBox("Server List", this);
    m_serverLayout = new QVBoxLayout(m_serverGroup);

    // Create table
    m_serverTable = new QTableWidget(this);
    m_serverTable->setColumnCount(COL_COUNT);

    // Set table headers
    QStringList headers;
    headers << "Select" << "Process ID" << "Server ID" << "Server Name" << "Game Name"
        << "Status" << "Config Path" << "Plugin Path" << "Executable" << "Working Dir";
    m_serverTable->setHorizontalHeaderLabels(headers);

    // Set table properties
    m_serverTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_serverTable->setAlternatingRowColors(true);
    m_serverTable->setSortingEnabled(true);
    m_serverTable->verticalHeader()->setVisible(false);

    // Set column width and sorting properties
    QHeaderView* header = m_serverTable->horizontalHeader();

    // Disable auto stretch last column, manually control all column widths instead
    header->setStretchLastSection(false);

    // Set basic column widths (compress fixed columns to leave more space for path columns)
    header->resizeSection(COL_SELECT, 50);
    header->resizeSection(COL_PROCESS_ID, 70);
    header->resizeSection(COL_SERVER_ID, 100);
    header->resizeSection(COL_SERVER_NAME, 120);
    header->resizeSection(COL_GAME_NAME, 80);
    header->resizeSection(COL_STATUS, 60);

    // Set reasonable initial width for path-related columns
    header->resizeSection(COL_CONFIG_PATH, 200);
    header->resizeSection(COL_PLUGIN_PATH, 200);
    header->resizeSection(COL_EXECUTABLE_PATH, 200);
    header->resizeSection(COL_WORKING_DIR, 200);

    // Set stretch mode: fixed columns in front, path columns auto-stretch to fill remaining space
    header->setSectionResizeMode(COL_SELECT, QHeaderView::Fixed);
    header->setSectionResizeMode(COL_PROCESS_ID, QHeaderView::Fixed);
    header->setSectionResizeMode(COL_SERVER_ID, QHeaderView::Fixed);
    header->setSectionResizeMode(COL_SERVER_NAME, QHeaderView::Interactive);
    header->setSectionResizeMode(COL_GAME_NAME, QHeaderView::Fixed);
    header->setSectionResizeMode(COL_STATUS, QHeaderView::Fixed);
    // Path columns use Stretch mode to automatically fill remaining space
    header->setSectionResizeMode(COL_CONFIG_PATH, QHeaderView::Stretch);
    header->setSectionResizeMode(COL_PLUGIN_PATH, QHeaderView::Stretch);
    header->setSectionResizeMode(COL_EXECUTABLE_PATH, QHeaderView::Stretch);
    header->setSectionResizeMode(COL_WORKING_DIR, QHeaderView::Stretch);

    // Set reasonable minimum column width
    header->setMinimumSectionSize(50); // Global minimum width

    // Set larger minimum width for path columns to ensure readability
    header->resizeSection(COL_CONFIG_PATH, 150); // Reset minimum initial width
    header->resizeSection(COL_PLUGIN_PATH, 150);
    header->resizeSection(COL_EXECUTABLE_PATH, 150);
    header->resizeSection(COL_WORKING_DIR, 150);

    // Set table minimum size
    m_serverTable->setMinimumSize(1300, 200);

    // Enable horizontal scroll bar when content is too wide
    m_serverTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Set sorting related properties
    header->setSortIndicator(-1, Qt::AscendingOrder); // No sorting in initial state

    // Selection operation button area
    QHBoxLayout* selectLayout = new QHBoxLayout();

    m_selectAllBtn = new QPushButton("Select All", this);
    m_selectNoneBtn = new QPushButton("Select None", this);
    m_selectRunningBtn = new QPushButton("Select Running", this);
    m_selectStoppedBtn = new QPushButton("Select Stopped", this);
    m_refreshBtn = new QPushButton("Refresh Status", this);
    m_interruptBtn = new QPushButton("Interrupt", this);

    // Set button styles
    m_selectAllBtn->setProperty("class", "info");
    m_selectNoneBtn->setProperty("class", "warning");
    m_selectRunningBtn->setProperty("class", "info");
    m_selectStoppedBtn->setProperty("class", "warning");
    m_refreshBtn->setProperty("class", "info");
    m_interruptBtn->setProperty("class", "danger");
    
    // Interrupt button is disabled in initial state
    m_interruptBtn->setEnabled(false);

    selectLayout->addWidget(m_selectAllBtn);
    selectLayout->addWidget(m_selectNoneBtn);
    selectLayout->addWidget(m_selectRunningBtn);
    selectLayout->addWidget(m_selectStoppedBtn);
    selectLayout->addStretch();
    selectLayout->addWidget(m_interruptBtn);
    selectLayout->addWidget(m_refreshBtn);

    m_serverLayout->addLayout(selectLayout);
    m_serverLayout->addWidget(m_serverTable);
}

void NFServerControllerQt::createControlPanelWidget()
{
    m_controlGroup = new QGroupBox("Control Panel", this);
    m_controlLayout = new QVBoxLayout(m_controlGroup);

    // Create tab pages
    m_controlTabs = new QTabWidget(this);

    // Global operations tab page
    QWidget* globalTab = new QWidget();
    QGridLayout* globalLayout = new QGridLayout(globalTab);

    m_startAllBtn = new QPushButton("Start All Servers", this);
    m_stopAllBtn = new QPushButton("Stop All Servers", this);
    m_restartAllBtn = new QPushButton("Restart All Servers", this);
    m_reloadAllBtn = new QPushButton("Reload All Configs", this);

    // Set button styles
    m_startAllBtn->setProperty("class", "");
    m_stopAllBtn->setProperty("class", "danger");
    m_restartAllBtn->setProperty("class", "warning");
    m_reloadAllBtn->setProperty("class", "info");

    globalLayout->addWidget(m_startAllBtn, 0, 0);
    globalLayout->addWidget(m_stopAllBtn, 0, 1);
    globalLayout->addWidget(m_restartAllBtn, 1, 0);
    globalLayout->addWidget(m_reloadAllBtn, 1, 1);
    globalLayout->setRowStretch(2, 1);

    // Selected server operations tab page (moved to first position)
    QWidget* selectedTab = new QWidget();
    QGridLayout* selectedLayout = new QGridLayout(selectedTab);

    m_startSelectedBtn = new QPushButton("Start Selected Servers", this);
    m_stopSelectedBtn = new QPushButton("Stop Selected Servers", this);
    m_restartSelectedBtn = new QPushButton("Restart Selected Servers", this);
    m_reloadSelectedBtn = new QPushButton("Reload Selected Configs", this);

    // Set button styles
    m_startSelectedBtn->setProperty("class", "");
    m_stopSelectedBtn->setProperty("class", "danger");
    m_restartSelectedBtn->setProperty("class", "warning");
    m_reloadSelectedBtn->setProperty("class", "info");

    selectedLayout->addWidget(m_startSelectedBtn, 0, 0);
    selectedLayout->addWidget(m_stopSelectedBtn, 0, 1);
    selectedLayout->addWidget(m_restartSelectedBtn, 1, 0);
    selectedLayout->addWidget(m_reloadSelectedBtn, 1, 1);
    selectedLayout->setRowStretch(2, 1);

    m_controlTabs->addTab(selectedTab, "Selected Operations");

    // Pattern operations tab page (moved to second position)
    QWidget* patternTab = new QWidget();
    QVBoxLayout* patternLayout = new QVBoxLayout(patternTab);

    QLabel* patternLabel = new QLabel("Server Pattern (e.g: *.*.5.* for all type 5 servers):", this);
    m_patternEdit = new QLineEdit("*.*.*.*", this);

    QGridLayout* patternBtnLayout = new QGridLayout();
    m_startPatternBtn = new QPushButton("Start Pattern Servers", this);
    m_stopPatternBtn = new QPushButton("Stop Pattern Servers", this);
    m_restartPatternBtn = new QPushButton("Restart Pattern Servers", this);
    m_reloadPatternBtn = new QPushButton("Reload Pattern Configs", this);

    // Set button styles
    m_startPatternBtn->setProperty("class", "");
    m_stopPatternBtn->setProperty("class", "danger");
    m_restartPatternBtn->setProperty("class", "warning");
    m_reloadPatternBtn->setProperty("class", "info");

    patternBtnLayout->addWidget(m_startPatternBtn, 0, 0);
    patternBtnLayout->addWidget(m_stopPatternBtn, 0, 1);
    patternBtnLayout->addWidget(m_restartPatternBtn, 1, 0);
    patternBtnLayout->addWidget(m_reloadPatternBtn, 1, 1);

    patternLayout->addWidget(patternLabel);
    patternLayout->addWidget(m_patternEdit);
    patternLayout->addLayout(patternBtnLayout);
    patternLayout->addStretch();

    m_controlTabs->addTab(patternTab, "Pattern Operations");

    // Global operations tab page (moved to last position)
    m_controlTabs->addTab(globalTab, "Global Operations");

    m_controlLayout->addWidget(m_controlTabs);
}

void NFServerControllerQt::createLogWidget()
{
    m_logGroup = new QGroupBox("Operation Log", this);
    m_logLayout = new QVBoxLayout(m_logGroup);

    // Log display area
    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    // Note: setMaximumBlockCount may not be available in all Qt versions
    // We'll manage log size manually in addLogMessage method

    // Log operation buttons
    QHBoxLayout* logBtnLayout = new QHBoxLayout();
    m_clearLogBtn = new QPushButton("Clear Log", this);
    m_saveLogBtn = new QPushButton("Save Log", this);

    m_clearLogBtn->setProperty("class", "warning");
    m_saveLogBtn->setProperty("class", "info");

    logBtnLayout->addWidget(m_clearLogBtn);
    logBtnLayout->addWidget(m_saveLogBtn);
    logBtnLayout->addStretch();

    m_logLayout->addWidget(m_logEdit);
    m_logLayout->addLayout(logBtnLayout);

    // Add initial log message
    addLogMessage("NFServerController Qt version started", "INFO");
}

void NFServerControllerQt::setupMenuBar()
{
    m_menuBar = menuBar();

    // File menu
    m_fileMenu = m_menuBar->addMenu("File");

    m_openConfigAction = m_fileMenu->addAction("Open Config File...");
    m_openConfigAction->setShortcut(QKeySequence::Open);

    m_reloadConfigAction = m_fileMenu->addAction("Reload Config");
    m_reloadConfigAction->setShortcut(QKeySequence::Refresh);

    m_fileMenu->addSeparator();

    m_saveSettingsAction = m_fileMenu->addAction("Save Settings");
    m_saveSettingsAction->setShortcut(QKeySequence::Save);

    m_fileMenu->addSeparator();

    m_exitAction = m_fileMenu->addAction("Exit");
    m_exitAction->setShortcut(QKeySequence::Quit);

    // Server menu
    m_serverMenu = m_menuBar->addMenu("Server");

    m_startAllAction = m_serverMenu->addAction("Start All");
    m_stopAllAction = m_serverMenu->addAction("Stop All");
    m_restartAllAction = m_serverMenu->addAction("Restart All");
    m_reloadAllAction = m_serverMenu->addAction("Reload All");

    m_serverMenu->addSeparator();

    m_refreshAction = m_serverMenu->addAction("Refresh Status");
    m_refreshAction->setShortcut(QKeySequence("F5"));

    // View menu
    m_viewMenu = m_menuBar->addMenu("View");

    m_clearLogAction = m_viewMenu->addAction("Clear Log");
    m_saveLogAction = m_viewMenu->addAction("Save Log...");

    // Help menu
    m_helpMenu = m_menuBar->addMenu("Help");

    m_settingsAction = m_helpMenu->addAction("Settings...");
    m_helpMenu->addSeparator();
    m_aboutAction = m_helpMenu->addAction("About...");
}

void NFServerControllerQt::setupToolBar()
{
    m_toolBar = addToolBar("Main Toolbar");
    m_toolBar->setMovable(false);

    // Config file path
    m_toolBar->addWidget(new QLabel("Config File: "));
    m_configPathEdit = new QLineEdit(this);
    m_configPathEdit->setMinimumWidth(300);
    m_configPathEdit->setText(m_currentConfigFile);
    m_toolBar->addWidget(m_configPathEdit);

    m_browseConfigBtn = new QPushButton("Browse...", this);
    m_browseConfigBtn->setProperty("class", "info");
    m_toolBar->addWidget(m_browseConfigBtn);

    m_reloadConfigBtn = new QPushButton("Reload", this);
    m_reloadConfigBtn->setProperty("class", "info");
    m_toolBar->addWidget(m_reloadConfigBtn);

    m_toolBar->addSeparator();

    // Auto refresh
    m_autoRefreshCheckBox = new QCheckBox("Auto Refresh", this);
    m_autoRefreshCheckBox->setChecked(m_autoRefreshEnabled);
    m_toolBar->addWidget(m_autoRefreshCheckBox);

    m_refreshIntervalCombo = new QComboBox(this);
    m_refreshIntervalCombo->addItems({"1s", "2s", "5s", "10s", "30s", "60s"});
    m_refreshIntervalCombo->setCurrentText(QString("%1s").arg(m_refreshInterval));
    m_toolBar->addWidget(m_refreshIntervalCombo);
}

void NFServerControllerQt::setupStatusBar()
{
    m_statusBar = statusBar();

    m_statusLabel = new QLabel("Ready", this);
    m_statusBar->addWidget(m_statusLabel);

    m_statusBar->addPermanentWidget(new QLabel("|"));

    m_serverCountLabel = new QLabel("Servers: 0", this);
    m_statusBar->addPermanentWidget(m_serverCountLabel);

    m_runningCountLabel = new QLabel("Running: 0", this);
    m_statusBar->addPermanentWidget(m_runningCountLabel);

    m_stoppedCountLabel = new QLabel("Stopped: 0", this);
    m_statusBar->addPermanentWidget(m_stoppedCountLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(200);
    m_statusBar->addPermanentWidget(m_progressBar);
}

void NFServerControllerQt::setupConnections()
{
    // Menu action connections
    connect(m_openConfigAction, &QAction::triggered, this, &NFServerControllerQt::openConfigFile);
    connect(m_reloadConfigAction, &QAction::triggered, this, &NFServerControllerQt::reloadConfigFile);
    connect(m_saveSettingsAction, &QAction::triggered, this, &NFServerControllerQt::saveSettings);
    connect(m_exitAction, &QAction::triggered, this, &NFServerControllerQt::exitApplication);

    connect(m_startAllAction, &QAction::triggered, this, &NFServerControllerQt::startAllServers);
    connect(m_stopAllAction, &QAction::triggered, this, &NFServerControllerQt::stopAllServers);
    connect(m_restartAllAction, &QAction::triggered, this, &NFServerControllerQt::restartAllServers);
    connect(m_reloadAllAction, &QAction::triggered, this, &NFServerControllerQt::reloadAllServers);
    connect(m_refreshAction, &QAction::triggered, this, &NFServerControllerQt::refreshServerStatus);

    connect(m_clearLogAction, &QAction::triggered, this, &NFServerControllerQt::clearLog);
    connect(m_saveLogAction, &QAction::triggered, this, &NFServerControllerQt::saveLogToFile);
    connect(m_aboutAction, &QAction::triggered, this, &NFServerControllerQt::showAbout);
    connect(m_settingsAction, &QAction::triggered, this, &NFServerControllerQt::showSettings);

    // Toolbar connections
    connect(m_browseConfigBtn, &QPushButton::clicked, this, &NFServerControllerQt::openConfigFile);
    connect(m_reloadConfigBtn, &QPushButton::clicked, this, &NFServerControllerQt::reloadConfigFile);
    connect(m_autoRefreshCheckBox, &QCheckBox::toggled, this, &NFServerControllerQt::toggleAutoRefresh);

    // Refresh interval change
    connect(m_refreshIntervalCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
            [this](const QString& text)
            {
                QString numStr = text;
                numStr.remove("s");
                bool ok;
                int interval = numStr.toInt(&ok);
                if (ok && interval > 0)
                {
                    m_refreshInterval = interval;
                    if (m_autoRefreshEnabled)
                    {
                        m_autoRefreshTimer->start(m_refreshInterval * 1000);
                    }
                }
            });

    // Server operation button connections
    connect(m_startAllBtn, &QPushButton::clicked, this, &NFServerControllerQt::startAllServers);
    connect(m_stopAllBtn, &QPushButton::clicked, this, &NFServerControllerQt::stopAllServers);
    connect(m_restartAllBtn, &QPushButton::clicked, this, &NFServerControllerQt::restartAllServers);
    connect(m_reloadAllBtn, &QPushButton::clicked, this, &NFServerControllerQt::reloadAllServers);

    connect(m_startSelectedBtn, &QPushButton::clicked, this, &NFServerControllerQt::startSelectedServers);
    connect(m_stopSelectedBtn, &QPushButton::clicked, this, &NFServerControllerQt::stopSelectedServers);
    connect(m_restartSelectedBtn, &QPushButton::clicked, this, &NFServerControllerQt::restartSelectedServers);
    connect(m_reloadSelectedBtn, &QPushButton::clicked, this, &NFServerControllerQt::reloadSelectedServers);

    connect(m_startPatternBtn, &QPushButton::clicked, this, &NFServerControllerQt::startServerByPattern);
    connect(m_stopPatternBtn, &QPushButton::clicked, this, &NFServerControllerQt::stopServerByPattern);
    connect(m_restartPatternBtn, &QPushButton::clicked, this, &NFServerControllerQt::restartServerByPattern);
    connect(m_reloadPatternBtn, &QPushButton::clicked, this, &NFServerControllerQt::reloadServerByPattern);

    // Selection button connections
    connect(m_selectAllBtn, &QPushButton::clicked, this, &NFServerControllerQt::selectAllServers);
    connect(m_selectNoneBtn, &QPushButton::clicked, this, &NFServerControllerQt::selectNoServers);
    connect(m_selectRunningBtn, &QPushButton::clicked, this, &NFServerControllerQt::selectRunningServers);
    connect(m_selectStoppedBtn, &QPushButton::clicked, this, &NFServerControllerQt::selectStoppedServers);
    connect(m_interruptBtn, &QPushButton::clicked, this, &NFServerControllerQt::interruptCurrentOperation);
    connect(m_refreshBtn, &QPushButton::clicked, this, &NFServerControllerQt::refreshServerStatus);

    // Table connections
    connect(m_serverTable, &QTableWidget::itemSelectionChanged, this, &NFServerControllerQt::onServerSelectionChanged);
    connect(m_serverTable, &QTableWidget::cellDoubleClicked, this, &NFServerControllerQt::onServerDoubleClicked);

    // Log button connections
    connect(m_clearLogBtn, &QPushButton::clicked, this, &NFServerControllerQt::clearLog);
    connect(m_saveLogBtn, &QPushButton::clicked, this, &NFServerControllerQt::saveLogToFile);

    // Timer connections
    connect(m_autoRefreshTimer, &QTimer::timeout, this, &NFServerControllerQt::onAutoRefreshTimer);
    connect(m_statusUpdateTimer, &QTimer::timeout, this, &NFServerControllerQt::updateStatusBar);
}

void NFServerControllerQt::updateServerTable()
{
    if (!m_controller || !m_isInitialized)
    {
        return;
    }

    // Note: Don't use QMutexLocker here as NFServerController has its own locking
    // QMutexLocker locker(&m_updateMutex);

    try
    {
        auto configs = m_controller->GetAllServerConfigs();
        auto configOrder = m_controller->GetServerConfigOrder();

        // Save current sorting state
        bool sortingWasEnabled = m_serverTable->isSortingEnabled();
        int sortColumn = -1;
        Qt::SortOrder sortOrder = Qt::AscendingOrder;

        if (sortingWasEnabled)
        {
            QHeaderView* header = m_serverTable->horizontalHeader();
            sortColumn = header->sortIndicatorSection();
            sortOrder = header->sortIndicatorOrder();

            // Temporarily disable sorting to avoid row misalignment during data update
            m_serverTable->setSortingEnabled(false);
        }

        // Set table row count
        m_serverTable->setRowCount(configOrder.size());

        int row = 0;
        // Display servers in configuration file order
        for (const auto& serverId : configOrder)
        {
            auto it = configs.find(serverId);
            if (it != configs.end() && it->second)
            {
                updateServerRow(row, it->second);
            row++;
            }
        }
        
        // If there are missing configurations, adjust row count
        if (row != configOrder.size())
        {
            m_serverTable->setRowCount(row);
        }

        // Restore sorting functionality and user's sorting state
        if (sortingWasEnabled)
        {
            m_serverTable->setSortingEnabled(true);

            // If there was sorting before, restore sorting state
            if (sortColumn >= 0 && sortColumn < m_serverTable->columnCount())
            {
                m_serverTable->sortByColumn(sortColumn, sortOrder);
            }
        }

        // Comment out auto column width adjustment to maintain our stretch mode
        // m_serverTable->resizeColumnsToContents();
    }
    catch (const std::exception& e)
    {
        addLogMessage(QString("Update server table exception: %1").arg(e.what()), "ERROR");
    } catch (...)
    {
        addLogMessage("Update server table failed with unknown exception", "ERROR");
    }
}

void NFServerControllerQt::updateServerRow(int row, const std::shared_ptr<NFServerConfig>& config)
{
    if (!config)
    {
        return;
    }

    // 复选框列
    QCheckBox* checkBox = qobject_cast<QCheckBox*>(m_serverTable->cellWidget(row, COL_SELECT));
    if (!checkBox)
    {
        checkBox = new QCheckBox();
        m_serverTable->setCellWidget(row, COL_SELECT, checkBox);
    }

    // 进程ID - 只有在运行状态下才显示进程ID
    QString processId = "N/A";
    if (config->m_status == NFServerStatus::SERVER_STATUS_RUNNING ||
        config->m_status == NFServerStatus::SERVER_STATUS_STARTING ||
        config->m_status == NFServerStatus::SERVER_STATUS_STOPPING)
    {
        if (config->m_processId > 0)
        {
            processId = QString::number(config->m_processId);
        }
    }
    QTableWidgetItem* pidItem = new QTableWidgetItem(processId);
    pidItem->setTextAlignment(Qt::AlignCenter);
    m_serverTable->setItem(row, COL_PROCESS_ID, pidItem);

    // 服务器ID
    QTableWidgetItem* serverIdItem = new QTableWidgetItem(QString::fromStdString(config->m_serverId));
    serverIdItem->setTextAlignment(Qt::AlignCenter);
    m_serverTable->setItem(row, COL_SERVER_ID, serverIdItem);

    // 服务器名称
    QTableWidgetItem* nameItem = new QTableWidgetItem(QString::fromStdString(config->m_serverName));
    m_serverTable->setItem(row, COL_SERVER_NAME, nameItem);

    // 游戏名称
    QTableWidgetItem* gameItem = new QTableWidgetItem(QString::fromStdString(config->m_gameName));
    m_serverTable->setItem(row, COL_GAME_NAME, gameItem);

    // 状态
    QString statusText = statusToString(config->m_status);
    QTableWidgetItem* statusItem = new QTableWidgetItem(statusText);
    statusItem->setTextAlignment(Qt::AlignCenter);

    // 设置状态颜色
    QColor statusColor = statusToColor(config->m_status);
    statusItem->setForeground(QBrush(statusColor));
    if (config->m_status == NFServerStatus::SERVER_STATUS_RUNNING)
    {
        statusItem->setBackground(QBrush(QColor(200, 255, 200))); // 浅绿色背景
    }
    else if (config->m_status == NFServerStatus::SERVER_STATUS_ERROR)
    {
        statusItem->setBackground(QBrush(QColor(255, 200, 200))); // 浅红色背景
    }

    m_serverTable->setItem(row, COL_STATUS, statusItem);

    // 配置路径
    QTableWidgetItem* configItem = new QTableWidgetItem(QString::fromStdString(config->m_configPath));
    m_serverTable->setItem(row, COL_CONFIG_PATH, configItem);

    // 插件路径
    QTableWidgetItem* pluginItem = new QTableWidgetItem(QString::fromStdString(config->m_pluginPath));
    m_serverTable->setItem(row, COL_PLUGIN_PATH, pluginItem);

    // 执行文件路径
    QTableWidgetItem* exeItem = new QTableWidgetItem(QString::fromStdString(config->m_executablePath));
    m_serverTable->setItem(row, COL_EXECUTABLE_PATH, exeItem);

    // 工作目录
    QTableWidgetItem* workDirItem = new QTableWidgetItem(QString::fromStdString(config->m_workingDir));
    m_serverTable->setItem(row, COL_WORKING_DIR, workDirItem);
}

QString NFServerControllerQt::statusToString(NFServerStatus status)
{
    switch (status)
    {
        case NFServerStatus::SERVER_STATUS_STOPPED: return "Stopped";
        case NFServerStatus::SERVER_STATUS_STARTING: return "Starting";
        case NFServerStatus::SERVER_STATUS_RUNNING: return "Running";
        case NFServerStatus::SERVER_STATUS_STOPPING: return "Stopping";
        case NFServerStatus::SERVER_STATUS_ERROR: return "Error";
        default: return "Unknown";
    }
}

QColor NFServerControllerQt::statusToColor(NFServerStatus status)
{
    switch (status)
    {
        case NFServerStatus::SERVER_STATUS_STOPPED: return QColor(128, 128, 128); // 灰色
        case NFServerStatus::SERVER_STATUS_STARTING: return QColor(255, 165, 0); // 橙色
        case NFServerStatus::SERVER_STATUS_RUNNING: return QColor(0, 128, 0); // 绿色
        case NFServerStatus::SERVER_STATUS_STOPPING: return QColor(255, 165, 0); // 橙色
        case NFServerStatus::SERVER_STATUS_ERROR: return QColor(255, 0, 0); // 红色
        default: return QColor(0, 0, 0); // 黑色
    }
}

void NFServerControllerQt::updateStatusBar()
{
    if (!m_controller || !m_isInitialized)
    {
        return;
    }

    try
    {
        auto statusMap = m_controller->GetAllServerStatus();

        int total = statusMap.size();
        int running = 0;
        int stopped = 0;
        int error = 0;

        for (const auto& pair : statusMap)
        {
            switch (pair.second)
            {
                case NFServerStatus::SERVER_STATUS_RUNNING:
                    running++;
                    break;
                case NFServerStatus::SERVER_STATUS_STOPPED:
                    stopped++;
                    break;
                case NFServerStatus::SERVER_STATUS_ERROR:
                    error++;
                    break;
                default:
                    break;
            }
        }

        m_serverCountLabel->setText(QString("Servers: %1").arg(total));
        m_runningCountLabel->setText(QString("Running: %1").arg(running));
        m_stoppedCountLabel->setText(QString("Stopped: %1").arg(stopped));

        // Update status label
        if (error > 0)
        {
            m_statusLabel->setText(QString("%1 servers have errors").arg(error));
            m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
        }
        else if (running == total && total > 0)
        {
            m_statusLabel->setText("All servers running normally");
            m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
        }
        else if (running == 0 && total > 0)
        {
            m_statusLabel->setText("All servers stopped");
            m_statusLabel->setStyleSheet("color: gray;");
        }
        else
        {
            m_statusLabel->setText("Some servers running");
            m_statusLabel->setStyleSheet("color: orange;");
        }
    }
    catch (const std::exception& e)
    {
        m_statusLabel->setText(QString("Status update error: %1").arg(e.what()));
        m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
    } catch (...)
    {
        m_statusLabel->setText("Status update failed");
        m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
    }
}

void NFServerControllerQt::addLogMessage(const QString& message, const QString& type)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString coloredMessage;

    if (type == "ERROR")
    {
        coloredMessage = QString("<span style='color: #ff4444;'>[%1] [ERROR] %2</span>").arg(timestamp, message);
    }
    else if (type == "WARN")
    {
        coloredMessage = QString("<span style='color: #ffaa00;'>[%1] [WARN] %2</span>").arg(timestamp, message);
    }
    else if (type == "INFO")
    {
        coloredMessage = QString("<span style='color: #00aa00;'>[%1] [INFO] %2</span>").arg(timestamp, message);
    }
    else
    {
        coloredMessage = QString("<span style='color: #cccccc;'>[%1] %2</span>").arg(timestamp, message);
    }

    m_logEdit->append(coloredMessage);

    // 优化的日志大小管理，减少频繁检查
    static int logCounter = 0;
    logCounter++;

    // 每100条日志才检查一次大小，提高性能
    if (logCounter % 100 == 0)
    {
        QTextDocument* doc = m_logEdit->document();
        const int maxBlocks = 1000;
        if (doc->blockCount() > maxBlocks)
        {
            // 一次性删除多行，减少操作次数
            QTextCursor cursor(doc);
            cursor.movePosition(QTextCursor::Start);
            int linesToDelete = doc->blockCount() - maxBlocks + 100; // 多删除一些，减少下次清理频率
            for (int i = 0; i < linesToDelete && !cursor.atEnd(); ++i)
            {
                cursor.select(QTextCursor::BlockUnderCursor);
                cursor.removeSelectedText();
                if (!cursor.atEnd())
                {
                    cursor.deleteChar(); // 删除换行符
                }
            }
        }
    }

    // 确保日志滚动到底部，方便查看最新消息
    QTextCursor cursor = m_logEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logEdit->setTextCursor(cursor);
    m_logEdit->ensureCursorVisible();
}

std::vector<std::string> NFServerControllerQt::getSelectedServerIds()
{
    std::vector<std::string> selectedIds;

    for (int row = 0; row < m_serverTable->rowCount(); ++row)
    {
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(m_serverTable->cellWidget(row, COL_SELECT));
        if (checkBox && checkBox->isChecked())
        {
            QTableWidgetItem* serverIdItem = m_serverTable->item(row, COL_SERVER_ID);
            if (serverIdItem)
            {
                selectedIds.push_back(safeQStringToStdString(serverIdItem->text()));
            }
        }
    }

    return selectedIds;
}

// 槽函数实现

void NFServerControllerQt::closeEvent(QCloseEvent* event)
{
    saveSettingsToFile();
    event->accept();
}

void NFServerControllerQt::openConfigFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Open Server Config File",
                                                    QDir::currentPath(),
                                                    "Config Files (*.conf *.txt);;All Files (*.*)");

    if (!fileName.isEmpty())
    {
        m_currentConfigFile = fileName;
        m_configPathEdit->setText(fileName);
        reloadConfigFile();
    }
}

void NFServerControllerQt::reloadConfigFile()
{
    if (m_currentConfigFile.isEmpty())
    {
        addLogMessage("No config file specified", "ERROR");
        return;
    }

    m_currentConfigFile = m_configPathEdit->text();

    addLogMessage("Reloading config file: " + m_currentConfigFile, "INFO");

    if (m_controller)
    {
        try
        {
            // 安全的字符串转换
            std::string configPath = safeQStringToStdString(m_currentConfigFile);
            if (m_controller->Initialize(configPath))
            {
                m_isInitialized = true;
                addLogMessage("Config file loaded successfully", "INFO");
                updateServerTable();
                updateStatusBar();
            }
            else
            {
                m_isInitialized = false;
                addLogMessage("Failed to load config file: " + m_currentConfigFile, "ERROR");
                m_serverTable->setRowCount(0);
                updateStatusBar();
            }
        }
        catch (const std::exception& e)
        {
            m_isInitialized = false;
            addLogMessage(QString("Config reload exception: %1").arg(e.what()), "ERROR");
            m_serverTable->setRowCount(0);
            updateStatusBar();
        } catch (...)
        {
            m_isInitialized = false;
            addLogMessage("Config reload failed with unknown exception", "ERROR");
            m_serverTable->setRowCount(0);
            updateStatusBar();
        }
    }
    else
    {
        m_isInitialized = false;
        addLogMessage("Controller is not available", "ERROR");
        m_serverTable->setRowCount(0);
        updateStatusBar();
    }
}

void NFServerControllerQt::saveSettings()
{
    saveSettingsToFile();
    addLogMessage("Settings saved", "INFO");
}

void NFServerControllerQt::exitApplication()
{
    close();
}

void NFServerControllerQt::executeServerOperation(const std::function<bool()>& operation, const QString& operationName)
{
    // 如果有操作正在进行中，拒绝新的操作
    if (m_operationInProgress)
    {
        addLogMessage("Another operation is in progress. Please wait or interrupt the current operation.", "WARN");
        return;
    }

    // 设置操作状态
    m_operationInProgress = true;
    setOperationButtonsEnabled(false);
    setInterruptButtonEnabled(true);

    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0); // Show infinite progress bar
    m_statusLabel->setText("Executing: " + operationName);

    // Execute operation in background thread
    ServerOperation* serverOp = new ServerOperation(operation, this);
    m_currentOperation = serverOp;

    // 使用Qt::QueuedConnection确保UI操作在主线程中执行
    connect(serverOp, &ServerOperation::finished, this, [this, serverOp, operationName]()
    {
        bool success = serverOp->getResult();
        QString errorMessage = serverOp->getErrorMessage();

        try
        {
            m_progressBar->setVisible(false);

            // 记录详细的操作日志
            if (!m_lastOperationResults.empty() && !m_lastOperationServerInfos.empty())
            {
                int successCount = 0;
                int failCount = 0;
                int alreadyRunningCount = 0;

                for (size_t i = 0; i < m_lastOperationResults.size() && i < m_lastOperationServerInfos.size(); ++i)
                {
                    const auto& result = m_lastOperationResults[i];
                    const auto& serverInfo = m_lastOperationServerInfos[i];

                    const std::string& serverId = result.first;
                    bool operationSuccess = result.second;
                    const std::string& serverName = serverInfo.second;

                    // 查找服务器的初始状态
                    NFServerStatus initialStatus = NFServerStatus::SERVER_STATUS_STOPPED;
                    if (i < m_lastOperationInitialStates.size())
                    {
                        initialStatus = m_lastOperationInitialStates[i].second;
                    }

                    // 查找PID验证详情
                    std::string pidDetail;
                    if (i < m_lastOperationPidDetails.size())
                    {
                        pidDetail = m_lastOperationPidDetails[i].second;
                    }

                    if (operationSuccess)
                    {
                        // 检查是否是"已经在运行"的情况（基于初始状态）
                        bool wasAlreadyRunning = (operationName.contains("Start") &&
                            initialStatus == NFServerStatus::SERVER_STATUS_RUNNING);

                        if (wasAlreadyRunning)
                        {
                            alreadyRunningCount++;
                            addLogMessage(QString("[ALREADY RUNNING] %1: %2 (ID: %3) - Server was already running")
                                          .arg(operationName)
                                          .arg(QString::fromStdString(serverName))
                                          .arg(QString::fromStdString(serverId)), "INFO");
                        }
                        else
                        {
                            successCount++;
                            QString message = QString("[SUCCESS] %1 succeeded: %2 (ID: %3)")
                                              .arg(operationName)
                                              .arg(QString::fromStdString(serverName))
                                              .arg(QString::fromStdString(serverId));

                            // 添加PID验证详情（如果有的话）
                            if (!pidDetail.empty())
                            {
                                message += " - " + QString::fromStdString(pidDetail);
                            }

                            addLogMessage(message, "INFO");
                        }
                    }
                    else
                    {
                        failCount++;
                        QString message = QString("[FAILED] %1 failed: %2 (ID: %3)")
                                          .arg(operationName)
                                          .arg(QString::fromStdString(serverName))
                                          .arg(QString::fromStdString(serverId));

                        // 添加PID验证详情（如果有的话）
                        if (!pidDetail.empty())
                        {
                            message += " - " + QString::fromStdString(pidDetail);
                        }
                        else
                        {
                            message += " - Check server configuration and dependencies";
                        }

                        addLogMessage(message, "ERROR");
                    }
                }

                // 记录总体结果 - 更准确的状态描述
                int totalProcessed = successCount + alreadyRunningCount + failCount;

                if (failCount == 0)
                {
                    if (alreadyRunningCount > 0)
                    {
                        addLogMessage(QString("[ALL SUCCESS] %1 - %2 servers succeeded, %3 already running")
                                      .arg(operationName).arg(successCount).arg(alreadyRunningCount), "INFO");
                        m_statusLabel->setText(QString("%1 - %2 succeeded, %3 already running")
                                               .arg(operationName).arg(successCount).arg(alreadyRunningCount));
                    }
                    else
                    {
                        addLogMessage(QString("[ALL SUCCESS] %1 - All %2 servers succeeded").arg(operationName).arg(successCount), "INFO");
                        m_statusLabel->setText(QString("%1 - All %2 servers succeeded").arg(operationName).arg(successCount));
                    }
                }
                else if (successCount + alreadyRunningCount == 0)
                {
                    addLogMessage(QString("[ALL FAILED] %1 - All %2 servers failed").arg(operationName).arg(failCount), "ERROR");
                    m_statusLabel->setText(QString("%1 - All servers failed").arg(operationName));
                }
                else
                {
                    QString message = QString("[PARTIAL SUCCESS] %1 completed: %2 succeeded").arg(operationName).arg(successCount);
                    QString statusMessage = QString("%1 - %2 succeeded").arg(operationName).arg(successCount);

                    if (alreadyRunningCount > 0)
                    {
                        message += QString(", %1 already running").arg(alreadyRunningCount);
                        statusMessage += QString(", %1 already running").arg(alreadyRunningCount);
                    }

                    message += QString(", %1 failed").arg(failCount);
                    statusMessage += QString(", %1 failed").arg(failCount);

                    addLogMessage(message, "WARN");
                    m_statusLabel->setText(statusMessage);
                }

                // 保存操作结果用于延迟验证（在清空之前）
                auto savedResults = m_lastOperationResults;
                auto savedInfos = m_lastOperationServerInfos;

                // 清空结果缓存
                m_lastOperationResults.clear();
                m_lastOperationServerInfos.clear();
                m_lastOperationInitialStates.clear();
                m_lastOperationPidDetails.clear();

                // 延迟刷新服务器状态（针对启动操作），但不产生额外的日志
                if (operationName.contains("Start") && !savedResults.empty())
                {
                    QTimer::singleShot(3000, this, [this]()
                    {
                        try
                        {
                            if (m_controller)
                            {
                                // 仅刷新界面状态，让用户通过界面看到真实的服务器状态
                                m_controller->UpdateServerStatus();
                                updateServerTable();
                                updateStatusBar();
                            }
                        }
                        catch (...)
                        {
                            // 忽略刷新中的异常
                        }
                    });
                }
            }
            else
            {
                // 兜底的简单日志记录 - 改进逻辑避免误导
                if (success)
                {
                    addLogMessage(operationName + " operation completed successfully", "INFO");
                    m_statusLabel->setText(operationName + " succeeded");
                }
                else
                {
                    // 更准确的失败描述，避免误导用户
                    QString failureMsg = operationName + " operation completed with issues";
                    if (!errorMessage.isEmpty())
                    {
                        failureMsg += " - " + errorMessage;
                    }
                    else
                    {
                        failureMsg += " - Some servers may have failed to start, check individual server status";
                    }
                    addLogMessage(failureMsg, "WARN"); // 使用WARN而不是ERROR
                    m_statusLabel->setText(operationName + " completed with issues");
                }
            }

            // 立即刷新界面状态
            updateServerTable();
            updateStatusBar();
        }
        catch (const std::exception& e)
        {
            addLogMessage(QString("UI update exception: %1").arg(e.what()), "ERROR");
        }
        catch (...)
        {
            addLogMessage("UI update failed with unknown exception", "ERROR");
        }

        // 恢复按钮状态
        m_operationInProgress = false;
        m_currentOperation = nullptr;
        setOperationButtonsEnabled(true);
        setInterruptButtonEnabled(false);

        serverOp->deleteLater();
    }, Qt::QueuedConnection);

    serverOp->start();
}

void NFServerControllerQt::startAllServers()
{
    auto allConfigs = m_controller->GetAllServerConfigs();
    addLogMessage(QString("Starting all servers (%1 total) in configuration order...").arg(allConfigs.size()), "INFO");

    executeServerOperation([this]()
    {
        auto configOrder = m_controller->GetServerConfigOrder();
        bool allSuccess = true;
        int successCount = 0;
        int failCount = 0;

        for (const std::string& serverId : configOrder)
        {
            // 检查中断标志 - 阻止后续服务器执行
            if (m_currentOperation && m_currentOperation->isInterrupted())
            {
                break; // 跳出循环，不再处理后续服务器
            }

            bool result = m_controller->StartServer(serverId);
            if (result)
            {
                successCount++;
            }
            else
            {
                failCount++;
                allSuccess = false;
            }
            
            // 再次检查中断标志 - 在延迟前检查，避免不必要的等待
            if (m_currentOperation && m_currentOperation->isInterrupted())
            {
                break; // 跳出循环，不再处理后续服务器
        }

            // 服务器启动之间的延迟
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        return allSuccess;
    }, "Start All Servers");
}

void NFServerControllerQt::stopAllServers()
{
    int ret = QMessageBox::question(this, "Confirm Operation",
                                    "Are you sure to stop all servers?",
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);

    if (ret == QMessageBox::Yes)
    {
        auto allConfigs = m_controller->GetAllServerConfigs();
        addLogMessage(QString("Stopping all servers (%1 total) in reverse configuration order...").arg(allConfigs.size()), "INFO");

        executeServerOperation([this]()
        {
            auto configOrder = m_controller->GetServerConfigOrder();
            bool allSuccess = true;
            int successCount = 0;
            int failCount = 0;

            // 停止服务器按逆序进行
            for (auto it = configOrder.rbegin(); it != configOrder.rend(); ++it)
            {
                const std::string& serverId = *it;
                
                // 检查中断标志 - 阻止后续服务器执行
                if (m_currentOperation && m_currentOperation->isInterrupted())
                {
                    break; // 跳出循环，不再处理后续服务器
                }

                bool result = m_controller->StopServer(serverId);
                if (result)
                {
                    successCount++;
                }
                else
                {
                    failCount++;
                    allSuccess = false;
                }
                
                // 再次检查中断标志 - 在延迟前检查，避免不必要的等待
                if (m_currentOperation && m_currentOperation->isInterrupted())
                {
                    break; // 跳出循环，不再处理后续服务器
            }

                // 服务器停止之间的延迟
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            return allSuccess;
        }, "Stop All Servers");
    }
}

void NFServerControllerQt::restartAllServers()
{
    int ret = QMessageBox::question(this, "Confirm Operation",
                                    "Are you sure to restart all servers?",
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);

    if (ret == QMessageBox::Yes)
    {
        auto allConfigs = m_controller->GetAllServerConfigs();
        addLogMessage(QString("Restarting all servers (%1 total) in configuration order...").arg(allConfigs.size()), "INFO");

        executeServerOperation([this]()
        {
            auto configOrder = m_controller->GetServerConfigOrder();
            bool allSuccess = true;
            int successCount = 0;
            int failCount = 0;

            // 重启服务器按配置顺序进行
            for (const std::string& serverId : configOrder)
            {
                // 检查中断标志 - 阻止后续服务器执行
                if (m_currentOperation && m_currentOperation->isInterrupted())
                {
                    break; // 跳出循环，不再处理后续服务器
                }

                bool result = m_controller->RestartServer(serverId);
                if (result)
                {
                    successCount++;
                }
                else
                {
                    failCount++;
                    allSuccess = false;
                }
                
                // 再次检查中断标志 - 在延迟前检查，避免不必要的等待
                if (m_currentOperation && m_currentOperation->isInterrupted())
                {
                    break; // 跳出循环，不再处理后续服务器
            }

                // 服务器重启之间的延迟
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }

            return allSuccess;
        }, "Restart All Servers");
    }
}

void NFServerControllerQt::reloadAllServers()
{
    auto allConfigs = m_controller->GetAllServerConfigs();
    addLogMessage(QString("Reloading configuration for all servers (%1 total) in configuration order...").arg(allConfigs.size()), "INFO");

    executeServerOperation([this]()
    {
        auto configOrder = m_controller->GetServerConfigOrder();
        bool allSuccess = true;
        int successCount = 0;
        int failCount = 0;

        // 重载服务器配置按配置顺序进行
        for (const std::string& serverId : configOrder)
        {
            // 检查中断标志 - 阻止后续服务器执行
            if (m_currentOperation && m_currentOperation->isInterrupted())
            {
                break; // 跳出循环，不再处理后续服务器
            }

            bool result = m_controller->ReloadServer(serverId);
            if (result)
            {
                successCount++;
            }
            else
            {
                failCount++;
                allSuccess = false;
            }
            
            // 再次检查中断标志 - 在延迟前检查，避免不必要的等待
            if (m_currentOperation && m_currentOperation->isInterrupted())
            {
                break; // 跳出循环，不再处理后续服务器
        }

            // 服务器重载之间的延迟
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        return allSuccess;
    }, "Reload All Server Configs");
}

void NFServerControllerQt::startSelectedServers()
{
    auto selectedIds = getSelectedServerIds();
    if (selectedIds.empty())
    {
        QMessageBox::information(this, "Tip", "Please select servers to operate");
        return;
    }

    int ret = QMessageBox::question(this, "Confirm Operation",
                                    QString("Are you sure to start %1 selected servers?").arg(selectedIds.size()),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);

    if (ret == QMessageBox::Yes)
    {
        // 按配置文件顺序对选中的服务器进行排序
        if (m_controller)
        {
            const auto& configOrder = m_controller->GetServerConfigOrder();
            std::vector<std::string> orderedSelectedIds;
            
            // 按配置顺序添加选中的服务器
            for (const auto& serverId : configOrder)
            {
                if (std::find(selectedIds.begin(), selectedIds.end(), serverId) != selectedIds.end())
                {
                    orderedSelectedIds.push_back(serverId);
                }
            }
            selectedIds = orderedSelectedIds;
        }

        // 记录开始操作的日志
        addLogMessage(QString("Starting %1 selected servers in configuration order...").arg(selectedIds.size()), "INFO");

        // 获取服务器信息用于日志记录
        std::vector<std::pair<std::string, std::string>> serverInfos; // serverId, serverName
        std::vector<std::pair<std::string, NFServerStatus>> serverInitialStates; // serverId, initialStatus

        if (m_controller)
        {
            auto allConfigs = m_controller->GetAllServerConfigs();
            for (const auto& serverId : selectedIds)
            {
                auto configIt = allConfigs.find(serverId);
                if (configIt != allConfigs.end() && configIt->second)
                {
                    serverInfos.emplace_back(serverId, configIt->second->m_serverName);
                    serverInitialStates.emplace_back(serverId, configIt->second->m_status);
                }
                else
                {
                    serverInfos.emplace_back(serverId, serverId); // 使用ID作为名称
                    serverInitialStates.emplace_back(serverId, NFServerStatus::SERVER_STATUS_UNKNOWN);
                }
            }
        }

        executeServerOperation([this, selectedIds, serverInfos, serverInitialStates]()
        {
            std::vector<std::pair<std::string, bool>> results; // serverId, success
            std::vector<std::pair<std::string, std::string>> pidDetails; // serverId, 详情
            bool allSuccess = true;
            int successCount = 0;
            int failCount = 0;
            int alreadyRunningCount = 0;

            // 获取所有服务器配置信息
            auto allConfigs = m_controller->GetAllServerConfigs();

            for (const auto& serverId : selectedIds)
            {
                // 检查中断标志 - 阻止后续服务器执行
                if (m_currentOperation && m_currentOperation->isInterrupted())
                {
                    break; // 跳出循环，不再处理后续服务器
                }

                // 查找服务器配置以获取详细信息
                auto configIt = allConfigs.find(serverId);
                std::string serverName = serverId; // 默认使用ID
                if (configIt != allConfigs.end() && configIt->second)
                {
                    serverName = configIt->second->m_serverName;

                    // 如果服务器已经在运行，视为成功（避免重复启动）
                    if (configIt->second->m_status == NFServerStatus::SERVER_STATUS_RUNNING)
                    {
                        results.emplace_back(serverId, true);
                        pidDetails.emplace_back(serverId, "Server already running");
                        alreadyRunningCount++;
                        continue;
                    }
                }

                // 调用启动操作
                bool result = m_controller->StartServer(serverId);
                results.emplace_back(serverId, result);

                if (result)
                {
                    successCount++;
                    pidDetails.emplace_back(serverId, "Start command executed successfully");
                }
                else
                {
                    failCount++;
                    allSuccess = false;
                    pidDetails.emplace_back(serverId, "Start command failed");
                }

                // 再次检查中断标志 - 在延迟前检查，避免不必要的等待
                if (m_currentOperation && m_currentOperation->isInterrupted())
                {
                    break; // 跳出循环，不再处理后续服务器
                }

                // 服务器启动之间的延迟
                if (selectedIds.size() > 1) // 只有多个服务器时才延迟
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }

            // 将结果存储到成员变量中，供回调使用
            m_lastOperationResults = results;
            m_lastOperationServerInfos = serverInfos;
            m_lastOperationInitialStates = serverInitialStates;
            m_lastOperationPidDetails = pidDetails;

            return allSuccess;
        }, QString("Start selected servers"));
    }
}

void NFServerControllerQt::stopSelectedServers()
{
    auto selectedIds = getSelectedServerIds();
    if (selectedIds.empty())
    {
        QMessageBox::information(this, "Tip", "Please select servers to operate");
        return;
    }

    int ret = QMessageBox::question(this, "Confirm Operation",
                                    QString("Are you sure to stop %1 selected servers?").arg(selectedIds.size()),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);

    if (ret == QMessageBox::Yes)
    {
        // 按配置文件逆序对选中的服务器进行排序（停止服务器应该按逆序）
        if (m_controller)
        {
            const auto& configOrder = m_controller->GetServerConfigOrder();
            std::vector<std::string> orderedSelectedIds;
            
            // 按配置逆序添加选中的服务器
            for (auto it = configOrder.rbegin(); it != configOrder.rend(); ++it)
            {
                const auto& serverId = *it;
                if (std::find(selectedIds.begin(), selectedIds.end(), serverId) != selectedIds.end())
                {
                    orderedSelectedIds.push_back(serverId);
                }
            }
            selectedIds = orderedSelectedIds;
        }

        // 记录开始操作的日志
        addLogMessage(QString("Stopping %1 selected servers in reverse configuration order...").arg(selectedIds.size()), "INFO");

        executeServerOperation([this, selectedIds]()
        {
            bool allSuccess = true;
            int successCount = 0;
            int failCount = 0;

            // 获取所有服务器配置信息
            auto allConfigs = m_controller->GetAllServerConfigs();

            for (const auto& serverId : selectedIds)
            {
                // 检查中断标志 - 阻止后续服务器执行
                if (m_currentOperation && m_currentOperation->isInterrupted())
                {
                    break; // 跳出循环，不再处理后续服务器
                }

                // 查找服务器配置以获取详细信息
                auto configIt = allConfigs.find(serverId);
                std::string serverName = serverId; // 默认使用ID
                if (configIt != allConfigs.end() && configIt->second)
                {
                    serverName = configIt->second->m_serverName;
                }

                bool result = m_controller->StopServer(serverId);
                if (result)
                {
                    successCount++;
                }
                else
                {
                    failCount++;
                    allSuccess = false;
                }

                // 再次检查中断标志 - 在延迟前检查，避免不必要的等待
                if (m_currentOperation && m_currentOperation->isInterrupted())
                {
                    break; // 跳出循环，不再处理后续服务器
                }

                // 服务器停止之间的延迟
                if (selectedIds.size() > 1) // 只有多个服务器时才延迟
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }

            // 记录总体结果
            if (allSuccess)
            {
                addLogMessage(QString("[ALL SUCCESS] All %1 selected servers stopped successfully").arg(successCount), "INFO");
            }
            else if (successCount > 0)
            {
                addLogMessage(QString("[WARNING] Stop operation completed: %1 succeeded, %2 failed").arg(successCount).arg(failCount), "WARN");
            }
            else
            {
                addLogMessage("❌ All selected servers failed to stop", "ERROR");
            }

            return allSuccess;
        }, QString("Stop selected servers"));
    }
}

void NFServerControllerQt::restartSelectedServers()
{
    auto selectedIds = getSelectedServerIds();
    if (selectedIds.empty())
    {
        QMessageBox::information(this, "Tip", "Please select servers to operate");
        return;
    }

    int ret = QMessageBox::question(this, "Confirm Operation",
                                    QString("Are you sure to restart %1 selected servers?").arg(selectedIds.size()),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);

    if (ret == QMessageBox::Yes)
    {
        // 按配置文件顺序对选中的服务器进行排序
        if (m_controller)
        {
            const auto& configOrder = m_controller->GetServerConfigOrder();
            std::vector<std::string> orderedSelectedIds;
            
            // 按配置顺序添加选中的服务器
            for (const auto& serverId : configOrder)
            {
                if (std::find(selectedIds.begin(), selectedIds.end(), serverId) != selectedIds.end())
                {
                    orderedSelectedIds.push_back(serverId);
                }
            }
            selectedIds = orderedSelectedIds;
        }

        // 记录开始操作的日志
        addLogMessage(QString("Restarting %1 selected servers in configuration order...").arg(selectedIds.size()), "INFO");

        executeServerOperation([this, selectedIds]()
        {
            bool allSuccess = true;
            int successCount = 0;
            int failCount = 0;

            // 获取所有服务器配置信息
            auto allConfigs = m_controller->GetAllServerConfigs();

            for (const auto& serverId : selectedIds)
            {
                // 检查中断标志 - 阻止后续服务器执行
                if (m_currentOperation && m_currentOperation->isInterrupted())
                {
                    break; // 跳出循环，不再处理后续服务器
                }

                // 查找服务器配置以获取详细信息
                auto configIt = allConfigs.find(serverId);
                std::string serverName = serverId; // 默认使用ID
                if (configIt != allConfigs.end() && configIt->second)
                {
                    serverName = configIt->second->m_serverName;
                }

                bool result = m_controller->RestartServer(serverId);
                if (result)
                {
                    successCount++;
                }
                else
                {
                    failCount++;
                    allSuccess = false;
                }

                // 再次检查中断标志 - 在延迟前检查，避免不必要的等待
                if (m_currentOperation && m_currentOperation->isInterrupted())
                {
                    break; // 跳出循环，不再处理后续服务器
                }

                // 服务器重启之间的延迟
                if (selectedIds.size() > 1) // 只有多个服务器时才延迟
                {
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }
            }

            // 记录总体结果
            if (allSuccess)
            {
                addLogMessage(QString("[ALL SUCCESS] All %1 servers restarted successfully").arg(successCount), "INFO");
            }
            else
            {
                addLogMessage(QString("[WARNING] Restart operation completed: %1 succeeded, %2 failed")
                              .arg(successCount).arg(failCount), "WARN");
            }

            return allSuccess;
        }, QString("Restart selected servers"));
    }
}

void NFServerControllerQt::reloadSelectedServers()
{
    auto selectedIds = getSelectedServerIds();
    if (selectedIds.empty())
    {
        QMessageBox::information(this, "Tip", "Please select servers to operate");
        return;
    }

    // 按配置文件顺序对选中的服务器进行排序
    if (m_controller)
    {
        const auto& configOrder = m_controller->GetServerConfigOrder();
        std::vector<std::string> orderedSelectedIds;
        
        // 按配置顺序添加选中的服务器
        for (const auto& serverId : configOrder)
        {
            if (std::find(selectedIds.begin(), selectedIds.end(), serverId) != selectedIds.end())
            {
                orderedSelectedIds.push_back(serverId);
            }
        }
        selectedIds = orderedSelectedIds;
    }

    // 记录开始操作的日志
    addLogMessage(QString("Reloading configuration for %1 selected servers in configuration order...").arg(selectedIds.size()), "INFO");

    executeServerOperation([this, selectedIds]()
    {
        bool allSuccess = true;
        int successCount = 0;
        int failCount = 0;

        // 获取所有服务器配置信息
        auto allConfigs = m_controller->GetAllServerConfigs();

        for (const auto& serverId : selectedIds)
        {
            // 检查中断标志 - 阻止后续服务器执行
            if (m_currentOperation && m_currentOperation->isInterrupted())
            {
                break; // 跳出循环，不再处理后续服务器
            }

            // 查找服务器配置以获取详细信息
            auto configIt = allConfigs.find(serverId);
            std::string serverName = serverId; // 默认使用ID
            if (configIt != allConfigs.end() && configIt->second)
            {
                serverName = configIt->second->m_serverName;
            }

            bool result = m_controller->ReloadServer(serverId);
            if (result)
            {
                successCount++;
            }
            else
            {
                failCount++;
                allSuccess = false;
            }

            // 再次检查中断标志 - 在延迟前检查，避免不必要的等待
            if (m_currentOperation && m_currentOperation->isInterrupted())
            {
                break; // 跳出循环，不再处理后续服务器
            }

            // 服务器重载之间的延迟
            if (selectedIds.size() > 1) // 只有多个服务器时才延迟
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        // 记录总体结果
        if (allSuccess)
        {
            addLogMessage(QString("[ALL SUCCESS] All %1 server configurations reloaded successfully").arg(successCount), "INFO");
        }
        else
        {
            addLogMessage(QString("[WARNING] Reload operation completed: %1 succeeded, %2 failed")
                          .arg(successCount).arg(failCount), "WARN");
        }

        return allSuccess;
    }, QString("Reload selected servers"));
}

void NFServerControllerQt::startServerByPattern()
{
    QString pattern = m_patternEdit->text().trimmed();
    if (pattern.isEmpty())
    {
        QMessageBox::information(this, "Tip", "Please enter server pattern");
        return;
    }

    executeServerOperation([this, pattern]()
    {
        return m_controller->StartServersByPattern(safeQStringToStdString(pattern));
    }, QString("Start servers matching pattern '%1'").arg(pattern));
}

void NFServerControllerQt::stopServerByPattern()
{
    QString pattern = m_patternEdit->text().trimmed();
    if (pattern.isEmpty())
    {
        QMessageBox::information(this, "Tip", "Please enter server pattern");
        return;
    }

    int ret = QMessageBox::question(this, "Confirm Operation",
                                    QString("Are you sure to stop all servers matching pattern '%1'?").arg(pattern),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);

    if (ret == QMessageBox::Yes)
    {
        executeServerOperation([this, pattern]()
        {
            return m_controller->StopServersByPattern(safeQStringToStdString(pattern));
        }, QString("Stop servers matching pattern '%1'").arg(pattern));
    }
}

void NFServerControllerQt::restartServerByPattern()
{
    QString pattern = m_patternEdit->text().trimmed();
    if (pattern.isEmpty())
    {
        QMessageBox::information(this, "Tip", "Please enter server pattern");
        return;
    }

    int ret = QMessageBox::question(this, "Confirm Operation",
                                    QString("Are you sure to restart all servers matching pattern '%1'?").arg(pattern),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);

    if (ret == QMessageBox::Yes)
    {
        executeServerOperation([this, pattern]()
        {
            return m_controller->RestartServersByPattern(safeQStringToStdString(pattern));
        }, QString("Restart servers matching pattern '%1'").arg(pattern));
    }
}

void NFServerControllerQt::reloadServerByPattern()
{
    QString pattern = m_patternEdit->text().trimmed();
    if (pattern.isEmpty())
    {
        QMessageBox::information(this, "Tip", "Please enter server pattern");
        return;
    }

    executeServerOperation([this, pattern]()
    {
        return m_controller->ReloadServersByPattern(safeQStringToStdString(pattern));
    }, QString("Reload configs for servers matching pattern '%1'").arg(pattern));
}

void NFServerControllerQt::refreshServerStatus()
{
    if (!m_isInitialized)
    {
        return;
    }

    addLogMessage("Refreshing server status...", "INFO");

    // 首先调用核心控制器的状态更新逻辑，实际检查进程状态
    if (m_controller)
    {
        m_controller->UpdateServerStatus();
    }

    // 然后更新UI显示
    updateServerTable();
    updateStatusBar();
    addLogMessage("Server status refresh completed", "INFO");
}

void NFServerControllerQt::onServerSelectionChanged()
{
    // 可以在这里添加选择变化时的逻辑
}

void NFServerControllerQt::onServerDoubleClicked(int row, int column)
{
    // 双击行时可以显示服务器详细信息
    QTableWidgetItem* serverIdItem = m_serverTable->item(row, COL_SERVER_ID);
    if (serverIdItem)
    {
        QString serverId = serverIdItem->text();

        // 这里可以添加显示服务器详细信息的对话框
        QMessageBox::information(this, "Server Information",
                                 QString("Server ID: %1\nDouble-click feature not implemented yet").arg(serverId));
    }
}

void NFServerControllerQt::toggleAutoRefresh(bool enabled)
{
    m_autoRefreshEnabled = enabled;

    if (enabled)
    {
        m_autoRefreshTimer->start(m_refreshInterval * 1000);
        addLogMessage(QString("Auto refresh enabled, interval: %1 seconds").arg(m_refreshInterval), "INFO");
    }
    else
    {
        m_autoRefreshTimer->stop();
        addLogMessage("Auto refresh disabled", "INFO");
    }
}

void NFServerControllerQt::onAutoRefreshTimer()
{
    if (m_isInitialized)
    {
        // 首先调用核心控制器的状态更新逻辑，实际检查进程状态
        if (m_controller)
        {
            m_controller->UpdateServerStatus();
        }

        // 然后更新UI显示
        updateServerTable();
        updateStatusBar();
    }
}

void NFServerControllerQt::clearLog()
{
    m_logEdit->clear();
    addLogMessage("Log cleared", "INFO");
}

void NFServerControllerQt::saveLogToFile()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save Log File",
                                                    QDir::currentPath() + "/server_log_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".txt",
                                                    "Text Files (*.txt);;All Files (*.*)");

    if (!fileName.isEmpty())
    {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            out << m_logEdit->toPlainText();
            file.close();
            addLogMessage("Log saved to: " + fileName, "INFO");
        }
        else
        {
            addLogMessage("Failed to save log: " + fileName, "ERROR");
        }
    }
}

void NFServerControllerQt::showAbout()
{
    QMessageBox::about(this, "About NFServerController",
                       "<h3>NFServerController Qt Version</h3>"
                       "<p>Version: 2.0</p>"
                       "<p>NFrame Game Server Management Tool</p>"
                       "<p>Features:</p>"
                       "<ul>"
                       "<li>Graphical server management interface</li>"
                       "<li>Support batch operations and pattern matching</li>"
                       "<li>Real-time status monitoring</li>"
                       "<li>Detailed operation logs</li>"
                       "<li>Auto refresh functionality</li>"
                       "</ul>"
                       "<p>© 2024 NFrame Project Team</p>");
}

void NFServerControllerQt::showSettings()
{
    // 这里可以添加设置对话框
    QMessageBox::information(this, "Settings", "Settings feature not implemented yet\nVarious configuration options can be added here");
}

void NFServerControllerQt::startMonitoring()
{
    if (m_controller && m_isInitialized)
    {
        m_controller->MonitorServers();
        addLogMessage("Server monitoring started", "INFO");
    }
}

void NFServerControllerQt::stopMonitoring()
{
    if (m_controller)
    {
        m_controller->StopMonitoring();
        addLogMessage("Server monitoring stopped", "INFO");
    }
}

void NFServerControllerQt::selectAllServers()
{
    for (int row = 0; row < m_serverTable->rowCount(); ++row)
    {
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(m_serverTable->cellWidget(row, COL_SELECT));
        if (checkBox)
        {
            checkBox->setChecked(true);
        }
    }
}

void NFServerControllerQt::selectNoServers()
{
    for (int row = 0; row < m_serverTable->rowCount(); ++row)
    {
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(m_serverTable->cellWidget(row, COL_SELECT));
        if (checkBox)
        {
            checkBox->setChecked(false);
        }
    }
}

void NFServerControllerQt::selectRunningServers()
{
    for (int row = 0; row < m_serverTable->rowCount(); ++row)
    {
        QTableWidgetItem* statusItem = m_serverTable->item(row, COL_STATUS);
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(m_serverTable->cellWidget(row, COL_SELECT));

        if (statusItem && checkBox)
        {
            bool isRunning = (statusItem->text() == "Running");
            checkBox->setChecked(isRunning);
        }
    }
}

void NFServerControllerQt::selectStoppedServers()
{
    for (int row = 0; row < m_serverTable->rowCount(); ++row)
    {
        QTableWidgetItem* statusItem = m_serverTable->item(row, COL_STATUS);
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(m_serverTable->cellWidget(row, COL_SELECT));

        if (statusItem && checkBox)
        {
            bool isStopped = (statusItem->text() == "Stopped");
            checkBox->setChecked(isStopped);
        }
    }
}

void NFServerControllerQt::loadSettings()
{
    QSettings settings("NFFrame", "NFServerController");

    // 加载窗口几何
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // 加载设置
#if NF_PLATFORM == NF_PLATFORM_WIN
    m_currentConfigFile = settings.value("configFile", "win_servers.conf").toString();
#else
    m_currentConfigFile = settings.value("configFile", "linux_servers.conf").toString();
#endif
    m_autoRefreshEnabled = settings.value("autoRefresh", true).toBool();
    m_refreshInterval = settings.value("refreshInterval", 5).toInt();

    // 加载分割器状态
    m_mainSplitter->restoreState(settings.value("mainSplitterState").toByteArray());
    m_topSplitter->restoreState(settings.value("topSplitterState").toByteArray());

    // 应用设置
    m_configPathEdit->setText(m_currentConfigFile);
    m_autoRefreshCheckBox->setChecked(m_autoRefreshEnabled);
    m_refreshIntervalCombo->setCurrentText(QString("%1s").arg(m_refreshInterval));
}

std::string NFServerControllerQt::safeQStringToStdString(const QString& qstr)
{
    if (qstr.isEmpty())
    {
        return std::string();
    }

    try
    {
        // 使用toLocal8Bit()而不是toStdString()，更安全
        QByteArray utf8Data = qstr.toLocal8Bit();
        return std::string(utf8Data.constData(), utf8Data.length());
    }
    catch (...)
    {
        // 如果转换失败，返回空字符串
        return std::string();
    }
}

void NFServerControllerQt::saveSettingsToFile()
{
    QSettings settings("NFFrame", "NFServerController");

    // 保存窗口几何
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    // 保存设置
    settings.setValue("configFile", m_currentConfigFile);
    settings.setValue("autoRefresh", m_autoRefreshEnabled);
    settings.setValue("refreshInterval", m_refreshInterval);

    // 保存分割器状态
    settings.setValue("mainSplitterState", m_mainSplitter->saveState());
    settings.setValue("topSplitterState", m_topSplitter->saveState());
}

void NFServerControllerQt::setOperationButtonsEnabled(bool enabled)
{
    // All 操作按钮
    m_startAllBtn->setEnabled(enabled);
    m_stopAllBtn->setEnabled(enabled);
    m_restartAllBtn->setEnabled(enabled);
    m_reloadAllBtn->setEnabled(enabled);
    
    // Selected 操作按钮
    m_startSelectedBtn->setEnabled(enabled);
    m_stopSelectedBtn->setEnabled(enabled);
    m_restartSelectedBtn->setEnabled(enabled);
    m_reloadSelectedBtn->setEnabled(enabled);
    
    // Pattern 操作按钮
    m_startPatternBtn->setEnabled(enabled);
    m_stopPatternBtn->setEnabled(enabled);
    m_restartPatternBtn->setEnabled(enabled);
    m_reloadPatternBtn->setEnabled(enabled);
}

void NFServerControllerQt::setInterruptButtonEnabled(bool enabled)
{
    m_interruptBtn->setEnabled(enabled);
}

void NFServerControllerQt::interruptCurrentOperation()
{
    if (m_currentOperation && m_operationInProgress)
    {
        addLogMessage("🛑 User requested operation interruption. Current server will complete, but remaining servers will be skipped.", "WARN");
        m_currentOperation->interrupt();
        setInterruptButtonEnabled(false);
        
        // 更新状态栏显示中断状态
        m_statusLabel->setText("Interrupting operation - completing current server...");
    }
    else
    {
        addLogMessage("No operation is currently in progress to interrupt.", "INFO");
    }
}





