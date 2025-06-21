#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

enum class NFServerStatus
{
    SERVER_STATUS_UNKNOWN = 0,
    SERVER_STATUS_STOPPED = 1,
    SERVER_STATUS_STARTING = 2,
    SERVER_STATUS_RUNNING = 3,
    SERVER_STATUS_STOPPING = 4,
    SERVER_STATUS_ERROR = 5,
};

// Server configuration structure
struct NFServerConfig
{
    std::string serverName; // Server name, e.g. "MasterServer"
    std::string serverId; // Server ID, e.g. "1.13.1.1"
    std::string configPath; // Configuration file path
    std::string pluginPath; // Plugin path
    std::string luaScriptPath; // Lua script path
    std::string logPath; // Log path
    std::string gameName; // Game name
    std::string executablePath; // Executable file path
    std::string workingDir; // Working directory
    int processId; // Process ID
    NFServerStatus status; // Server status
    std::chrono::system_clock::time_point lastHeartbeat; // Last heartbeat time

    NFServerConfig()
    {
        processId = 0;
        status = NFServerStatus::SERVER_STATUS_STOPPED;
        lastHeartbeat = std::chrono::system_clock::now();
    }
};

// Server controller class
class NFServerController
{
public:
    NFServerController();
    ~NFServerController();

    // Initialize controller
    bool Initialize(const std::string& configFile);

    // Start specified server
    bool StartServer(const std::string& serverName);

    // Stop specified server
    bool StopServer(const std::string& serverName);

    // Restart specified server
    bool RestartServer(const std::string& serverName);

    // Reload specified server configuration
    bool ReloadServer(const std::string& serverName);

    // Start all servers
    bool StartAllServers();

    // Stop all servers
    bool StopAllServers();

    // Restart all servers
    bool RestartAllServers();

    // Get server status
    NFServerStatus GetServerStatus(const std::string& serverName);

    // Get all server status
    std::map<std::string, NFServerStatus> GetAllServerStatus();

    // Monitor server status (blocking call)
    void MonitorServers();

    // Stop monitoring
    void StopMonitoring();

    // Check if server is running
    bool IsServerRunning(const std::string& serverName);

    // Get server configuration list
    std::vector<std::string> GetServerList();

    // Set log level
    void SetLogLevel(int level);

    // Print help information
    void PrintHelp();

private:
    // Load server configurations
    bool LoadServerConfigs(const std::string& configFile);

    // Create default configurations
    void CreateDefaultConfigs();

    // Start single server process
    bool StartServerProcess(NFServerConfig& config);

    // Stop single server process
    bool StopServerProcess(NFServerConfig& config);

    // Check if process exists
    bool IsProcessRunning(int processId);

    // Find process ID by process name
    std::vector<int> FindProcessByName(const std::string& processName);

    // Kill process
    bool KillProcess(int processId);

    // Wait for process exit
    bool WaitForProcessExit(int processId, int timeoutSeconds = 10);

    // Update server status
    void UpdateServerStatus();

    // Monitor thread function
    void MonitorThread();

    // Log output
    void Log(const std::string& message);
    void LogError(const std::string& message);
    void LogInfo(const std::string& message);
    void LogWarning(const std::string& message);

    // Get current time string
    std::string GetCurrentTimeString();

    // Parse server dependencies
    void ParseServerDependencies();

    // Start servers in dependency order
    bool StartServersInOrder();

    // Stop servers in dependency order
    bool StopServersInOrder();

private:
    std::string m_baseDir;
    std::string m_configFile;
    std::map<std::string, std::shared_ptr<NFServerConfig>> m_serverConfigs;
    std::map<std::string, std::vector<std::string>> m_serverDependencies;
    std::atomic<bool> m_isMonitoring;
    std::unique_ptr<std::thread> m_monitorThread;
    int m_logLevel;

    // Platform-specific mutex
#ifdef _WIN32
    CRITICAL_SECTION m_mutex;
#else
    pthread_mutex_t m_mutex;
#endif

    void Lock();
    void Unlock();
};
