#include "Commands/UnrealMCPBlueprintNodeCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "K2Node_IfThenElse.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "GameFramework/InputSettings.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "EdGraphSchema_K2.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"

// Declare the log category
DEFINE_LOG_CATEGORY_STATIC(LogUnrealMCP, Log, All);

FUnrealMCPBlueprintNodeCommands::FUnrealMCPBlueprintNodeCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("connect_blueprint_nodes"))
    {
        return HandleConnectBlueprintNodes(Params);
    }
    else if (CommandType == TEXT("add_blueprint_get_self_component_reference"))
    {
        return HandleAddBlueprintGetSelfComponentReference(Params);
    }
    else if (CommandType == TEXT("add_blueprint_event_node"))
    {
        return HandleAddBlueprintEvent(Params);
    }
    else if (CommandType == TEXT("add_blueprint_function_node"))
    {
        return HandleAddBlueprintFunctionCall(Params);
    }
    else if (CommandType == TEXT("add_blueprint_variable"))
    {
        return HandleAddBlueprintVariable(Params);
    }
    else if (CommandType == TEXT("add_blueprint_input_action_node"))
    {
        return HandleAddBlueprintInputActionNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_self_reference"))
    {
        return HandleAddBlueprintSelfReference(Params);
    }
    else if (CommandType == TEXT("find_blueprint_nodes"))
    {
        return HandleFindBlueprintNodes(Params);
    }
    // Missing pre-existing node commands
    else if (CommandType == TEXT("add_blueprint_branch_node"))
    {
        FString BPName; Params->TryGetStringField(TEXT("blueprint_name"), BPName);
        UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BPName);
        if (!BP) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
        UEdGraph* Graph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(BP);
        const TArray<TSharedPtr<FJsonValue>>* PosArr = nullptr;
        FVector2D Pos(0, 0);
        if (Params->TryGetArrayField(TEXT("position"), PosArr) && PosArr && PosArr->Num() >= 2)
            Pos = FVector2D((*PosArr)[0]->AsNumber(), (*PosArr)[1]->AsNumber());
        UK2Node_IfThenElse* Node = NewObject<UK2Node_IfThenElse>(Graph);
        Graph->AddNode(Node, false, false);
        Node->CreateNewGuid(); Node->PostPlacedNewNode(); Node->AllocateDefaultPins();
        Node->NodePosX = (int32)Pos.X; Node->NodePosY = (int32)Pos.Y;
        FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
        return R;
    }
    else if (CommandType == TEXT("add_blueprint_variable_get_node"))
    {
        FString BPName, VarName;
        Params->TryGetStringField(TEXT("blueprint_name"), BPName);
        Params->TryGetStringField(TEXT("variable_name"), VarName);
        UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BPName);
        if (!BP) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
        UEdGraph* Graph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(BP);
        const TArray<TSharedPtr<FJsonValue>>* PosArr = nullptr;
        FVector2D Pos(0, 0);
        if (Params->TryGetArrayField(TEXT("position"), PosArr) && PosArr && PosArr->Num() >= 2)
            Pos = FVector2D((*PosArr)[0]->AsNumber(), (*PosArr)[1]->AsNumber());
        UK2Node_VariableGet* Node = FUnrealMCPCommonUtils::CreateVariableGetNode(Graph, BP, VarName, Pos);
        if (!Node) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create variable get node"));
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
        return R;
    }
    else if (CommandType == TEXT("add_blueprint_variable_set_node"))
    {
        FString BPName, VarName;
        Params->TryGetStringField(TEXT("blueprint_name"), BPName);
        Params->TryGetStringField(TEXT("variable_name"), VarName);
        UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BPName);
        if (!BP) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
        UEdGraph* Graph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(BP);
        const TArray<TSharedPtr<FJsonValue>>* PosArr = nullptr;
        FVector2D Pos(0, 0);
        if (Params->TryGetArrayField(TEXT("position"), PosArr) && PosArr && PosArr->Num() >= 2)
            Pos = FVector2D((*PosArr)[0]->AsNumber(), (*PosArr)[1]->AsNumber());
        UK2Node_VariableSet* Node = FUnrealMCPCommonUtils::CreateVariableSetNode(Graph, BP, VarName, Pos);
        if (!Node) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create variable set node"));
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
        return R;
    }
    else if (CommandType == TEXT("delete_blueprint_node"))
    {
        FString BPName, NodeId;
        Params->TryGetStringField(TEXT("blueprint_name"), BPName);
        Params->TryGetStringField(TEXT("node_id"), NodeId);
        UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BPName);
        if (!BP) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
        UEdGraph* Graph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(BP);
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node && Node->NodeGuid.ToString() == NodeId)
            {
                Graph->RemoveNode(Node);
                FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
                TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
                R->SetStringField(TEXT("deleted"), NodeId);
                return R;
            }
        }
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Node not found"));
    }
    else if (CommandType == TEXT("clear_blueprint_graph"))
    {
        FString BPName;
        Params->TryGetStringField(TEXT("blueprint_name"), BPName);
        UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BPName);
        if (!BP) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
        FString GraphName = TEXT("EventGraph");
        Params->TryGetStringField(TEXT("graph_name"), GraphName);
        UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(BP, GraphName);
        if (!Graph) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
        TArray<UEdGraphNode*> NodesToRemove = Graph->Nodes;
        for (UEdGraphNode* Node : NodesToRemove) { Graph->RemoveNode(Node); }
        FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetNumberField(TEXT("removed"), NodesToRemove.Num());
        return R;
    }
    else if (CommandType == TEXT("disconnect_blueprint_nodes"))
    {
        FString BPName, NodeId, PinName;
        Params->TryGetStringField(TEXT("blueprint_name"), BPName);
        Params->TryGetStringField(TEXT("node_id"), NodeId);
        Params->TryGetStringField(TEXT("pin_name"), PinName);
        UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BPName);
        if (!BP) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
        UEdGraph* Graph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(BP);
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node && Node->NodeGuid.ToString() == NodeId)
            {
                UEdGraphPin* Pin = FUnrealMCPCommonUtils::FindPin(Node, PinName);
                if (Pin) { Pin->BreakAllPinLinks(); }
                FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
                TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
                R->SetStringField(TEXT("disconnected"), PinName);
                return R;
            }
        }
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Node not found"));
    }
    // Stage 2: Blueprint introspection
    else if (CommandType == TEXT("inspect_blueprint"))
    {
        return HandleInspectBlueprint(Params);
    }
    else if (CommandType == TEXT("get_blueprint_graph"))
    {
        return HandleGetBlueprintGraph(Params);
    }
    else if (CommandType == TEXT("get_blueprint_nodes"))
    {
        return HandleGetBlueprintNodes(Params);
    }
    else if (CommandType == TEXT("get_blueprint_variables"))
    {
        return HandleGetBlueprintVariables(Params);
    }
    else if (CommandType == TEXT("get_blueprint_functions"))
    {
        return HandleGetBlueprintFunctions(Params);
    }
    else if (CommandType == TEXT("get_node_pins"))
    {
        return HandleGetNodePins(Params);
    }
    else if (CommandType == TEXT("search_blueprint_nodes"))
    {
        return HandleSearchBlueprintNodes(Params);
    }
    else if (CommandType == TEXT("find_references"))
    {
        return HandleFindReferences(Params);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint node command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleConnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString SourceNodeId;
    if (!Params->TryGetStringField(TEXT("source_node_id"), SourceNodeId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_node_id' parameter"));
    }

    FString TargetNodeId;
    if (!Params->TryGetStringField(TEXT("target_node_id"), TargetNodeId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_node_id' parameter"));
    }

    FString SourcePinName;
    if (!Params->TryGetStringField(TEXT("source_pin"), SourcePinName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_pin' parameter"));
    }

    FString TargetPinName;
    if (!Params->TryGetStringField(TEXT("target_pin"), TargetPinName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_pin' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find the nodes
    UEdGraphNode* SourceNode = nullptr;
    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == SourceNodeId)
        {
            SourceNode = Node;
        }
        else if (Node->NodeGuid.ToString() == TargetNodeId)
        {
            TargetNode = Node;
        }
    }

    if (!SourceNode || !TargetNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Source or target node not found"));
    }

    // Connect the nodes
    if (FUnrealMCPCommonUtils::ConnectGraphNodes(EventGraph, SourceNode, SourcePinName, TargetNode, TargetPinName))
    {
        // Mark the blueprint as modified
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("source_node_id"), SourceNodeId);
        ResultObj->SetStringField(TEXT("target_node_id"), TargetNodeId);
        return ResultObj;
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to connect nodes"));
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintGetSelfComponentReference(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }
    
    // We'll skip component verification since the GetAllNodes API may have changed in UE5.5
    
    // Create the variable get node directly
    UK2Node_VariableGet* GetComponentNode = NewObject<UK2Node_VariableGet>(EventGraph);
    if (!GetComponentNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create get component node"));
    }
    
    // Set up the variable reference properly for UE5.5
    FMemberReference& VarRef = GetComponentNode->VariableReference;
    VarRef.SetSelfMember(FName(*ComponentName));
    
    // Set node position
    GetComponentNode->NodePosX = NodePosition.X;
    GetComponentNode->NodePosY = NodePosition.Y;
    
    // Add to graph
    EventGraph->AddNode(GetComponentNode);
    GetComponentNode->CreateNewGuid();
    GetComponentNode->PostPlacedNewNode();
    GetComponentNode->AllocateDefaultPins();
    
    // Explicitly reconstruct node for UE5.5
    GetComponentNode->ReconstructNode();
    
    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), GetComponentNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintEvent(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString EventName;
    if (!Params->TryGetStringField(TEXT("event_name"), EventName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the event node
    UK2Node_Event* EventNode = FUnrealMCPCommonUtils::CreateEventNode(EventGraph, EventName, NodePosition);
    if (!EventNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create event node"));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), EventNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintFunctionCall(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString FunctionName;
    if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Check for target parameter (optional)
    FString Target;
    Params->TryGetStringField(TEXT("target"), Target);

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find the function
    UFunction* Function = nullptr;
    UK2Node_CallFunction* FunctionNode = nullptr;
    
    // Add extensive logging for debugging
    UE_LOG(LogTemp, Display, TEXT("Looking for function '%s' in target '%s'"), 
           *FunctionName, Target.IsEmpty() ? TEXT("Blueprint") : *Target);
    
    // Check if we have a target class specified
    if (!Target.IsEmpty())
    {
        // Try to find the target class
        UClass* TargetClass = nullptr;
        
        // First try without a prefix
        TargetClass = FindObject<UClass>(static_cast<UObject*>(nullptr), *Target);
        UE_LOG(LogTemp, Display, TEXT("Tried to find class '%s': %s"), 
               *Target, TargetClass ? TEXT("Found") : TEXT("Not found"));
        
        // If not found, try with U prefix (common convention for UE classes)
        if (!TargetClass && !Target.StartsWith(TEXT("U")))
        {
            FString TargetWithPrefix = FString(TEXT("U")) + Target;
            TargetClass = FindObject<UClass>(static_cast<UObject*>(nullptr), *TargetWithPrefix);
            UE_LOG(LogTemp, Display, TEXT("Tried to find class '%s': %s"), 
                   *TargetWithPrefix, TargetClass ? TEXT("Found") : TEXT("Not found"));
        }
        
        // If still not found, try with common component names
        if (!TargetClass)
        {
            // Try some common component class names
            TArray<FString> PossibleClassNames;
            PossibleClassNames.Add(FString(TEXT("U")) + Target + TEXT("Component"));
            PossibleClassNames.Add(Target + TEXT("Component"));
            
            for (const FString& ClassName : PossibleClassNames)
            {
                TargetClass = FindObject<UClass>(static_cast<UObject*>(nullptr), *ClassName);
                if (TargetClass)
                {
                    UE_LOG(LogTemp, Display, TEXT("Found class using alternative name '%s'"), *ClassName);
                    break;
                }
            }
        }
        
        // Special case handling for common classes like UGameplayStatics
        if (!TargetClass && Target == TEXT("UGameplayStatics"))
        {
            // For UGameplayStatics, use a direct reference to known class
            TargetClass = FindObject<UClass>(static_cast<UObject*>(nullptr), TEXT("UGameplayStatics"));
            if (!TargetClass)
            {
                // Try loading it from its known package
                TargetClass = LoadObject<UClass>(nullptr, TEXT("/Script/Engine.GameplayStatics"));
                UE_LOG(LogTemp, Display, TEXT("Explicitly loading GameplayStatics: %s"), 
                       TargetClass ? TEXT("Success") : TEXT("Failed"));
            }
        }
        
        // If we found a target class, look for the function there
        if (TargetClass)
        {
            UE_LOG(LogTemp, Display, TEXT("Looking for function '%s' in class '%s'"), 
                   *FunctionName, *TargetClass->GetName());
                   
            // First try exact name
            Function = TargetClass->FindFunctionByName(*FunctionName);
            
            // If not found, try class hierarchy
            UClass* CurrentClass = TargetClass;
            while (!Function && CurrentClass)
            {
                UE_LOG(LogTemp, Display, TEXT("Searching in class: %s"), *CurrentClass->GetName());
                
                // Try exact match
                Function = CurrentClass->FindFunctionByName(*FunctionName);
                
                // Try case-insensitive match
                if (!Function)
                {
                    for (TFieldIterator<UFunction> FuncIt(CurrentClass); FuncIt; ++FuncIt)
                    {
                        UFunction* AvailableFunc = *FuncIt;
                        UE_LOG(LogTemp, Display, TEXT("  - Available function: %s"), *AvailableFunc->GetName());
                        
                        if (AvailableFunc->GetName().Equals(FunctionName, ESearchCase::IgnoreCase))
                        {
                            UE_LOG(LogTemp, Display, TEXT("  - Found case-insensitive match: %s"), *AvailableFunc->GetName());
                            Function = AvailableFunc;
                            break;
                        }
                    }
                }
                
                // Move to parent class
                CurrentClass = CurrentClass->GetSuperClass();
            }
            
            // Special handling for known functions
            if (!Function)
            {
                if (TargetClass->GetName() == TEXT("GameplayStatics") && 
                    (FunctionName == TEXT("GetActorOfClass") || FunctionName.Equals(TEXT("GetActorOfClass"), ESearchCase::IgnoreCase)))
                {
                    UE_LOG(LogTemp, Display, TEXT("Using special case handling for GameplayStatics::GetActorOfClass"));
                    
                    // Create the function node directly
                    FunctionNode = NewObject<UK2Node_CallFunction>(EventGraph);
                    if (FunctionNode)
                    {
                        // Direct setup for known function
                        FunctionNode->FunctionReference.SetExternalMember(
                            FName(TEXT("GetActorOfClass")), 
                            TargetClass
                        );
                        
                        FunctionNode->NodePosX = NodePosition.X;
                        FunctionNode->NodePosY = NodePosition.Y;
                        EventGraph->AddNode(FunctionNode);
                        FunctionNode->CreateNewGuid();
                        FunctionNode->PostPlacedNewNode();
                        FunctionNode->AllocateDefaultPins();
                        
                        UE_LOG(LogTemp, Display, TEXT("Created GetActorOfClass node directly"));
                        
                        // List all pins
                        for (UEdGraphPin* Pin : FunctionNode->Pins)
                        {
                            UE_LOG(LogTemp, Display, TEXT("  - Pin: %s, Direction: %d, Category: %s"), 
                                   *Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
                        }
                    }
                }
            }
        }
    }
    
    // If we still haven't found the function, try in the blueprint's class
    if (!Function && !FunctionNode)
    {
        UE_LOG(LogTemp, Display, TEXT("Trying to find function in blueprint class"));
        Function = Blueprint->GeneratedClass->FindFunctionByName(*FunctionName);
    }
    
    // Create the function call node if we found the function
    if (Function && !FunctionNode)
    {
        FunctionNode = FUnrealMCPCommonUtils::CreateFunctionCallNode(EventGraph, Function, NodePosition);
    }
    
    if (!FunctionNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function not found: %s in target %s"), *FunctionName, Target.IsEmpty() ? TEXT("Blueprint") : *Target));
    }

    // Set parameters if provided
    if (Params->HasField(TEXT("params")))
    {
        const TSharedPtr<FJsonObject>* ParamsObj;
        if (Params->TryGetObjectField(TEXT("params"), ParamsObj))
        {
            // Process parameters
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Param : (*ParamsObj)->Values)
            {
                const FString& ParamName = Param.Key;
                const TSharedPtr<FJsonValue>& ParamValue = Param.Value;
                
                // Find the parameter pin
                UEdGraphPin* ParamPin = FUnrealMCPCommonUtils::FindPin(FunctionNode, ParamName, EGPD_Input);
                if (ParamPin)
                {
                    UE_LOG(LogTemp, Display, TEXT("Found parameter pin '%s' of category '%s'"), 
                           *ParamName, *ParamPin->PinType.PinCategory.ToString());
                    UE_LOG(LogTemp, Display, TEXT("  Current default value: '%s'"), *ParamPin->DefaultValue);
                    if (ParamPin->PinType.PinSubCategoryObject.IsValid())
                    {
                        UE_LOG(LogTemp, Display, TEXT("  Pin subcategory: '%s'"), 
                               *ParamPin->PinType.PinSubCategoryObject->GetName());
                    }
                    
                    // Set parameter based on type
                    if (ParamValue->Type == EJson::String)
                    {
                        FString StringVal = ParamValue->AsString();
                        UE_LOG(LogTemp, Display, TEXT("  Setting string parameter '%s' to: '%s'"), 
                               *ParamName, *StringVal);
                        
                        // Handle class reference parameters (e.g., ActorClass in GetActorOfClass)
                        if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
                        {
                            // For class references, we require the exact class name with proper prefix
                            // - Actor classes must start with 'A' (e.g., ACameraActor)
                            // - Non-actor classes must start with 'U' (e.g., UObject)
                            const FString& ClassName = StringVal;
                            
                            // TODO: This likely won't work in UE5.5+, so don't rely on it.
                            UClass* Class = FindObject<UClass>(static_cast<UObject*>(nullptr), *ClassName);

                            if (!Class)
                            {
                                Class = LoadObject<UClass>(nullptr, *ClassName);
                                UE_LOG(LogUnrealMCP, Display, TEXT("FindObject<UClass> failed. Assuming soft path  path: %s"), *ClassName);
                            }
                            
                            // If not found, try with Engine module path
                            if (!Class)
                            {
                                FString EngineClassName = FString::Printf(TEXT("/Script/Engine.%s"), *ClassName);
                                Class = LoadObject<UClass>(nullptr, *EngineClassName);
                                UE_LOG(LogUnrealMCP, Display, TEXT("Trying Engine module path: %s"), *EngineClassName);
                            }
                            
                            if (!Class)
                            {
                                UE_LOG(LogUnrealMCP, Error, TEXT("Failed to find class '%s'. Make sure to use the exact class name with proper prefix (A for actors, U for non-actors)"), *ClassName);
                                return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to find class '%s'"), *ClassName));
                            }

                            const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(EventGraph->GetSchema());
                            if (!K2Schema)
                            {
                                UE_LOG(LogUnrealMCP, Error, TEXT("Failed to get K2Schema"));
                                return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get K2Schema"));
                            }

                            K2Schema->TrySetDefaultObject(*ParamPin, Class);
                            if (ParamPin->DefaultObject != Class)
                            {
                                UE_LOG(LogUnrealMCP, Error, TEXT("Failed to set class reference for pin '%s' to '%s'"), *ParamPin->PinName.ToString(), *ClassName);
                                return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to set class reference for pin '%s'"), *ParamPin->PinName.ToString()));
                            }

                            UE_LOG(LogUnrealMCP, Log, TEXT("Successfully set class reference for pin '%s' to '%s'"), *ParamPin->PinName.ToString(), *ClassName);
                            continue;
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
                        {
                            // Ensure we're using an integer value (no decimal)
                            int32 IntValue = FMath::RoundToInt(ParamValue->AsNumber());
                            ParamPin->DefaultValue = FString::FromInt(IntValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set integer parameter '%s' to: %d (string: '%s')"), 
                                   *ParamName, IntValue, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Float)
                        {
                            // For other numeric types
                            float FloatValue = ParamValue->AsNumber();
                            ParamPin->DefaultValue = FString::SanitizeFloat(FloatValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set float parameter '%s' to: %f (string: '%s')"), 
                                   *ParamName, FloatValue, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
                        {
                            bool BoolValue = ParamValue->AsBool();
                            ParamPin->DefaultValue = BoolValue ? TEXT("true") : TEXT("false");
                            UE_LOG(LogTemp, Display, TEXT("  Set boolean parameter '%s' to: %s"), 
                                   *ParamName, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && ParamPin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get())
                        {
                            // Handle array parameters - like Vector parameters
                            const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
                            if (ParamValue->TryGetArray(ArrayValue))
                            {
                                // Check if this could be a vector (array of 3 numbers)
                                if (ArrayValue->Num() == 3)
                                {
                                    // Create a proper vector string: (X=0.0,Y=0.0,Z=1000.0)
                                    float X = (*ArrayValue)[0]->AsNumber();
                                    float Y = (*ArrayValue)[1]->AsNumber();
                                    float Z = (*ArrayValue)[2]->AsNumber();
                                    
                                    FString VectorString = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z);
                                    ParamPin->DefaultValue = VectorString;
                                    
                                    UE_LOG(LogTemp, Display, TEXT("  Set vector parameter '%s' to: %s"), 
                                           *ParamName, *VectorString);
                                    UE_LOG(LogTemp, Display, TEXT("  Final pin value: '%s'"), 
                                           *ParamPin->DefaultValue);
                                }
                                else
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("Array parameter type not fully supported yet"));
                                }
                            }
                        }
                    }
                    else if (ParamValue->Type == EJson::Number)
                    {
                        // Handle integer vs float parameters correctly
                        if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
                        {
                            // Ensure we're using an integer value (no decimal)
                            int32 IntValue = FMath::RoundToInt(ParamValue->AsNumber());
                            ParamPin->DefaultValue = FString::FromInt(IntValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set integer parameter '%s' to: %d (string: '%s')"), 
                                   *ParamName, IntValue, *ParamPin->DefaultValue);
                        }
                        else
                        {
                            // For other numeric types
                            float FloatValue = ParamValue->AsNumber();
                            ParamPin->DefaultValue = FString::SanitizeFloat(FloatValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set float parameter '%s' to: %f (string: '%s')"), 
                                   *ParamName, FloatValue, *ParamPin->DefaultValue);
                        }
                    }
                    else if (ParamValue->Type == EJson::Boolean)
                    {
                        bool BoolValue = ParamValue->AsBool();
                        ParamPin->DefaultValue = BoolValue ? TEXT("true") : TEXT("false");
                        UE_LOG(LogTemp, Display, TEXT("  Set boolean parameter '%s' to: %s"), 
                               *ParamName, *ParamPin->DefaultValue);
                    }
                    else if (ParamValue->Type == EJson::Array)
                    {
                        UE_LOG(LogTemp, Display, TEXT("  Processing array parameter '%s'"), *ParamName);
                        // Handle array parameters - like Vector parameters
                        const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
                        if (ParamValue->TryGetArray(ArrayValue))
                        {
                            // Check if this could be a vector (array of 3 numbers)
                            if (ArrayValue->Num() == 3 && 
                                (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct) &&
                                (ParamPin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get()))
                            {
                                // Create a proper vector string: (X=0.0,Y=0.0,Z=1000.0)
                                float X = (*ArrayValue)[0]->AsNumber();
                                float Y = (*ArrayValue)[1]->AsNumber();
                                float Z = (*ArrayValue)[2]->AsNumber();
                                
                                FString VectorString = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z);
                                ParamPin->DefaultValue = VectorString;
                                
                                UE_LOG(LogTemp, Display, TEXT("  Set vector parameter '%s' to: %s"), 
                                       *ParamName, *VectorString);
                                UE_LOG(LogTemp, Display, TEXT("  Final pin value: '%s'"), 
                                       *ParamPin->DefaultValue);
                            }
                            else
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Array parameter type not fully supported yet"));
                            }
                        }
                    }
                    // Add handling for other types as needed
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Parameter pin '%s' not found"), *ParamName);
                }
            }
        }
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), FunctionNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    FString VariableType;
    if (!Params->TryGetStringField(TEXT("variable_type"), VariableType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_type' parameter"));
    }

    // Get optional parameters
    bool IsExposed = false;
    if (Params->HasField(TEXT("is_exposed")))
    {
        IsExposed = Params->GetBoolField(TEXT("is_exposed"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Create variable based on type
    FEdGraphPinType PinType;
    
    // Set up pin type based on variable_type string
    if (VariableType == TEXT("Boolean"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    }
    else if (VariableType == TEXT("Integer") || VariableType == TEXT("Int"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
    }
    else if (VariableType == TEXT("Float"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Float;
    }
    else if (VariableType == TEXT("String"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_String;
    }
    else if (VariableType == TEXT("Vector"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unsupported variable type: %s"), *VariableType));
    }

    // Create the variable
    FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VariableName), PinType);

    // Set variable properties
    FBPVariableDescription* NewVar = nullptr;
    for (FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName == FName(*VariableName))
        {
            NewVar = &Variable;
            break;
        }
    }

    if (NewVar)
    {
        // Set exposure in editor
        if (IsExposed)
        {
            NewVar->PropertyFlags |= CPF_Edit;
        }
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetStringField(TEXT("variable_type"), VariableType);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintInputActionNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the input action node
    UK2Node_InputAction* InputActionNode = FUnrealMCPCommonUtils::CreateInputActionNode(EventGraph, ActionName, NodePosition);
    if (!InputActionNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create input action node"));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), InputActionNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintSelfReference(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the self node
    UK2Node_Self* SelfNode = FUnrealMCPCommonUtils::CreateSelfReferenceNode(EventGraph, NodePosition);
    if (!SelfNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create self node"));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), SelfNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleFindBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeType;
    if (!Params->TryGetStringField(TEXT("node_type"), NodeType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_type' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create a JSON array for the node GUIDs
    TArray<TSharedPtr<FJsonValue>> NodeGuidArray;
    
    // Filter nodes by the exact requested type
    if (NodeType == TEXT("Event"))
    {
        FString EventName;
        if (!Params->TryGetStringField(TEXT("event_name"), EventName))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name' parameter for Event node search"));
        }
        
        // Look for nodes with exact event name (e.g., ReceiveBeginPlay)
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
            if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
            {
                UE_LOG(LogTemp, Display, TEXT("Found event node with name %s: %s"), *EventName, *EventNode->NodeGuid.ToString());
                NodeGuidArray.Add(MakeShared<FJsonValueString>(EventNode->NodeGuid.ToString()));
            }
        }
    }
    // Add other node types as needed (InputAction, etc.)
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("node_guids"), NodeGuidArray);

    return ResultObj;
}

// ============================================================================
// Stage 2: Blueprint introspection handlers
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleInspectBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!BP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("name"), BP->GetName());
    Result->SetStringField(TEXT("path"), BP->GetPathName());
    Result->SetStringField(TEXT("class"), BP->GeneratedClass ? BP->GeneratedClass->GetName() : TEXT(""));
    Result->SetStringField(TEXT("parent_class"), BP->ParentClass ? BP->ParentClass->GetPathName() : TEXT(""));
    Result->SetStringField(TEXT("blueprint_type"), StaticEnum<EBlueprintType>()->GetValueAsString(BP->BlueprintType));

    // Graphs
    TArray<TSharedPtr<FJsonValue>> GraphArr;
    for (UEdGraph* Graph : BP->UbergraphPages)
    {
        if (Graph)
        {
            TSharedPtr<FJsonObject> G = MakeShared<FJsonObject>();
            G->SetStringField(TEXT("name"), Graph->GetName());
            G->SetStringField(TEXT("type"), TEXT("ubergraph"));
            G->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
            GraphArr.Add(MakeShared<FJsonValueObject>(G));
        }
    }
    for (UEdGraph* Graph : BP->FunctionGraphs)
    {
        if (Graph)
        {
            TSharedPtr<FJsonObject> G = MakeShared<FJsonObject>();
            G->SetStringField(TEXT("name"), Graph->GetName());
            G->SetStringField(TEXT("type"), TEXT("function"));
            G->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
            GraphArr.Add(MakeShared<FJsonValueObject>(G));
        }
    }
    for (UEdGraph* Graph : BP->MacroGraphs)
    {
        if (Graph)
        {
            TSharedPtr<FJsonObject> G = MakeShared<FJsonObject>();
            G->SetStringField(TEXT("name"), Graph->GetName());
            G->SetStringField(TEXT("type"), TEXT("macro"));
            G->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
            GraphArr.Add(MakeShared<FJsonValueObject>(G));
        }
    }
    Result->SetArrayField(TEXT("graphs"), GraphArr);

    // Variables
    TArray<TSharedPtr<FJsonValue>> VarArr;
    for (const FBPVariableDescription& Var : BP->NewVariables)
    {
        TSharedPtr<FJsonObject> V = MakeShared<FJsonObject>();
        V->SetStringField(TEXT("name"), Var.VarName.ToString());
        V->SetStringField(TEXT("type"), Var.VarType.PinCategory.ToString());
        if (Var.VarType.PinSubCategoryObject.IsValid())
        {
            V->SetStringField(TEXT("sub_type"), Var.VarType.PinSubCategoryObject->GetName());
        }
        V->SetBoolField(TEXT("is_array"), Var.VarType.IsArray());
        VarArr.Add(MakeShared<FJsonValueObject>(V));
    }
    Result->SetArrayField(TEXT("variables"), VarArr);

    // Functions (names only in inspect; get_blueprint_functions returns detail)
    TArray<TSharedPtr<FJsonValue>> FuncArr;
    for (UEdGraph* Graph : BP->FunctionGraphs)
    {
        if (Graph)
        {
            FuncArr.Add(MakeShared<FJsonValueString>(Graph->GetName()));
        }
    }
    Result->SetArrayField(TEXT("functions"), FuncArr);

    // Components
    TArray<TSharedPtr<FJsonValue>> CompArr;
    if (BP->SimpleConstructionScript)
    {
        for (USCS_Node* SCSNode : BP->SimpleConstructionScript->GetAllNodes())
        {
            if (SCSNode && SCSNode->ComponentTemplate)
            {
                TSharedPtr<FJsonObject> C = MakeShared<FJsonObject>();
                C->SetStringField(TEXT("name"), SCSNode->GetVariableName().ToString());
                C->SetStringField(TEXT("class"), SCSNode->ComponentTemplate->GetClass()->GetName());
                CompArr.Add(MakeShared<FJsonValueObject>(C));
            }
        }
    }
    Result->SetArrayField(TEXT("components"), CompArr);

    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleGetBlueprintGraph(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString GraphName = TEXT("EventGraph");
    Params->TryGetStringField(TEXT("graph_name"), GraphName);

    UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!BP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(BP, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *GraphName));
    }

    return FUnrealMCPCommonUtils::SerializeGraphToJson(Graph);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleGetBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString GraphName = TEXT("EventGraph");
    Params->TryGetStringField(TEXT("graph_name"), GraphName);

    FString Filter;
    Params->TryGetStringField(TEXT("filter"), Filter);

    UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!BP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(BP, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *GraphName));
    }

    TArray<TSharedPtr<FJsonValue>> NodeArr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node) continue;
        if (!Filter.IsEmpty())
        {
            const FString ClassName = Node->GetClass()->GetName();
            const FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
            if (!ClassName.Contains(Filter, ESearchCase::IgnoreCase) &&
                !Title.Contains(Filter, ESearchCase::IgnoreCase))
            {
                continue;
            }
        }
        NodeArr.Add(MakeShared<FJsonValueObject>(FUnrealMCPCommonUtils::SerializeNodeToJson(Node, false)));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("graph"), Graph->GetName());
    Result->SetNumberField(TEXT("count"), NodeArr.Num());
    Result->SetArrayField(TEXT("nodes"), NodeArr);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleGetBlueprintVariables(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!BP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    TArray<TSharedPtr<FJsonValue>> VarArr;
    for (const FBPVariableDescription& Var : BP->NewVariables)
    {
        TSharedPtr<FJsonObject> V = MakeShared<FJsonObject>();
        V->SetStringField(TEXT("name"), Var.VarName.ToString());
        V->SetStringField(TEXT("type_category"), Var.VarType.PinCategory.ToString());
        V->SetStringField(TEXT("type_sub_category"), Var.VarType.PinSubCategory.ToString());
        if (Var.VarType.PinSubCategoryObject.IsValid())
        {
            V->SetStringField(TEXT("type_object"), Var.VarType.PinSubCategoryObject->GetPathName());
        }
        V->SetBoolField(TEXT("is_array"), Var.VarType.IsArray());
        V->SetBoolField(TEXT("is_set"), Var.VarType.IsSet());
        V->SetBoolField(TEXT("is_map"), Var.VarType.IsMap());
        V->SetStringField(TEXT("default_value"), Var.DefaultValue);
        V->SetBoolField(TEXT("is_instance_editable"), !Var.HasMetaData(FName(TEXT("BlueprintPrivate"))));
        V->SetStringField(TEXT("replication"),
            Var.HasMetaData(FName(TEXT("RepNotify"))) ? TEXT("RepNotify") :
            Var.ReplicationCondition != COND_None ? TEXT("Replicated") : TEXT("None"));
        V->SetStringField(TEXT("category"), Var.Category.ToString());
        VarArr.Add(MakeShared<FJsonValueObject>(V));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetNumberField(TEXT("count"), VarArr.Num());
    Result->SetArrayField(TEXT("variables"), VarArr);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleGetBlueprintFunctions(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!BP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    TArray<TSharedPtr<FJsonValue>> FuncArr;
    for (UEdGraph* FuncGraph : BP->FunctionGraphs)
    {
        if (!FuncGraph) continue;
        TSharedPtr<FJsonObject> F = MakeShared<FJsonObject>();
        F->SetStringField(TEXT("name"), FuncGraph->GetName());
        F->SetNumberField(TEXT("node_count"), FuncGraph->Nodes.Num());

        // Try to find the function entry node to get signature
        for (UEdGraphNode* Node : FuncGraph->Nodes)
        {
            UK2Node_Event* EntryNode = Cast<UK2Node_Event>(Node);
            if (!EntryNode) continue;

            TArray<TSharedPtr<FJsonValue>> ParamArr;
            for (UEdGraphPin* Pin : EntryNode->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
                {
                    TSharedPtr<FJsonObject> P = MakeShared<FJsonObject>();
                    P->SetStringField(TEXT("name"), Pin->PinName.ToString());
                    P->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                    ParamArr.Add(MakeShared<FJsonValueObject>(P));
                }
            }
            F->SetArrayField(TEXT("parameters"), ParamArr);
            break;
        }

        FuncArr.Add(MakeShared<FJsonValueObject>(F));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetNumberField(TEXT("count"), FuncArr.Num());
    Result->SetArrayField(TEXT("functions"), FuncArr);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleGetNodePins(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeId;
    if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
    }

    FString GraphName = TEXT("EventGraph");
    Params->TryGetStringField(TEXT("graph_name"), GraphName);

    UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!BP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(BP, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *GraphName));
    }

    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node && Node->NodeGuid.ToString() == NodeId)
        {
            TargetNode = Node;
            break;
        }
    }

    if (!TargetNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
    }

    TArray<TSharedPtr<FJsonValue>> PinArr;
    for (UEdGraphPin* Pin : TargetNode->Pins)
    {
        PinArr.Add(MakeShared<FJsonValueObject>(FUnrealMCPCommonUtils::SerializePinToJson(Pin)));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("node_id"), NodeId);
    Result->SetStringField(TEXT("node_class"), TargetNode->GetClass()->GetName());
    Result->SetStringField(TEXT("node_title"), TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString());
    Result->SetNumberField(TEXT("pin_count"), PinArr.Num());
    Result->SetArrayField(TEXT("pins"), PinArr);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleSearchBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString Query;
    if (!Params->TryGetStringField(TEXT("query"), Query))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'query' parameter"));
    }

    UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!BP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    TArray<TSharedPtr<FJsonValue>> Matches;
    auto SearchGraph = [&](UEdGraph* Graph)
    {
        if (!Graph) return;
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (!Node) continue;
            const FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
            const FString ClassName = Node->GetClass()->GetName();
            const FString Comment = Node->NodeComment;

            bool bMatch = Title.Contains(Query, ESearchCase::IgnoreCase) ||
                          ClassName.Contains(Query, ESearchCase::IgnoreCase) ||
                          Comment.Contains(Query, ESearchCase::IgnoreCase);

            // Also check pin default values
            if (!bMatch)
            {
                for (UEdGraphPin* Pin : Node->Pins)
                {
                    if (Pin && Pin->DefaultValue.Contains(Query, ESearchCase::IgnoreCase))
                    {
                        bMatch = true;
                        break;
                    }
                }
            }

            if (bMatch)
            {
                TSharedPtr<FJsonObject> M = FUnrealMCPCommonUtils::SerializeNodeToJson(Node, false);
                M->SetStringField(TEXT("graph"), Graph->GetName());
                Matches.Add(MakeShared<FJsonValueObject>(M));
            }
        }
    };

    for (UEdGraph* G : BP->UbergraphPages) SearchGraph(G);
    for (UEdGraph* G : BP->FunctionGraphs) SearchGraph(G);
    for (UEdGraph* G : BP->MacroGraphs) SearchGraph(G);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("query"), Query);
    Result->SetNumberField(TEXT("count"), Matches.Num());
    Result->SetArrayField(TEXT("matches"), Matches);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleFindReferences(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString TargetName;
    if (!Params->TryGetStringField(TEXT("name"), TargetName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter (variable or function name)"));
    }

    UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!BP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    TArray<TSharedPtr<FJsonValue>> Refs;
    auto ScanGraph = [&](UEdGraph* Graph)
    {
        if (!Graph) return;
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (!Node) continue;
            bool bIsRef = false;
            FString RefType;

            if (UK2Node_VariableGet* VG = Cast<UK2Node_VariableGet>(Node))
            {
                if (VG->GetVarNameString().Equals(TargetName, ESearchCase::IgnoreCase))
                {
                    bIsRef = true;
                    RefType = TEXT("variable_get");
                }
            }
            else if (UK2Node_VariableSet* VS = Cast<UK2Node_VariableSet>(Node))
            {
                if (VS->GetVarNameString().Equals(TargetName, ESearchCase::IgnoreCase))
                {
                    bIsRef = true;
                    RefType = TEXT("variable_set");
                }
            }
            else if (UK2Node_CallFunction* CF = Cast<UK2Node_CallFunction>(Node))
            {
                if (UFunction* Fn = CF->GetTargetFunction())
                {
                    if (Fn->GetName().Equals(TargetName, ESearchCase::IgnoreCase))
                    {
                        bIsRef = true;
                        RefType = TEXT("function_call");
                    }
                }
            }

            if (bIsRef)
            {
                TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
                R->SetStringField(TEXT("graph"), Graph->GetName());
                R->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
                R->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
                R->SetStringField(TEXT("reference_type"), RefType);
                R->SetNumberField(TEXT("pos_x"), Node->NodePosX);
                R->SetNumberField(TEXT("pos_y"), Node->NodePosY);
                Refs.Add(MakeShared<FJsonValueObject>(R));
            }
        }
    };

    for (UEdGraph* G : BP->UbergraphPages) ScanGraph(G);
    for (UEdGraph* G : BP->FunctionGraphs) ScanGraph(G);
    for (UEdGraph* G : BP->MacroGraphs) ScanGraph(G);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("name"), TargetName);
    Result->SetNumberField(TEXT("count"), Refs.Num());
    Result->SetArrayField(TEXT("references"), Refs);
    return Result;
} 