#include "NFServerController.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#endif

// Convert status to string
std::string StatusToString(NFServerStatus status)
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

// Get current process ID - renamed to avoid conflict with Windows API
int GetProcessId()
{
#ifdef _WIN32
    return static_cast<int>(::GetCurrentProcessId());
#else
    return static_cast<int>(getpid());
#endif
}

// Check if process exists by PID
bool IsProcessRunning(int pid)
{
    if (pid <= 0) return false;

#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (hProcess == NULL) return false;
    
    DWORD exitCode;
    bool result = GetExitCodeProcess(hProcess, &exitCode);
    CloseHandle(hProcess);
    
    return result && (exitCode == STILL_ACTIVE);
#else
    // Use kill(pid, 0) to check if process exists
    return kill(static_cast<pid_t>(pid), 0) == 0;
#endif
}

// Read PID from file
int ReadPidFromFile(const std::string& pidFile)
{
    std::ifstream file(pidFile);
    if (!file.is_open())
    {
        return -1; // File doesn't exist or can't be opened
    }

    int pid = -1;
    file >> pid;
    file.close();

    return pid;
}

// Write PID to file
bool WritePidToFile(const std::string& pidFile, int pid)
{
    std::ofstream file(pidFile);
    if (!file.is_open())
    {
        std::cerr << "Error: Failed to open PID file for writing: " << pidFile << std::endl;
        return false;
    }

    file << pid << std::endl;
    file.close();

    if (file.fail())
    {
        std::cerr << "Error: Failed to write PID to file: " << pidFile << std::endl;
        return false;
    }

    return true;
}

// Remove PID file
void RemovePidFile(const std::string& pidFile)
{
    std::remove(pidFile.c_str());
}

// Global variable to store PID file path for cleanup
std::string g_pidFilePath;

// Cleanup function for atexit
void CleanupPidFile()
{
    if (!g_pidFilePath.empty())
    {
        RemovePidFile(g_pidFilePath);
    }
}

// Initialize PID management
bool InitializePidManagement(const std::string& pidFile)
{
    // Store global PID file path for cleanup
    g_pidFilePath = pidFile;

    // Read existing PID from file
    int existingPid = ReadPidFromFile(pidFile);

    if (existingPid > 0)
    {
        // Check if the process with this PID is still running
        if (IsProcessRunning(existingPid))
        {
            std::cerr << "Error: NFServerController is already running with PID " << existingPid << std::endl;
            std::cerr << "PID file: " << pidFile << std::endl;
            std::cerr << "If you are sure the process is not running, please remove the PID file manually." << std::endl;
            return false;
        }
        else
        {
            // Process is not running, remove stale PID file
            std::cout << "Removing stale PID file (PID " << existingPid << " not running)" << std::endl;
            RemovePidFile(pidFile);
        }
    }

    // Write current PID to file
    int currentPid = GetProcessId();
    if (!WritePidToFile(pidFile, currentPid))
    {
        return false;
    }

    std::cout << "NFServerController started with PID: " << currentPid << std::endl;
    std::cout << "PID file: " << pidFile << std::endl;

    return true;
}

// Print server status table
void PrintServerStatus(NFServerController& controller)
{
    auto configMap = controller.GetAllServerConfigs();
    auto configOrder = controller.GetServerConfigOrder();

    std::cout << "\n=== Server Status ===" << std::endl;
    std::cout << std::left << std::setw(20) << "Proc"
        << std::setw(20) << "ServerID"
        << std::setw(20) << "ServerName"
        << std::setw(20) << "Game"
        << std::setw(20) << "Status" << std::endl;
    std::cout << std::string(100, '-') << std::endl;

    int total = 0;
    int success = 0;
    int failed = 0;

    for (const std::string& serverId : configOrder)
    {
        if (configMap.find(serverId) == configMap.end())
        {
            continue;
        }
        auto config = configMap[serverId];
        std::string procInfo = config->m_processId > 0 ? std::to_string(config->m_processId) : "N/A";
        std::string status = StatusToString(config->m_status);

        total++;
        if (config->m_status == NFServerStatus::SERVER_STATUS_RUNNING)
        {
            success++;
        }
        else if (config->m_status == NFServerStatus::SERVER_STATUS_ERROR ||
            config->m_status == NFServerStatus::SERVER_STATUS_STOPPED)
        {
            failed++;
        }

        std::cout << std::left << std::setw(20) << procInfo
            << std::setw(20) << config->m_serverId
            << std::setw(20) << config->m_serverName
            << std::setw(20) << config->m_gameName
            << std::setw(20) << status << std::endl;
    }

    std::cout << std::string(100, '-') << std::endl;
    std::cout << "Check total(" << total << ") success(" << success
        << ") Failed or Timeout(" << failed << ")" << std::endl;
    std::cout << std::endl;
}

// Parse command from string
std::vector<std::string> ParseCommand(const std::string& command)
{
    std::vector<std::string> tokens;
    std::stringstream ss(command);
    std::string token;

    while (ss >> token)
    {
        tokens.push_back(token);
    }

    return tokens;
}

// Check if target is a wildcard pattern
bool IsWildcardPattern(const std::string& target)
{
    return target.find('*') != std::string::npos;
}

// Execute command with target
bool ExecuteCommand(NFServerController& controller, const std::string& action, const std::string& target)
{
    std::string cmd = action;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    if (cmd == "start")
    {
        if (target.empty() || target == "*.*.*.*")
        {
            std::cout << "Starting all servers..." << std::endl;
            bool success = controller.StartAllServers();
            std::cout << (success ? "All servers started successfully" : "Failed to start some servers") << std::endl;
            return success;
        }
        else if (IsWildcardPattern(target))
        {
            std::cout << "Starting servers matching pattern: " << target << std::endl;
            bool success = controller.StartServersByPattern(target);
            std::cout << (success ? "Servers started successfully" : "Failed to start some servers") << std::endl;
            return success;
        }
        else
        {
            std::cout << "Starting server: " << target << std::endl;
            bool success = controller.StartServer(target);
            std::cout << (success ? "Server started successfully" : "Failed to start server") << std::endl;
            return success;
        }
    }
    else if (cmd == "stop")
    {
        if (target.empty() || target == "*.*.*.*")
        {
            std::cout << "Stopping all servers..." << std::endl;
            bool success = controller.StopAllServers();
            std::cout << (success ? "All servers stopped successfully" : "Failed to stop some servers") << std::endl;
            return success;
        }
        else if (IsWildcardPattern(target))
        {
            std::cout << "Stopping servers matching pattern: " << target << std::endl;
            bool success = controller.StopServersByPattern(target);
            std::cout << (success ? "Servers stopped successfully" : "Failed to stop some servers") << std::endl;
            return success;
        }
        else
        {
            std::cout << "Stopping server: " << target << std::endl;
            bool success = controller.StopServer(target);
            std::cout << (success ? "Server stopped successfully" : "Failed to stop server") << std::endl;
            return success;
        }
    }
    else if (cmd == "restart")
    {
        if (target.empty() || target == "*.*.*.*")
        {
            std::cout << "Restarting all servers..." << std::endl;
            bool success = controller.RestartAllServers();
            std::cout << (success ? "All servers restarted successfully" : "Failed to restart some servers") << std::endl;
            return success;
        }
        else if (IsWildcardPattern(target))
        {
            std::cout << "Restarting servers matching pattern: " << target << std::endl;
            bool success = controller.RestartServersByPattern(target);
            std::cout << (success ? "Servers restarted successfully" : "Failed to restart some servers") << std::endl;
            return success;
        }
        else
        {
            std::cout << "Restarting server: " << target << std::endl;
            bool success = controller.RestartServer(target);
            std::cout << (success ? "Server restarted successfully" : "Failed to restart server") << std::endl;
            return success;
        }
    }
    else if (cmd == "check")
    {
        if (target.empty() || target == "*.*.*.*")
        {
            PrintServerStatus(controller);
            return true;
        }
        else if (IsWildcardPattern(target))
        {
            auto servers = controller.GetMatchingServers(target);
            auto configMap = controller.GetAllServerConfigs();

            std::cout << "\n=== Servers matching pattern: " << target << " ===" << std::endl;
            std::cout << std::left << std::setw(20) << "Proc"
                << std::setw(20) << "ServerID"
                << std::setw(20) << "ServerName"
                << std::setw(20) << "Game"
                << std::setw(20) << "Status" << std::endl;
            std::cout << std::string(100, '-') << std::endl;

            int total = 0;
            int success = 0;
            int failed = 0;

            for (const auto& serverId : servers)
            {
                auto it = configMap.find(serverId);
                if (it != configMap.end())
                {
                    const auto& config = it->second;
                    std::string procInfo = config->m_processId > 0 ? std::to_string(config->m_processId) : "N/A";
                    std::string status = StatusToString(config->m_status);

                    total++;
                    if (config->m_status == NFServerStatus::SERVER_STATUS_RUNNING)
                    {
                        success++;
                    }
                    else if (config->m_status == NFServerStatus::SERVER_STATUS_ERROR ||
                        config->m_status == NFServerStatus::SERVER_STATUS_STOPPED)
                    {
                        failed++;
                    }

                    std::cout << std::left << std::setw(20) << procInfo
                        << std::setw(20) << config->m_serverId
                        << std::setw(20) << config->m_serverName
                        << std::setw(20) << config->m_gameName
                        << std::setw(20) << status << std::endl;
                }
            }

            std::cout << std::string(100, '-') << std::endl;
            std::cout << "Check total(" << total << ") success(" << success
                << ") Failed or Timeout(" << failed << ")" << std::endl;
            std::cout << std::endl;
            return true;
        }
        else
        {
            auto configMap = controller.GetAllServerConfigs();
            auto it = configMap.find(target);
            if (it != configMap.end())
            {
                const auto& config = it->second;
                std::string procInfo = config->m_processId > 0 ? std::to_string(config->m_processId) : "N/A";
                std::string status = StatusToString(config->m_status);

                std::cout << "\n=== Server: " << target << " ===" << std::endl;
                std::cout << std::left << std::setw(20) << "Proc"
                    << std::setw(20) << "Server ID"
                    << std::setw(20) << "Server Name"
                    << std::setw(20) << "Game"
                    << std::setw(20) << "Status" << std::endl;
                std::cout << std::string(100, '-') << std::endl;
                std::cout << std::left << std::setw(20) << procInfo
                    << std::setw(20) << config->m_serverId
                    << std::setw(20) << config->m_serverName
                    << std::setw(20) << config->m_gameName
                    << std::setw(20) << status << std::endl;
                std::cout << std::string(100, '-') << std::endl;

                if (config->m_status == NFServerStatus::SERVER_STATUS_RUNNING)
                {
                    std::cout << "Check total(1) success(1) Failed or Timeout(0)" << std::endl;
                }
                else
                {
                    std::cout << "Check total(1) success(0) Failed or Timeout(1)" << std::endl;
                }
                std::cout << std::endl;
            }
            else
            {
                std::cout << "Server not found: " << target << std::endl;
            }
            return true;
        }
    }
    else if (cmd == "reload")
    {
        if (target.empty() || target == "*.*.*.*")
        {
            std::cout << "Reloading all servers..." << std::endl;
            bool success = controller.ReloadAllServers();
            std::cout << (success ? "All servers reloaded successfully" : "Failed to reload some servers") << std::endl;
            return success;
        }
        else if (IsWildcardPattern(target))
        {
            std::cout << "Reloading servers matching pattern: " << target << std::endl;
            bool success = controller.ReloadServersByPattern(target);
            std::cout << (success ? "Servers reloaded successfully" : "Failed to reload some servers") << std::endl;
            return success;
        }
        else
        {
            std::cout << "Reloading server: " << target << std::endl;
            bool success = controller.ReloadServer(target);
            std::cout << (success ? "Server reloaded successfully" : "Failed to reload server") << std::endl;
            return success;
        }
    }
    else if (cmd == "clear")
    {
        if (target.empty() || target == "*.*.*.*")
        {
            std::cout << "Clear all servers shm..." << std::endl;
            bool success = controller.ClearShmAllServers();
            std::cout << (success ? "Clear All servers shm successfully" : "Failed to clear some servers shm") << std::endl;
            return success;
        }
        else if (IsWildcardPattern(target))
        {
            std::cout << "clear servers shm matching pattern: " << target << std::endl;
            bool success = controller.ClearShmServersByPattern(target);
            std::cout << (success ? "Servers clear shm successfully" : "Failed to clear some servers shm") << std::endl;
            return success;
        }
        else
        {
            std::cout << "Clear shm server: " << target << std::endl;
            bool success = controller.ClearShmServer(target);
            std::cout << (success ? "Server clear shm successfully" : "Failed to clear server shm") << std::endl;
            return success;
        }
    }
    else
    {
        std::cerr << "Unknown command: " << cmd << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    std::cout << "NFServerController v2.0 - NFrame Server Controller" << std::endl;
    std::cout << "Supports wildcard patterns for server management" << std::endl;

    // Default config file path
#ifdef _WIN32
    std::string configFile = "win_servers.conf";
    std::string pidFile = "NFServerController.pid";
#else
    std::string configFile = "linux_servers.conf";
    std::string pidFile = "/tmp/NFServerController.pid";
#endif
    bool verbose = false;
    bool quiet = false;
    std::string commandString;

    // Initialize PID management first
    if (!InitializePidManagement(pidFile))
    {
        return 1;
    }

    // Setup cleanup handler to remove PID file on exit
    std::atexit(CleanupPidFile);

    NFServerController controller;

    // Parse command line arguments
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help")
        {
            controller.PrintHelp();
            return 0;
        }
        else if (arg == "-c" || arg == "--config")
        {
            if (i + 1 < argc)
            {
                configFile = argv[++i];
            }
            else
            {
                std::cerr << "Error: " << arg << " requires config file path" << std::endl;
                return 1;
            }
        }
        else if (arg == "-v" || arg == "--verbose")
        {
            verbose = true;
        }
        else if (arg == "-q" || arg == "--quiet")
        {
            quiet = true;
        }
        else if (arg[0] != '-')
        {
            // This is the command string
            commandString = arg;
            break;
        }
        else
        {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            return 1;
        }
    }

    // Set log level
    if (quiet)
    {
        controller.SetLogLevel(0);
    }
    else if (verbose)
    {
        controller.SetLogLevel(3);
    }
    else
    {
        controller.SetLogLevel(2);
    }

    // Initialize controller
    if (!controller.Initialize(configFile))
    {
        std::cerr << "Error: Failed to initialize server controller" << std::endl;
        return 1;
    }

    // If no command string provided, show status
    if (commandString.empty())
    {
        PrintServerStatus(controller);
        std::cout << "\nUsage: NFServerController [options] \"<command> <target>\"" << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  NFServerController \"start *.*.*.*\"       # Start all servers" << std::endl;
        std::cout << "  NFServerController \"restart *.*.5.*\"    # Restart all type 5 servers" << std::endl;
        std::cout << "  NFServerController \"stop 1.13.1.1\"      # Stop specific server" << std::endl;
        std::cout << "  NFServerController \"reload *.*.10.*\"    # Reload all game servers config" << std::endl;
        std::cout << "  NFServerController \"check *.*.*.*\"              # Show all server status" << std::endl;
        std::cout << "  NFServerController \"clear *.*.*.*\"              # Clear all server Shm (server-specific)" << std::endl;
        std::cout << "  NFServerController \"clear 1.13.1.1\"            # Clear specific server Shm by serverId" << std::endl;
        return 0;
    }

    // Parse command string
    std::vector<std::string> commandTokens = ParseCommand(commandString);
    if (commandTokens.empty())
    {
        std::cerr << "Error: Empty command" << std::endl;
        return 1;
    }

    std::string action = commandTokens[0];
    std::string target = commandTokens.size() > 1 ? commandTokens[1] : "";

    // Execute command
    bool success = ExecuteCommand(controller, action, target);

    return success ? 0 : 1;
}
