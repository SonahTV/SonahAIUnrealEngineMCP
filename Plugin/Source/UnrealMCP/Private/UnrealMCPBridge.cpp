#include "UnrealMCPBridge.h"
#include "MCPServerRunnable.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "HAL/RunnableThread.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "JsonObjectConverter.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
#include "UnrealMCPTaskManager.h"
// Add Blueprint related includes
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
// UE5.5 correct includes
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/Field.h"
#include "UObject/FieldPath.h"
// Blueprint Graph specific includes
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "GameFramework/InputSettings.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
// Include our new command handler classes
#include "Commands/UnrealMCPEditorCommands.h"
#include "Commands/UnrealMCPBlueprintCommands.h"
#include "Commands/UnrealMCPBlueprintNodeCommands.h"
#include "Commands/UnrealMCPProjectCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Commands/UnrealMCPUMGCommands.h"

// Default settings
#define MCP_SERVER_HOST "127.0.0.1"
#define MCP_SERVER_PORT 55557

UUnrealMCPBridge::UUnrealMCPBridge()
{
    EditorCommands = MakeShared<FUnrealMCPEditorCommands>();
    BlueprintCommands = MakeShared<FUnrealMCPBlueprintCommands>();
    BlueprintNodeCommands = MakeShared<FUnrealMCPBlueprintNodeCommands>();
    ProjectCommands = MakeShared<FUnrealMCPProjectCommands>();
    UMGCommands = MakeShared<FUnrealMCPUMGCommands>();
    AnimCommands = MakeShared<FUnrealMCPAnimCommands>();
}

UUnrealMCPBridge::~UUnrealMCPBridge()
{
    EditorCommands.Reset();
    BlueprintCommands.Reset();
    BlueprintNodeCommands.Reset();
    ProjectCommands.Reset();
    UMGCommands.Reset();
    AnimCommands.Reset();
}

// Initialize subsystem
void UUnrealMCPBridge::Initialize(FSubsystemCollectionBase& Collection)
{
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Initializing"));
    
    bIsRunning = false;
    ListenerSocket = nullptr;
    ConnectionSocket = nullptr;
    ServerThread = nullptr;
    Port = MCP_SERVER_PORT;
    FIPv4Address::Parse(MCP_SERVER_HOST, ServerAddress);

    // Start the server automatically
    StartServer();
}

// Clean up resources when subsystem is destroyed
void UUnrealMCPBridge::Deinitialize()
{
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Shutting down"));
    StopServer();
}

// Start the MCP server
void UUnrealMCPBridge::StartServer()
{
    if (bIsRunning)
    {
        UE_LOG(LogTemp, Warning, TEXT("UnrealMCPBridge: Server is already running"));
        return;
    }

    // Create socket subsystem
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to get socket subsystem"));
        return;
    }

    // Create listener socket
    TSharedPtr<FSocket> NewListenerSocket = MakeShareable(SocketSubsystem->CreateSocket(NAME_Stream, TEXT("UnrealMCPListener"), false));
    if (!NewListenerSocket.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to create listener socket"));
        return;
    }

    // Allow address reuse for quick restarts
    NewListenerSocket->SetReuseAddr(true);
    NewListenerSocket->SetNonBlocking(true);

    // Bind to address
    FIPv4Endpoint Endpoint(ServerAddress, Port);
    if (!NewListenerSocket->Bind(*Endpoint.ToInternetAddr()))
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to bind listener socket to %s:%d"), *ServerAddress.ToString(), Port);
        return;
    }

    // Start listening
    if (!NewListenerSocket->Listen(5))
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to start listening"));
        return;
    }

    ListenerSocket = NewListenerSocket;
    bIsRunning = true;
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Server started on %s:%d"), *ServerAddress.ToString(), Port);

    // Start server thread
    ServerThread = FRunnableThread::Create(
        new FMCPServerRunnable(this, ListenerSocket),
        TEXT("UnrealMCPServerThread"),
        0, TPri_Normal
    );

    if (!ServerThread)
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to create server thread"));
        StopServer();
        return;
    }
}

// Stop the MCP server
void UUnrealMCPBridge::StopServer()
{
    if (!bIsRunning)
    {
        return;
    }

    bIsRunning = false;

    // Clean up thread
    if (ServerThread)
    {
        ServerThread->Kill(true);
        delete ServerThread;
        ServerThread = nullptr;
    }

    // Close sockets
    if (ConnectionSocket.IsValid())
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket.Get());
        ConnectionSocket.Reset();
    }

    if (ListenerSocket.IsValid())
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket.Get());
        ListenerSocket.Reset();
    }

    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Server stopped"));
}

// Execute a command received from a client
FString UUnrealMCPBridge::ExecuteCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Executing command: %s"), *CommandType);
    
    // Create a promise to wait for the result
    TPromise<FString> Promise;
    TFuture<FString> Future = Promise.GetFuture();
    
    // Queue execution on Game Thread
    AsyncTask(ENamedThreads::GameThread, [this, CommandType, Params, Promise = MoveTemp(Promise)]() mutable
    {
        TSharedPtr<FJsonObject> ResponseJson = MakeShareable(new FJsonObject);
        
        try
        {
            TSharedPtr<FJsonObject> ResultJson;
            
            if (CommandType == TEXT("ping"))
            {
                ResultJson = MakeShareable(new FJsonObject);
                ResultJson->SetStringField(TEXT("message"), TEXT("pong"));
            }
            // Editor Commands (including actor manipulation)
            else if (CommandType == TEXT("get_actors_in_level") || 
                     CommandType == TEXT("find_actors_by_name") ||
                     CommandType == TEXT("spawn_actor") ||
                     CommandType == TEXT("create_actor") ||
                     CommandType == TEXT("delete_actor") || 
                     CommandType == TEXT("set_actor_transform") ||
                     CommandType == TEXT("get_actor_properties") ||
                     CommandType == TEXT("set_actor_property") ||
                     CommandType == TEXT("spawn_blueprint_actor") ||
                     CommandType == TEXT("focus_viewport") ||
                     CommandType == TEXT("take_screenshot") ||
                     CommandType == TEXT("capture_viewport") ||
                     CommandType == TEXT("create_material_instance") ||
                     CommandType == TEXT("set_material_instance_scalar_param") ||
                     CommandType == TEXT("set_material_instance_vector_param") ||
                     CommandType == TEXT("get_material_info") ||
                     CommandType == TEXT("save_current_level") ||
                     CommandType == TEXT("save_all_assets") ||
                     CommandType == TEXT("create_new_level") ||
                     CommandType == TEXT("delete_blueprint_asset") ||
                     CommandType == TEXT("create_material") ||
                     CommandType == TEXT("set_actor_material") ||
                     CommandType == TEXT("set_actor_material_color") ||
                     CommandType == TEXT("set_actor_light_color") ||
                     CommandType == TEXT("set_component_material_color") ||
                     CommandType == TEXT("execute_console_command") ||
                     CommandType == TEXT("start_pie") ||
                     CommandType == TEXT("stop_pie"))
            {
                ResultJson = EditorCommands->HandleCommand(CommandType, Params);
            }
            // Blueprint Commands
            else if (CommandType == TEXT("create_blueprint") || 
                     CommandType == TEXT("add_component_to_blueprint") || 
                     CommandType == TEXT("set_component_property") || 
                     CommandType == TEXT("set_physics_properties") || 
                     CommandType == TEXT("compile_blueprint") || 
                     CommandType == TEXT("set_blueprint_property") || 
                     CommandType == TEXT("set_static_mesh_properties") ||
                     CommandType == TEXT("set_pawn_properties") ||
                     CommandType == TEXT("create_character_blueprint") ||
                     CommandType == TEXT("assign_anim_blueprint") ||
                     CommandType == TEXT("set_movement_param") ||
                     CommandType == TEXT("set_blueprint_variable_default"))
            {
                ResultJson = BlueprintCommands->HandleCommand(CommandType, Params);
            }
            // Blueprint Node Commands
            else if (CommandType == TEXT("connect_blueprint_nodes") || 
                     CommandType == TEXT("add_blueprint_get_self_component_reference") ||
                     CommandType == TEXT("add_blueprint_self_reference") ||
                     CommandType == TEXT("find_blueprint_nodes") ||
                     CommandType == TEXT("add_blueprint_event_node") ||
                     CommandType == TEXT("add_blueprint_input_action_node") ||
                     CommandType == TEXT("add_blueprint_function_node") ||
                     CommandType == TEXT("add_blueprint_get_component_node") ||
                     CommandType == TEXT("add_blueprint_variable") ||
                     CommandType == TEXT("add_blueprint_branch_node") ||
                     CommandType == TEXT("add_blueprint_variable_get_node") ||
                     CommandType == TEXT("add_blueprint_variable_set_node") ||
                     CommandType == TEXT("delete_blueprint_node") ||
                     CommandType == TEXT("clear_blueprint_graph") ||
                     CommandType == TEXT("disconnect_blueprint_nodes") ||
                     CommandType == TEXT("inspect_blueprint") ||
                     CommandType == TEXT("get_blueprint_graph") ||
                     CommandType == TEXT("get_blueprint_nodes") ||
                     CommandType == TEXT("get_blueprint_variables") ||
                     CommandType == TEXT("get_blueprint_functions") ||
                     CommandType == TEXT("get_node_pins") ||
                     CommandType == TEXT("search_blueprint_nodes") ||
                     CommandType == TEXT("find_references"))
            {
                ResultJson = BlueprintNodeCommands->HandleCommand(CommandType, Params);
            }
            // Project Commands
            else if (CommandType == TEXT("create_input_mapping") ||
                     CommandType == TEXT("list_input_actions") ||
                     CommandType == TEXT("remove_input_mapping") ||
                     CommandType == TEXT("asset_search") ||
                     CommandType == TEXT("asset_dependencies") ||
                     CommandType == TEXT("asset_referencers") ||
                     CommandType == TEXT("duplicate_asset") ||
                     CommandType == TEXT("rename_asset") ||
                     CommandType == TEXT("move_asset") ||
                     CommandType == TEXT("reimport_asset"))
            {
                ResultJson = ProjectCommands->HandleCommand(CommandType, Params);
            }
            // UMG Commands
            else if (CommandType == TEXT("create_umg_widget_blueprint") ||
                     CommandType == TEXT("add_text_block_to_widget") ||
                     CommandType == TEXT("add_button_to_widget") ||
                     CommandType == TEXT("bind_widget_event") ||
                     CommandType == TEXT("set_text_block_binding") ||
                     CommandType == TEXT("add_widget_to_viewport"))
            {
                ResultJson = UMGCommands->HandleCommand(CommandType, Params);
            }
            // Stage 6: Animation Blueprint commands
            else if (CommandType == TEXT("create_anim_blueprint") ||
                     CommandType == TEXT("create_state_machine") ||
                     CommandType == TEXT("add_anim_state") ||
                     CommandType == TEXT("set_state_animation") ||
                     CommandType == TEXT("create_anim_transition") ||
                     CommandType == TEXT("add_anim_notify") ||
                     CommandType == TEXT("list_anim_states") ||
                     CommandType == TEXT("inspect_anim_blueprint") ||
                     CommandType == TEXT("set_transition_rule"))
            {
                ResultJson = AnimCommands->HandleCommand(CommandType, Params);
            }
            // Stage 4: Async task queue commands
            else if (CommandType == TEXT("task_submit"))
            {
                FString TaskType;
                Params->TryGetStringField(TEXT("task_type"), TaskType);
                FGuid TaskId = FUnrealMCPTaskManager::Get().SubmitTask(TaskType, Params);
                ResultJson = MakeShareable(new FJsonObject);
                ResultJson->SetStringField(TEXT("task_id"), TaskId.ToString());
                ResultJson->SetStringField(TEXT("status"), TEXT("pending"));
            }
            else if (CommandType == TEXT("task_status"))
            {
                FString TaskIdStr;
                Params->TryGetStringField(TEXT("task_id"), TaskIdStr);
                FGuid TaskId;
                FGuid::Parse(TaskIdStr, TaskId);
                const auto* State = FUnrealMCPTaskManager::Get().GetTask(TaskId);
                if (State)
                {
                    ResultJson = FUnrealMCPTaskManager::TaskStateToJson(*State);
                }
                else
                {
                    ResultJson = MakeShareable(new FJsonObject);
                    ResultJson->SetBoolField(TEXT("success"), false);
                    ResultJson->SetStringField(TEXT("error"), TEXT("Task not found"));
                }
            }
            else if (CommandType == TEXT("task_result"))
            {
                FString TaskIdStr;
                Params->TryGetStringField(TEXT("task_id"), TaskIdStr);
                double WaitSeconds = 0;
                Params->TryGetNumberField(TEXT("wait_seconds"), WaitSeconds);
                FGuid TaskId;
                FGuid::Parse(TaskIdStr, TaskId);
                const auto* State = FUnrealMCPTaskManager::Get().WaitForResult(TaskId, (float)WaitSeconds);
                if (State)
                {
                    ResultJson = FUnrealMCPTaskManager::TaskStateToJson(*State);
                }
                else
                {
                    ResultJson = MakeShareable(new FJsonObject);
                    ResultJson->SetBoolField(TEXT("success"), false);
                    ResultJson->SetStringField(TEXT("error"), TEXT("Task not found"));
                }
            }
            else if (CommandType == TEXT("task_list"))
            {
                auto AllTasks = FUnrealMCPTaskManager::Get().ListTasks();
                TArray<TSharedPtr<FJsonValue>> Arr;
                for (const auto* S : AllTasks)
                {
                    Arr.Add(MakeShared<FJsonValueObject>(FUnrealMCPTaskManager::TaskStateToJson(*S)));
                }
                ResultJson = MakeShareable(new FJsonObject);
                ResultJson->SetNumberField(TEXT("count"), Arr.Num());
                ResultJson->SetArrayField(TEXT("tasks"), Arr);
            }
            else if (CommandType == TEXT("task_cancel"))
            {
                FString TaskIdStr;
                Params->TryGetStringField(TEXT("task_id"), TaskIdStr);
                FGuid TaskId;
                FGuid::Parse(TaskIdStr, TaskId);
                bool bCancelled = FUnrealMCPTaskManager::Get().CancelTask(TaskId);
                ResultJson = MakeShareable(new FJsonObject);
                ResultJson->SetBoolField(TEXT("cancelled"), bCancelled);
            }
            else
            {
                ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                ResponseJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Unknown command: %s"), *CommandType));
                
                FString ResultString;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
                FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);
                Promise.SetValue(ResultString);
                return;
            }
            
            // Check if the result contains an error
            bool bSuccess = true;
            FString ErrorMessage;
            
            if (ResultJson->HasField(TEXT("success")))
            {
                bSuccess = ResultJson->GetBoolField(TEXT("success"));
                if (!bSuccess && ResultJson->HasField(TEXT("error")))
                {
                    ErrorMessage = ResultJson->GetStringField(TEXT("error"));
                }
            }
            
            if (bSuccess)
            {
                // Set success status and include the result
                ResponseJson->SetStringField(TEXT("status"), TEXT("success"));
                ResponseJson->SetObjectField(TEXT("result"), ResultJson);
            }
            else
            {
                // Set error status and include the error message
                ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                ResponseJson->SetStringField(TEXT("error"), ErrorMessage);
            }
        }
        catch (const std::exception& e)
        {
            ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
            ResponseJson->SetStringField(TEXT("error"), FString(e.what()));
        }
        
        FString ResultString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
        FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);
        Promise.SetValue(ResultString);
    });
    
    return Future.Get();
}