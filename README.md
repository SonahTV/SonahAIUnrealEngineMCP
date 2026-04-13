<div align="center">

# SonahAI Unreal Engine MCP

**95+ tools for fully autonomous UE5 development via Model Context Protocol**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.7-orange)](https://www.unrealengine.com)
[![Python](https://img.shields.io/badge/Python-3.10%2B-yellow)](https://www.python.org)

</div>

## What is this?

An MCP (Model Context Protocol) server that gives AI assistants (Claude Code, Cursor, etc.) **direct hands-on control** of the Unreal Engine 5 editor. Create blueprints, spawn actors, edit materials, build animation state machines, capture viewport screenshots, manage assets, run PIE sessions, read logs -- all programmatically through 95+ tools.

Forked from [chongdashu/unreal-mcp](https://github.com/chongdashu/unreal-mcp) and massively extended.

## Tool Categories

| Category | Tools | What it does |
|----------|-------|-------------|
| **Actors & Level** | 15 | Spawn, delete, transform, get properties, find by name, save level |
| **Blueprints** | 12 | Create, compile, add components, set properties, spawn actors from BPs |
| **Blueprint Introspection** | 8 | Inspect, dump graph, list variables/functions, search nodes, find references |
| **Blueprint Nodes** | 12 | Add events, functions, branches, variables, connect/disconnect, delete nodes |
| **Materials** | 8 | Create materials/instances, set scalar/vector params, assign to actors, inspect |
| **Assets** | 7 | Search, dependencies, referencers, duplicate, rename, move, reimport |
| **Character** | 3 | Create character BPs, assign AnimBP, set movement params |
| **Animation** | 9 | Create AnimBP, state machines, states, transitions, transition rules, notifies |
| **Enhanced Input** | 3 | Create/list/remove input mappings |
| **UMG/Widgets** | 6 | Create widgets, add text/buttons, bind events, add to viewport |
| **Viewport** | 2 | Capture viewport screenshot (returns base64 PNG), focus viewport |
| **Build & Run** | 6 | Compile, hot reload, full rebuild cycle, PIE start/stop, console commands |
| **Async Tasks** | 5 | Submit long-running ops, poll status, get results, list/cancel tasks |
| **UE5 Context** | 1 | Curated cheatsheets for animation, blueprints, actors, assets, replication, slate |
| **Project** | 4 | Get project info, list assets, read logs, create projects |

## Quick Start

### 1. Install the UE5 Plugin

Copy the `Plugin/` folder into your Unreal project:

```
YourProject/
  Plugins/
    UnrealMCP/
      Source/
      UnrealMCP.uplugin
```

Rebuild your project. The plugin starts a TCP bridge on port 55557 when the editor loads.

### 2. Install the Python MCP Server

```bash
cd Python/
uv run --python 3.13 unreal_mcp_server.py
```

### 3. Configure Claude Code

Add to your project's `.mcp.json`:

```json
{
  "mcpServers": {
    "SonahAIUnrealEngineMCP": {
      "command": "uv",
      "args": [
        "--directory", "/path/to/SonahAIUnrealEngineMCP/Python",
        "run", "--python", "3.13",
        "unreal_mcp_server.py"
      ]
    }
  }
}
```

## Architecture

```
Claude Code / Cursor / AI Assistant
        |
        | (MCP protocol over stdio)
        v
  Python MCP Server (FastMCP)
        |
        | (TCP JSON over port 55557)
        v
  C++ UE5 Plugin (UnrealMCPBridge)
        |
        | (UE5 Editor APIs)
        v
  Unreal Engine 5 Editor
```

## Requirements

- Unreal Engine 5.7 (may work with 5.5+, tested on 5.7)
- Python 3.10+ with `uv`
- An MCP-compatible AI client (Claude Code, Cursor, etc.)

## Credits

- Originally forked from [chongdashu/unreal-mcp](https://github.com/chongdashu/unreal-mcp)
- Extended by SonahAI with 40+ additional tools, UE 5.7 compatibility fixes, and full AnimBP support

## License

MIT
