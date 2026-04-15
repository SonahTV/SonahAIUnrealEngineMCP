"""
Project Tools for Unreal MCP.

This module provides tools for managing project-wide settings and configuration,
including build/compile, save, and workflow control.
"""

import logging
import subprocess
import os
import json
import shutil
import glob
from typing import Dict, Any, List
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

# Auto-detect paths
def _find_ue_engine_dir():
    """Find UE engine directory."""
    candidates = [
        r"C:\Program Files\Epic Games\UE_5.7",
        r"C:\Program Files\Epic Games\UE_5.6",
        r"C:\Program Files\Epic Games\UE_5.5",
        r"C:\Program Files\Epic Games\UE_5.4",
    ]
    for c in candidates:
        if os.path.isdir(c):
            return c
    return None

def _find_project_file():
    """Find the .uproject file in common locations."""
    candidates = [
        r"C:\Users\Tyron\Documents\Unreal Projects\InkGames\InkGames.uproject",
        r"C:\Users\Tyron\Documents\Unreal Projects\MyProject\MyProject.uproject",
    ]
    for c in candidates:
        if os.path.isfile(c):
            return c
    # Search Documents/Unreal Projects for any .uproject
    base = r"C:\Users\Tyron\Documents\Unreal Projects"
    if os.path.isdir(base):
        for d in os.listdir(base):
            proj_dir = os.path.join(base, d)
            if os.path.isdir(proj_dir):
                for f in os.listdir(proj_dir):
                    if f.endswith(".uproject"):
                        return os.path.join(proj_dir, f)
    return None

def register_project_tools(mcp: FastMCP):
    """Register project tools with the MCP server."""
    
    @mcp.tool()
    def create_input_mapping(
        ctx: Context,
        action_name: str,
        key: str,
        input_type: str = "Action"
    ) -> Dict[str, Any]:
        """
        Create an input mapping for the project.
        
        Args:
            action_name: Name of the input action
            key: Key to bind (SpaceBar, LeftMouseButton, etc.)
            input_type: Type of input mapping (Action or Axis)
            
        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "action_name": action_name,
                "key": key,
                "input_type": input_type
            }
            
            logger.info(f"Creating input mapping '{action_name}' with key '{key}'")
            response = unreal.send_command("create_input_mapping", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Input mapping creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error creating input mapping: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def recompile_plugin(
        ctx: Context,
        project_path: str = "",
        wait: bool = True
    ) -> Dict[str, Any]:
        """
        Recompile the Unreal C++ plugin using UnrealBuildTool.
        Triggers a build of the project so new C++ tools become available.
        After completion, UE Live Coding should hot-reload the changes.

        Args:
            project_path: Path to .uproject file (auto-detected if empty)
            wait: Whether to wait for build to complete (default True)

        Returns:
            Build result with stdout/stderr output
        """
        try:
            uproject = project_path or _find_project_file()
            if not uproject or not os.path.isfile(uproject):
                return {"success": False, "message": f"Project file not found: {uproject}"}

            engine_dir = _find_ue_engine_dir()
            if not engine_dir:
                return {"success": False, "message": "UE engine directory not found"}

            build_bat = os.path.join(engine_dir, "Engine", "Build", "BatchFiles", "Build.bat")
            if not os.path.isfile(build_bat):
                return {"success": False, "message": f"Build.bat not found at {build_bat}"}

            project_name = os.path.splitext(os.path.basename(uproject))[0]

            cmd = [
                build_bat,
                f"{project_name}Editor",
                "Win64",
                "Development",
                f"-Project={uproject}",
                "-WaitMutex",
                "-FromMsBuild"
            ]

            logger.info(f"Starting build: {' '.join(cmd)}")

            if wait:
                result = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True,
                    timeout=600,
                    cwd=os.path.dirname(uproject)
                )

                success = result.returncode == 0
                output = result.stdout[-2000:] if len(result.stdout) > 2000 else result.stdout
                errors = result.stderr[-1000:] if len(result.stderr) > 1000 else result.stderr

                logger.info(f"Build {'succeeded' if success else 'failed'} (exit code {result.returncode})")

                return {
                    "success": success,
                    "exit_code": result.returncode,
                    "output": output,
                    "errors": errors,
                    "message": "Build succeeded! UE Live Coding should hot-reload. If not, restart UE." if success else "Build failed. Check errors."
                }
            else:
                subprocess.Popen(cmd, cwd=os.path.dirname(uproject))
                return {"success": True, "message": "Build started in background. Check UE editor for hot-reload notification."}

        except subprocess.TimeoutExpired:
            return {"success": False, "message": "Build timed out after 600 seconds"}
        except Exception as e:
            return {"success": False, "message": f"Build error: {str(e)}"}

    @mcp.tool()
    def trigger_hot_reload(ctx: Context) -> Dict[str, Any]:
        """
        Trigger a Live Coding / Hot Reload in the running Unreal Editor.
        Sends a console command to the editor to recompile C++ changes.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("execute_console_command", {"command": "LiveCoding.Compile"})
            return response or {"success": True, "message": "Hot reload triggered. Check UE editor for compilation status."}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_project_info(ctx: Context) -> Dict[str, Any]:
        """
        Get information about the current Unreal project — path, engine version, plugins.
        """
        try:
            uproject = _find_project_file()
            if not uproject:
                return {"success": False, "message": "Project file not found"}

            with open(uproject, 'r') as f:
                project_data = json.load(f)

            engine_dir = _find_ue_engine_dir()

            return {
                "success": True,
                "project_name": os.path.splitext(os.path.basename(uproject))[0],
                "project_path": uproject,
                "project_dir": os.path.dirname(uproject),
                "engine_dir": engine_dir,
                "engine_version": project_data.get("EngineAssociation", "unknown"),
                "modules": project_data.get("Modules", []),
                "plugins": [p.get("Name") for p in project_data.get("Plugins", []) if p.get("Enabled", False)]
            }
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def execute_console_command(ctx: Context, command: str) -> Dict[str, Any]:
        """
        Execute an Unreal Editor console command. Useful for editor automation.

        Common commands:
        - "LiveCoding.Compile" — trigger hot reload
        - "shot.size 1920x1080" — set screenshot size
        - "stat fps" — toggle FPS display
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("execute_console_command", {"command": command})
            return response or {"success": True, "message": f"Command sent: {command}"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def list_content_assets(ctx: Context, path: str = "/Game/Blueprints") -> Dict[str, Any]:
        """
        List all assets in a content directory. Useful for discovering what exists.

        Args:
            path: Content path to list (e.g., "/Game/Blueprints", "/Game/Materials")
        """
        try:
            uproject = _find_project_file()
            if not uproject:
                return {"success": False, "message": "Project not found"}

            project_dir = os.path.dirname(uproject)
            # Convert /Game/ path to filesystem path
            relative = path.replace("/Game/", "")
            content_dir = os.path.join(project_dir, "Content", relative)

            if not os.path.isdir(content_dir):
                return {"success": True, "path": path, "assets": [], "message": "Directory does not exist"}

            assets = []
            for f in os.listdir(content_dir):
                if f.endswith(".uasset") or f.endswith(".umap"):
                    assets.append({
                        "name": os.path.splitext(f)[0],
                        "type": "map" if f.endswith(".umap") else "asset",
                        "file": f
                    })

            return {"success": True, "path": path, "assets": assets}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def create_project(
        ctx: Context,
        project_name: str = "InkCreatures",
        template: str = "ThirdPerson",
        location: str = r"C:\Users\Tyron\Documents\Unreal Projects",
        cpp: bool = True
    ) -> Dict[str, Any]:
        """
        Create a new Unreal Engine project from a template.

        Args:
            project_name: Name of the new project
            template: Template to use (ThirdPerson, FirstPerson, TopDown, Blank)
            location: Directory to create the project in
            cpp: Whether to use C++ (True) or Blueprint-only (False)

        Returns:
            Project creation result with path
        """
        try:
            engine_dir = _find_ue_engine_dir()
            if not engine_dir:
                return {"success": False, "message": "UE engine directory not found"}

            project_dir = os.path.join(location, project_name)
            uproject_path = os.path.join(project_dir, f"{project_name}.uproject")

            if os.path.exists(project_dir):
                return {"success": False, "message": f"Project directory already exists: {project_dir}"}

            # Find the template directory
            # UE stores templates in Engine/Content/Editor/Templates/
            # The actual template projects are at different locations depending on UE version
            # For UE5, templates are usually in:
            # Engine/Content/Editor/Templates/TP_ThirdPerson (for ThirdPerson)
            template_map = {
                "ThirdPerson": "TP_ThirdPerson",
                "FirstPerson": "TP_FirstPerson",
                "TopDown": "TP_TopDown",
                "Blank": "TP_Blank",
                "Puzzle": "TP_Puzzle",
                "Vehicle": "TP_VehicleAdv",
            }

            template_id = template_map.get(template, template)

            # Use UE's command-line project generation
            # UnrealVersionSelector or direct copy approach

            # Method 1: Copy template and rename
            template_search_paths = [
                os.path.join(engine_dir, "Templates", template_id),
                os.path.join(engine_dir, "Content", "Editor", "Templates", template_id),
                os.path.join(engine_dir, "Samples", template_id),
            ]

            # Also search for template .uproject files
            template_uproject = None
            for search_path in template_search_paths:
                if os.path.isdir(search_path):
                    for f in os.listdir(search_path):
                        if f.endswith(".uproject"):
                            template_uproject = os.path.join(search_path, f)
                            break
                if template_uproject:
                    break

            # If template not found via directory, try using UnrealBuildTool/UAT
            if not template_uproject:
                # Search more broadly
                pattern = os.path.join(engine_dir, "**", template_id, "*.uproject")
                results = glob.glob(pattern, recursive=True)
                if results:
                    template_uproject = results[0]

            if not template_uproject:
                # Fallback: Create project manually using .uproject JSON
                os.makedirs(project_dir, exist_ok=True)

                # Create a minimal .uproject that references the template
                uproject_content = {
                    "FileVersion": 3,
                    "EngineAssociation": "5.7",
                    "Category": "",
                    "Description": "",
                    "Modules": [
                        {
                            "Name": project_name,
                            "Type": "Runtime",
                            "LoadingPhase": "Default"
                        }
                    ] if cpp else [],
                    "Plugins": [
                        {"Name": "ModelingToolsEditorMode", "Enabled": True, "TargetAllowList": ["Editor"]}
                    ]
                }

                with open(uproject_path, 'w') as f:
                    json.dump(uproject_content, f, indent='\t')

                # If C++, create Source directory with target files
                if cpp:
                    source_dir = os.path.join(project_dir, "Source")
                    os.makedirs(source_dir, exist_ok=True)
                    module_dir = os.path.join(source_dir, project_name)
                    os.makedirs(module_dir, exist_ok=True)

                    # Target files
                    with open(os.path.join(source_dir, f"{project_name}.Target.cs"), 'w') as f:
                        f.write(f"""using UnrealBuildTool;
public class {project_name}Target : TargetRules
{{
    public {project_name}Target(TargetInfo Target) : base(Target)
    {{
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("{project_name}");
    }}
}}
""")
                    with open(os.path.join(source_dir, f"{project_name}Editor.Target.cs"), 'w') as f:
                        f.write(f"""using UnrealBuildTool;
public class {project_name}EditorTarget : TargetRules
{{
    public {project_name}EditorTarget(TargetInfo Target) : base(Target)
    {{
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("{project_name}");
    }}
}}
""")
                    # Build.cs
                    with open(os.path.join(module_dir, f"{project_name}.Build.cs"), 'w') as f:
                        f.write(f"""using UnrealBuildTool;
public class {project_name} : ModuleRules
{{
    public {project_name}(ReadOnlyTargetRules Target) : base(Target)
    {{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {{ "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" }});
    }}
}}
""")
                    # Module cpp/h
                    with open(os.path.join(module_dir, f"{project_name}.cpp"), 'w') as f:
                        f.write(f"""#include "{project_name}.h"
#include "Modules/ModuleManager.h"
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, {project_name}, "{project_name}");
""")
                    with open(os.path.join(module_dir, f"{project_name}.h"), 'w') as f:
                        f.write(f"""#pragma once
#include "CoreMinimal.h"
""")

                # Create Config directory with DefaultEngine.ini
                config_dir = os.path.join(project_dir, "Config")
                os.makedirs(config_dir, exist_ok=True)
                with open(os.path.join(config_dir, "DefaultEngine.ini"), 'w') as f:
                    f.write("[/Script/EngineSettings.GameMapsSettings]\nEditorStartupMap=/Game/Maps/Default\nGameDefaultMap=/Game/Maps/Default\n")
                with open(os.path.join(config_dir, "DefaultGame.ini"), 'w') as f:
                    f.write(f"[/Script/EngineSettings.GeneralProjectSettings]\nProjectID=\nProjectName={project_name}\n")
                with open(os.path.join(config_dir, "DefaultInput.ini"), 'w') as f:
                    f.write("[/Script/EnhancedInput.EnhancedInputDeveloperSettings]\n")
                with open(os.path.join(config_dir, "DefaultEditor.ini"), 'w') as f:
                    f.write("")

                # Create Content directory
                os.makedirs(os.path.join(project_dir, "Content", "Maps"), exist_ok=True)
                os.makedirs(os.path.join(project_dir, "Content", "Blueprints"), exist_ok=True)
                os.makedirs(os.path.join(project_dir, "Content", "Materials"), exist_ok=True)

                return {
                    "success": True,
                    "project_name": project_name,
                    "project_path": uproject_path,
                    "project_dir": project_dir,
                    "template": "manual_creation",
                    "message": f"Project created at {project_dir}. Open the .uproject file in UE and it will set up the Third Person template content on first open."
                }
            else:
                # Copy template directory
                template_dir = os.path.dirname(template_uproject)
                shutil.copytree(template_dir, project_dir)

                # Rename .uproject file
                old_name = os.path.basename(template_uproject)
                old_project_name = os.path.splitext(old_name)[0]
                os.rename(
                    os.path.join(project_dir, old_name),
                    uproject_path
                )

                # Update references in source files
                for root, dirs, files in os.walk(project_dir):
                    for fname in files:
                        if fname.endswith(('.cs', '.cpp', '.h', '.uproject', '.ini', '.Build.cs')):
                            filepath = os.path.join(root, fname)
                            try:
                                with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                                    content = f.read()
                                if old_project_name in content:
                                    content = content.replace(old_project_name, project_name)
                                    with open(filepath, 'w', encoding='utf-8') as f:
                                        f.write(content)
                            except:
                                pass
                        # Rename files containing old project name
                        if old_project_name in fname:
                            new_fname = fname.replace(old_project_name, project_name)
                            os.rename(
                                os.path.join(root, fname),
                                os.path.join(root, new_fname)
                            )
                    # Rename directories containing old project name
                    for dirname in dirs:
                        if old_project_name in dirname:
                            new_dirname = dirname.replace(old_project_name, project_name)
                            os.rename(
                                os.path.join(root, dirname),
                                os.path.join(root, new_dirname)
                            )

                return {
                    "success": True,
                    "project_name": project_name,
                    "project_path": uproject_path,
                    "template": template,
                    "message": f"Project created from {template} template at {project_dir}"
                }
        except Exception as e:
            return {"success": False, "message": f"Error creating project: {str(e)}"}

    @mcp.tool()
    def install_plugin(
        ctx: Context,
        target_project: str = "",
        plugin_source: str = ""
    ) -> Dict[str, Any]:
        """
        Install the UnrealMCP plugin into a target project by copying it.

        Args:
            target_project: Path to target .uproject file (auto-detected if empty)
            plugin_source: Path to plugin source directory (auto-detected if empty)
        """
        try:
            # Auto-detect target project
            if not target_project:
                target_project = _find_project_file()
            if not target_project or not os.path.isfile(target_project):
                return {"success": False, "message": f"Target project not found: {target_project}"}

            # Auto-detect plugin source
            if not plugin_source:
                # Look in the current InkGames project
                candidates = [
                    r"C:\Users\Tyron\Documents\Unreal Projects\InkGames\Plugins\UnrealMCP",
                    r"C:\Users\Tyron\Desktop\unreal-mcp-main\MCPGameProject\Plugins\UnrealMCP",
                ]
                for c in candidates:
                    if os.path.isdir(c) and os.path.isdir(os.path.join(c, "Source")):
                        plugin_source = c
                        break

            if not plugin_source or not os.path.isdir(plugin_source):
                return {"success": False, "message": f"Plugin source not found: {plugin_source}"}

            target_dir = os.path.dirname(target_project)
            target_plugin_dir = os.path.join(target_dir, "Plugins", "UnrealMCP")

            if os.path.exists(target_plugin_dir):
                shutil.rmtree(target_plugin_dir)

            # Copy plugin source (not binaries — they need recompiling)
            shutil.copytree(plugin_source, target_plugin_dir,
                           ignore=shutil.ignore_patterns('Binaries', 'Intermediate', '*.dll', '*.pdb'))

            # Also enable the plugin in the .uproject
            with open(target_project, 'r') as f:
                project_data = json.load(f)

            # Check if UnrealMCP is already in plugins
            plugins = project_data.get("Plugins", [])
            has_mcp = any(p.get("Name") == "UnrealMCP" for p in plugins)
            if not has_mcp:
                plugins.append({"Name": "UnrealMCP", "Enabled": True})
                project_data["Plugins"] = plugins
                with open(target_project, 'w') as f:
                    json.dump(project_data, f, indent='\t')

            return {
                "success": True,
                "plugin_installed": target_plugin_dir,
                "target_project": target_project,
                "message": f"UnrealMCP plugin installed. Project needs compilation to activate."
            }
        except Exception as e:
            return {"success": False, "message": f"Error installing plugin: {str(e)}"}

    @mcp.tool()
    def open_project(
        ctx: Context,
        project_path: str = ""
    ) -> Dict[str, Any]:
        """
        Open a .uproject file in Unreal Editor.

        Args:
            project_path: Path to .uproject file
        """
        try:
            if not project_path:
                project_path = _find_project_file()
            if not project_path or not os.path.isfile(project_path):
                return {"success": False, "message": f"Project not found: {project_path}"}

            engine_dir = _find_ue_engine_dir()
            if not engine_dir:
                return {"success": False, "message": "UE engine not found"}

            editor_path = os.path.join(engine_dir, "Engine", "Binaries", "Win64", "UnrealEditor.exe")
            if not os.path.isfile(editor_path):
                return {"success": False, "message": f"Editor not found: {editor_path}"}

            subprocess.Popen([editor_path, project_path])

            return {
                "success": True,
                "message": f"Opening {project_path} in Unreal Editor...",
                "project_path": project_path
            }
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def read_log(
        ctx: Context,
        project_path: str = "",
        lines: int = 50,
        filter_text: str = ""
    ) -> Dict[str, Any]:
        """
        Read the Unreal Editor log file. Essential for debugging — see errors, warnings,
        GameMode info, pawn spawning, blueprint compilation results, etc.

        Args:
            project_path: Path to .uproject file (auto-detected if empty)
            lines: Number of lines from the end to read (default 50)
            filter_text: Optional text to filter for (e.g., "error", "gamemode", "pawn")
        """
        try:
            uproject = project_path or _find_project_file()
            if not uproject:
                return {"success": False, "message": "Project not found"}

            project_dir = os.path.dirname(uproject)
            project_name = os.path.splitext(os.path.basename(uproject))[0]
            log_path = os.path.join(project_dir, "Saved", "Logs", f"{project_name}.log")

            if not os.path.isfile(log_path):
                return {"success": False, "message": f"Log file not found: {log_path}"}

            with open(log_path, 'r', encoding='utf-8', errors='ignore') as f:
                all_lines = f.readlines()

            if filter_text:
                filtered = [l.strip() for l in all_lines if filter_text.lower() in l.lower()]
                output = filtered[-lines:]
            else:
                output = [l.strip() for l in all_lines[-lines:]]

            return {
                "success": True,
                "log_file": log_path,
                "total_lines": len(all_lines),
                "returned_lines": len(output),
                "filter": filter_text or "none",
                "content": "\n".join(output)
            }
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def play_in_editor(
        ctx: Context,
        duration_seconds: int = 5
    ) -> Dict[str, Any]:
        """
        Start Play In Editor (PIE), wait for a duration, then stop and read the log.
        This is the automated test loop — play, observe, report.

        Args:
            duration_seconds: How long to let the game run before stopping (default 5)
        """
        import time
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            # Start PIE using native C++ command
            start_result = unreal.send_command("start_pie", {})
            if start_result and start_result.get("status") == "error":
                return {"success": False, "message": f"Failed to start PIE: {start_result.get('error', 'unknown')}"}

            # Wait for the specified duration
            time.sleep(duration_seconds)

            # Stop PIE
            unreal.send_command("stop_pie", {})
            time.sleep(1)

            # Read the log to see what happened
            uproject = _find_project_file()
            if uproject:
                project_dir = os.path.dirname(uproject)
                project_name = os.path.splitext(os.path.basename(uproject))[0]
                log_path = os.path.join(project_dir, "Saved", "Logs", f"{project_name}.log")

                if os.path.isfile(log_path):
                    with open(log_path, 'r', encoding='utf-8', errors='ignore') as f:
                        all_lines = f.readlines()

                    # Get lines from around when PIE started
                    relevant = [l.strip() for l in all_lines
                               if any(kw in l.lower() for kw in [
                                   'error', 'warning', 'gamemode', 'pawn', 'possess',
                                   'spawn', 'playevel', 'pie', 'beginplay', 'character',
                                   'playercontroller', 'defaultpawn', 'game class',
                                   'blueprint failed', 'compile'
                               ])]

                    return {
                        "success": True,
                        "message": f"PIE ran for {duration_seconds}s",
                        "relevant_log_lines": len(relevant),
                        "log": "\n".join(relevant[-40:])
                    }

            return {"success": True, "message": f"PIE ran for {duration_seconds}s, could not read log"}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def full_rebuild_cycle(
        ctx: Context,
        project_path: str = ""
    ) -> Dict[str, Any]:
        """
        Full automated rebuild cycle: close UE -> recompile -> reopen.
        No user interaction needed.

        Args:
            project_path: Path to .uproject (auto-detected if empty)
        """
        import time
        try:
            uproject = project_path or _find_project_file()
            if not uproject:
                return {"success": False, "message": "Project not found"}

            steps = []

            # Step 1: Kill UE
            result = subprocess.run(
                ["taskkill", "/F", "/IM", "UnrealEditor.exe"],
                capture_output=True, text=True
            )
            subprocess.run(
                ["taskkill", "/F", "/IM", "UnrealTraceServer.exe"],
                capture_output=True, text=True
            )
            time.sleep(3)
            steps.append("UE closed")

            # Step 2: Compile
            engine_dir = _find_ue_engine_dir()
            if not engine_dir:
                return {"success": False, "message": "Engine not found", "steps": steps}

            build_bat = os.path.join(engine_dir, "Engine", "Build", "BatchFiles", "Build.bat")
            project_name = os.path.splitext(os.path.basename(uproject))[0]

            build_result = subprocess.run(
                [build_bat, f"{project_name}Editor", "Win64", "Development",
                 f"-Project={uproject}", "-WaitMutex", "-FromMsBuild"],
                capture_output=True, text=True, timeout=600,
                cwd=os.path.dirname(uproject)
            )

            if build_result.returncode != 0:
                return {
                    "success": False,
                    "message": "Build failed",
                    "steps": steps,
                    "build_output": build_result.stdout[-1500:]
                }
            steps.append("Build succeeded")

            # Step 2.5: Clear the stale hot-reload marker that triggers "would you like to build?" dialog
            project_dir = os.path.dirname(uproject)
            hot_reload_marker = os.path.join(project_dir, "Intermediate", "Build", "BuildRules")
            if os.path.isdir(hot_reload_marker):
                import shutil
                shutil.rmtree(hot_reload_marker, ignore_errors=True)
                steps.append("Cleared BuildRules cache")

            # Step 3: Open UE (suppress module-rebuild dialog — we already compiled)
            editor_path = os.path.join(engine_dir, "Engine", "Binaries", "Win64", "UnrealEditor.exe")
            subprocess.Popen([editor_path, uproject, "-SkipCompilationOnStartup"])
            steps.append("UE launching")

            return {
                "success": True,
                "message": "Full rebuild cycle complete. UE is launching.",
                "steps": steps
            }
        except Exception as e:
            return {"success": False, "message": str(e)}

    # ========================================================================
    # ========================================================================
    # Stage 5 — Enhanced Input richer
    # ========================================================================

    @mcp.tool()
    def list_input_actions(ctx: Context) -> Dict[str, Any]:
        """List all legacy input action key mappings defined in the project's Input Settings."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("list_input_actions", {})
            return response or {}
        except Exception as e:
            logger.error(f"Error listing input actions: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def remove_input_mapping(ctx: Context, action_name: str, key: str = "") -> Dict[str, Any]:
        """Remove an input action mapping. If key is omitted, removes all mappings for the action name.

        Args:
            action_name: Name of the action to remove.
            key: Optional specific key to remove (e.g. "SpaceBar"). Empty = remove all keys for this action.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {"action_name": action_name}
            if key: params["key"] = key
            response = unreal.send_command("remove_input_mapping", params)
            return response or {}
        except Exception as e:
            logger.error(f"Error removing input mapping: {e}")
            return {"success": False, "message": str(e)}

    # Stage 3 — Asset operations
    # ========================================================================

    @mcp.tool()
    def asset_search(ctx: Context, query: str = "", path_filter: str = "", class_filter: str = "", limit: int = 100, offset: int = 0) -> Dict[str, Any]:
        """Search the asset registry for assets by name substring, path, or class.

        Args:
            query: Substring to match against asset names. Empty returns all matching other filters.
            path_filter: Content browser path to restrict search (e.g. "/Game/Materials").
            class_filter: UE class name to restrict (e.g. "Material", "Blueprint", "StaticMesh").
            limit: Max results to return (default 100).
            offset: Pagination offset.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {"limit": limit, "offset": offset}
            if query: params["query"] = query
            if path_filter: params["path_filter"] = path_filter
            if class_filter: params["class_filter"] = class_filter
            response = unreal.send_command("asset_search", params)
            return response or {}
        except Exception as e:
            logger.error(f"Error in asset_search: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def asset_dependencies(ctx: Context, asset_path: str) -> Dict[str, Any]:
        """List all assets that a given asset depends on (imports/references)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("asset_dependencies", {"asset_path": asset_path})
            return response or {}
        except Exception as e:
            logger.error(f"Error in asset_dependencies: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def asset_referencers(ctx: Context, asset_path: str) -> Dict[str, Any]:
        """List all assets that reference (depend on) a given asset."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("asset_referencers", {"asset_path": asset_path})
            return response or {}
        except Exception as e:
            logger.error(f"Error in asset_referencers: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def duplicate_asset(ctx: Context, source_path: str, dest_name: str, dest_folder: str = "") -> Dict[str, Any]:
        """Duplicate an asset to a new name and/or folder.

        Args:
            source_path: Full path of the source asset (e.g. "/Game/Materials/M_Base").
            dest_name: Name for the duplicated asset.
            dest_folder: Destination folder. Defaults to same folder as source.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {"source_path": source_path, "dest_name": dest_name}
            if dest_folder: params["dest_folder"] = dest_folder
            response = unreal.send_command("duplicate_asset", params)
            return response or {}
        except Exception as e:
            logger.error(f"Error in duplicate_asset: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def rename_asset(ctx: Context, source_path: str, new_name: str) -> Dict[str, Any]:
        """Rename an asset in place (fixes redirectors)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("rename_asset", {"source_path": source_path, "new_name": new_name})
            return response or {}
        except Exception as e:
            logger.error(f"Error in rename_asset: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def move_asset(ctx: Context, source_path: str, dest_folder: str) -> Dict[str, Any]:
        """Move an asset to a different folder (fixes redirectors)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("move_asset", {"source_path": source_path, "dest_folder": dest_folder})
            return response or {}
        except Exception as e:
            logger.error(f"Error in move_asset: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def reimport_asset(ctx: Context, asset_path: str) -> Dict[str, Any]:
        """Reimport an asset from its original source file (textures, meshes, etc.)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("reimport_asset", {"asset_path": asset_path})
            return response or {}
        except Exception as e:
            logger.error(f"Error in reimport_asset: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def import_fbx_batch(
        ctx: Context,
        source_folder: str,
        dest_package: str,
        as_skeletal: bool = False,
        combine_meshes: bool = True,
    ) -> Dict[str, Any]:
        """
        Import every .fbx under source_folder into UE at dest_package.
        Uses per-file serialized imports + legacy FBX pipeline (options.bAutomatedImportShouldDetectType=False)
        to avoid the UE 5.7 Interchange async crash that kills full-folder batch imports.

        Args:
            source_folder: Absolute filesystem path containing .fbx files.
            dest_package: UE content-browser path ("/Game/Characters").
            as_skeletal: True for rigged characters, False for static props.
            combine_meshes: Combine multiple meshes in a single fbx into one static mesh.

        Returns: {"success": bool, "imported": int, "skipped": int, "errors": [...]}
        """
        from unreal_mcp_server import get_unreal_connection

        # Guard against the /Game/Characters mount which maps to engine
        # TemplateResources on Third-Person-derived projects.
        if dest_package.rstrip("/").lower() == "/game/characters":
            logger.warning("import_fbx_batch: /Game/Characters is mounted to engine templates; use /Game/InkChars or similar.")
            return {"success": False, "message": "/Game/Characters is a virtual engine mount; choose a different package like /Game/InkChars"}

        script_path = os.path.join(
            os.path.dirname(_find_project_file() or ""),
            "Content", "Python", "_mcp_import_fbx.py"
        )
        os.makedirs(os.path.dirname(script_path), exist_ok=True)
        with open(script_path, "w", encoding="utf-8") as f:
            f.write(f'''"""Auto-generated by import_fbx_batch MCP tool. Deferred per-tick import."""
import os, unreal

SRC = r"{source_folder}"
DST = r"{dest_package}"
AS_SKELETAL = {str(as_skeletal)}
COMBINE = {str(combine_meshes)}

_state = {{"handle": None, "done": False}}


def _do_import():
    if _state["done"]: return
    _state["done"] = True
    if _state["handle"] is not None:
        try: unreal.unregister_slate_post_tick_callback(_state["handle"])
        except Exception: pass
    if not os.path.isdir(SRC):
        unreal.log_error(f"[import_fbx] missing {{SRC}}"); return
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    imported = skipped = failed = 0
    for name in sorted(os.listdir(SRC)):
        if not name.lower().endswith(".fbx"): continue
        base = os.path.splitext(name)[0]
        if unreal.EditorAssetLibrary.does_asset_exist(f"{{DST}}/{{base}}"):
            skipped += 1; continue
        try:
            opts = unreal.FbxImportUI()
            opts.import_mesh = True
            opts.import_as_skeletal = AS_SKELETAL
            opts.import_materials = True
            opts.import_textures = True
            opts.automated_import_should_detect_type = False
            if AS_SKELETAL:
                opts.original_import_type = unreal.FBXImportType.FBXIT_SKELETAL_MESH
            else:
                opts.original_import_type = unreal.FBXImportType.FBXIT_STATIC_MESH
                opts.static_mesh_import_data.combine_meshes = COMBINE
            task = unreal.AssetImportTask()
            task.filename = os.path.join(SRC, name)
            task.destination_path = DST
            task.destination_name = base
            task.replace_existing = True
            task.save = True
            task.automated = True
            task.options = opts
            tools.import_asset_tasks([task])
            imported += 1
        except Exception as e:
            failed += 1
            unreal.log_error(f"[import_fbx] {{base}} failed: {{e}}")
    try: unreal.EditorAssetLibrary.save_directory(DST, recursive=True, only_if_is_dirty=True)
    except Exception as e: unreal.log_warning(f"[import_fbx] save failed: {{e}}")
    unreal.log(f"[import_fbx] DONE: {{imported}} imported, {{skipped}} skipped, {{failed}} failed")


_state["handle"] = unreal.register_slate_post_tick_callback(lambda dt: _do_import())
unreal.log("[import_fbx] deferred to next tick")
''')
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("execute_console_command", {"command": f'py "{script_path}"'})
            return {"success": True, "script": script_path, "response": response or {}}
        except Exception as e:
            logger.error(f"Error in import_fbx_batch: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def import_kenney_all(
        ctx: Context,
        raw_root: str = "",
        chars_pkg: str = "/Game/InkChars",
        weapons_pkg: str = "/Game/InkWeapons",
        props_pkg: str = "/Game/InkProps",
    ) -> Dict[str, Any]:
        """
        One-shot: import all staged Kenney CC0 content into UE without
        human-in-the-loop. Uses the safe deferred per-tick pattern, so
        UE's Interchange task graph stays stable.

        Imports:
          - Content/Audio/{UI,SFX,Footsteps}/*.ogg -> USoundWave .uasset
          - ThirdParty/raw/blocky-characters -> chars_pkg (static meshes)
          - ThirdParty/raw/blaster-kit       -> weapons_pkg
          - ThirdParty/raw/platformer-kit    -> props_pkg

        Args:
            raw_root: override Content/ThirdParty/raw path (autodetect if empty).
            chars_pkg / weapons_pkg / props_pkg: UE content paths.
                NOTE: /Game/Characters is a virtual mount to engine templates
                on Third-Person-derived projects. Keep the /InkXxx default.

        Returns {"success": bool, "scheduled": {"audio": n, "chars": n, ...}}.
        """
        from unreal_mcp_server import get_unreal_connection
        proj_dir = os.path.dirname(_find_project_file() or "")
        if not proj_dir:
            return {"success": False, "message": "project not found"}
        content = os.path.join(proj_dir, "Content")
        if not raw_root:
            raw_root = os.path.join(content, "ThirdParty", "raw")

        script_path = os.path.join(content, "Python", "_mcp_import_kenney_all.py")
        os.makedirs(os.path.dirname(script_path), exist_ok=True)
        with open(script_path, "w", encoding="utf-8") as f:
            f.write(f'''"""Auto-generated by import_kenney_all MCP tool."""
import os, unreal

CONTENT = r"{content}"
RAW     = r"{raw_root}"
CHARS   = r"{chars_pkg}"
WEAPONS = r"{weapons_pkg}"
PROPS   = r"{props_pkg}"

_state = {{"handle": None, "done": False}}

def _import_oggs(folder_rel):
    src = os.path.join(CONTENT, folder_rel)
    if not os.path.isdir(src): return 0
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    n = 0
    dst = "/Game/" + folder_rel.replace(os.sep, "/")
    for name in sorted(os.listdir(src)):
        if not name.lower().endswith(".ogg"): continue
        base = os.path.splitext(name)[0]
        if unreal.EditorAssetLibrary.does_asset_exist(f"{{dst}}/{{base}}"): continue
        task = unreal.AssetImportTask()
        task.filename = os.path.join(src, name)
        task.destination_path = dst
        task.destination_name = base
        task.replace_existing = True
        task.save = True
        task.automated = True
        tools.import_asset_tasks([task])
        n += 1
    return n

def _import_fbxs(src_abs, dst_pkg, as_skel=False):
    if not os.path.isdir(src_abs): return 0
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    n = 0
    for name in sorted(os.listdir(src_abs)):
        if not name.lower().endswith(".fbx"): continue
        base = os.path.splitext(name)[0]
        if unreal.EditorAssetLibrary.does_asset_exist(f"{{dst_pkg}}/{{base}}"): continue
        opts = unreal.FbxImportUI()
        opts.import_mesh = True
        opts.import_as_skeletal = as_skel
        opts.import_materials = True
        opts.import_textures = True
        opts.automated_import_should_detect_type = False
        opts.original_import_type = (unreal.FBXImportType.FBXIT_SKELETAL_MESH if as_skel
                                     else unreal.FBXImportType.FBXIT_STATIC_MESH)
        if not as_skel:
            opts.static_mesh_import_data.combine_meshes = True
        task = unreal.AssetImportTask()
        task.filename = os.path.join(src_abs, name)
        task.destination_path = dst_pkg
        task.destination_name = base
        task.replace_existing = True
        task.save = True
        task.automated = True
        task.options = opts
        tools.import_asset_tasks([task])
        n += 1
    return n

def _do_work():
    if _state["done"]: return
    _state["done"] = True
    if _state["handle"] is not None:
        try: unreal.unregister_slate_post_tick_callback(_state["handle"])
        except Exception: pass

    report = {{}}
    # Audio first — safe, synchronous, small.
    report["audio_ui"]       = _import_oggs("Audio/UI")
    report["audio_sfx"]      = _import_oggs("Audio/SFX")
    report["audio_footsteps"]= _import_oggs("Audio/Footsteps")
    # FBX. Static meshes; skeletal import requires separate rig auth.
    report["chars"]  = _import_fbxs(os.path.join(RAW, "blocky-characters", "Models", "FBX format"), CHARS)
    report["weapons"]= _import_fbxs(os.path.join(RAW, "blaster-kit",       "Models", "FBX format"), WEAPONS)
    report["props"]  = _import_fbxs(os.path.join(RAW, "platformer-kit",    "Models", "FBX format"), PROPS)
    # Flush
    for pkg in ("/Game/Audio", CHARS, WEAPONS, PROPS):
        try: unreal.EditorAssetLibrary.save_directory(pkg, recursive=True, only_if_is_dirty=True)
        except Exception: pass
    unreal.log(f"[kenney_all] DONE: {{report}}")


_state["handle"] = unreal.register_slate_post_tick_callback(lambda dt: _do_work())
unreal.log("[kenney_all] scheduled")
''')
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("execute_console_command", {"command": f'py "{script_path}"'})
            return {
                "success": True,
                "scheduled": True,
                "script": script_path,
                "response": response or {},
                "message": "Kenney import scheduled on next tick. Check UE log for [kenney_all] DONE line with counts.",
            }
        except Exception as e:
            logger.error(f"Error in import_kenney_all: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def package_project(
        ctx: Context,
        output_dir: str = "",
        config: str = "Shipping",
        platform: str = "Win64",
    ) -> Dict[str, Any]:
        """
        Package the UE project to a shippable build using RunUAT BuildCookRun.
        Runs in the background; poll with task_status.

        Args:
            output_dir: Where to archive the build (default Desktop/InkCreatures_Shipping).
            config: "Shipping" (default), "Development", "Test".
            platform: "Win64" (default), "Mac", "Linux".

        Returns: {"success": bool, "output_dir": str, "status": "started"|"error"}
        """
        try:
            uproject = _find_project_file()
            if not uproject:
                return {"success": False, "message": "Project file not found"}
            engine_dir = _find_ue_engine_dir()
            if not engine_dir:
                return {"success": False, "message": "UE engine directory not found"}
            if not output_dir:
                output_dir = os.path.join(os.path.expanduser("~"), "Desktop", "InkCreatures_Shipping")
            os.makedirs(output_dir, exist_ok=True)
            run_uat = os.path.join(engine_dir, "Engine", "Build", "BatchFiles", "RunUAT.bat")
            if not os.path.isfile(run_uat):
                return {"success": False, "message": f"RunUAT.bat not found: {run_uat}"}

            log_path = os.path.join(output_dir, "package.log")
            cmd = [
                run_uat, "BuildCookRun",
                f"-project={uproject}",
                "-noP4",
                f"-platform={platform}",
                f"-clientconfig={config}",
                "-cook", "-allmaps", "-build", "-stage", "-pak", "-archive",
                f"-archivedirectory={output_dir}",
                "-utf8output",
            ]
            logger.info(f"Packaging: {' '.join(cmd)}")
            with open(log_path, "w") as logf:
                p = subprocess.Popen(cmd, stdout=logf, stderr=subprocess.STDOUT, cwd=os.path.dirname(uproject))
            return {
                "success": True,
                "status": "started",
                "pid": p.pid,
                "output_dir": output_dir,
                "log": log_path,
                "message": "Packaging started in background. Tail the log to monitor; this takes 10-20 minutes.",
            }
        except Exception as e:
            logger.error(f"Error in package_project: {e}")
            return {"success": False, "message": str(e)}

    logger.info("Project tools registered successfully")