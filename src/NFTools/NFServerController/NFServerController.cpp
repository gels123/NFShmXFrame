#include "NFServerController.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <NFComm/NFCore/NFCommon.h>
#include <NFComm/NFCore/NFServerIDUtil.h>

#include "NFComm/NFCore/NFFileUtility.h"

#if NF_PLATFORM == NF_PLATFORM_WIN

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

NFServerController::NFServerController()
    : m_isMonitoring(false)
      , m_logLevel(1)
{
#if NF_PLATFORM == NF_PLATFORM_WIN
    InitializeCriticalSection(&m_mutex);
#else
    pthread_mutex_init(&m_mutex, nullptr);
#endif
}

NFServerController::~NFServerController()
{
    StopMonitoring();

#if NF_PLATFORM == NF_PLATFORM_WIN
    DeleteCriticalSection(&m_mutex);
#else
    pthread_mutex_destroy(&m_mutex);
#endif
}

void NFServerController::Lock()
{
#if NF_PLATFORM == NF_PLATFORM_WIN
    EnterCriticalSection(&m_mutex);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}

void NFServerController::Unlock()
{
#if NF_PLATFORM == NF_PLATFORM_WIN
    LeaveCriticalSection(&m_mutex);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}

bool NFServerController::Initialize(const std::string& configFile)
{
    m_configFile = configFile;

    // Get base directory
    m_baseDir = NFFileUtility::GetParentDir(configFile);
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

    LogInfo("Server controller initialization completed");
    return true;
}

bool NFServerController::LoadServerConfigs(const std::string& configFile)
{
    // Clear existing configurations
    m_serverConfigs.clear();
    m_serverConfigOrder.clear();

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
            config->m_serverName = tokens[0];
            config->m_serverId = tokens[1];
            config->m_configPath = tokens[2];
            config->m_pluginPath = tokens[3];
            config->m_luaScriptPath = tokens[4];
            config->m_logPath = tokens[5];
            config->m_gameName = tokens[6];
            config->m_executablePath = tokens[7];
            config->m_workingDir = tokens[8];

            // Check for duplicate server IDs
            if (m_serverConfigs.find(config->m_serverId) != m_serverConfigs.end())
            {
                LogWarning("Duplicate server ID found, overriding: " + config->m_serverId);
            }

            m_serverConfigs[config->m_serverId] = config;
            m_serverConfigOrder.push_back(config->m_serverId); // Maintain loading order
            LogInfo("Loaded server config: " + config->m_serverName + " (ID: " + config->m_serverId + " BusId:" + NFCommon::tostr(NFServerIDUtil::GetBusID(config->m_serverId)) + " shmKeyId:" + NFCommon::tostr(NFServerIDUtil::GetShmObjKey(config->m_serverId)) + ")");
        }
    }

    file.close();

    // If no config file found, use default configuration
    if (m_serverConfigs.empty())
    {
        CreateDefaultConfigs();
    }

    // After loading all configs, read PID files and update server status
    LogInfo("Reading PID files and updating server status...");
    for (auto& pair : m_serverConfigs)
    {
        auto& config = pair.second;

        // Read PID from file
        int pidFromFile = ReadPidFile(*config);
        if (pidFromFile > 0)
        {
            // Verify if the process is actually running and is the correct process
            if (IsProcessRunningAndValid(pidFromFile, *config))
            {
                config->m_processId = pidFromFile;
                config->m_status = NFServerStatus::SERVER_STATUS_RUNNING;
                LogInfo("Found verified running server from PID file: " + config->m_serverId + " (PID: " + std::to_string(pidFromFile) + ")");
            }
            else
            {
                // Process not running or not the correct process
                config->m_processId = 0;
                config->m_status = NFServerStatus::SERVER_STATUS_STOPPED;
                LogWarning("PID file exists but process not running or identity mismatch for: " + config->m_serverId + " (PID: " + std::to_string(pidFromFile) + ")");
            }
        }
        else
        {
            // No PID file found
            config->m_processId = 0;
            config->m_status = NFServerStatus::SERVER_STATUS_STOPPED;
            LogInfo("No PID file found for: " + config->m_serverId + " (status: stopped)");
        }
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
        config->m_serverName = std::get<0>(serverInfo);
        config->m_serverId = std::get<1>(serverInfo);
        config->m_configPath = m_baseDir + "../../Config";
        config->m_pluginPath = m_baseDir + "../../Plugin";
        config->m_luaScriptPath = m_baseDir + "../../LuaScript";
        config->m_logPath = m_baseDir + "../../Bin/Debug/logs";
        config->m_gameName = "LieRen";

#if NF_PLATFORM == NF_PLATFORM_WIN
        config->m_executablePath = m_baseDir + "/../Install/Bin/NFServerStatic.exe";
#else
        config->m_executablePath = m_baseDir + "/../Install/Bin/NFServerStatic";
#endif
        config->m_workingDir = m_baseDir + "/../Install";

        m_serverConfigs[config->m_serverId] = config;
        m_serverConfigOrder.push_back(config->m_serverId); // Maintain loading order
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

    // Check if process is already running by reading PID file and verifying process
    bool isActuallyRunning = VerifyProcessByPidFile(*config);

    if (isActuallyRunning || config->m_status == NFServerStatus::SERVER_STATUS_RUNNING)
    {
        LogError("Server is already running, please stop it first: " + config->m_serverName + " (ID: " + serverId + ")");
        if (isActuallyRunning && config->m_status != NFServerStatus::SERVER_STATUS_RUNNING)
        {
            // Update status if process is running but status shows otherwise
            config->m_status = NFServerStatus::SERVER_STATUS_RUNNING;
            LogInfo("Updated server status to running based on actual process state: " + serverId);
        }
        return false;
    }

    LogInfo("Starting server: " + config->m_serverName + " (ID: " + serverId + ")");
    config->m_status = NFServerStatus::SERVER_STATUS_STARTING;

    bool success = StartServerProcess(*config);
    if (success)
    {
        config->m_status = NFServerStatus::SERVER_STATUS_RUNNING;
        config->m_lastHeartbeat = std::chrono::system_clock::now();
        LogInfo("Server started successfully: " + config->m_serverName + " (ID: " + serverId + ")");
    }
    else
    {
        config->m_status = NFServerStatus::SERVER_STATUS_ERROR;
        LogError("Server startup failed: " + config->m_serverName + " (ID: " + serverId + ")");
    }

    return success;
}

bool NFServerController::StartServerProcess(NFServerConfig& config)
{
    std::string executablePath = NFFileUtility::GetAbsolutePathName(config.m_executablePath);

#if NF_PLATFORM == NF_PLATFORM_WIN
    // On Windows, ensure executable has .exe extension
    if (executablePath.length() > 4 && executablePath.substr(executablePath.length() - 4) != ".exe")
    {
        executablePath += ".exe";
    }

#endif

    std::string cmdLine = "\"" + executablePath + "\"";
    cmdLine += " --Server=" + config.m_serverName;
    cmdLine += " --ID=" + config.m_serverId;
    cmdLine += " --Config=" + config.m_configPath;
    cmdLine += " --Plugin=" + config.m_pluginPath;
    cmdLine += " --LuaScript=" + config.m_luaScriptPath;
    cmdLine += " --Log=" + config.m_logPath;
    cmdLine += " --Game=" + config.m_gameName;
    cmdLine += " --Start";
    cmdLine += " -d";

    LogInfo("Start command line: " + cmdLine);
    LogInfo("Working directory: " + config.m_workingDir);

#if NF_PLATFORM == NF_PLATFORM_WIN
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Create process using ANSI version
    BOOL result = CreateProcessA(
        executablePath.c_str(), // Application name (use full path)
        const_cast<char*>(cmdLine.c_str()), // Command line
        NULL, // Process security attributes
        NULL, // Thread security attributes
        FALSE, // Handle inheritance flag
        CREATE_NEW_CONSOLE, // Creation flags
        NULL, // Environment variables
        config.m_workingDir.c_str(), // Working directory
        &si, // Startup info
        &pi // Process info
    );

    if (result)
    {
        config.m_processId = pi.dwProcessId;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // Wait for process to start up stably
        std::this_thread::sleep_for(std::chrono::seconds(2));

        return IsProcessRunning(config.m_processId);
    }
    else
    {
        DWORD errorCode = GetLastError();
        LogError("CreateProcessA failed, error code: " + std::to_string(errorCode));
        LogError("Executable path: " + executablePath);
        LogError("Command line: " + cmdLine);
        LogError("Working directory: " + config.m_workingDir);

        // Check if file exists
        if (NFFileUtility::IsFileExist(executablePath))
        {
            LogError("Executable file exists but cannot be started");
        }
        else
        {
            LogError("Executable file does not exist: " + executablePath);
        }

        return false;
    }
#else
    // Linux-specific checks before forking
    // Check if executable file exists
    if (!NFFileUtility::IsFileExist(executablePath))
    {
        LogError("Executable file does not exist: " + executablePath);
        return false;
    }

    // Check if executable file has execute permission
    if (access(executablePath.c_str(), X_OK) != 0)
    {
        LogError("Executable file lacks execute permission: " + executablePath + " (error: " + strerror(errno) + ")");
        LogError("Try: chmod +x " + executablePath);
        return false;
    }

    // Check if working directory exists
    if (!NFFileUtility::IsDir(config.m_workingDir))
    {
        LogError("Working directory does not exist: " + config.m_workingDir);
        return false;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process
        if (chdir(config.m_workingDir.c_str()) != 0)
        {
            LogError("Cannot change to working directory: " + config.m_workingDir);
            exit(1);
        }

        // Build arguments array for execv
        std::vector<std::string> args;
        args.push_back(executablePath); // argv[0] should be the program name
        args.push_back("--Server=" + config.m_serverName);
        args.push_back("--ID=" + config.m_serverId);
        args.push_back("--Config=" + config.m_configPath);
        args.push_back("--Plugin=" + config.m_pluginPath);
        args.push_back("--LuaScript=" + config.m_luaScriptPath);
        args.push_back("--Log=" + config.m_logPath);
        args.push_back("--Game=" + config.m_gameName);
        args.push_back("--Start");
        args.push_back(" -d");

        // Convert to char* array
        std::vector<char*> argv;
        for (const auto& a : args)
        {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);

        // Execute program
        execv(executablePath.c_str(), argv.data());

        // If we reach here, execv failed
        LogError("execv failed: " + std::string(strerror(errno)));
        exit(1);
    }
    else if (pid > 0)
    {
        // Parent process
        config.m_processId = pid;

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

bool NFServerController::RestartServerProcess(NFServerConfig& config)
{
    std::string executablePath = NFFileUtility::GetAbsolutePathName(config.m_executablePath);

#if NF_PLATFORM == NF_PLATFORM_WIN
    // On Windows, ensure executable has .exe extension
    if (executablePath.length() > 4 && executablePath.substr(executablePath.length() - 4) != ".exe")
    {
        executablePath += ".exe";
    }
#endif

    std::string cmdLine = "\"" + executablePath + "\"";
    cmdLine += " --Server=" + config.m_serverName;
    cmdLine += " --ID=" + config.m_serverId;
    cmdLine += " --Config=" + config.m_configPath;
    cmdLine += " --Plugin=" + config.m_pluginPath;
    cmdLine += " --LuaScript=" + config.m_luaScriptPath;
    cmdLine += " --Log=" + config.m_logPath;
    cmdLine += " --Game=" + config.m_gameName;
    cmdLine += " --Restart"; // Use --Restart instead of --Start
    cmdLine += " -d";

    LogInfo("Restart command line: " + cmdLine);
    LogInfo("Working directory: " + config.m_workingDir);

#if NF_PLATFORM == NF_PLATFORM_WIN
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Create process using ANSI version
    BOOL result = CreateProcessA(
        executablePath.c_str(), // Application name (use full path)
        const_cast<char*>(cmdLine.c_str()), // Command line
        NULL, // Process security attributes
        NULL, // Thread security attributes
        FALSE, // Handle inheritance flag
        CREATE_NEW_CONSOLE, // Creation flags
        NULL, // Environment variables
        config.m_workingDir.c_str(), // Working directory
        &si, // Startup info
        &pi // Process info
    );

    if (result)
    {
        config.m_processId = pi.dwProcessId;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // Wait for process to restart stably
        std::this_thread::sleep_for(std::chrono::seconds(3));
        return IsProcessRunning(config.m_processId);
    }
    else
    {
        DWORD errorCode = GetLastError();
        LogError("CreateProcessA failed for restart, error code: " + std::to_string(errorCode));
        LogError("Executable path: " + executablePath);
        LogError("Command line: " + cmdLine);
        LogError("Working directory: " + config.m_workingDir);

        // Check if file exists
        if (NFFileUtility::IsFileExist(executablePath))
        {
            LogError("Executable file exists but cannot be started");
        }
        else
        {
            LogError("Executable file does not exist: " + executablePath);
        }

        return false;
    }
#else
    // Linux-specific checks before forking
    // Check if executable file exists
    if (!NFFileUtility::IsFileExist(executablePath))
    {
        LogError("Executable file does not exist: " + executablePath);
        return false;
    }

    // Check if executable file has execute permission
    if (access(executablePath.c_str(), X_OK) != 0)
    {
        LogError("Executable file lacks execute permission: " + executablePath + " (error: " + strerror(errno) + ")");
        LogError("Try: chmod +x " + executablePath);
        return false;
    }

    // Check if working directory exists
    if (!NFFileUtility::IsDir(config.m_workingDir))
    {
        LogError("Working directory does not exist: " + config.m_workingDir);
        return false;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process
        if (chdir(config.m_workingDir.c_str()) != 0)
        {
            LogError("Cannot change to working directory: " + config.m_workingDir);
            exit(1);
        }

        // Build arguments array for execv
        std::vector<std::string> args;
        args.push_back(executablePath); // argv[0] should be the program name
        args.push_back("--Server=" + config.m_serverName);
        args.push_back("--ID=" + config.m_serverId);
        args.push_back("--Config=" + config.m_configPath);
        args.push_back("--Plugin=" + config.m_pluginPath);
        args.push_back("--LuaScript=" + config.m_luaScriptPath);
        args.push_back("--Log=" + config.m_logPath);
        args.push_back("--Game=" + config.m_gameName);
        args.push_back("--Restart");
        args.push_back("-d");

        // Convert to char* array
        std::vector<char*> argv;
        for (const auto& a : args)
        {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);

        // Execute program
        execv(executablePath.c_str(), argv.data());

        // If we reach here, execv failed
        LogError("execv failed for restart: " + std::string(strerror(errno)));
        exit(1);
    }
    else if (pid > 0)
    {
        // Parent process - wait for restart command to complete
        int status;
        waitpid(pid, &status, 0);

        LogInfo("Restart command executed, waiting for server to restart...");

        // Wait for server to restart and read new PID from file
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Try to read new PID from PID file
        int newPid = ReadPidFile(config);
        if (newPid > 0 && IsProcessRunningAndValid(newPid, config))
        {
            config.m_processId = newPid;
            return true;
        }
        else
        {
            LogError("Server restart failed - no valid process found after restart");
            config.m_processId = 0;
            return false;
        }
    }
    else
    {
        // Fork failed
        LogError("fork failed for restart: " + std::string(strerror(errno)));
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

    if (config->m_status == NFServerStatus::SERVER_STATUS_STOPPED)
    {
        LogWarning("Server is already stopped: " + config->m_serverName + " (ID: " + serverId + ")");
        return true;
    }

    LogInfo("Stopping server: " + config->m_serverName + " (ID: " + serverId + ")");
    config->m_status = NFServerStatus::SERVER_STATUS_STOPPING;

    bool success = StopServerProcess(*config);
    if (success)
    {
        config->m_status = NFServerStatus::SERVER_STATUS_STOPPED;
        config->m_processId = 0;
        LogInfo("Server stopped successfully: " + config->m_serverName + " (ID: " + serverId + ")");
    }
    else
    {
        LogError("Server stop failed: " + config->m_serverName + " (ID: " + serverId + ")");
    }

    return success;
}

bool NFServerController::StopServerProcess(NFServerConfig& config)
{
    if (config.m_processId <= 0)
    {
        LogInfo("No valid process ID found for " + config.m_serverId);
        return true;
    }

    // Print current process status
    LogInfo("Current process status for " + config.m_serverId + ":");
    LogInfo("  Server Name: " + config.m_serverName);
    LogInfo("  Server ID: " + config.m_serverId);
    LogInfo("  Process ID: " + std::to_string(config.m_processId));
    LogInfo("  Working Directory: " + config.m_workingDir);

    // Check if process exists
    if (!IsProcessRunning(config.m_processId))
    {
        LogWarning("Process " + std::to_string(config.m_processId) + " is not running");
        config.m_processId = 0;
        config.m_status = NFServerStatus::SERVER_STATUS_STOPPED;
        return true;
    }

    LogInfo("Process " + std::to_string(config.m_processId) + " is currently running");
    LogInfo("Attempting graceful shutdown using --stop parameter");

    // Use --stop parameter to gracefully shutdown the server
    std::string executablePath = NFFileUtility::GetAbsolutePathName(config.m_executablePath);

#if NF_PLATFORM == NF_PLATFORM_WIN
    // On Windows, ensure executable has .exe extension
    if (executablePath.length() > 4 && executablePath.substr(executablePath.length() - 4) != ".exe")
    {
        executablePath += ".exe";
    }

#endif

    std::string cmdLine = "\"" + executablePath + "\"";
    cmdLine += " --Server=" + config.m_serverName;
    cmdLine += " --ID=" + config.m_serverId;
    cmdLine += " --Config=" + config.m_configPath;
    cmdLine += " --Plugin=" + config.m_pluginPath;
    cmdLine += " --LuaScript=" + config.m_luaScriptPath;
    cmdLine += " --Log=" + config.m_logPath;
    cmdLine += " --Game=" + config.m_gameName;
    cmdLine += " --Stop"; // Use --Stop instead of --Start
    cmdLine += " -d"; // Use --Stop instead of --Start

    LogInfo("Stop command line: " + cmdLine);
    LogInfo("Working directory: " + config.m_workingDir);

#if NF_PLATFORM == NF_PLATFORM_WIN
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Create process using ANSI version
    BOOL result = CreateProcessA(
        executablePath.c_str(), // Application name (use full path)
        const_cast<char*>(cmdLine.c_str()), // Command line
        NULL, // Process security attributes
        NULL, // Thread security attributes
        FALSE, // Handle inheritance flag
        CREATE_NEW_CONSOLE, // Creation flags
        NULL, // Environment variables
        config.m_workingDir.c_str(), // Working directory
        &si, // Startup info
        &pi // Process info
    );

    if (result)
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        LogInfo("Stop command executed successfully, waiting for server to shutdown...");

        // Wait for the original process to exit
        if (WaitForProcessExit(config.m_processId, 30))
        {
            LogInfo("Server shutdown completed successfully");
            config.m_processId = 0;
            config.m_status = NFServerStatus::SERVER_STATUS_STOPPED;
            return true;
        }
        else
        {
            LogWarning("Server did not shutdown within timeout, process may still be running");
            return false;
        }
    }
    else
    {
        DWORD errorCode = GetLastError();
        LogError("Failed to execute stop command, error code: " + std::to_string(errorCode));
        return false;
    }
#else
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process
        if (chdir(config.m_workingDir.c_str()) != 0)
        {
            LogError("Cannot change to working directory: " + config.m_workingDir);
            exit(1);
        }

        // Build arguments array for execv
        std::vector<std::string> args;
        args.push_back(executablePath); // argv[0] should be the program name
        args.push_back("--Server=" + config.m_serverName);
        args.push_back("--ID=" + config.m_serverId);
        args.push_back("--Config=" + config.m_configPath);
        args.push_back("--Plugin=" + config.m_pluginPath);
        args.push_back("--LuaScript=" + config.m_luaScriptPath);
        args.push_back("--Log=" + config.m_logPath);
        args.push_back("--Game=" + config.m_gameName);
        args.push_back("--Stop");
        args.push_back("-d");

        // Convert to char* array
        std::vector<char*> argv;
        for (const auto& a : args)
        {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);

        // Execute stop command
        execv(executablePath.c_str(), argv.data());

        // If we reach here, execv failed
        LogError("execv failed for stop command: " + std::string(strerror(errno)));
        exit(1);
    }
    else if (pid > 0)
    {
        // Parent process - wait for stop command to complete
        int status;
        waitpid(pid, &status, 0);

        LogInfo("Stop command executed, waiting for server to shutdown...");

        // Wait for the original process to exit
        if (WaitForProcessExit(config.m_processId, 30))
        {
            LogInfo("Server shutdown completed successfully");
            config.m_processId = 0;
            config.m_status = NFServerStatus::SERVER_STATUS_STOPPED;
            return true;
        }
        else
        {
            LogWarning("Server did not shutdown within timeout, process may still be running");
            return false;
        }
    }
    else
    {
        LogError("fork failed for stop command: " + std::string(strerror(errno)));
        return false;
    }
#endif
}

bool NFServerController::RestartServer(const std::string& serverId)
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

    LogInfo("Restarting server: " + config->m_serverName + " (ID: " + serverId + ")");
    config->m_status = NFServerStatus::SERVER_STATUS_STARTING;

    bool success = RestartServerProcess(*config);
    if (success)
    {
        config->m_status = NFServerStatus::SERVER_STATUS_RUNNING;
        config->m_lastHeartbeat = std::chrono::system_clock::now();
        LogInfo("Server restarted successfully: " + config->m_serverName + " (ID: " + serverId + ")");
    }
    else
    {
        config->m_status = NFServerStatus::SERVER_STATUS_ERROR;
        LogError("Server restart failed: " + config->m_serverName + " (ID: " + serverId + ")");
    }

    return success;
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

    // Check if process is actually running by reading PID file and verifying process
    bool isActuallyRunning = VerifyProcessByPidFile(*config);

    if (!isActuallyRunning)
    {
        LogError("Server is not running, cannot reload: " + config->m_serverName + " (ID: " + serverId + ")");
        if (config->m_status == NFServerStatus::SERVER_STATUS_RUNNING)
        {
            // Update status if process is not running but status shows running
            config->m_status = NFServerStatus::SERVER_STATUS_STOPPED;
            LogInfo("Updated server status to stopped based on actual process state: " + serverId);
        }
        return false;
    }

    // Ensure status is consistent
    if (config->m_status != NFServerStatus::SERVER_STATUS_RUNNING)
    {
        config->m_status = NFServerStatus::SERVER_STATUS_RUNNING;
        LogInfo("Updated server status to running based on actual process state: " + serverId);
    }

    LogInfo("Reloading server configuration: " + config->m_serverName + " (ID: " + serverId + ")");

    // Use --reload parameter to reload server configuration
    std::string executablePath = NFFileUtility::GetAbsolutePathName(config->m_executablePath);

#if NF_PLATFORM == NF_PLATFORM_WIN
    // On Windows, ensure executable has .exe extension
    if (executablePath.length() > 4 && executablePath.substr(executablePath.length() - 4) != ".exe")
    {
        executablePath += ".exe";
    }

#endif

    std::string cmdLine = "\"" + executablePath + "\"";
    cmdLine += " --Server=" + config->m_serverName;
    cmdLine += " --ID=" + config->m_serverId;
    cmdLine += " --Config=" + config->m_configPath;
    cmdLine += " --Plugin=" + config->m_pluginPath;
    cmdLine += " --LuaScript=" + config->m_luaScriptPath;
    cmdLine += " --Log=" + config->m_logPath;
    cmdLine += " --Game=" + config->m_gameName;
    cmdLine += " --Reload"; // Use --Reload parameter
    cmdLine += " -d"; // Use --Reload parameter

    LogInfo("Reload command line: " + cmdLine);
    LogInfo("Working directory: " + config->m_workingDir);

#if NF_PLATFORM == NF_PLATFORM_WIN
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Create process using ANSI version
    BOOL result = CreateProcessA(
        executablePath.c_str(), // Application name (use full path)
        const_cast<char*>(cmdLine.c_str()), // Command line
        NULL, // Process security attributes
        NULL, // Thread security attributes
        FALSE, // Handle inheritance flag
        CREATE_NEW_CONSOLE, // Creation flags
        NULL, // Environment variables
        config->m_workingDir.c_str(), // Working directory
        &si, // Startup info
        &pi // Process info
    );

    if (result)
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        LogInfo("Reload command executed successfully");
        return true;
    }
    else
    {
        DWORD errorCode = GetLastError();
        LogError("Failed to execute reload command, error code: " + std::to_string(errorCode));
        return false;
    }
#else
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process
        if (chdir(config->m_workingDir.c_str()) != 0)
        {
            LogError("Cannot change to working directory: " + config->m_workingDir);
            exit(1);
        }

        // Build arguments array for execv
        std::vector<std::string> args;
        args.push_back(executablePath); // argv[0] should be the program name
        args.push_back("--Server=" + config->m_serverName);
        args.push_back("--ID=" + config->m_serverId);
        args.push_back("--Config=" + config->m_configPath);
        args.push_back("--Plugin=" + config->m_pluginPath);
        args.push_back("--LuaScript=" + config->m_luaScriptPath);
        args.push_back("--Log=" + config->m_logPath);
        args.push_back("--Game=" + config->m_gameName);
        args.push_back("--Reload");
        args.push_back("-d");

        // Convert to char* array
        std::vector<char*> argv;
        for (const auto& a : args)
        {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);

        // Execute reload command
        execv(executablePath.c_str(), argv.data());

        // If we reach here, execv failed
        LogError("execv failed for reload command: " + std::string(strerror(errno)));
        exit(1);
    }
    else if (pid > 0)
    {
        // Parent process - wait for reload command to complete
        int status;
        waitpid(pid, &status, 0);

        LogInfo("Reload command executed successfully");
        return true;
    }
    else
    {
        LogError("fork failed for reload command: " + std::string(strerror(errno)));
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

    // Restart servers in configuration order using --Restart
    for (const std::string& serverId : m_serverConfigOrder)
    {
        if (!RestartServer(serverId))
        {
            auto it = m_serverConfigs.find(serverId);
            if (it != m_serverConfigs.end())
            {
                LogError("Failed to restart server: " + it->second->m_serverName + " (ID: " + serverId + ")");
            }
            else
            {
                LogError("Failed to restart server with ID: " + serverId);
            }
            return false;
        }

        // Wait for some time after each server restarts
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return true;
}


bool NFServerController::StartServersInOrder()
{
    // Start servers in the order they were loaded from config file
    for (const std::string& serverId : m_serverConfigOrder)
    {
        if (!StartServer(serverId))
        {
            auto it = m_serverConfigs.find(serverId);
            if (it != m_serverConfigs.end())
            {
                LogError("Failed to start server: " + it->second->m_serverName + " (ID: " + serverId + ")");
            }
            else
            {
                LogError("Failed to start server with ID: " + serverId);
            }
            return false;
        }

        // Wait for some time after each server starts
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return true;
}

bool NFServerController::StopServersInOrder()
{
    // Stop servers in reverse order of configuration file loading
    bool allSuccess = true;

    // Iterate in reverse order
    for (auto it = m_serverConfigOrder.rbegin(); it != m_serverConfigOrder.rend(); ++it)
    {
        const std::string& serverId = *it;
        if (!StopServer(serverId))
        {
            auto configIt = m_serverConfigs.find(serverId);
            if (configIt != m_serverConfigs.end())
            {
                LogError("Failed to stop server: " + configIt->second->m_serverName + " (ID: " + serverId + ")");
            }
            else
            {
                LogError("Failed to stop server with ID: " + serverId);
            }
            allSuccess = false;
        }

        // Wait for some time after each server stops
        std::this_thread::sleep_for(std::chrono::seconds(1));
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

    NFServerStatus status = it->second->m_status;
    Unlock();

    return status;
}

std::map<std::string, NFServerStatus> NFServerController::GetAllServerStatus()
{
    std::map<std::string, NFServerStatus> statusMap;

    Lock();
    for (const auto& pair : m_serverConfigs)
    {
        statusMap[pair.first] = pair.second->m_status;
    }
    Unlock();

    return statusMap;
}

std::map<std::string, std::shared_ptr<NFServerConfig>> NFServerController::GetAllServerConfigs()
{
    std::map<std::string, std::shared_ptr<NFServerConfig>> configMap;

    Lock();
    for (const auto& pair : m_serverConfigs)
    {
        configMap[pair.first] = pair.second;
    }
    Unlock();

    return configMap;
}

const std::vector<std::string>& NFServerController::GetServerConfigOrder() const
{
    return m_serverConfigOrder;
}

bool NFServerController::IsServerRunning(const std::string& serverId)
{
    return GetServerStatus(serverId) == NFServerStatus::SERVER_STATUS_RUNNING;
}


bool NFServerController::MatchesPattern(const std::string& serverId, const std::string& pattern)
{
    // Split serverId and pattern by '.'
    std::vector<std::string> serverParts;
    std::vector<std::string> patternParts;

    std::stringstream serverStream(serverId);
    std::stringstream patternStream(pattern);
    std::string part;

    while (std::getline(serverStream, part, '.'))
    {
        serverParts.push_back(part);
    }

    while (std::getline(patternStream, part, '.'))
    {
        patternParts.push_back(part);
    }

    // Both should have 4 parts (WorldID.RegionID.ServerType.Index)
    if (serverParts.size() != 4 || patternParts.size() != 4)
    {
        return false;
    }

    // Check each part
    for (size_t i = 0; i < 4; ++i)
    {
        if (patternParts[i] != "*" && patternParts[i] != serverParts[i])
        {
            return false;
        }
    }

    return true;
}

std::vector<std::string> NFServerController::GetMatchingServers(const std::string& pattern)
{
    std::vector<std::string> matchingServers;

    Lock();
    // Use configuration order to maintain proper sequence
    for (const std::string& serverId : m_serverConfigOrder)
    {
        auto it = m_serverConfigs.find(serverId);
        if (it != m_serverConfigs.end() && MatchesPattern(serverId, pattern))
        {
            matchingServers.push_back(serverId);
        }
    }
    Unlock();

    return matchingServers;
}

bool NFServerController::StartServersByPattern(const std::string& pattern)
{
    std::vector<std::string> servers = GetMatchingServers(pattern);

    if (servers.empty())
    {
        LogWarning("No servers found matching pattern: " + pattern);
        return false;
    }

    LogInfo("Starting " + std::to_string(servers.size()) + " servers matching pattern: " + pattern + " (in configuration order)");

    bool allSuccess = true;
    for (const std::string& serverId : servers)
    {
        if (!StartServer(serverId))
        {
            allSuccess = false;
        }
        // Wait between starts
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return allSuccess;
}

bool NFServerController::StopServersByPattern(const std::string& pattern)
{
    std::vector<std::string> servers = GetMatchingServers(pattern);

    if (servers.empty())
    {
        LogWarning("No servers found matching pattern: " + pattern);
        return false;
    }

    LogInfo("Stopping " + std::to_string(servers.size()) + " servers matching pattern: " + pattern + " (in reverse order)");

    bool allSuccess = true;
    // Stop servers in reverse order
    for (auto it = servers.rbegin(); it != servers.rend(); ++it)
    {
        const std::string& serverId = *it;
        if (!StopServer(serverId))
        {
            allSuccess = false;
        }
    }

    return allSuccess;
}

bool NFServerController::RestartServersByPattern(const std::string& pattern)
{
    std::vector<std::string> servers = GetMatchingServers(pattern);

    if (servers.empty())
    {
        LogWarning("No servers found matching pattern: " + pattern);
        return false;
    }

    LogInfo("Restarting " + std::to_string(servers.size()) + " servers matching pattern: " + pattern + " (in configuration order)");

    bool allSuccess = true;
    for (const std::string& serverId : servers)
    {
        if (!RestartServer(serverId))
        {
            allSuccess = false;
        }
        // Wait between restarts
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return allSuccess;
}

bool NFServerController::ReloadAllServers()
{
    LogInfo("Reloading all servers...");

    bool allSuccess = true;
    for (const std::string& serverId : m_serverConfigOrder)
    {
        if (!ReloadServer(serverId))
        {
            auto it = m_serverConfigs.find(serverId);
            if (it != m_serverConfigs.end())
            {
                LogError("Failed to reload server: " + it->second->m_serverName + " (ID: " + serverId + ")");
            }
            else
            {
                LogError("Failed to reload server with ID: " + serverId);
            }
            allSuccess = false;
        }

        // Wait for some time after each server reloads
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return allSuccess;
}

bool NFServerController::ReloadServersByPattern(const std::string& pattern)
{
    std::vector<std::string> servers = GetMatchingServers(pattern);

    if (servers.empty())
    {
        LogWarning("No servers found matching pattern: " + pattern);
        return false;
    }

    LogInfo("Reloading " + std::to_string(servers.size()) + " servers matching pattern: " + pattern + " (in configuration order)");

    bool allSuccess = true;
    for (const std::string& serverId : servers)
    {
        if (!ReloadServer(serverId))
        {
            allSuccess = false;
        }
        // Wait between reloads
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return allSuccess;
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

        // 🔧 Fix race condition: if server is starting, skip PID update to avoid overwriting startup thread settings
        if (config->m_status == NFServerStatus::SERVER_STATUS_STARTING)
        {
            // Starting in progress, skip monitoring update to avoid competing with startup thread
            continue;
        }

        // 🔒 Safe PID verification and update logic
        // Step 1: Read candidate PID from PID file
        int currentPidFromFile = ReadPidFile(*config);

        // Step 2: If there's a new PID in the PID file, verify its validity first
        if (currentPidFromFile > 0 && currentPidFromFile != config->m_processId)
        {
            // 🔍 Critical safety check: verify if the new PID is actually our server process
            if (IsProcessRunningAndValid(currentPidFromFile, *config))
            {
                // ✅ Verification passed: this is indeed our server process, safely update PID
                int oldPid = config->m_processId;
                config->m_processId = currentPidFromFile;
                LogInfo("Verified and updated PID for server " + config->m_serverId +
                    ": " + std::to_string(oldPid) + " -> " + std::to_string(currentPidFromFile));
            }
            else
            {
                // ❌ Verification failed: PID in file is invalid or occupied by other process
                if (config->m_processId > 0)
                {
                    LogWarning("PID file contains invalid PID " + std::to_string(currentPidFromFile) +
                        " for server " + config->m_serverId + " - ignoring unsafe PID update");
                    config->m_processId = 0;
                }
            }
        }
        else if (currentPidFromFile <= 0 && config->m_processId > 0)
        {
            // PID file doesn't exist or is invalid, but there's still a PID in config, check if existing process is still running
            LogInfo("PID file not found or invalid for server " + config->m_serverId +
                ", checking if existing process " + std::to_string(config->m_processId) + " still exists");
            config->m_processId = 0;
        }

        // Step 3: Judge and update running status based on final PID state
        if (config->m_processId > 0)
        {
            bool isRunningAndValid = IsProcessRunningAndValid(config->m_processId, *config);

            if (isRunningAndValid)
            {
                // Process is running and valid
                if (config->m_status != NFServerStatus::SERVER_STATUS_RUNNING)
                {
                    config->m_status = NFServerStatus::SERVER_STATUS_RUNNING;
                    LogInfo("Detected server status changed to running: " + config->m_serverName +
                        " (ID: " + config->m_serverId + ", PID: " + std::to_string(config->m_processId) + ")");
                }
                config->m_lastHeartbeat = std::chrono::system_clock::now();
            }
            else
            {
                // Process is not running or invalid
                if (config->m_status == NFServerStatus::SERVER_STATUS_RUNNING)
                {
                    LogWarning("Detected server abnormal stop or PID reuse: " + config->m_serverName +
                        " (ID: " + config->m_serverId + ", PID: " + std::to_string(config->m_processId) + ")");
                    config->m_status = NFServerStatus::SERVER_STATUS_STOPPED;
                    config->m_processId = 0; // Clear invalid PID
                }
            }
        }
        else
        {
            // Process ID is 0, ensure status is consistent
            if (config->m_status == NFServerStatus::SERVER_STATUS_RUNNING)
            {
                config->m_status = NFServerStatus::SERVER_STATUS_STOPPED;
                LogInfo("Updated status to stopped for server with no valid PID: " + config->m_serverId);
            }
        }
    }

    Unlock();
}

bool NFServerController::IsProcessRunning(int processId)
{
    if (processId <= 0)
    {
        return false;
    }

#if NF_PLATFORM == NF_PLATFORM_WIN
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (hProcess == NULL)
    {
        return false;
    }

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

#if NF_PLATFORM == NF_PLATFORM_WIN
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
    {
        LogError("Failed to open /proc directory: " + std::string(strerror(errno)));
        return processes;
    }

    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr)
    {
        if (!isdigit(entry->d_name[0]))
            continue;

        int pid;
        try
        {
            pid = std::stoi(entry->d_name);
        }
        catch (const std::exception&)
        {
            // Skip invalid PID entries
            continue;
        }

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
#if NF_PLATFORM == NF_PLATFORM_WIN
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
    std::cout << "Usage: NFServerController [options] \"<command> <target>\"" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  -c, --config <file>     Specify config file path" << std::endl;
    std::cout << "  -v, --verbose           Verbose output" << std::endl;
    std::cout << "  -q, --quiet             Quiet mode" << std::endl;
    std::cout << "  -h, --help              Show help information" << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  start <target>          Start specified server(s)" << std::endl;
    std::cout << "  stop <target>           Stop specified server(s)" << std::endl;
    std::cout << "  restart <target>        Restart specified server(s)" << std::endl;
    std::cout << "  reload <target>         Reload specified server(s) configuration" << std::endl;
    std::cout << "  check [target]          Show server status" << std::endl;

    std::cout << "\nTarget Format:" << std::endl;
    std::cout << "  Server ID format: WorldID.RegionID.ServerType.Index" << std::endl;
    std::cout << "  Use '*' as wildcard for any component" << std::endl;
    std::cout << "\nWildcard Examples:" << std::endl;
    std::cout << "  *.*.*.*       - All servers" << std::endl;
    std::cout << "  *.*.5.*       - All servers with ServerType 5" << std::endl;
    std::cout << "  1.13.*.*      - All servers in World 1, Region 13" << std::endl;
    std::cout << "  1.13.1.1      - Specific server" << std::endl;
    std::cout << "\nUsage Examples:" << std::endl;
    std::cout << "  NFServerController \"start *.*.*.*\"           # Start all servers" << std::endl;
    std::cout << "  NFServerController \"restart *.*.5.*\"        # Restart all type 5 servers" << std::endl;
    std::cout << "  NFServerController \"stop 1.13.1.1\"          # Stop specific server" << std::endl;
    std::cout << "  NFServerController \"reload *.*.10.*\"        # Reload all type 10 servers config" << std::endl;
    std::cout << "  NFServerController \"check *.*.*.*\"          # Show all server status" << std::endl;

    std::cout << "\nServer Types:" << std::endl;
    std::cout << "  1  - MasterServer       3  - RouteAgentServer    5  - ProxyServer" << std::endl;
    std::cout << "  6  - StoreServer        7  - LoginServer         8  - WorldServer" << std::endl;
    std::cout << "  9  - LogicServer        10 - GameServer" << std::endl;
    std::cout << std::endl;
}

// PID file management functions implementation
std::string NFServerController::GetPidFileName(const NFServerConfig& config)
{
    std::string fileName = config.m_serverName + "_" + config.m_serverId + ".pid";
    return NFFileUtility::Join(config.m_workingDir, fileName);
}

int NFServerController::ReadPidFile(const NFServerConfig& config)
{
    std::string pidFileName = GetPidFileName(config);

    if (!NFFileUtility::IsFileExist(pidFileName))
    {
        return 0; // PID file doesn't exist
    }

    try
    {
        std::ifstream pidFile(pidFileName);
        if (!pidFile.is_open())
        {
            LogWarning("Failed to open PID file: " + pidFileName);
            return -1;
        }

        int pid;
        pidFile >> pid;
        pidFile.close();

        if (pid <= 0)
        {
            LogWarning("Invalid PID in file: " + pidFileName);
            return -1;
        }

        return pid;
    }
    catch (const std::exception& e)
    {
        LogError("Exception while reading PID file " + pidFileName + ": " + e.what());
        return -1;
    }
}

bool NFServerController::VerifyProcessByPidFile(NFServerConfig& config)
{
    // First try to read PID from file
    int pidFromFile = ReadPidFile(config);

    if (pidFromFile <= 0)
    {
        // No valid PID file found
        if (config.m_processId > 0)
        {
            // Config has PID but no PID file, check if process is still running
            if (IsProcessRunning(config.m_processId))
            {
                LogInfo("Process running but PID file missing for: " + config.m_serverId);
                return true;
            }
            else
            {
                // Process not running, clear the PID
                config.m_processId = 0;
                return false;
            }
        }
        return false;
    }

    // PID file exists, verify if process is actually running
    if (IsProcessRunningAndValid(pidFromFile, config))
    {
        // Process is running and verified, update config PID if different
        if (config.m_processId != pidFromFile)
        {
            LogInfo("Updating verified PID from file for " + config.m_serverId + ": " + std::to_string(pidFromFile));
            config.m_processId = pidFromFile;
        }
        return true;
    }
    else
    {
        // Process not running or identity mismatch
        LogWarning("Process not running or identity mismatch for PID file: " + config.m_serverId);
        config.m_processId = 0;
        return false;
    }
}

// Enhanced process verification functions
bool NFServerController::IsProcessRunningAndValid(int processId, const NFServerConfig& config)
{
    if (processId <= 0)
        return false;

    // First check if process exists
    if (!IsProcessRunning(processId))
        return false;

    // Then verify the process identity
    return VerifyProcessIdentity(processId, config);
}

std::string NFServerController::GetProcessExecutablePath(int processId)
{
    if (processId <= 0)
        return "";

#if NF_PLATFORM == NF_PLATFORM_WIN
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL)
        return "";

    char exePath[MAX_PATH];
    DWORD pathSize = MAX_PATH;

    // Try QueryFullProcessImageName first (Vista+)
    if (QueryFullProcessImageNameA(hProcess, 0, exePath, &pathSize))
    {
        CloseHandle(hProcess);
        return std::string(exePath);
    }

    // Fallback to GetModuleFileNameEx
    if (GetModuleFileNameExA(hProcess, NULL, exePath, MAX_PATH))
    {
        CloseHandle(hProcess);
        return std::string(exePath);
    }

    CloseHandle(hProcess);
    return "";
#else
    // Linux: read from /proc/[pid]/exe
    std::string exePath = "/proc/" + std::to_string(processId) + "/exe";
    char resolvedPath[PATH_MAX];
    ssize_t len = readlink(exePath.c_str(), resolvedPath, sizeof(resolvedPath) - 1);
    if (len != -1)
    {
        resolvedPath[len] = '\0';
        return std::string(resolvedPath);
    }
    return "";
#endif
}

std::string NFServerController::GetProcessCommandLine(int processId)
{
    if (processId <= 0)
        return "";

#if NF_PLATFORM == NF_PLATFORM_WIN
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL)
        return "";

    // Note: Getting command line on Windows requires more complex code
    // For now, we'll focus on executable path verification
    CloseHandle(hProcess);
    return "";
#else
    // Linux: read from /proc/[pid]/cmdline
    std::string cmdlinePath = "/proc/" + std::to_string(processId) + "/cmdline";
    std::ifstream file(cmdlinePath);
    if (file.is_open())
    {
        std::string cmdline;
        std::getline(file, cmdline);

        // Replace null characters with spaces for readability
        for (char& c : cmdline)
        {
            if (c == '\0')
                c = ' ';
        }

        return cmdline;
    }
    return "";
#endif
}

bool NFServerController::VerifyProcessIdentity(int processId, const NFServerConfig& config)
{
    // Get the actual executable path of the running process
    std::string actualExePath = GetProcessExecutablePath(processId);
    if (actualExePath.empty())
    {
        LogWarning("Could not get executable path for PID " + std::to_string(processId));
        return false;
    }

    // Extract file names for comparison (ignore directory paths)
    std::string actualExeName = ExtractFileName(actualExePath);
    std::string expectedExeName = ExtractFileName(config.m_executablePath);

    // Check if the executable file names match (with smart extension handling)
    if (!CompareExecutableNames(actualExeName, expectedExeName))
    {
        LogWarning("Process executable name mismatch for PID " + std::to_string(processId) +
            ": expected '" + expectedExeName + "', found '" + actualExeName + "'");
        LogInfo("Note: Comparison ignores directory paths and handles .exe extension differences");
        return false;
    }

    LogInfo("Process executable name verified for PID " + std::to_string(processId) +
        ": " + actualExeName + " (from " + actualExePath + ")");

    // Additional verification: check command line arguments (Linux only for now)
#if NF_PLATFORM != NF_PLATFORM_WIN
    std::string cmdline = GetProcessCommandLine(processId);
    if (!cmdline.empty())
    {
        // Verify that the command line contains the expected config file path
        if (!config.m_configPath.empty() && cmdline.find(config.m_configPath) == std::string::npos)
        {
            LogWarning("Process command line does not contain expected config path for PID " +
                std::to_string(processId) + ": " + cmdline);
            // This is a warning, not a hard failure, as config paths might be relative
        }

        // Verify that the command line contains the expected server ID
        if (cmdline.find(config.m_serverId) == std::string::npos)
        {
            LogWarning("Process command line does not contain expected server ID for PID " +
                std::to_string(processId) + ": " + cmdline);
            // This is also a warning, as server ID might not always appear in command line
        }
    }
#endif

    LogInfo("Process identity verified for PID " + std::to_string(processId) +
        " (" + config.m_serverId + ")");
    return true;
}

std::string NFServerController::ExtractFileName(const std::string& filePath)
{
    if (filePath.empty())
    {
        return "";
    }

    // Find the last occurrence of path separator (both / and \ for cross-platform support)
    size_t lastSlash = filePath.find_last_of('/');
    size_t lastBackslash = filePath.find_last_of('\\');

    size_t lastSeparator = std::string::npos;
    if (lastSlash != std::string::npos && lastBackslash != std::string::npos)
    {
        lastSeparator = std::max(lastSlash, lastBackslash);
    }
    else if (lastSlash != std::string::npos)
    {
        lastSeparator = lastSlash;
    }
    else if (lastBackslash != std::string::npos)
    {
        lastSeparator = lastBackslash;
    }

    if (lastSeparator != std::string::npos)
    {
        return filePath.substr(lastSeparator + 1);
    }

    // No separator found, return the entire string
    return filePath;
}

bool NFServerController::CompareExecutableNames(const std::string& actualName, const std::string& expectedName)
{
    std::string actual = actualName;
    std::string expected = expectedName;

#if NF_PLATFORM == NF_PLATFORM_WIN
    // Windows: case-insensitive comparison
    std::transform(actual.begin(), actual.end(), actual.begin(), ::tolower);
    std::transform(expected.begin(), expected.end(), expected.begin(), ::tolower);
#endif

    // Direct comparison first
    if (actual == expected)
    {
        return true;
    }

    // Handle .exe extension differences
    std::string actualWithoutExt = actual;
    std::string expectedWithoutExt = expected;

    // Remove .exe extension if present
    if (actualWithoutExt.length() > 4 && actualWithoutExt.substr(actualWithoutExt.length() - 4) == ".exe")
    {
        actualWithoutExt = actualWithoutExt.substr(0, actualWithoutExt.length() - 4);
    }
    if (expectedWithoutExt.length() > 4 && expectedWithoutExt.substr(expectedWithoutExt.length() - 4) == ".exe")
    {
        expectedWithoutExt = expectedWithoutExt.substr(0, expectedWithoutExt.length() - 4);
    }

    // Compare without extension
    if (actualWithoutExt == expectedWithoutExt)
    {
        return true;
    }

    // Also handle cases where one side might have .exe and the other doesn't
    if (actual == expected + ".exe" || actual + ".exe" == expected)
    {
        return true;
    }

    return false;
}

void NFServerController::TestProcessVerification()
{
    LogInfo("=== Testing Process Verification Functions ===");

    // Test current process (should always succeed)
#if NF_PLATFORM == NF_PLATFORM_WIN
    int currentPid = GetCurrentProcessId();
#else
    int currentPid = getpid();
#endif

    LogInfo("Testing with current process PID: " + std::to_string(currentPid));

    // Test executable path retrieval
    std::string exePath = GetProcessExecutablePath(currentPid);
    if (!exePath.empty())
    {
        LogInfo("✓ Executable path: " + exePath);
    }
    else
    {
        LogWarning("✗ Failed to get executable path");
    }

    // Test command line retrieval
    std::string cmdLine = GetProcessCommandLine(currentPid);
    if (!cmdLine.empty())
    {
        LogInfo("✓ Command line: " + cmdLine);
    }
    else
    {
        LogInfo("ℹ Command line retrieval not supported or empty");
    }

    // Test with invalid PID
    LogInfo("Testing with invalid PID: 99999");
    std::string invalidPath = GetProcessExecutablePath(99999);
    if (invalidPath.empty())
    {
        LogInfo("✓ Correctly handled invalid PID");
    }
    else
    {
        LogWarning("✗ Unexpected result for invalid PID");
    }

    // Test basic process existence check
    if (IsProcessRunning(currentPid))
    {
        LogInfo("✓ Current process detected as running");
    }
    else
    {
        LogWarning("✗ Current process not detected as running");
    }

    LogInfo("=== Process Verification Test Complete ===");
}

// Shared memory clearing functions implementation
bool NFServerController::ClearShmServer(const std::string& serverName)
{
    LogInfo("Clearing shared memory for server: " + serverName);

    // Find the server configuration
    auto it = m_serverConfigs.find(serverName);
    if (it == m_serverConfigs.end())
    {
        LogError("Server not found: " + serverName);
        return false;
    }

    auto& config = it->second;

    // Stop the server first if it's running
    if (config->m_status == NFServerStatus::SERVER_STATUS_RUNNING)
    {
        LogInfo("Stopping server before clearing shared memory: " + serverName);
        if (!StopServer(serverName))
        {
            LogWarning("Failed to stop server, proceeding with shared memory cleanup anyway");
        }
        else
        {
            // Wait a moment for the server to fully stop
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    // Use server-specific cleanup based on serverId
    bool success = ClearShmByServerId(config->m_serverId);

    if (success)
    {
        LogInfo("Shared memory cleared successfully for server: " + serverName + " (ID: " + config->m_serverId + ")");
    }
    else
    {
        LogError("Failed to clear shared memory for server: " + serverName + " (ID: " + config->m_serverId + ")");
    }

    return success;
}

bool NFServerController::ClearShmAllServers()
{
    LogInfo("Clearing shared memory for all servers");

    // Stop all servers first
    LogInfo("Stopping all servers before clearing shared memory");
    StopAllServers();

    // Wait for all servers to stop
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Collect all server IDs
    std::vector<std::string> serverIds;
    for (const auto& pair : m_serverConfigs)
    {
        serverIds.push_back(pair.second->m_serverId);
    }

    // Use server-specific cleanup for all servers
    bool success = ClearShmByServerIds(serverIds);

    if (success)
    {
        LogInfo("Shared memory cleared successfully for all servers");
    }
    else
    {
        LogError("Failed to clear shared memory for some servers");
    }

    return success;
}

bool NFServerController::ClearShmServersByPattern(const std::string& pattern)
{
    LogInfo("Clearing shared memory for servers matching pattern: " + pattern);

    // Get matching servers
    std::vector<std::string> matchingServers = GetMatchingServers(pattern);

    if (matchingServers.empty())
    {
        LogWarning("No servers match the pattern: " + pattern);
        return true; // Not an error, just no servers to clear
    }

    // Stop matching servers first and collect their server IDs
    LogInfo("Stopping matching servers before clearing shared memory");
    std::vector<std::string> serverIds;
    for (const auto& serverName : matchingServers)
    {
        auto it = m_serverConfigs.find(serverName);
        if (it != m_serverConfigs.end())
        {
            if (it->second->m_status == NFServerStatus::SERVER_STATUS_RUNNING)
            {
                StopServer(serverName);
            }
            serverIds.push_back(it->second->m_serverId);
        }
    }

    // Wait for servers to stop
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Use server-specific cleanup for matching servers
    bool success = ClearShmByServerIds(serverIds);

    if (success)
    {
        LogInfo("Shared memory cleared successfully for pattern: " + pattern);
    }
    else
    {
        LogError("Failed to clear shared memory for pattern: " + pattern);
    }

    return success;
}

bool NFServerController::ExecuteShmCleanScript(const std::string& username)
{
    LogInfo("Executing shared memory cleanup");

#ifdef _WIN32
    // On Windows, we need to implement Windows-specific shared memory cleanup
    LogWarning("Shared memory cleanup on Windows is not implemented yet");
    LogWarning("Please manually clear shared memory or use WSL with ipcs/ipcrm commands");
    return false;
#else
    // On Linux/Unix systems - implement native C++ version without shell script

    std::string targetUser = username;
    if (targetUser.empty())
    {
        // Get current user
        char* user = getenv("USER");
        if (user)
        {
            targetUser = user;
        }
        else
        {
            LogWarning("Unable to determine current user, using 'unknown'");
            targetUser = "unknown";
        }
    }

    LogInfo("Clearing shared memory segments for user: " + targetUser);

    // Get shared memory segments for the target user
    std::vector<std::pair<std::string, std::string>> segments = GetSharedMemorySegments(targetUser);

    if (segments.empty())
    {
        LogInfo("No orphaned shared memory segments found for user: " + targetUser);
        return true; // Not an error
    }

    // Display found segments and calculate total size
    LogInfo("Found " + std::to_string(segments.size()) + " orphaned shared memory segments to clear:");
    size_t totalSize = 0;
    for (const auto& segment : segments)
    {
        LogInfo("  ID: " + segment.first + ", Size: " + segment.second + " bytes");
        try
        {
            totalSize += std::stoull(segment.second);
        }
        catch (const std::exception&)
        {
            // Ignore size parsing errors
        }
    }
    LogInfo("Total size to be freed: " + std::to_string(totalSize) + " bytes");

    // Remove shared memory segments using system calls
    bool allSuccess = true;
    int removedCount = 0;

    for (const auto& segment : segments)
    {
        if (RemoveSharedMemorySegment(segment.first))
        {
            removedCount++;
        }
        else
        {
            allSuccess = false;
        }

        // Small delay between operations to avoid overwhelming the system
        if (segments.size() > 10)
        {
            usleep(10000); // 10ms delay for large numbers of segments
        }
    }

    if (allSuccess)
    {
        LogInfo("Shared memory cleanup completed successfully for user: " + targetUser +
            " (" + std::to_string(removedCount) + " segments removed)");
    }
    else
    {
        LogWarning("Shared memory cleanup completed with some errors for user: " + targetUser +
            " (" + std::to_string(removedCount) + "/" + std::to_string(segments.size()) + " segments removed)");
    }

    return allSuccess;
#endif
}

// Shared memory helper functions implementation
#ifndef _WIN32
bool NFServerController::RemoveSharedMemorySegment(const std::string& shmid)
{
    // Validate shmid is numeric
    if (shmid.empty() || !std::all_of(shmid.begin(), shmid.end(), ::isdigit))
    {
        LogError("Invalid shared memory ID: " + shmid);
        return false;
    }

    // Convert to integer for validation
    int shmidInt;
    try
    {
        shmidInt = std::stoi(shmid);
        if (shmidInt < 0)
        {
            LogError("Invalid shared memory ID (negative): " + shmid);
            return false;
        }
    }
    catch (const std::exception& e)
    {
        LogError("Failed to parse shared memory ID: " + shmid + " - " + e.what());
        return false;
    }

    // Use shmctl system call to remove the segment
    if (shmctl(shmidInt, IPC_RMID, nullptr) == 0)
    {
        LogInfo("Shared memory segment removed: " + shmid);
        return true;
    }
    else
    {
        std::string errorMsg = "Failed to remove shared memory segment " + shmid + ": " + strerror(errno);
        LogError(errorMsg);
        return false;
    }
}

std::vector<std::pair<std::string, std::string>> NFServerController::GetSharedMemorySegments(const std::string& username)
{
    std::vector<std::pair<std::string, std::string>> segments; // ID, Size pairs

    // Execute ipcs -m command to get shared memory information
    FILE* pipe = popen("ipcs -m 2>/dev/null", "r");
    if (!pipe)
    {
        LogError("Failed to execute ipcs command");
        return segments;
    }

    char buffer[1024];
    int lineCount = 0;

    // Parse ipcs output
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        lineCount++;
        std::string line(buffer);

        // Remove trailing newline
        if (!line.empty() && line.back() == '\n')
        {
            line.pop_back();
        }

        // Skip header lines and empty lines
        if (line.empty() ||
            line.find("shmid") != std::string::npos ||
            line.find("Shared Memory") != std::string::npos ||
            line.find("key") != std::string::npos ||
            line.find("--") != std::string::npos ||
            line.find("IPC status") != std::string::npos)
        {
            continue;
        }

        // Parse line format: key shmid owner perms bytes nattch status
        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;

        while (iss >> token)
        {
            tokens.push_back(token);
        }

        // Need at least 6 fields: key, shmid, owner, perms, bytes, nattch
        if (tokens.size() >= 6)
        {
            std::string key = tokens[0];
            std::string shmid = tokens[1];
            std::string owner = tokens[2];
            std::string perms = tokens[3];
            std::string bytes = tokens[4];
            std::string nattch = tokens[5];

            // Validate shmid is numeric
            bool isValidShmid = !shmid.empty() && std::all_of(shmid.begin(), shmid.end(), ::isdigit);

            // Check if this segment belongs to target user and has 0 attachments
            if (isValidShmid && owner == username && nattch == "0")
            {
                segments.push_back(std::make_pair(shmid, bytes));
                LogInfo("Found orphaned shm segment: ID=" + shmid + ", Size=" + bytes + " bytes, Key=" + key);
            }
        }
    }

    int pcloseResult = pclose(pipe);
    if (pcloseResult != 0)
    {
        LogWarning("ipcs command returned non-zero exit code: " + std::to_string(pcloseResult));
    }

    return segments;
}
#endif

std::vector<std::pair<std::string, std::string>> NFServerController::QuerySharedMemoryInfo(const std::string& username)
{
#ifdef _WIN32
    LogWarning("Shared memory query on Windows is not implemented yet");
    return std::vector<std::pair<std::string, std::string>>();
#else
    std::string targetUser = username;
    if (targetUser.empty())
    {
        // Get current user
        char* user = getenv("USER");
        if (user)
        {
            targetUser = user;
        }
        else
        {
            targetUser = "unknown";
        }
    }

    LogInfo("Querying shared memory segments for user: " + targetUser);

    std::vector<std::pair<std::string, std::string>> segments = GetSharedMemorySegments(targetUser);

    if (segments.empty())
    {
        LogInfo("No orphaned shared memory segments found for user: " + targetUser);
    }
    else
    {
        size_t totalSize = 0;
        LogInfo("Found " + std::to_string(segments.size()) + " orphaned shared memory segments:");
        for (const auto& segment : segments)
        {
            LogInfo("  ID: " + segment.first + ", Size: " + segment.second + " bytes");
            try
            {
                totalSize += std::stoull(segment.second);
            }
            catch (const std::exception&)
            {
                // Ignore size parsing errors
            }
        }
        LogInfo("Total size of orphaned segments: " + std::to_string(totalSize) + " bytes");
    }

    return segments;
#endif
}

// Server-specific shared memory clearing functions implementation
bool NFServerController::ClearShmByServerId(const std::string& serverId)
{
    LogInfo("Clearing shared memory for server ID: " + serverId);

#ifdef _WIN32
    LogWarning("Server-specific shared memory cleanup on Windows is not implemented yet");
    return false;
#else
    // Get shared memory keys for this server
    std::vector<uint32_t> keys = GetServerShmKeys(serverId);
    
    if (keys.empty())
    {
        LogInfo("No shared memory keys found for server: " + serverId);
        return true; // Not an error
    }
    
    LogInfo("Found " + std::to_string(keys.size()) + " shared memory keys for server: " + serverId);
    
    bool allSuccess = true;
    int removedCount = 0;
    
    for (uint32_t key : keys)
    {
        std::ostringstream keyHex;
        keyHex << "0x" << std::hex << key;
        LogInfo("Processing shared memory key: " + keyHex.str() + " for server: " + serverId);
        
        if (RemoveSharedMemoryByKey(key))
        {
            removedCount++;
        }
        else
        {
            allSuccess = false;
        }
    }
    
    if (allSuccess)
    {
        LogInfo("Shared memory cleanup completed successfully for server: " + serverId + 
                " (" + std::to_string(removedCount) + " segments removed)");
    }
    else
    {
        LogWarning("Shared memory cleanup completed with some errors for server: " + serverId +
                   " (" + std::to_string(removedCount) + "/" + std::to_string(keys.size()) + " segments removed)");
    }
    
    return allSuccess;
#endif
}

bool NFServerController::ClearShmByServerIds(const std::vector<std::string>& serverIds)
{
    LogInfo("Clearing shared memory for " + std::to_string(serverIds.size()) + " servers");

    bool allSuccess = true;
    int successCount = 0;

    for (const std::string& serverId : serverIds)
    {
        if (ClearShmByServerId(serverId))
        {
            successCount++;
        }
        else
        {
            allSuccess = false;
        }
    }

    if (allSuccess)
    {
        LogInfo("Shared memory cleanup completed successfully for all " + std::to_string(successCount) + " servers");
    }
    else
    {
        LogWarning("Shared memory cleanup completed with some errors (" +
            std::to_string(successCount) + "/" + std::to_string(serverIds.size()) + " servers successful)");
    }

    return allSuccess;
}

#ifndef _WIN32
int NFServerController::GetShmIdByKey(uint32_t key)
{
    // Try to get existing shared memory segment by key
    int shmid = shmget(key, 0, 0666);
    if (shmid < 0)
    {
        // Segment doesn't exist or we don't have permission
        std::ostringstream keyHex;
        keyHex << "0x" << std::hex << key;
        
        if (errno == ENOENT)
        {
            LogInfo("Shared memory segment with key " + keyHex.str() + " does not exist");
        }
        else
        {
            LogWarning("Failed to get shared memory segment with key " + keyHex.str() + ": " + strerror(errno));
        }
        return -1;
    }
    
    return shmid;
}

std::vector<uint32_t> NFServerController::GetServerShmKeys(const std::string& serverId)
{
    std::vector<uint32_t> keys;
    
    try
    {
        // Get BusID key
        uint32_t busIdKey = NFServerIDUtil::GetBusID(serverId);
        keys.push_back(busIdKey);
        
        std::ostringstream busIdHex;
        busIdHex << "0x" << std::hex << busIdKey;
        LogInfo("Server " + serverId + " BusID key: " + busIdHex.str());
        
        // Get ShmObjKey
        uint32_t shmObjKey = NFServerIDUtil::GetShmObjKey(serverId);
        keys.push_back(shmObjKey);
        
        std::ostringstream shmObjHex;
        shmObjHex << "0x" << std::hex << shmObjKey;
        LogInfo("Server " + serverId + " ShmObj key: " + shmObjHex.str());
        
        // Remove duplicates if BusID and ShmObjKey are the same
        if (busIdKey == shmObjKey && keys.size() > 1)
        {
            keys.pop_back();
            LogInfo("BusID and ShmObj keys are identical, using single key");
        }
    }
    catch (const std::exception& e)
    {
        LogError("Failed to get shared memory keys for server " + serverId + ": " + e.what());
    }
    
    return keys;
}

bool NFServerController::RemoveSharedMemoryByKey(uint32_t key)
{
    // First, try to get the shmid for this key
    int shmid = GetShmIdByKey(key);
    if (shmid < 0)
    {
        std::ostringstream keyHex;
        keyHex << "0x" << std::hex << key;
        LogInfo("Shared memory segment with key " + keyHex.str() + " not found or already removed");
        return true; // Not an error if it doesn't exist
    }
    
    std::ostringstream keyHex;
    keyHex << "0x" << std::hex << key;
    
    // Get information about the shared memory segment
    struct shmid_ds shmInfo;
    if (shmctl(shmid, IPC_STAT, &shmInfo) == 0)
    {
        LogInfo("Found shared memory segment: ID=" + std::to_string(shmid) + 
                ", Key=" + keyHex.str() + 
                ", Size=" + std::to_string(shmInfo.shm_segsz) + " bytes" +
                ", Attachments=" + std::to_string(shmInfo.shm_nattch));
        
        // Check if anyone is still attached
        if (shmInfo.shm_nattch > 0)
        {
            LogWarning("Shared memory segment " + std::to_string(shmid) + " has " + 
                      std::to_string(shmInfo.shm_nattch) + " attachments, removing anyway");
        }
    }
    else
    {
        LogWarning("Failed to get info for shared memory segment " + std::to_string(shmid) + ": " + strerror(errno));
    }
    
    // Remove the shared memory segment
    if (shmctl(shmid, IPC_RMID, nullptr) == 0)
    {
        LogInfo("Shared memory segment removed: ID=" + std::to_string(shmid) + ", Key=" + keyHex.str());
        return true;
    }
    else
    {
        LogError("Failed to remove shared memory segment " + std::to_string(shmid) + 
                " (key " + keyHex.str() + "): " + strerror(errno));
        return false;
    }
}
#endif

void NFServerController::TestServerShmKeys(const std::string& serverId)
{
    LogInfo("=== Testing Shared Memory Keys for Server: " + serverId + " ===");

#ifdef _WIN32
    LogWarning("Shared memory testing on Windows is not implemented yet");
    return;
#else
    try
    {
        // Get and display the keys
        std::vector<uint32_t> keys = GetServerShmKeys(serverId);
        
        if (keys.empty())
        {
            LogWarning("No keys generated for server: " + serverId);
            return;
        }
        
        LogInfo("Generated " + std::to_string(keys.size()) + " key(s) for server " + serverId);
        
        // Test each key
        for (uint32_t key : keys)
        {
            std::ostringstream keyHex;
            keyHex << "0x" << std::hex << key;
            
            LogInfo("Testing key: " + keyHex.str());
            
            int shmid = GetShmIdByKey(key);
            if (shmid >= 0)
            {
                LogInfo("  -> Found existing shared memory segment: ID=" + std::to_string(shmid));
                
                // Get detailed information
                struct shmid_ds shmInfo;
                if (shmctl(shmid, IPC_STAT, &shmInfo) == 0)
                {
                    LogInfo("  -> Size: " + std::to_string(shmInfo.shm_segsz) + " bytes");
                    LogInfo("  -> Attachments: " + std::to_string(shmInfo.shm_nattch));
                    LogInfo("  -> Creator PID: " + std::to_string(shmInfo.shm_cpid));
                    LogInfo("  -> Last attach PID: " + std::to_string(shmInfo.shm_lpid));
                }
                else
                {
                    LogWarning("  -> Failed to get segment info: " + std::string(strerror(errno)));
                }
            }
            else
            {
                LogInfo("  -> No existing shared memory segment found (this is normal if server is not running)");
            }
        }
    }
    catch (const std::exception& e)
    {
        LogError("Exception during shared memory key testing: " + std::string(e.what()));
    }
    
    LogInfo("=== Shared Memory Key Test Complete ===");
#endif
}
