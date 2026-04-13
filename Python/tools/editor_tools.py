"""
Editor Tools for Unreal MCP.

This module provides tools for controlling the Unreal Editor viewport and other editor functionality.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_editor_tools(mcp: FastMCP):
    """Register editor tools with the MCP server."""
    
    @mcp.tool()
    def get_actors_in_level(ctx: Context) -> List[Dict[str, Any]]:
        """Get a list of all actors in the current level."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return []
                
            response = unreal.send_command("get_actors_in_level", {})
            
            if not response:
                logger.warning("No response from Unreal Engine")
                return []
                
            # Log the complete response for debugging
            logger.info(f"Complete response from Unreal: {response}")
            
            # Check response format
            if "result" in response and "actors" in response["result"]:
                actors = response["result"]["actors"]
                logger.info(f"Found {len(actors)} actors in level")
                return actors
            elif "actors" in response:
                actors = response["actors"]
                logger.info(f"Found {len(actors)} actors in level")
                return actors
                
            logger.warning(f"Unexpected response format: {response}")
            return []
            
        except Exception as e:
            logger.error(f"Error getting actors: {e}")
            return []

    @mcp.tool()
    def find_actors_by_name(ctx: Context, pattern: str) -> List[str]:
        """Find actors by name pattern."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return []
                
            response = unreal.send_command("find_actors_by_name", {
                "pattern": pattern
            })
            
            if not response:
                return []
                
            return response.get("actors", [])
            
        except Exception as e:
            logger.error(f"Error finding actors: {e}")
            return []
    
    @mcp.tool()
    def spawn_actor(
        ctx: Context,
        name: str,
        type: str,
        location: List[float] = [0.0, 0.0, 0.0],
        rotation: List[float] = [0.0, 0.0, 0.0]
    ) -> Dict[str, Any]:
        """Create a new actor in the current level.
        
        Args:
            ctx: The MCP context
            name: The name to give the new actor (must be unique)
            type: The type of actor to create (e.g. StaticMeshActor, PointLight)
            location: The [x, y, z] world location to spawn at
            rotation: The [pitch, yaw, roll] rotation in degrees
            
        Returns:
            Dict containing the created actor's properties
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            # Ensure all parameters are properly formatted
            params = {
                "name": name,
                "type": type.upper(),  # Make sure type is uppercase
                "location": location,
                "rotation": rotation
            }
            
            # Validate location and rotation formats
            for param_name in ["location", "rotation"]:
                param_value = params[param_name]
                if not isinstance(param_value, list) or len(param_value) != 3:
                    logger.error(f"Invalid {param_name} format: {param_value}. Must be a list of 3 float values.")
                    return {"success": False, "message": f"Invalid {param_name} format. Must be a list of 3 float values."}
                # Ensure all values are float
                params[param_name] = [float(val) for val in param_value]
            
            logger.info(f"Creating actor '{name}' of type '{type}' with params: {params}")
            response = unreal.send_command("spawn_actor", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            # Log the complete response for debugging
            logger.info(f"Actor creation response: {response}")
            
            # Handle error responses correctly
            if response.get("status") == "error":
                error_message = response.get("error", "Unknown error")
                logger.error(f"Error creating actor: {error_message}")
                return {"success": False, "message": error_message}
            
            return response
            
        except Exception as e:
            error_msg = f"Error creating actor: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def delete_actor(ctx: Context, name: str) -> Dict[str, Any]:
        """Delete an actor by name."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            response = unreal.send_command("delete_actor", {
                "name": name
            })
            return response or {}
            
        except Exception as e:
            logger.error(f"Error deleting actor: {e}")
            return {}
    
    @mcp.tool()
    def set_actor_transform(
        ctx: Context,
        name: str,
        location: List[float]  = None,
        rotation: List[float]  = None,
        scale: List[float] = None
    ) -> Dict[str, Any]:
        """Set the transform of an actor."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            params = {"name": name}
            if location is not None:
                params["location"] = location
            if rotation is not None:
                params["rotation"] = rotation
            if scale is not None:
                params["scale"] = scale
                
            response = unreal.send_command("set_actor_transform", params)
            return response or {}
            
        except Exception as e:
            logger.error(f"Error setting transform: {e}")
            return {}
    
    @mcp.tool()
    def get_actor_properties(ctx: Context, name: str) -> Dict[str, Any]:
        """Get all properties of an actor."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            response = unreal.send_command("get_actor_properties", {
                "name": name
            })
            return response or {}
            
        except Exception as e:
            logger.error(f"Error getting properties: {e}")
            return {}

    @mcp.tool()
    def set_actor_property(
        ctx: Context,
        name: str,
        property_name: str,
        property_value,
    ) -> Dict[str, Any]:
        """
        Set a property on an actor.
        
        Args:
            name: Name of the actor
            property_name: Name of the property to set
            property_value: Value to set the property to
            
        Returns:
            Dict containing response from Unreal with operation status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            response = unreal.send_command("set_actor_property", {
                "name": name,
                "property_name": property_name,
                "property_value": property_value
            })
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set actor property response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting actor property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # @mcp.tool() commented out because it's buggy
    def focus_viewport(
        ctx: Context,
        target: str = None,
        location: List[float] = None,
        distance: float = 1000.0,
        orientation: List[float] = None
    ) -> Dict[str, Any]:
        """
        Focus the viewport on a specific actor or location.
        
        Args:
            target: Name of the actor to focus on (if provided, location is ignored)
            location: [X, Y, Z] coordinates to focus on (used if target is None)
            distance: Distance from the target/location
            orientation: Optional [Pitch, Yaw, Roll] for the viewport camera
            
        Returns:
            Response from Unreal Engine
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            params = {}
            if target:
                params["target"] = target
            elif location:
                params["location"] = location
            
            if distance:
                params["distance"] = distance
                
            if orientation:
                params["orientation"] = orientation
                
            response = unreal.send_command("focus_viewport", params)
            return response or {}
            
        except Exception as e:
            logger.error(f"Error focusing viewport: {e}")
            return {"status": "error", "message": str(e)}

    @mcp.tool()
    def spawn_blueprint_actor(
        ctx: Context,
        blueprint_name: str,
        actor_name: str,
        location: List[float] = [0.0, 0.0, 0.0],
        rotation: List[float] = [0.0, 0.0, 0.0]
    ) -> Dict[str, Any]:
        """Spawn an actor from a Blueprint.
        
        Args:
            ctx: The MCP context
            blueprint_name: Name of the Blueprint to spawn from
            actor_name: Name to give the spawned actor
            location: The [x, y, z] world location to spawn at
            rotation: The [pitch, yaw, roll] rotation in degrees
            
        Returns:
            Dict containing the spawned actor's properties
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            # Ensure all parameters are properly formatted
            params = {
                "blueprint_name": blueprint_name,
                "actor_name": actor_name,
                "location": location or [0.0, 0.0, 0.0],
                "rotation": rotation or [0.0, 0.0, 0.0]
            }
            
            # Validate location and rotation formats
            for param_name in ["location", "rotation"]:
                param_value = params[param_name]
                if not isinstance(param_value, list) or len(param_value) != 3:
                    logger.error(f"Invalid {param_name} format: {param_value}. Must be a list of 3 float values.")
                    return {"success": False, "message": f"Invalid {param_name} format. Must be a list of 3 float values."}
                # Ensure all values are float
                params[param_name] = [float(val) for val in param_value]
            
            logger.info(f"Spawning blueprint actor with params: {params}")
            response = unreal.send_command("spawn_blueprint_actor", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Spawn blueprint actor response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error spawning blueprint actor: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_actor_material_color(
        ctx: Context,
        name: str,
        color: List[float] = [1.0, 0.0, 0.0, 1.0],
        component_name: str = ""
    ) -> Dict[str, Any]:
        """
        Set a color on a spawned actor's mesh material by creating a dynamic material instance.

        Args:
            name: Name of the actor in the level
            color: [R, G, B, A] color values (0.0 to 1.0). Defaults to red.
            component_name: Optional specific StaticMeshComponent name. If empty, uses the first one found.

        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            # Ensure color has 4 components
            if len(color) == 3:
                color = color + [1.0]

            params = {
                "name": name,
                "color": [float(c) for c in color]
            }
            if component_name:
                params["component_name"] = component_name

            logger.info(f"Setting actor material color with params: {params}")
            response = unreal.send_command("set_actor_material_color", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set actor material color response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting actor material color: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def save_current_level(ctx: Context) -> Dict[str, Any]:
        """Save the current level to disk."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            return unreal.send_command("save_current_level", {}) or {"success": False}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def create_new_level(ctx: Context, level_name: str = "InkArena") -> Dict[str, Any]:
        """Create a new empty level. Escapes Open World template issues."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            return unreal.send_command("create_new_level", {"level_name": level_name}) or {"success": False}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def save_all_assets(ctx: Context) -> Dict[str, Any]:
        """Save all modified assets to disk (equivalent to Ctrl+Shift+S)."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            return unreal.send_command("save_all_assets", {}) or {"success": False}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def delete_blueprint_asset(ctx: Context, blueprint_name: str) -> Dict[str, Any]:
        """Delete a blueprint asset from disk."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect"}
            return unreal.send_command("delete_blueprint_asset", {"blueprint_name": blueprint_name}) or {"success": False}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def create_material(
        ctx: Context,
        material_name: str,
        color: List[float] = [1.0, 0.0, 0.0, 1.0]
    ) -> Dict[str, Any]:
        """Create a colored material asset in /Game/Materials/. Color is [R,G,B,A] 0.0-1.0."""
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            if len(color) == 3:
                color = color + [1.0]

            params = {
                "material_name": material_name,
                "color": [float(c) for c in color]
            }

            logger.info(f"Creating material with params: {params}")
            response = unreal.send_command("create_material", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create material response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating material: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_actor_material(
        ctx: Context,
        actor_name: str,
        material_path: str
    ) -> Dict[str, Any]:
        """Assign a material to an actor's mesh. material_path like '/Game/Materials/M_Red'."""
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "actor_name": actor_name,
                "material_path": material_path
            }

            logger.info(f"Setting actor material with params: {params}")
            response = unreal.send_command("set_actor_material", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set actor material response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting actor material: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_actor_light_color(
        ctx: Context,
        actor_name: str,
        color: List[float] = [1.0, 1.0, 1.0],
        intensity: float = 5000.0
    ) -> Dict[str, Any]:
        """Set color and intensity on a light actor. Color is [R,G,B] 0.0-1.0."""
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "actor_name": actor_name,
                "color": [float(c) for c in color],
                "intensity": float(intensity)
            }

            logger.info(f"Setting light color with params: {params}")
            response = unreal.send_command("set_actor_light_color", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set light color response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting light color: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # ========================================================================
    # Stage 1.1 — capture_viewport (returns base64 PNG to the model)
    # ========================================================================
    @mcp.tool()
    def capture_viewport(
        ctx: Context,
        return_base64: bool = True,
        filepath: str = None
    ) -> Dict[str, Any]:
        """Capture the active editor viewport as a PNG.

        Args:
            return_base64: If True, returns the PNG bytes base64-encoded so the model can see the image. Default True.
            filepath: Optional absolute path to also save the PNG to disk. If omitted and return_base64 is True, the image is only returned in-memory.

        Returns:
            Dict with width, height, and either filepath or image_base64 (or both).
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {"return_base64": bool(return_base64)}
            if filepath:
                params["filepath"] = filepath
            response = unreal.send_command("capture_viewport", params)
            return response or {}
        except Exception as e:
            logger.error(f"Error capturing viewport: {e}")
            return {"success": False, "message": str(e)}

    # ========================================================================
    # Stage 1.3 — Material instance ops
    # ========================================================================
    @mcp.tool()
    def create_material_instance(
        ctx: Context,
        parent_material: str,
        instance_name: str,
        save_folder: str = "/Game/Materials"
    ) -> Dict[str, Any]:
        """Create a new UMaterialInstanceConstant asset from a parent material.

        Args:
            parent_material: Asset path to the parent UMaterial or UMaterialInterface (e.g. "/Engine/BasicShapes/BasicShapeMaterial").
            instance_name: Name for the new material instance asset.
            save_folder: Content browser folder to save into. Defaults to /Game/Materials.

        Returns:
            Dict with instance_path on success.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("create_material_instance", {
                "parent_material": parent_material,
                "instance_name": instance_name,
                "save_folder": save_folder,
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error creating material instance: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_material_instance_scalar_param(
        ctx: Context,
        instance_path: str,
        param_name: str,
        value: float
    ) -> Dict[str, Any]:
        """Set a scalar parameter on an existing material instance constant."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_material_instance_scalar_param", {
                "instance_path": instance_path,
                "param_name": param_name,
                "value": float(value),
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error setting scalar param: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_material_instance_vector_param(
        ctx: Context,
        instance_path: str,
        param_name: str,
        color: List[float]
    ) -> Dict[str, Any]:
        """Set a vector (color) parameter on an existing material instance constant.

        Args:
            color: [r, g, b] or [r, g, b, a] in 0..1 linear color space.
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_material_instance_vector_param", {
                "instance_path": instance_path,
                "param_name": param_name,
                "color": [float(c) for c in color],
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error setting vector param: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_material_info(
        ctx: Context,
        material_path: str
    ) -> Dict[str, Any]:
        """Inspect a material or material instance: returns its scalar/vector/texture parameter list and base material chain."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_material_info", {
                "material_path": material_path,
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error getting material info: {e}")
            return {"success": False, "message": str(e)}

    logger.info("Editor tools registered successfully")
