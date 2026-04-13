"""Async task queue tools for SonahUnrealEngineMCP.

Submit long-running operations and poll for results without blocking the MCP socket.
"""

import logging
from typing import Any, Dict

from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_task_tools(mcp: FastMCP):
    @mcp.tool()
    def task_submit(ctx: Context, task_type: str) -> Dict[str, Any]:
        """Submit a long-running task for async execution. Returns immediately with a task_id.

        Args:
            task_type: The command type to execute (e.g. "full_rebuild_cycle").

        Returns:
            Dict with task_id and status "pending".
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("task_submit", {"task_type": task_type})
            return response or {}
        except Exception as e:
            logger.error(f"Error submitting task: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def task_status(ctx: Context, task_id: str) -> Dict[str, Any]:
        """Check the current status of an async task.

        Args:
            task_id: The GUID returned by task_submit.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("task_status", {"task_id": task_id})
            return response or {}
        except Exception as e:
            logger.error(f"Error checking task status: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def task_result(ctx: Context, task_id: str, wait_seconds: float = 0) -> Dict[str, Any]:
        """Get the result of an async task, optionally waiting up to N seconds.

        Args:
            task_id: The GUID returned by task_submit.
            wait_seconds: Seconds to block waiting for completion (0 = immediate return).
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("task_result", {"task_id": task_id, "wait_seconds": float(wait_seconds)})
            return response or {}
        except Exception as e:
            logger.error(f"Error getting task result: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def task_list(ctx: Context) -> Dict[str, Any]:
        """List all async tasks from the last hour."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("task_list", {})
            return response or {}
        except Exception as e:
            logger.error(f"Error listing tasks: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def task_cancel(ctx: Context, task_id: str) -> Dict[str, Any]:
        """Cancel a pending async task. Running tasks cannot be cancelled.

        Args:
            task_id: The GUID returned by task_submit.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("task_cancel", {"task_id": task_id})
            return response or {}
        except Exception as e:
            logger.error(f"Error cancelling task: {e}")
            return {"success": False, "message": str(e)}

    logger.info("Task tools registered successfully")
