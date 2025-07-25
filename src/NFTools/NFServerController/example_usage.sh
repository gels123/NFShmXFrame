#!/bin/bash

# NFServerController v2.0 Usage Examples
# This script demonstrates the new wildcard pattern matching functionality

echo "=== NFServerController v2.0 Usage Examples ==="
echo ""

# Set the executable path (adjust as needed)
CONTROLLER="./NFServerController"

echo "1. Show all server status:"
echo "Command: $CONTROLLER \"check\""
echo ""

echo "2. Start all servers:"
echo "Command: $CONTROLLER \"start *.*.*.*\""
echo ""

echo "3. Restart all game servers (type 10):"
echo "Command: $CONTROLLER \"restart *.*.10.*\""
echo "Note: Uses --restart parameter directly, no stop+start needed"
echo ""

echo "4. Stop specific server:"
echo "Command: $CONTROLLER \"stop 1.13.1.1\""
echo ""

echo "5. Reload server configuration (hot update):"
echo "Command: $CONTROLLER \"reload *.*.10.*\""
echo "Note: Reloads configuration without restarting the server process"
echo ""



echo "6. Check status of all servers in region 13:"
echo "Command: $CONTROLLER \"check *.13.*.*\""
echo ""

echo "7. Start all logic and game servers (types 9 and 10):"
echo "Command: $CONTROLLER \"start *.*.9.*\""
echo "Command: $CONTROLLER \"start *.*.10.*\""
echo ""

echo "8. Show help information:"
echo "Command: $CONTROLLER --help"
echo ""

echo "=== Pattern Matching Examples ==="
echo ""
echo "Pattern: *.*.*.*     - Matches all servers"
echo "Pattern: *.*.5.*     - Matches all servers with ServerType 5 (ProxyServer)"
echo "Pattern: 1.13.*.*    - Matches all servers in World 1, Region 13"
echo "Pattern: *.*.1?.*    - Use ? for single character wildcard (if supported)"
echo ""

echo "=== Server Type Reference ==="
echo ""
echo "Type 1  - MasterServer"
echo "Type 3  - RouteAgentServer"
echo "Type 5  - ProxyServer"
echo "Type 6  - StoreServer"
echo "Type 7  - LoginServer"
echo "Type 8  - WorldServer"
echo "Type 9  - LogicServer"
echo "Type 10 - GameServer"
echo ""

echo "=== Batch Operations Example ==="
echo ""
echo "# Complete server restart sequence:"
echo "1. $CONTROLLER \"restart *.*.*.*\"      # Restart all servers (uses --restart)"
echo ""
echo "# Configuration hot reload sequence:"
echo "1. $CONTROLLER \"reload *.*.10.*\"      # Reload all game servers config"
echo "2. $CONTROLLER \"reload *.*.9.*\"       # Reload all logic servers config"
echo "Note: Each server restarts independently, no stop+start"
echo ""

echo "# Selective restart of backend services:"
echo "1. $CONTROLLER \"restart *.*.6.*\"    # Restart all StoreServers"
echo "2. $CONTROLLER \"restart *.*.9.*\"    # Restart all LogicServers"
echo "3. $CONTROLLER \"restart *.*.10.*\"   # Restart all GameServers"
echo "Note: All restart operations use --restart parameter"
echo ""

echo "=== Configuration File Format ==="
echo ""
echo "Each line in servers.conf should follow this format:"
echo "ServerName|ServerID|ConfigPath|PluginPath|LuaScriptPath|LogPath|GameName|ExecutablePath|WorkingDir"
echo ""
echo "Example:"
echo "MasterServer|1.13.1.1|../Install/Config|../Install/LieRenPlugin|../Install/LuaScript|../Install/logs|LieRen|../Install/Bin/NFServerStatic|../Install"
echo ""

echo "=== Command Line Options ==="
echo ""
echo "-c, --config <file>  Specify configuration file path"
echo "-v, --verbose        Enable verbose logging"
echo "-q, --quiet          Enable quiet mode (minimal output)"
echo "-h, --help           Show help information"
echo ""

echo "=== Tips ==="
echo ""
echo "1. Always enclose commands with quotes, especially when using wildcards"
echo "2. Use -v flag for detailed operation logs during troubleshooting"
echo "3. Servers start/stop in configuration file order (forward/reverse)"
echo "4. Make sure server dependencies are ordered correctly in config file"
echo "5. Wildcard patterns are case-sensitive"
echo "6. Server IDs must follow the WorldID.RegionID.ServerType.Index format"
echo ""

echo "=== Configuration File Order Example ==="
echo ""
echo "Recommended order in servers.conf:"
echo "1. MasterServer|1.13.1.1|..."
echo "2. RouteAgentServer|1.13.3.1|..."
echo "3. StoreServer|1.13.6.1|..."
echo "4. LoginServer|1.13.7.1|..."
echo "5. WorldServer|1.13.8.1|..."
echo "6. LogicServer|1.13.9.1|..."
echo "7. GameServer|1.13.10.1|..."
echo "8. ProxyServer|1.13.5.1|..."
echo ""
echo "Startup order: 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> 8"
echo "Shutdown order: 8 -> 7 -> 6 -> 5 -> 4 -> 3 -> 2 -> 1"
echo "" 