#include "NFServerController.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#endif

NFServerController::NFServerController()
    : m_isMonitoring(false)
      , m_logLevel(1)
{
#ifdef _WIN32
    InitializeCriticalSection(&m_mutex);
#else
    pthread_mutex_init(&m_mutex, nullptr);
#endif
}

NFServerController::~NFServerController()
{
    StopMonitoring();
    StopAllServers();

#ifdef _WIN32
    DeleteCriticalSection(&m_mutex);
#else
    pthread_mutex_destroy(&m_mutex);
#endif
}

void NFServerController::Lock()
{
#ifdef _WIN32
    EnterCriticalSection(&m_mutex);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}

void NFServerController::Unlock()
{
#ifdef _WIN32
    LeaveCriticalSection(&m_mutex);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}

bool NFServerController::Initialize(const std::string& configFile)
{
    m_configFile = configFile;

    // Get base directory
    std::filesystem::path configPath(configFile);
    m_baseDir = configPath.parent_path().string();
    if (m_baseDir.empty())
    {
        m_baseDir = ".";
    }

    LogInfo("Initializing server controller...");
    LogInfo("Config file: " + configFile);
    LogInfo("Base directory: " + m_baseDir);

    if (!LoadServerConfigs(configFile))
    {
        LogError("Failed to load server configurations");
        return false;
    }

    ParseServerDependencies();

    LogInfo("Server controller initialization completed");
    return true;
}

bool NFServerController::LoadServerConfigs(const std::string& configFile)
{
    std::ifstream file(configFile);
    if (!file.is_open())
    {
        LogError("Unable to open config file: " + configFile);
        return false;
    }

    // Simple config file parsing, each line format: ServerName|ServerID|ConfigPath|PluginPath|LuaScriptPath|LogPath|GameName|ExecutablePath|WorkingDir
    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;

        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string token;

        while (std::getline(ss, token, '|'))
        {
            tokens.push_back(token);
        }

        if (tokens.size() >= 9)
        {
            auto config = std::make_shared<NFServerConfig>();
            config->serverName = tokens[0];
            config->serverId = tokens[1];
            config->configPath = tokens[2];
            config->pluginPath = tokens[3];
            config->luaScriptPath = tokens[4];
            config->logPath = tokens[5];
            config->gameName = tokens[6];
            config->executablePath = tokens[7];
            config->workingDir = tokens[8];

            m_serverConfigs[config->serverId] = config;
            LogInfo("Loaded server config: " + config->serverName + " (ID: " + config->serverId + ")");
        }
    }

    file.close();

    // If no config file found, use default configuration
    if (m_serverConfigs.empty())
    {
        CreateDefaultConfigs();
    }

    return !m_serverConfigs.empty();
}

void NFServerController::CreateDefaultConfigs()
{
    LogInfo("Using default server configurations");

    // Default server configuration list
    std::vector<std::tuple<std::string, std::string, int>> defaultServers = {
        {"MasterServer", "1.13.1.1", 1},
        {"RouteAgentServer", "1.13.3.1", 3},
        {"ProxyServer", "1.13.5.1", 5},
        {"StoreServer", "1.13.6.1", 6},
        {"LoginServer", "1.13.7.1", 7},
        {"WorldServer", "1.13.8.1", 8},
        {"LogicServer", "1.13.9.1", 9},
        {"GameServer", "1.13.10.1", 10}
    };

    for (const auto& serverInfo : defaultServers)
    {
        auto config = std::make_shared<NFServerConfig>();
        config->serverName = std::get<0>(serverInfo);
        config->serverId = std::get<1>(serverInfo);
        config->configPath = m_baseDir + "/../Install/Config";
        config->pluginPath = m_baseDir + "/../Install/LieRenPlugin";
        config->luaScriptPath = m_baseDir + "/../Install/LuaScript";
        config->logPath = m_baseDir + "/../Install/logs";
        config->gameName = "LieRen";

#ifdef _WIN32
        config->executablePath = m_baseDir + "/../Install/Bin/NFServerStatic.exe";
#else
        config->executablePath = m_baseDir + "/../Install/Bin/NFServerStatic";
#endif
        config->workingDir = m_baseDir + "/../Install";

        m_serverConfigs[config->serverId] = config;
    }
}

bool NFServerController::StartServer(const std::string& serverId)
{
    Lock();
    auto it = m_serverConfigs.find(serverId);
    if (it == m_serverConfigs.end())
    {
        Unlock();
        LogError("Server configuration not found: " + serverId);
        return false;
    }

    auto config = it->second;
    Unlock();

    if (config->status == NFServerStatus::SERVER_STATUS_RUNNING)
    {
        LogWarning("Server is already running: " + config->serverName + " (ID: " + serverId + ")");
        return true;
    }

    LogInfo("Starting server: " + config->serverName + " (ID: " + serverId + ")");
    config->status = NFServerStatus::SERVER_STATUS_STARTING;

    bool success = StartServerProcess(*config);
    if (success)
    {
        config->status = NFServerStatus::SERVER_STATUS_RUNNING;
        config->lastHeartbeat = std::chrono::system_clock::now();
        LogInfo("Server started successfully: " + config->serverName + " (ID: " + serverId + ")");
    }
    else
    {
        config->status = NFServerStatus::SERVER_STATUS_ERROR;
        LogError("Server startup failed: " + config->serverName + " (ID: " + serverId + ")");
    }

    return success;
}

bool NFServerController::StartServerProcess(NFServerConfig& config)
{
    std::string cmdLine = config.executablePath;
    cmdLine += " --Server=" + config.serverName;
    cmdLine += " --ID=" + config.serverId;
    cmdLine += " --Config=" + config.configPath;
    cmdLine += " --Plugin=" + config.pluginPath;
    cmdLine += " --LuaScript=" + config.luaScriptPath;
    cmdLine += " --LogPath=" + config.logPath;
    cmdLine += " --Game=" + config.gameName;
    cmdLine += " --Start";

#ifdef _WIN32
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Create process using ANSI version
    BOOL result = CreateProcessA(
        NULL, // Application name
        const_cast<char*>(cmdLine.c_str()), // Command line
        NULL, // Process security attributes
        NULL, // Thread security attributes
        FALSE, // Handle inheritance flag
        CREATE_NEW_CONSOLE, // Creation flags
        NULL, // Environment variables
        config.workingDir.c_str(), // Working directory
        &si, // Startup info
        &pi // Process info
    );

    if (result)
    {
        config.processId = pi.dwProcessId;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // Wait for process to start up stably
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return IsProcessRunning(config.processId);
    }
    else
    {
        LogError("CreateProcessA failed, error code: " + std::to_string(GetLastError()));
        return false;
    }
#else
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process
        if (chdir(config.workingDir.c_str()) != 0)
        {
            LogError("Cannot change to working directory: " + config.workingDir);
            exit(1);
        }

        // Split command line arguments
        std::vector<std::string> args;
        std::stringstream ss(cmdLine);
        std::string arg;
        while (ss >> arg)
        {
            args.push_back(arg);
        }

        // Convert to char* array
        std::vector<char*> argv;
        for (const auto& a : args)
        {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);

        // Execute program
        execv(config.executablePath.c_str(), argv.data());

        // If we reach here, execv failed
        LogError("execv failed: " + std::string(strerror(errno)));
        exit(1);
    }
    else if (pid > 0)
    {
        // Parent process
        config.processId = pid;

        // Wait for process to start up stably
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Check if child process is still running
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == 0)
        {
            // Child process is still running
            return true;
        }
        else
        {
            // Child process has exited
            LogError("Child process exited immediately after startup, exit status: " + std::to_string(status));
            return false;
        }
    }
    else
    {
        // Fork failed
        LogError("fork failed: " + std::string(strerror(errno)));
        return false;
    }
#endif
}

bool NFServerController::StopServer(const std::string& serverId)
{
    Lock();
    auto it = m_serverConfigs.find(serverId);
    if (it == m_serverConfigs.end())
    {
        Unlock();
        LogError("Server configuration not found: " + serverId);
        return false;
    }

    auto config = it->second;
    Unlock();

    if (config->status == NFServerStatus::SERVER_STATUS_STOPPED)
    {
        LogWarning("Server is already stopped: " + config->serverName + " (ID: " + serverId + ")");
        return true;
    }

    LogInfo("Stopping server: " + config->serverName + " (ID: " + serverId + ")");
    config->status = NFServerStatus::SERVER_STATUS_STOPPING;

    bool success = StopServerProcess(*config);
    if (success)
    {
        config->status = NFServerStatus::SERVER_STATUS_STOPPED;
        config->processId = 0;
        LogInfo("Server stopped successfully: " + config->serverName + " (ID: " + serverId + ")");
    }
    else
    {
        LogError("Server stop failed: " + config->serverName + " (ID: " + serverId + ")");
    }

    return success;
}

bool NFServerController::StopServerProcess(NFServerConfig& config)
{
    if (config.processId <= 0)
    {
        return true;
    }

    // First try graceful shutdown
    LogInfo("Attempting graceful shutdown of process " + std::to_string(config.processId));

#ifdef _WIN32
    // Send close signal on Windows
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, FALSE, config.processId);
    if (hProcess != NULL)
    {
        // Try to send WM_CLOSE message via PostMessage
        HWND hwnd = NULL;
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
        {
            DWORD processId;
            GetWindowThreadProcessId(hwnd, &processId);
            if (processId == static_cast<DWORD>(lParam))
            {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                return FALSE; // Stop enumeration
            }
            return TRUE; // Continue enumeration
        }, config.processId);

        // Wait for process to exit
        if (WaitForProcessExit(config.processId, 10))
        {
            CloseHandle(hProcess);
            return true;
        }

        // Force terminate
        LogWarning("Graceful shutdown failed, force terminating process");
        BOOL result = TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);

        if (result)
        {
            WaitForProcessExit(config.processId, 5);
            return true;
        }
    }
#else
    // Send SIGTERM signal on Linux
    if (kill(config.processId, SIGTERM) == 0)
    {
        // Wait for process to exit
        if (WaitForProcessExit(config.processId, 10))
        {
            return true;
        }
    }

    // Force terminate
    LogWarning("Graceful shutdown failed, force terminating process");
    if (kill(config.processId, SIGKILL) == 0)
    {
        WaitForProcessExit(config.processId, 5);
        return true;
    }
#endif

    return false;
}

bool NFServerController::RestartServer(const std::string& serverId)
{
    LogInfo("Restarting server: " + serverId);

    if (!StopServer(serverId))
    {
        LogError("Failed to stop server, cannot restart: " + serverId);
        return false;
    }

    // Wait for some time to ensure process completely exits
    std::this_thread::sleep_for(std::chrono::seconds(2));

    return StartServer(serverId);
}

bool NFServerController::ReloadServer(const std::string& serverId)
{
    Lock();
    auto it = m_serverConfigs.find(serverId);
    if (it == m_serverConfigs.end())
    {
        Unlock();
        LogError("Server configuration not found: " + serverId);
        return false;
    }

    auto config = it->second;
    Unlock();

    if (config->status != NFServerStatus::SERVER_STATUS_RUNNING)
    {
        LogError("Server is not running, cannot reload: " + config->serverName + " (ID: " + serverId + ")");
        return false;
    }

    LogInfo("Reloading server configuration: " + config->serverName + " (ID: " + serverId + ")");

    // Send reload signal to server process
#ifdef _WIN32
    // On Windows, can notify server to reload via named pipes or other IPC methods
    // Simplified here, just restart the server
    return RestartServer(serverId);
#else
    // On Linux, send SIGUSR1 signal for configuration reload
    if (kill(config->processId, SIGUSR1) == 0)
    {
        LogInfo("Reload signal sent successfully: " + config->serverName + " (ID: " + serverId + ")");
        return true;
    }
    else
    {
        LogError("Failed to send reload signal: " + config->serverName + " (ID: " + serverId + ")");
        return false;
    }
#endif
}

bool NFServerController::StartAllServers()
{
    LogInfo("Starting all servers...");
    return StartServersInOrder();
}

bool NFServerController::StopAllServers()
{
    LogInfo("Stopping all servers...");
    return StopServersInOrder();
}

bool NFServerController::RestartAllServers()
{
    LogInfo("Restarting all servers...");

    if (!StopAllServers())
    {
        LogError("Failed to stop all servers");
        return false;
    }

    // Wait for all processes to completely exit
    std::this_thread::sleep_for(std::chrono::seconds(3));

    return StartAllServers();
}

void NFServerController::ParseServerDependencies()
{
    // Define server startup dependencies
    // MasterServer must start first
    // RouteAgentServer depends on MasterServer
    // Other servers depend on MasterServer and RouteAgentServer

    m_serverDependencies.clear();

    for (const auto& pair : m_serverConfigs)
    {
        const std::string& serverId = pair.first;
        const auto& config = pair.second;

        if (config->serverName == "MasterServer")
        {
            // MasterServer has no dependencies
            m_serverDependencies[serverId] = {};
        }
        else if (config->serverName == "RouteAgentServer")
        {
            // Find MasterServer's ID
            std::string masterServerId;
            for (const auto& serverPair : m_serverConfigs)
            {
                if (serverPair.second->serverName == "MasterServer")
                {
                    masterServerId = serverPair.first;
                    break;
                }
            }
            // RouteAgentServer depends on MasterServer
            m_serverDependencies[serverId] = {masterServerId};
        }
        else
        {
            // Find MasterServer and RouteAgentServer IDs
            std::string masterServerId;
            std::string routeAgentServerId;
            for (const auto& serverPair : m_serverConfigs)
            {
                if (serverPair.second->serverName == "MasterServer")
                {
                    masterServerId = serverPair.first;
                }
                else if (serverPair.second->serverName == "RouteAgentServer")
                {
                    routeAgentServerId = serverPair.first;
                }
            }
            // Other servers depend on MasterServer and RouteAgentServer
            m_serverDependencies[serverId] = {masterServerId, routeAgentServerId};
        }
    }
}

bool NFServerController::StartServersInOrder()
{
    // Start servers according to dependencies
    std::vector<std::string> serverTypes = {
        "MasterServer",
        "RouteAgentServer",
        "StoreServer",
        "LoginServer",
        "WorldServer",
        "LogicServer",
        "GameServer",
        "ProxyServer"
    };

    for (const std::string& serverType : serverTypes)
    {
        // Find all servers of this type
        for (const auto& pair : m_serverConfigs)
        {
            if (pair.second->serverName == serverType)
            {
                if (!StartServer(pair.first))
                {
                    LogError("Failed to start server: " + pair.second->serverName + " (ID: " + pair.first + ")");
                    return false;
                }

                // Wait for some time after each server starts
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }
    }

    return true;
}

bool NFServerController::StopServersInOrder()
{
    // Stop servers in reverse order of startup
    std::vector<std::string> serverTypes = {
        "ProxyServer",
        "GameServer",
        "LogicServer",
        "WorldServer",
        "LoginServer",
        "StoreServer",
        "RouteAgentServer",
        "MasterServer"
    };

    bool allSuccess = true;

    for (const std::string& serverType : serverTypes)
    {
        // Find all servers of this type
        for (const auto& pair : m_serverConfigs)
        {
            if (pair.second->serverName == serverType)
            {
                if (!StopServer(pair.first))
                {
                    LogError("Failed to stop server: " + pair.second->serverName + " (ID: " + pair.first + ")");
                    allSuccess = false;
                }

                // Wait for some time after each server stops
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }

    return allSuccess;
}

NFServerStatus NFServerController::GetServerStatus(const std::string& serverId)
{
    Lock();
    auto it = m_serverConfigs.find(serverId);
    if (it == m_serverConfigs.end())
    {
        Unlock();
        return NFServerStatus::SERVER_STATUS_UNKNOWN;
    }

    NFServerStatus status = it->second->status;
    Unlock();

    return status;
}

std::map<std::string, NFServerStatus> NFServerController::GetAllServerStatus()
{
    std::map<std::string, NFServerStatus> statusMap;

    Lock();
    for (const auto& pair : m_serverConfigs)
    {
        statusMap[pair.first] = pair.second->status;
    }
    Unlock();

    return statusMap;
}

bool NFServerController::IsServerRunning(const std::string& serverId)
{
    return GetServerStatus(serverId) == NFServerStatus::SERVER_STATUS_RUNNING;
}

std::vector<std::string> NFServerController::GetServerList()
{
    std::vector<std::string> serverList;

    Lock();
    for (const auto& pair : m_serverConfigs)
    {
        serverList.push_back(pair.first); // Now returns server IDs
    }
    Unlock();

    return serverList;
}

void NFServerController::MonitorServers()
{
    if (m_isMonitoring.load())
    {
        LogWarning("Monitoring is already running");
        return;
    }

    LogInfo("Starting server status monitoring...");
    m_isMonitoring = true;

    m_monitorThread = std::make_unique<std::thread>(&NFServerController::MonitorThread, this);
}

void NFServerController::StopMonitoring()
{
    if (!m_isMonitoring.load())
    {
        return;
    }

    LogInfo("Stopping server status monitoring...");
    m_isMonitoring = false;

    if (m_monitorThread && m_monitorThread->joinable())
    {
        m_monitorThread->join();
    }

    m_monitorThread.reset();
}

void NFServerController::MonitorThread()
{
    while (m_isMonitoring.load())
    {
        UpdateServerStatus();

        // Check every 5 seconds
        for (int i = 0; i < 50 && m_isMonitoring.load(); ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void NFServerController::UpdateServerStatus()
{
    Lock();

    for (auto& pair : m_serverConfigs)
    {
        auto& config = pair.second;

        if (config->processId > 0)
        {
            bool isRunning = IsProcessRunning(config->processId);

            if (isRunning)
            {
                if (config->status != NFServerStatus::SERVER_STATUS_RUNNING)
                {
                    config->status = NFServerStatus::SERVER_STATUS_RUNNING;
                    LogInfo("Detected server status changed to running: " + config->serverName + " (ID: " + config->serverId + ")");
                }
                config->lastHeartbeat = std::chrono::system_clock::now();
            }
            else
            {
                if (config->status == NFServerStatus::SERVER_STATUS_RUNNING)
                {
                    config->status = NFServerStatus::SERVER_STATUS_STOPPED;
                    config->processId = 0;
                    LogWarning("Detected server abnormal stop: " + config->serverName + " (ID: " + config->serverId + ")");
                }
            }
        }
        else if (config->status == NFServerStatus::SERVER_STATUS_RUNNING)
        {
            // Process ID is 0 but status shows running, indicating status inconsistency
            config->status = NFServerStatus::SERVER_STATUS_STOPPED;
        }
    }

    Unlock();
}

bool NFServerController::IsProcessRunning(int processId)
{
    if (processId <= 0)
        return false;

#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (hProcess == NULL)
        return false;

    DWORD exitCode;
    BOOL result = GetExitCodeProcess(hProcess, &exitCode);
    CloseHandle(hProcess);

    return result && exitCode == STILL_ACTIVE;
#else
    // Send signal 0 to check if process exists
    return kill(processId, 0) == 0;
#endif
}

std::vector<int> NFServerController::FindProcessByName(const std::string& processName)
{
    std::vector<int> processes;

#ifdef _WIN32
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
        return processes;

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hProcessSnap, &pe32))
    {
        do
        {
            // Convert TCHAR to std::string for comparison
            std::string exeFileName;
#ifdef UNICODE
            // Convert wide string to narrow string
            int len = WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, NULL, 0, NULL, NULL);
            if (len > 0)
            {
                std::vector<char> buffer(len);
                WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, buffer.data(), len, NULL, NULL);
                exeFileName = std::string(buffer.data());
            }
#else
            exeFileName = std::string(pe32.szExeFile);
#endif

            if (processName == exeFileName)
            {
                processes.push_back(pe32.th32ProcessID);
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
#else
    // Find processes through /proc directory on Linux
    DIR* procDir = opendir("/proc");
    if (procDir == nullptr)
        return processes;

    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr)
    {
        if (!isdigit(entry->d_name[0]))
            continue;

        int pid = std::stoi(entry->d_name);
        std::string cmdlinePath = "/proc/" + std::string(entry->d_name) + "/cmdline";

        std::ifstream cmdlineFile(cmdlinePath);
        if (cmdlineFile.is_open())
        {
            std::string cmdline;
            std::getline(cmdlineFile, cmdline);

            if (cmdline.find(processName) != std::string::npos)
            {
                processes.push_back(pid);
            }
        }
    }

    closedir(procDir);
#endif

    return processes;
}

bool NFServerController::KillProcess(int processId)
{
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
    if (hProcess == NULL)
        return false;

    BOOL result = TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
    return result != FALSE;
#else
    return kill(processId, SIGKILL) == 0;
#endif
}

bool NFServerController::WaitForProcessExit(int processId, int timeoutSeconds)
{
    auto startTime = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(timeoutSeconds);

    while (std::chrono::steady_clock::now() - startTime < timeout)
    {
        if (!IsProcessRunning(processId))
        {
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return false;
}

void NFServerController::SetLogLevel(int level)
{
    m_logLevel = level;
}

std::string NFServerController::GetCurrentTimeString()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

void NFServerController::Log(const std::string& message)
{
    std::cout << "[" << GetCurrentTimeString() << "] " << message << std::endl;
}

void NFServerController::LogError(const std::string& message)
{
    if (m_logLevel >= 1)
    {
        std::cout << "[" << GetCurrentTimeString() << "] [ERROR] " << message << std::endl;
    }
}

void NFServerController::LogInfo(const std::string& message)
{
    if (m_logLevel >= 2)
    {
        std::cout << "[" << GetCurrentTimeString() << "] [INFO] " << message << std::endl;
    }
}

void NFServerController::LogWarning(const std::string& message)
{
    if (m_logLevel >= 1)
    {
        std::cout << "[" << GetCurrentTimeString() << "] [WARNING] " << message << std::endl;
    }
}

void NFServerController::PrintHelp()
{
    std::cout << "\n=== NFServerController Help Information ===" << std::endl;
    std::cout << "Usage: NFServerController [options] [command] [server name]" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  -c, --config <file>     Specify config file path" << std::endl;
    std::cout << "  -v, --verbose           Verbose output" << std::endl;
    std::cout << "  -q, --quiet             Quiet mode" << std::endl;
    std::cout << "  -h, --help              Show help information" << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  start [server name]     Start specified server or all servers" << std::endl;
    std::cout << "  stop [server name]      Stop specified server or all servers" << std::endl;
    std::cout << "  restart [server name]   Restart specified server or all servers" << std::endl;
    std::cout << "  reload [server name]    Reload specified server configuration" << std::endl;
    std::cout << "  status [server name]    View server status" << std::endl;
    std::cout << "  monitor                 Start monitoring mode" << std::endl;
    std::cout << "  list                    List all servers" << std::endl;
    std::cout << "\nServer Names:" << std::endl;
    std::cout << "  MasterServer            Master server" << std::endl;
    std::cout << "  RouteAgentServer        Route agent server" << std::endl;
    std::cout << "  ProxyServer             Proxy server" << std::endl;
    std::cout << "  StoreServer             Data store server" << std::endl;
    std::cout << "  LoginServer             Login server" << std::endl;
    std::cout << "  WorldServer             World server" << std::endl;
    std::cout << "  LogicServer             Logic server" << std::endl;
    std::cout << "  GameServer              Game server" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  NFServerController start                    # Start all servers" << std::endl;
    std::cout << "  NFServerController start MasterServer      # Start master server" << std::endl;
    std::cout << "  NFServerController stop                     # Stop all servers" << std::endl;
    std::cout << "  NFServerController status                   # View all server status" << std::endl;
    std::cout << "  NFServerController monitor                  # Start monitoring mode" << std::endl;
    std::cout << std::endl;
}
