"""Animation Blueprint tools for SonahUnrealEngineMCP.

State machine creation, states, transitions, notifies, and AnimBP inspection.
"""

import logging
from typing import Any, Dict

from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_anim_tools(mcp: FastMCP):
    @mcp.tool()
    def create_anim_blueprint(ctx: Context, name: str, skeleton_path: str, save_path: str = "/Game/Animations") -> Dict[str, Any]:
        """Create a new Animation Blueprint for a given skeleton.

        Args:
            name: Name for the new AnimBP.
            skeleton_path: Asset path to the USkeleton (e.g. "/Game/Characters/Mannequin/Mesh/SK_Mannequin_Skeleton").
            save_path: Content browser folder. Defaults to /Game/Animations.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("create_anim_blueprint", {
                "name": name, "skeleton_path": skeleton_path, "save_path": save_path
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error creating anim blueprint: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def create_state_machine(ctx: Context, anim_bp: str, name: str = "Locomotion") -> Dict[str, Any]:
        """Create a state machine node inside an AnimBP's AnimGraph.

        Args:
            anim_bp: Name or path of the AnimBP.
            name: Name for the state machine (default: Locomotion).
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("create_state_machine", {"anim_bp": anim_bp, "name": name})
            return response or {}
        except Exception as e:
            logger.error(f"Error creating state machine: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_anim_state(ctx: Context, anim_bp: str, state_name: str, pos_x: float = 200, pos_y: float = 0) -> Dict[str, Any]:
        """Add a state to the first state machine in an AnimBP.

        Args:
            anim_bp: Name or path of the AnimBP.
            state_name: Name for the new state (e.g. "Idle", "Run").
            pos_x: X position in the state machine graph.
            pos_y: Y position in the state machine graph.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_anim_state", {
                "anim_bp": anim_bp, "state_name": state_name, "pos_x": pos_x, "pos_y": pos_y
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error adding anim state: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def create_anim_transition(ctx: Context, anim_bp: str, from_state: str, to_state: str) -> Dict[str, Any]:
        """Create a transition between two states in an AnimBP's state machine.

        Args:
            anim_bp: Name or path of the AnimBP.
            from_state: Name of the source state.
            to_state: Name of the destination state.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("create_anim_transition", {
                "anim_bp": anim_bp, "from_state": from_state, "to_state": to_state
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error creating anim transition: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_anim_notify(ctx: Context, anim_sequence_path: str, notify_name: str, time: float = 0.0) -> Dict[str, Any]:
        """Add a notify event to an animation sequence at a given time.

        Args:
            anim_sequence_path: Asset path to the UAnimSequence.
            notify_name: Name for the notify.
            time: Time in seconds within the animation to place the notify.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_anim_notify", {
                "anim_sequence_path": anim_sequence_path, "notify_name": notify_name, "time": time
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error adding anim notify: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def list_anim_states(ctx: Context, anim_bp: str) -> Dict[str, Any]:
        """List all states in an AnimBP's state machines."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("list_anim_states", {"anim_bp": anim_bp})
            return response or {}
        except Exception as e:
            logger.error(f"Error listing anim states: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def inspect_anim_blueprint(ctx: Context, anim_bp: str) -> Dict[str, Any]:
        """Inspect an AnimBP: skeleton, graphs, variables."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("inspect_anim_blueprint", {"anim_bp": anim_bp})
            return response or {}
        except Exception as e:
            logger.error(f"Error inspecting anim blueprint: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_state_animation(ctx: Context, anim_bp: str, state_name: str, anim_sequence_path: str) -> Dict[str, Any]:
        """Set the animation sequence for a state in an AnimBP's state machine.
        NOTE: This is not yet fully automated. Currently returns an error message directing manual setup.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_state_animation", {
                "anim_bp": anim_bp, "state_name": state_name, "anim_sequence_path": anim_sequence_path
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error setting state animation: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_transition_rule(ctx: Context, anim_bp: str, from_state: str, to_state: str, bool_variable: str) -> Dict[str, Any]:
        """Set a transition rule by wiring a bool variable to the transition's result.

        Args:
            anim_bp: Name or path of the AnimBP.
            from_state: Source state name.
            to_state: Destination state name.
            bool_variable: Name of a bool variable on the AnimBP to drive the transition.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_transition_rule", {
                "anim_bp": anim_bp, "from_state": from_state, "to_state": to_state, "bool_variable": bool_variable
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error setting transition rule: {e}")
            return {"success": False, "message": str(e)}

    logger.info("Anim tools registered successfully")
