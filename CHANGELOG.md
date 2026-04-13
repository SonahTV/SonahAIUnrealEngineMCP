# Changelog

## v1.0.0 — SonahAIUnrealEngineMCP (2026-04-12)

Forked from `chongdashu/unreal-mcp`, renamed to **SonahAIUnrealEngineMCP**, and expanded from ~70 to ~95 tools.

### New tool categories

- **Viewport capture** — `capture_viewport` returns base64 PNG of the editor viewport
- **UE5 docs context** — `get_ue_context(topic)` returns curated cheatsheets (animation, blueprint, actor, assets, replication, slate)
- **Material instances** — `create_material_instance`, `set_material_instance_scalar_param`, `set_material_instance_vector_param`, `get_material_info`
- **Blueprint introspection** — `inspect_blueprint`, `get_blueprint_graph`, `get_blueprint_nodes`, `get_blueprint_variables`, `get_blueprint_functions`, `get_node_pins`, `search_blueprint_nodes`, `find_references`
- **Asset operations** — `asset_search`, `asset_dependencies`, `asset_referencers`, `duplicate_asset`, `rename_asset`, `move_asset`, `reimport_asset`
- **Async task queue** — `task_submit`, `task_status`, `task_result`, `task_list`, `task_cancel`
- **Character domain** — `create_character_blueprint`, `assign_anim_blueprint`, `set_movement_param`
- **Enhanced Input** — `list_input_actions`, `remove_input_mapping`
- **Animation Blueprint** — `create_anim_blueprint`, `create_state_machine`, `add_anim_state`, `create_anim_transition`, `add_anim_notify`, `list_anim_states`, `inspect_anim_blueprint`, `set_state_animation` (stub)

### Build.cs module additions
- `AssetTools` (private)
- `AnimGraph`, `AnimGraphRuntime` (editor-only)

### Known stubs
- `set_state_animation` — wiring UAnimGraphNode_SequencePlayer inside state sub-graph not yet automated
- Enhanced Input trigger/modifier/assign-mapping-context deferred (need EnhancedInput module dep)
