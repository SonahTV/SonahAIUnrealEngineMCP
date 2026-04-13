"""UE5 documentation context tool — keyword-routed cheatsheets.

Pure Python; does not touch the C++ MCP bridge.
"""

import logging
from typing import Any, Dict

from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_context_tools(mcp: FastMCP):
    @mcp.tool()
    def get_ue_context(ctx: Context, topic: str = "") -> Dict[str, Any]:
        """Get a curated UE5 cheatsheet for a topic.

        Available topics: animation, blueprint, actor, assets, replication, slate.
        Pass a free-text query like "state machine transition" or "replication rpc"
        and the loader will return the most relevant cheatsheet(s).

        Args:
            topic: Free-text topic query. If empty, returns the topic list.

        Returns:
            Dict with content (markdown) and matched topics.
        """
        try:
            from contexts.loader import get_context
            return get_context(topic)
        except Exception as e:
            logger.error(f"Error loading UE context: {e}")
            return {"success": False, "message": str(e)}

    logger.info("Context tools registered successfully")
