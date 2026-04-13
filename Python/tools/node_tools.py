"""
Blueprint Node Tools for Unreal MCP.

This module provides tools for manipulating Blueprint graph nodes and connections.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_blueprint_node_tools(mcp: FastMCP):
    """Register Blueprint node manipulation tools with the MCP server."""
    
    @mcp.tool()
    def add_blueprint_event_node(
        ctx: Context,
        blueprint_name: str,
        event_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add an event node to a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            event_name: Name of the event. Use 'Receive' prefix for standard events:
                       - 'ReceiveBeginPlay' for Begin Play
                       - 'ReceiveTick' for Tick
                       - etc.
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            # Handle default value within the method body
            if node_position is None:
                node_position = [0, 0]
            
            params = {
                "blueprint_name": blueprint_name,
                "event_name": event_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding event node '{event_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_event_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Event node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding event node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_input_action_node(
        ctx: Context,
        blueprint_name: str,
        action_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add an input action event node to a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            action_name: Name of the input action to respond to
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            # Handle default value within the method body
            if node_position is None:
                node_position = [0, 0]
            
            params = {
                "blueprint_name": blueprint_name,
                "action_name": action_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding input action node for '{action_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_input_action_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Input action node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding input action node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_function_node(
        ctx: Context,
        blueprint_name: str,
        target: str,
        function_name: str,
        params = None,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a function call node to a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            target: Target object for the function (component name or self)
            function_name: Name of the function to call
            params: Optional parameters to set on the function node
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            # Handle default values within the method body
            if params is None:
                params = {}
            if node_position is None:
                node_position = [0, 0]
            
            command_params = {
                "blueprint_name": blueprint_name,
                "target": target,
                "function_name": function_name,
                "params": params,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding function node '{function_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_function_node", command_params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Function node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding function node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
            
    @mcp.tool()
    def connect_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        source_node_id: str,
        source_pin: str,
        target_node_id: str,
        target_pin: str
    ) -> Dict[str, Any]:
        """
        Connect two nodes in a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            source_node_id: ID of the source node
            source_pin: Name of the output pin on the source node
            target_node_id: ID of the target node
            target_pin: Name of the input pin on the target node
            
        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            params = {
                "blueprint_name": blueprint_name,
                "source_node_id": source_node_id,
                "source_pin": source_pin,
                "target_node_id": target_node_id,
                "target_pin": target_pin
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Connecting nodes in blueprint '{blueprint_name}'")
            response = unreal.send_command("connect_blueprint_nodes", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Node connection response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error connecting nodes: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_variable(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        variable_type: str,
        is_exposed: bool = False
    ) -> Dict[str, Any]:
        """
        Add a variable to a Blueprint.
        
        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable
            variable_type: Type of the variable (Boolean, Integer, Float, Vector, etc.)
            is_exposed: Whether to expose the variable to the editor
            
        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            params = {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name,
                "variable_type": variable_type,
                "is_exposed": is_exposed
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding variable '{variable_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_variable", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Variable creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding variable: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_get_self_component_reference(
        ctx: Context,
        blueprint_name: str,
        component_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a node that gets a reference to a component owned by the current Blueprint.
        This creates a node similar to what you get when dragging a component from the Components panel.
        
        Args:
            blueprint_name: Name of the target Blueprint
            component_name: Name of the component to get a reference to
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            # Handle None case explicitly in the function
            if node_position is None:
                node_position = [0, 0]
            
            params = {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding self component reference node for '{component_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_get_self_component_reference", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Self component reference node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding self component reference node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_self_reference(
        ctx: Context,
        blueprint_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a 'Get Self' node to a Blueprint's event graph that returns a reference to this actor.
        
        Args:
            blueprint_name: Name of the target Blueprint
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            if node_position is None:
                node_position = [0, 0]
                
            params = {
                "blueprint_name": blueprint_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding self reference node to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_self_reference", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Self reference node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding self reference node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def find_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        node_type = None,
        event_type = None
    ) -> Dict[str, Any]:
        """
        Find nodes in a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            node_type: Optional type of node to find (Event, Function, Variable, etc.)
            event_type: Optional specific event type to find (BeginPlay, Tick, etc.)
            
        Returns:
            Response containing array of found node IDs and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            params = {
                "blueprint_name": blueprint_name,
                "node_type": node_type,
                "event_type": event_type
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Finding nodes in blueprint '{blueprint_name}'")
            response = unreal.send_command("find_blueprint_nodes", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Node find response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error finding nodes: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_branch_node(
        ctx: Context,
        blueprint_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a Branch (If/Then/Else) node to a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            node_position: Optional [X, Y] position in the graph

        Returns:
            Response containing the node ID. Pins: Execute (input exec), Condition (input bool), Then (output exec true), Else (output exec false)
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            if node_position is None:
                node_position = [0, 0]

            params = {
                "blueprint_name": blueprint_name,
                "node_position": node_position
            }

            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Adding branch node to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_branch_node", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Branch node creation response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding branch node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_blueprint_variable_get_node(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a Variable Get node to read a blueprint variable's value.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable to get (must already exist on the blueprint)
            node_position: Optional [X, Y] position in the graph

        Returns:
            Response containing the node ID. Output pin matches the variable name and type.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            if node_position is None:
                node_position = [0, 0]

            params = {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name,
                "node_position": node_position
            }

            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Adding variable get node for '{variable_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_variable_get_node", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Variable get node creation response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding variable get node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_blueprint_variable_set_node(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a Variable Set node to write a value to a blueprint variable.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable to set (must already exist on the blueprint)
            node_position: Optional [X, Y] position in the graph

        Returns:
            Response containing the node ID. Pins: Execute (input exec), Then (output exec), variable_name (input data pin matching variable type).
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            if node_position is None:
                node_position = [0, 0]

            params = {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name,
                "node_position": node_position
            }

            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Adding variable set node for '{variable_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_variable_set_node", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Variable set node creation response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding variable set node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def delete_blueprint_node(
        ctx: Context,
        blueprint_name: str,
        node_id: str
    ) -> Dict[str, Any]:
        """
        Delete a specific node from a blueprint by its node ID.

        Args:
            blueprint_name: Name of the target Blueprint
            node_id: The GUID of the node to delete (returned when the node was created)

        Returns:
            Response indicating success and the deleted node ID
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "node_id": node_id
            }

            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Deleting node '{node_id}' from blueprint '{blueprint_name}'")
            response = unreal.send_command("delete_blueprint_node", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Delete node response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error deleting blueprint node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def clear_blueprint_graph(
        ctx: Context,
        blueprint_name: str
    ) -> Dict[str, Any]:
        """
        Remove ALL nodes from a blueprint's event graph. Use for a fresh start.

        Args:
            blueprint_name: Name of the target Blueprint

        Returns:
            Response indicating success and the number of nodes removed
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name
            }

            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Clearing all nodes from blueprint '{blueprint_name}'")
            response = unreal.send_command("clear_blueprint_graph", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Clear graph response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error clearing blueprint graph: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def disconnect_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        node_id: str,
        pin_name: str
    ) -> Dict[str, Any]:
        """
        Break all connections on a specific pin of a node.

        Args:
            blueprint_name: Name of the target Blueprint
            node_id: The GUID of the node containing the pin
            pin_name: Name of the pin to disconnect (case-insensitive)

        Returns:
            Response indicating success
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "node_id": node_id,
                "pin_name": pin_name
            }

            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Disconnecting pin '{pin_name}' on node '{node_id}' in blueprint '{blueprint_name}'")
            response = unreal.send_command("disconnect_blueprint_nodes", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Disconnect response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error disconnecting blueprint nodes: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # ========================================================================
    # Stage 2 — Blueprint introspection tools
    # ========================================================================

    @mcp.tool()
    def inspect_blueprint(ctx: Context, blueprint_name: str) -> Dict[str, Any]:
        """Get a high-level overview of a Blueprint: parent class, graphs, variables, functions, components.

        Args:
            blueprint_name: Name or path of the Blueprint asset.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("inspect_blueprint", {"blueprint_name": blueprint_name})
            return response or {}
        except Exception as e:
            logger.error(f"Error inspecting blueprint: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_blueprint_graph(ctx: Context, blueprint_name: str, graph_name: str = "EventGraph") -> Dict[str, Any]:
        """Dump the full node graph of a Blueprint graph as JSON (nodes + pins + connections).

        Args:
            blueprint_name: Name or path of the Blueprint asset.
            graph_name: Graph name (default: EventGraph). Use inspect_blueprint first to list available graphs.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_blueprint_graph", {"blueprint_name": blueprint_name, "graph_name": graph_name})
            return response or {}
        except Exception as e:
            logger.error(f"Error getting blueprint graph: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_blueprint_nodes(ctx: Context, blueprint_name: str, graph_name: str = "EventGraph", filter: str = "") -> Dict[str, Any]:
        """List nodes in a graph, optionally filtered by class name or title substring.

        Args:
            blueprint_name: Name or path of the Blueprint asset.
            graph_name: Graph name (default: EventGraph).
            filter: Optional substring filter on node class or title (e.g. "CallFunction", "Branch").
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {"blueprint_name": blueprint_name, "graph_name": graph_name}
            if filter:
                params["filter"] = filter
            response = unreal.send_command("get_blueprint_nodes", params)
            return response or {}
        except Exception as e:
            logger.error(f"Error getting blueprint nodes: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_blueprint_variables(ctx: Context, blueprint_name: str) -> Dict[str, Any]:
        """List all user-declared variables in a Blueprint with type info, defaults, replication flags, and categories."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_blueprint_variables", {"blueprint_name": blueprint_name})
            return response or {}
        except Exception as e:
            logger.error(f"Error getting blueprint variables: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_blueprint_functions(ctx: Context, blueprint_name: str) -> Dict[str, Any]:
        """List all user-defined functions in a Blueprint with parameter signatures."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_blueprint_functions", {"blueprint_name": blueprint_name})
            return response or {}
        except Exception as e:
            logger.error(f"Error getting blueprint functions: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_node_pins(ctx: Context, blueprint_name: str, node_id: str, graph_name: str = "EventGraph") -> Dict[str, Any]:
        """Get detailed pin info for a specific node (inputs, outputs, types, connections).

        Args:
            blueprint_name: Name or path of the Blueprint asset.
            node_id: Node GUID (from get_blueprint_graph or find_blueprint_nodes).
            graph_name: Graph containing the node.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_node_pins", {"blueprint_name": blueprint_name, "node_id": node_id, "graph_name": graph_name})
            return response or {}
        except Exception as e:
            logger.error(f"Error getting node pins: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def search_blueprint_nodes(ctx: Context, blueprint_name: str, query: str) -> Dict[str, Any]:
        """Search all graphs in a Blueprint for nodes matching a query string (titles, class names, comments, pin defaults).

        Args:
            blueprint_name: Name or path of the Blueprint asset.
            query: Substring to search for.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("search_blueprint_nodes", {"blueprint_name": blueprint_name, "query": query})
            return response or {}
        except Exception as e:
            logger.error(f"Error searching blueprint nodes: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def find_references(ctx: Context, blueprint_name: str, name: str) -> Dict[str, Any]:
        """Find all get/set/call references to a variable or function within a Blueprint.

        Args:
            blueprint_name: Name or path of the Blueprint asset.
            name: Variable or function name to find references to.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("find_references", {"blueprint_name": blueprint_name, "name": name})
            return response or {}
        except Exception as e:
            logger.error(f"Error finding references: {e}")
            return {"success": False, "message": str(e)}

    logger.info("Blueprint node tools registered successfully")