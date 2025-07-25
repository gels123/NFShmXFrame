#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QScrollBar>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QStringList>
#include <QtGui/QFont>
#include <QtGui/QIcon>
#include <QtGui/QPixmap>
#include <QtGui/QPalette>
#include <QtGui/QCloseEvent>

#include "NFServerController.h"
#include <memory>
#include <map>
#include <atomic>

// Forward declaration
enum class NFServerStatus;
class ServerOperation;

class QAction;
class QMenu;

class NFServerControllerQt : public QMainWindow
{
    Q_OBJECT

public:
    explicit NFServerControllerQt(QWidget* parent = nullptr);
    ~NFServerControllerQt();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // File menu
    void openConfigFile();
    void reloadConfigFile();
    void saveSettings();
    void exitApplication();

    // Server operations
    void startAllServers();
    void stopAllServers();
    void restartAllServers();
    void reloadAllServers();

    void startSelectedServers();
    void stopSelectedServers();
    void restartSelectedServers();
    void reloadSelectedServers();

    void startServerByPattern();
    void stopServerByPattern();
    void restartServerByPattern();
    void reloadServerByPattern();

    // Table operations
    void refreshServerStatus();
    void interruptCurrentOperation();
    void onServerSelectionChanged();
    void onServerDoubleClicked(int row, int column);

    // Auto refresh
    void toggleAutoRefresh(bool enabled);
    void onAutoRefreshTimer();

    // Log operations
    void clearLog();
    void saveLogToFile();

    // Settings
    void showAbout();
    void showSettings();

    // Monitoring
    void startMonitoring();
    void stopMonitoring();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupConnections();

    void createServerListWidget();
    void createControlPanelWidget();
    void createLogWidget();

    void updateServerTable();
    void updateServerRow(int row, const std::shared_ptr<NFServerConfig>& config);
    void updateStatusBar();

    void addLogMessage(const QString& message, const QString& type = "INFO");
    void executeServerOperation(const std::function<bool()>& operation, const QString& operationName);
    
    // Button state management
    void setOperationButtonsEnabled(bool enabled);
    void setInterruptButtonEnabled(bool enabled);

    QString statusToString(NFServerStatus status);
    QColor statusToColor(NFServerStatus status);

    void loadSettings();
    void saveSettingsToFile();

    std::vector<std::string> getSelectedServerIds();
    void selectAllServers();
    void selectNoServers();
    void selectRunningServers();
    void selectStoppedServers();

    // Auxiliary function: safe string conversion
    std::string safeQStringToStdString(const QString& qstr);

private:
    // Core controller
    std::unique_ptr<NFServerController> m_controller;

    // Main interface components
    QWidget* m_centralWidget;
    QSplitter* m_mainSplitter;
    QSplitter* m_topSplitter;

    // Server list
    QGroupBox* m_serverGroup;
    QTableWidget* m_serverTable;
    QVBoxLayout* m_serverLayout;

    // Control panel
    QGroupBox* m_controlGroup;
    QVBoxLayout* m_controlLayout;
    QTabWidget* m_controlTabs;

    // Server operation buttons
    QPushButton* m_startAllBtn;
    QPushButton* m_stopAllBtn;
    QPushButton* m_restartAllBtn;
    QPushButton* m_reloadAllBtn;

    QPushButton* m_startSelectedBtn;
    QPushButton* m_stopSelectedBtn;
    QPushButton* m_restartSelectedBtn;
    QPushButton* m_reloadSelectedBtn;

    QPushButton* m_refreshBtn;
    QPushButton* m_interruptBtn;
    QPushButton* m_selectAllBtn;
    QPushButton* m_selectNoneBtn;
    QPushButton* m_selectRunningBtn;
    QPushButton* m_selectStoppedBtn;

    // Mode operations
    QLineEdit* m_patternEdit;
    QPushButton* m_startPatternBtn;
    QPushButton* m_stopPatternBtn;
    QPushButton* m_restartPatternBtn;
    QPushButton* m_reloadPatternBtn;

    // Log components
    QGroupBox* m_logGroup;
    QTextEdit* m_logEdit;
    QVBoxLayout* m_logLayout;
    QPushButton* m_clearLogBtn;
    QPushButton* m_saveLogBtn;

    // Toolbar
    QToolBar* m_toolBar;
    QLineEdit* m_configPathEdit;
    QPushButton* m_browseConfigBtn;
    QPushButton* m_reloadConfigBtn;
    QCheckBox* m_autoRefreshCheckBox;
    QComboBox* m_refreshIntervalCombo;

    // Menu
    QMenuBar* m_menuBar;
    QMenu* m_fileMenu;
    QMenu* m_serverMenu;
    QMenu* m_viewMenu;
    QMenu* m_helpMenu;

    QAction* m_openConfigAction;
    QAction* m_reloadConfigAction;
    QAction* m_saveSettingsAction;
    QAction* m_exitAction;

    QAction* m_startAllAction;
    QAction* m_stopAllAction;
    QAction* m_restartAllAction;
    QAction* m_reloadAllAction;
    QAction* m_refreshAction;

    QAction* m_clearLogAction;
    QAction* m_saveLogAction;
    QAction* m_aboutAction;
    QAction* m_settingsAction;

    // Status bar
    QStatusBar* m_statusBar;
    QLabel* m_statusLabel;
    QLabel* m_serverCountLabel;
    QLabel* m_runningCountLabel;
    QLabel* m_stoppedCountLabel;
    QProgressBar* m_progressBar;

    // Timer
    QTimer* m_autoRefreshTimer;
    QTimer* m_statusUpdateTimer;

    // Settings
    QString m_currentConfigFile;
    bool m_autoRefreshEnabled;
    int m_refreshInterval; // seconds
    QFont m_logFont;

    // Status
    bool m_isInitialized;
    QMutex m_updateMutex;
    
    // Operation state
    ServerOperation* m_currentOperation;
    bool m_operationInProgress;

    // Temporary storage for operation results (for detailed log records)
    std::vector<std::pair<std::string, bool>> m_lastOperationResults;
    std::vector<std::pair<std::string, std::string>> m_lastOperationServerInfos;
    std::vector<std::pair<std::string, NFServerStatus>> m_lastOperationInitialStates;
    std::vector<std::pair<std::string, std::string>> m_lastOperationPidDetails; // serverId, pid verification details

    // Column definitions
    enum ServerTableColumns
    {
        COL_SELECT = 0,
        COL_PROCESS_ID,
        COL_SERVER_ID,
        COL_SERVER_NAME,
        COL_GAME_NAME,
        COL_STATUS,
        COL_CONFIG_PATH,
        COL_PLUGIN_PATH,
        COL_EXECUTABLE_PATH,
        COL_WORKING_DIR,
        COL_COUNT
    };
};

class ServerOperation : public QThread
{
    Q_OBJECT

public:
    ServerOperation(std::function<bool()> operation, QObject* parent = nullptr)
        : QThread(parent), m_operation(operation), m_result(false), m_interrupted(false)
    {
    }

    bool getResult() const { return m_result; }
    QString getErrorMessage() const { return m_errorMessage; }
    
    void interrupt() 
    { 
        m_interrupted = true; 
        requestInterruption();
    }
    
    bool isInterrupted() const { return m_interrupted; }

protected:
    void run() override
    {
        try
        {
            // 检查是否在开始前就被中断
            if (m_interrupted)
            {
                m_result = false;
                m_errorMessage = "Operation was interrupted before execution";
                return;
            }
            
            m_result = m_operation();
            
            // 如果操作在执行过程中被中断，标记为失败
            if (m_interrupted)
            {
                m_result = false;
                m_errorMessage = "Operation was interrupted during execution";
            }
        }
        catch (const std::exception& e)
        {
            // 记录异常但不能调用UI函数，只能设置结果为失败
            m_result = false;
            m_errorMessage = QString::fromStdString(e.what());
        }
        catch (...)
        {
            m_result = false;
            m_errorMessage = "Unknown exception occurred in server operation";
        }
    }

signals:
    void operationCompleted(bool success);

private:
    std::function<bool()> m_operation;
    bool m_result;
    QString m_errorMessage;
    std::atomic<bool> m_interrupted;
};
