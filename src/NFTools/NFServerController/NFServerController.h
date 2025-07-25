#pragma once

#include "NFComm/NFCore/NFPlatform.h"

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
    std::string m_serverName; // Server name, e.g. "MasterServer"
    std::string m_serverId; // Server ID, e.g. "1.13.1.1"
    std::string m_configPath; // Configuration file path
    std::string m_pluginPath; // Plugin path
    std::string m_luaScriptPath; // Lua script path
    std::string m_logPath; // Log path
    std::string m_gameName; // Game name
    std::string m_executablePath; // Executable file path
    std::string m_workingDir; // Working directory
    int m_processId; // Process ID
    NFServerStatus m_status; // Server status
    std::chrono::system_clock::time_point m_lastHeartbeat; // Last heartbeat time

    NFServerConfig()
    {
        m_processId = 0;
        m_status = NFServerStatus::SERVER_STATUS_STOPPED;
        m_lastHeartbeat = std::chrono::system_clock::now();
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
    bool StartServer(const std::string& serverId);

    // Stop specified server
    bool StopServer(const std::string& serverId);

    // Restart specified server
    bool RestartServer(const std::string& serverId);

    // Reload specified server configuration
    bool ReloadServer(const std::string& serverId);

    bool ClearShmServer(const std::string& serverId);

    // Start all servers
    bool StartAllServers();

    // Stop all servers
    bool StopAllServers();

    // Restart all servers
    bool RestartAllServers();

    bool ClearShmAllServers();

    // Get server status
    NFServerStatus GetServerStatus(const std::string& serverId);

    // Get all server status
    std::map<std::string, NFServerStatus> GetAllServerStatus();

    // Get all server detailed information
    std::map<std::string, std::shared_ptr<NFServerConfig>> GetAllServerConfigs();

    // Get server configuration order
    const std::vector<std::string>& GetServerConfigOrder() const;

    // Monitor server status (blocking call)
    void MonitorServers();

    // Stop monitoring
    void StopMonitoring();

    // Check if server is running
    bool IsServerRunning(const std::string& serverName);


    // Get servers matching wildcard pattern (e.g., "*.*.*.*", "*.*.5.*")
    std::vector<std::string> GetMatchingServers(const std::string& pattern);

    // Start servers matching pattern
    bool StartServersByPattern(const std::string& pattern);

    // Stop servers matching pattern
    bool StopServersByPattern(const std::string& pattern);

    // Restart servers matching pattern
    bool RestartServersByPattern(const std::string& pattern);

    // Reload all servers
    bool ReloadAllServers();

    // Reload servers matching pattern
    bool ReloadServersByPattern(const std::string& pattern);

    // Set log level
    void SetLogLevel(int level);

    // Print help information
    void PrintHelp();

    // Update server status (moved to public for Qt interface access)
    void UpdateServerStatus();

    // PID file reading functions (moved to public for Qt interface access)
    std::string GetPidFileName(const NFServerConfig& config);
    int ReadPidFile(const NFServerConfig& config);
    bool VerifyProcessByPidFile(NFServerConfig& config);

    // Enhanced process verification functions (moved to public for Qt interface access)
    bool IsProcessRunning(int processId);
    bool IsProcessRunningAndValid(int processId, const NFServerConfig& config);

    // Shared memory clearing functions
    bool ClearShmServersByPattern(const std::string& pattern);
    bool ExecuteShmCleanScript(const std::string& username = "");

    // Clear shared memory by serverId using NFServerIDUtil
    bool ClearShmByServerId(const std::string& serverId);
    bool ClearShmByServerIds(const std::vector<std::string>& serverIds);

    // Query shared memory information without removing
    std::vector<std::pair<std::string, std::string>> QuerySharedMemoryInfo(const std::string& username = "");

    // Test function for shared memory key generation
    void TestServerShmKeys(const std::string& serverId);

private:
    // Load server configurations
    bool LoadServerConfigs(const std::string& configFile);

    // Create default configurations
    void CreateDefaultConfigs();

    // Start single server process
    bool StartServerProcess(NFServerConfig& config);

    // Restart single server process
    bool RestartServerProcess(NFServerConfig& config);

    // Stop single server process
    bool StopServerProcess(NFServerConfig& config);

    // Find process ID by process name
    std::vector<int> FindProcessByName(const std::string& processName);

    // Kill process
    bool KillProcess(int processId);

    // Wait for process exit
    bool WaitForProcessExit(int processId, int timeoutSeconds = 10);

    // Monitor thread function
    void MonitorThread();

    // Log output
    void Log(const std::string& message);
    void LogError(const std::string& message);
    void LogInfo(const std::string& message);
    void LogWarning(const std::string& message);

    // Get current time string
    std::string GetCurrentTimeString();

    // Start servers in configuration order
    bool StartServersInOrder();

    // Stop servers in reverse configuration order
    bool StopServersInOrder();

    // Check if server ID matches wildcard pattern
    bool MatchesPattern(const std::string& serverId, const std::string& pattern);

    // Enhanced process verification functions
    std::string GetProcessExecutablePath(int processId);
    std::string GetProcessCommandLine(int processId);
    bool VerifyProcessIdentity(int processId, const NFServerConfig& config);

    // Test function for process verification (development/debugging)
    void TestProcessVerification();

    // Helper function to extract file name from path (cross-platform)
    std::string ExtractFileName(const std::string& filePath);

    // Smart comparison of executable names (handles .exe extension differences)
    bool CompareExecutableNames(const std::string& actualName, const std::string& expectedName);

    // Shared memory helper functions
    bool RemoveSharedMemorySegment(const std::string& shmid);
    std::vector<std::pair<std::string, std::string>> GetSharedMemorySegments(const std::string& username);

    // Server-specific shared memory helper functions
    int GetShmIdByKey(uint32_t key);
    std::vector<uint32_t> GetServerShmKeys(const std::string& serverId);
    bool RemoveSharedMemoryByKey(uint32_t key);

private:
    std::string m_baseDir;
    std::string m_configFile;
    std::map<std::string, std::shared_ptr<NFServerConfig>> m_serverConfigs;
    std::vector<std::string> m_serverConfigOrder; // Maintain config file loading order
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
