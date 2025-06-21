#include "NFServerController.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
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

// Print server status table
void PrintServerStatus(NFServerController& controller)
{
    auto statusMap = controller.GetAllServerStatus();

    std::cout << "\n=== Server Status ===" << std::endl;
    std::cout << std::left << std::setw(20) << "Server Name"
        << std::setw(15) << "Status"
        << std::setw(10) << "Process ID" << std::endl;
    std::cout << std::string(45, '-') << std::endl;

    for (const auto& pair : statusMap)
    {
        std::cout << std::left << std::setw(20) << pair.first
            << std::setw(15) << StatusToString(pair.second);

        // Process ID display can be added here, requires interface extension
        std::cout << std::setw(10) << "-" << std::endl;
    }
    std::cout << std::endl;
}

// Interactive monitoring mode
void InteractiveMonitor(NFServerController& controller)
{
    controller.MonitorServers();

    std::cout << "\n=== Entering Interactive Monitoring Mode ===" << std::endl;
    std::cout << "Available commands:" << std::endl;
    std::cout << "  start <server name>     - Start server" << std::endl;
    std::cout << "  stop <server name>      - Stop server" << std::endl;
    std::cout << "  restart <server name>   - Restart server" << std::endl;
    std::cout << "  reload <server name>    - Reload server configuration" << std::endl;
    std::cout << "  status                  - Show status" << std::endl;
    std::cout << "  list                    - List all servers" << std::endl;
    std::cout << "  help                    - Show help" << std::endl;
    std::cout << "  quit or exit            - Exit monitoring" << std::endl;
    std::cout << "Press Ctrl+C or type quit to exit monitoring mode" << std::endl;

    std::string input;
    while (true)
    {
        std::cout << "\nNFServerController> ";
        if (!std::getline(std::cin, input))
        {
            break;
        }

        if (input.empty())
        {
            continue;
        }

        // Split input command
        std::vector<std::string> tokens;
        std::stringstream ss(input);
        std::string token;
        while (ss >> token)
        {
            tokens.push_back(token);
        }

        if (tokens.empty())
        {
            continue;
        }

        std::string command = tokens[0];
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        if (command == "quit" || command == "exit")
        {
            break;
        }
        else if (command == "help")
        {
            controller.PrintHelp();
        }
        else if (command == "status")
        {
            PrintServerStatus(controller);
        }
        else if (command == "list")
        {
            auto serverList = controller.GetServerList();
            std::cout << "\nAvailable servers:" << std::endl;
            for (const auto& server : serverList)
            {
                std::cout << "  " << server << std::endl;
            }
        }
        else if (command == "start")
        {
            if (tokens.size() > 1)
            {
                std::string serverName = tokens[1];
                std::cout << "Starting server: " << serverName << std::endl;
                bool success = controller.StartServer(serverName);
                std::cout << (success ? "Start successful" : "Start failed") << std::endl;
            }
            else
            {
                std::cout << "Starting all servers..." << std::endl;
                bool success = controller.StartAllServers();
                std::cout << (success ? "Start completed" : "Start failed") << std::endl;
            }
        }
        else if (command == "stop")
        {
            if (tokens.size() > 1)
            {
                std::string serverName = tokens[1];
                std::cout << "Stopping server: " << serverName << std::endl;
                bool success = controller.StopServer(serverName);
                std::cout << (success ? "Stop successful" : "Stop failed") << std::endl;
            }
            else
            {
                std::cout << "Stopping all servers..." << std::endl;
                bool success = controller.StopAllServers();
                std::cout << (success ? "Stop completed" : "Stop failed") << std::endl;
            }
        }
        else if (command == "restart")
        {
            if (tokens.size() > 1)
            {
                std::string serverName = tokens[1];
                std::cout << "Restarting server: " << serverName << std::endl;
                bool success = controller.RestartServer(serverName);
                std::cout << (success ? "Restart successful" : "Restart failed") << std::endl;
            }
            else
            {
                std::cout << "Restarting all servers..." << std::endl;
                bool success = controller.RestartAllServers();
                std::cout << (success ? "Restart completed" : "Restart failed") << std::endl;
            }
        }
        else if (command == "reload")
        {
            if (tokens.size() > 1)
            {
                std::string serverName = tokens[1];
                std::cout << "Reloading server configuration: " << serverName << std::endl;
                bool success = controller.ReloadServer(serverName);
                std::cout << (success ? "Reload successful" : "Reload failed") << std::endl;
            }
            else
            {
                std::cout << "Please specify the server name to reload" << std::endl;
            }
        }
        else
        {
            std::cout << "Unknown command: " << command << std::endl;
            std::cout << "Type help to see available commands" << std::endl;
        }
    }

    controller.StopMonitoring();
}

int main(int argc, char* argv[])
{
    std::cout << "NFServerController v1.0 - NFrame Server Controller" << std::endl;
    std::cout << "Supports server startup, stop, restart, and monitoring functions on Windows and Linux platforms" << std::endl;

    // Default config file path
    std::string configFile = "servers.conf";
    bool verbose = false;
    bool quiet = false;

    NFServerController controller;

    // Parse command line arguments
    std::vector<std::string> commands;
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
                std::cerr << "Error: " << arg << " requires specifying config file path" << std::endl;
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
            commands.push_back(arg);
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

    // If no command arguments, show status and enter interactive mode
    if (commands.empty())
    {
        PrintServerStatus(controller);
        InteractiveMonitor(controller);
        return 0;
    }

    // Process commands
    std::string command = commands[0];
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);

    if (command == "start")
    {
        if (commands.size() > 1)
        {
            // Start specified server
            std::string serverName = commands[1];
            std::cout << "Starting server: " << serverName << std::endl;
            bool success = controller.StartServer(serverName);
            if (success)
            {
                std::cout << "Server started successfully: " << serverName << std::endl;
            }
            else
            {
                std::cerr << "Server startup failed: " << serverName << std::endl;
                return 1;
            }
        }
        else
        {
            // Start all servers
            std::cout << "Starting all servers..." << std::endl;
            bool success = controller.StartAllServers();
            if (success)
            {
                std::cout << "All servers started successfully" << std::endl;
            }
            else
            {
                std::cerr << "Error occurred during server startup process" << std::endl;
                return 1;
            }
        }
    }
    else if (command == "stop")
    {
        if (commands.size() > 1)
        {
            // Stop specified server
            std::string serverName = commands[1];
            std::cout << "Stopping server: " << serverName << std::endl;
            bool success = controller.StopServer(serverName);
            if (success)
            {
                std::cout << "Server stopped successfully: " << serverName << std::endl;
            }
            else
            {
                std::cerr << "Server stop failed: " << serverName << std::endl;
                return 1;
            }
        }
        else
        {
            // Stop all servers
            std::cout << "Stopping all servers..." << std::endl;
            bool success = controller.StopAllServers();
            if (success)
            {
                std::cout << "All servers stopped successfully" << std::endl;
            }
            else
            {
                std::cerr << "Error occurred during server stop process" << std::endl;
                return 1;
            }
        }
    }
    else if (command == "restart")
    {
        if (commands.size() > 1)
        {
            // Restart specified server
            std::string serverName = commands[1];
            std::cout << "Restarting server: " << serverName << std::endl;
            bool success = controller.RestartServer(serverName);
            if (success)
            {
                std::cout << "Server restarted successfully: " << serverName << std::endl;
            }
            else
            {
                std::cerr << "Server restart failed: " << serverName << std::endl;
                return 1;
            }
        }
        else
        {
            // Restart all servers
            std::cout << "Restarting all servers..." << std::endl;
            bool success = controller.RestartAllServers();
            if (success)
            {
                std::cout << "All servers restarted successfully" << std::endl;
            }
            else
            {
                std::cerr << "Error occurred during server restart process" << std::endl;
                return 1;
            }
        }
    }
    else if (command == "reload")
    {
        if (commands.size() > 1)
        {
            // Reload specified server configuration
            std::string serverName = commands[1];
            std::cout << "Reloading server configuration: " << serverName << std::endl;
            bool success = controller.ReloadServer(serverName);
            if (success)
            {
                std::cout << "Server configuration reloaded successfully: " << serverName << std::endl;
            }
            else
            {
                std::cerr << "Server configuration reload failed: " << serverName << std::endl;
                return 1;
            }
        }
        else
        {
            std::cerr << "Error: reload command requires specifying server name" << std::endl;
            return 1;
        }
    }
    else if (command == "status")
    {
        if (commands.size() > 1)
        {
            // View specified server status
            std::string serverName = commands[1];
            NFServerStatus status = controller.GetServerStatus(serverName);
            std::cout << "Server " << serverName << " status: " << StatusToString(status) << std::endl;
        }
        else
        {
            // View all server status
            PrintServerStatus(controller);
        }
    }
    else if (command == "monitor")
    {
        // Start monitoring mode
        PrintServerStatus(controller);
        InteractiveMonitor(controller);
    }
    else if (command == "list")
    {
        // List all servers
        auto serverList = controller.GetServerList();
        std::cout << "\nAvailable servers:" << std::endl;
        for (const auto& server : serverList)
        {
            NFServerStatus status = controller.GetServerStatus(server);
            std::cout << "  " << std::left << std::setw(20) << server
                << StatusToString(status) << std::endl;
        }
    }
    else
    {
        std::cerr << "Error: Unknown command " << command << std::endl;
        controller.PrintHelp();
        return 1;
    }

    return 0;
}
