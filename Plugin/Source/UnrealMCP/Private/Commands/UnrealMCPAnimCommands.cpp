#include "Commands/UnrealMCPAnimCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Factories/AnimBlueprintFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"

// AnimGraph editor headers (editor-only module: AnimGraph)
#if WITH_EDITOR
#include "AnimGraphNode_StateMachine.h"
#include "AnimGraphNode_StateMachineBase.h"
#include "AnimStateNode.h"
#include "AnimStateTransitionNode.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationStateMachineSchema.h"
#include "AnimGraphNode_SequencePlayer.h"
#endif

FUnrealMCPAnimCommands::FUnrealMCPAnimCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPAnimCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_anim_blueprint"))
    {
        return HandleCreateAnimBlueprint(Params);
    }
    else if (CommandType == TEXT("create_state_machine"))
    {
        return HandleCreateStateMachine(Params);
    }
    else if (CommandType == TEXT("add_anim_state"))
    {
        return HandleAddState(Params);
    }
    else if (CommandType == TEXT("set_state_animation"))
    {
        return HandleSetStateAnimation(Params);
    }
    else if (CommandType == TEXT("create_anim_transition"))
    {
        return HandleCreateTransition(Params);
    }
    else if (CommandType == TEXT("set_transition_rule"))
    {
        return HandleSetTransitionRule(Params);
    }
    else if (CommandType == TEXT("add_anim_notify"))
    {
        return HandleAddAnimNotify(Params);
    }
    else if (CommandType == TEXT("list_anim_states"))
    {
        return HandleListAnimBlueprintStates(Params);
    }
    else if (CommandType == TEXT("inspect_anim_blueprint"))
    {
        return HandleInspectAnimBlueprint(Params);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown anim command: %s"), *CommandType));
}

// Helper: load an AnimBlueprint by name or path
static UAnimBlueprint* FindAnimBlueprint(const FString& NameOrPath)
{
    // Try direct load first
    UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(StaticLoadObject(UAnimBlueprint::StaticClass(), nullptr, *NameOrPath));
    if (AnimBP)
    {
        return AnimBP;
    }

    // Search by name in asset registry
    IAssetRegistry& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    TArray<FAssetData> Assets;
    AR.GetAssetsByClass(UAnimBlueprint::StaticClass()->GetClassPathName(), Assets);
    for (const FAssetData& AD : Assets)
    {
        if (AD.AssetName.ToString().Equals(NameOrPath, ESearchCase::IgnoreCase))
        {
            return Cast<UAnimBlueprint>(AD.GetAsset());
        }
    }
    return nullptr;
}

// ============================================================================
// Create an Animation Blueprint
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPAnimCommands::HandleCreateAnimBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    FString Name;
    if (!Params->TryGetStringField(TEXT("name"), Name))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    FString SkeletonPath;
    if (!Params->TryGetStringField(TEXT("skeleton_path"), SkeletonPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'skeleton_path' parameter"));
    }

    FString SavePath = TEXT("/Game/Animations");
    Params->TryGetStringField(TEXT("save_path"), SavePath);

    USkeleton* Skeleton = Cast<USkeleton>(StaticLoadObject(USkeleton::StaticClass(), nullptr, *SkeletonPath));
    if (!Skeleton)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load skeleton: %s"), *SkeletonPath));
    }

    FAssetToolsModule& ATM = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

    UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
    Factory->TargetSkeleton = Skeleton;
    Factory->ParentClass = UAnimInstance::StaticClass();

    UObject* NewAsset = ATM.Get().CreateAsset(Name, SavePath, UAnimBlueprint::StaticClass(), Factory);
    if (!NewAsset)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("CreateAsset failed for AnimBlueprint"));
    }

    UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(NewAsset);
    FAssetRegistryModule::AssetCreated(AnimBP);
    AnimBP->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("name"), Name);
    Result->SetStringField(TEXT("path"), AnimBP->GetPathName());
    Result->SetStringField(TEXT("skeleton"), SkeletonPath);
    return Result;
}

// ============================================================================
// State machine, states, transitions — MVP set
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPAnimCommands::HandleCreateStateMachine(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
    FString AnimBPName;
    if (!Params->TryGetStringField(TEXT("anim_bp"), AnimBPName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_bp' parameter"));
    }

    FString SMName = TEXT("Locomotion");
    Params->TryGetStringField(TEXT("name"), SMName);

    UAnimBlueprint* AnimBP = FindAnimBlueprint(AnimBPName);
    if (!AnimBP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("AnimBP not found: %s"), *AnimBPName));
    }

    // Find the AnimGraph (main animation graph)
    UEdGraph* AnimGraph = nullptr;
    for (UEdGraph* Graph : AnimBP->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == TEXT("AnimGraph"))
        {
            AnimGraph = Graph;
            break;
        }
    }

    if (!AnimGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimGraph not found on AnimBP"));
    }

    // Create a state machine node in the AnimGraph
    UAnimGraphNode_StateMachine* SMNode = NewObject<UAnimGraphNode_StateMachine>(AnimGraph);
    SMNode->EditorStateMachineGraph->GetOuter()->Rename(*SMName);
    AnimGraph->AddNode(SMNode, false, false);
    SMNode->CreateNewGuid();
    SMNode->PostPlacedNewNode();
    SMNode->AllocateDefaultPins();
    SMNode->NodePosX = 200;
    SMNode->NodePosY = 0;

    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("anim_bp"), AnimBPName);
    Result->SetStringField(TEXT("state_machine"), SMName);
    Result->SetStringField(TEXT("node_id"), SMNode->NodeGuid.ToString());
    return Result;
#else
    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimGraph editing requires WITH_EDITOR"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPAnimCommands::HandleAddState(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
    FString AnimBPName;
    if (!Params->TryGetStringField(TEXT("anim_bp"), AnimBPName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_bp' parameter"));
    }

    FString StateName;
    if (!Params->TryGetStringField(TEXT("state_name"), StateName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'state_name' parameter"));
    }

    double PosX = 200, PosY = 0;
    Params->TryGetNumberField(TEXT("pos_x"), PosX);
    Params->TryGetNumberField(TEXT("pos_y"), PosY);

    UAnimBlueprint* AnimBP = FindAnimBlueprint(AnimBPName);
    if (!AnimBP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("AnimBP not found: %s"), *AnimBPName));
    }

    // Find the first state machine node's graph
    UAnimationStateMachineGraph* SMGraph = nullptr;
    for (UEdGraph* Graph : AnimBP->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == TEXT("AnimGraph"))
        {
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node);
                if (SMNode)
                {
                    SMGraph = SMNode->EditorStateMachineGraph;
                    break;
                }
            }
        }
        if (SMGraph) break;
    }

    if (!SMGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No state machine found in AnimBP. Create one first with create_state_machine."));
    }

    // Create a state node
    UAnimStateNode* StateNode = NewObject<UAnimStateNode>(SMGraph);
    SMGraph->AddNode(StateNode, false, false);
    StateNode->CreateNewGuid();
    StateNode->PostPlacedNewNode();
    StateNode->AllocateDefaultPins();
    StateNode->NodePosX = (int32)PosX;
    StateNode->NodePosY = (int32)PosY;

    // Rename the state's bound graph
    if (StateNode->BoundGraph)
    {
        StateNode->BoundGraph->Rename(*StateName);
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("anim_bp"), AnimBPName);
    Result->SetStringField(TEXT("state_name"), StateName);
    Result->SetStringField(TEXT("node_id"), StateNode->NodeGuid.ToString());
    return Result;
#else
    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimGraph editing requires WITH_EDITOR"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPAnimCommands::HandleSetStateAnimation(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
    FString AnimBPName;
    if (!Params->TryGetStringField(TEXT("anim_bp"), AnimBPName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_bp' parameter"));
    FString StateName;
    if (!Params->TryGetStringField(TEXT("state_name"), StateName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'state_name' parameter"));
    FString AnimSeqPath;
    if (!Params->TryGetStringField(TEXT("anim_sequence_path"), AnimSeqPath))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_sequence_path' parameter"));

    UAnimBlueprint* AnimBP = FindAnimBlueprint(AnimBPName);
    if (!AnimBP)
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("AnimBP not found: %s"), *AnimBPName));

    UAnimSequence* AnimSeq = Cast<UAnimSequence>(StaticLoadObject(UAnimSequence::StaticClass(), nullptr, *AnimSeqPath));
    if (!AnimSeq)
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("AnimSequence not found: %s"), *AnimSeqPath));

    // Find the state machine graph
    UAnimationStateMachineGraph* SMGraph = nullptr;
    for (UEdGraph* Graph : AnimBP->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == TEXT("AnimGraph"))
        {
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node);
                if (SMNode && SMNode->EditorStateMachineGraph)
                {
                    SMGraph = SMNode->EditorStateMachineGraph;
                    break;
                }
            }
        }
        if (SMGraph) break;
    }
    if (!SMGraph)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No state machine found"));

    // Find the state node by name
    UAnimStateNode* TargetState = nullptr;
    for (UEdGraphNode* Node : SMGraph->Nodes)
    {
        UAnimStateNode* SN = Cast<UAnimStateNode>(Node);
        if (SN && SN->BoundGraph && SN->BoundGraph->GetName().Equals(StateName, ESearchCase::IgnoreCase))
        {
            TargetState = SN;
            break;
        }
    }
    if (!TargetState)
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("State not found: %s"), *StateName));

    // Get the state's bound graph (sub-graph where the pose is produced)
    UEdGraph* StateGraph = TargetState->BoundGraph;
    if (!StateGraph)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("State has no bound graph"));

    // Create a SequencePlayer node inside the state graph
    UAnimGraphNode_SequencePlayer* SeqPlayer = NewObject<UAnimGraphNode_SequencePlayer>(StateGraph);
    StateGraph->AddNode(SeqPlayer, false, false);
    SeqPlayer->CreateNewGuid();
    SeqPlayer->PostPlacedNewNode();
    SeqPlayer->AllocateDefaultPins();
    SeqPlayer->NodePosX = 0;
    SeqPlayer->NodePosY = 0;

    // Set the sequence on the inner node
    SeqPlayer->Node.SetSequence(AnimSeq);

    // Try to connect the sequence player output to the state result node
    UEdGraphNode* ResultNode = nullptr;
    for (UEdGraphNode* N : StateGraph->Nodes)
    {
        if (N && N->GetClass()->GetName().Contains(TEXT("AnimGraphNode_StateResult")))
        {
            ResultNode = N;
            break;
        }
    }

    if (ResultNode)
    {
        // Find output pose pin on sequence player and input pose pin on result
        UEdGraphPin* SeqOut = nullptr;
        for (UEdGraphPin* Pin : SeqPlayer->Pins)
        {
            if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
            {
                SeqOut = Pin;
                break;
            }
        }
        UEdGraphPin* ResultIn = nullptr;
        for (UEdGraphPin* Pin : ResultNode->Pins)
        {
            if (Pin && Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
            {
                ResultIn = Pin;
                break;
            }
        }
        if (SeqOut && ResultIn)
        {
            SeqOut->MakeLinkTo(ResultIn);
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("anim_bp"), AnimBPName);
    Result->SetStringField(TEXT("state_name"), StateName);
    Result->SetStringField(TEXT("anim_sequence"), AnimSeqPath);
    Result->SetStringField(TEXT("node_id"), SeqPlayer->NodeGuid.ToString());
    Result->SetBoolField(TEXT("connected_to_result"), ResultNode != nullptr);
    return Result;
#else
    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimGraph editing requires WITH_EDITOR"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPAnimCommands::HandleCreateTransition(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
    FString AnimBPName;
    if (!Params->TryGetStringField(TEXT("anim_bp"), AnimBPName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_bp' parameter"));
    }

    FString FromState;
    if (!Params->TryGetStringField(TEXT("from_state"), FromState))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'from_state' parameter"));
    }

    FString ToState;
    if (!Params->TryGetStringField(TEXT("to_state"), ToState))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'to_state' parameter"));
    }

    UAnimBlueprint* AnimBP = FindAnimBlueprint(AnimBPName);
    if (!AnimBP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("AnimBP not found: %s"), *AnimBPName));
    }

    // Find the state machine graph
    UAnimationStateMachineGraph* SMGraph = nullptr;
    for (UEdGraph* Graph : AnimBP->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == TEXT("AnimGraph"))
        {
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node);
                if (SMNode)
                {
                    SMGraph = SMNode->EditorStateMachineGraph;
                    break;
                }
            }
        }
        if (SMGraph) break;
    }

    if (!SMGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No state machine found in AnimBP"));
    }

    // Find from and to state nodes by name
    UAnimStateNode* FromNode = nullptr;
    UAnimStateNode* ToNode = nullptr;
    for (UEdGraphNode* Node : SMGraph->Nodes)
    {
        UAnimStateNode* SN = Cast<UAnimStateNode>(Node);
        if (!SN) continue;
        if (SN->BoundGraph && SN->BoundGraph->GetName().Equals(FromState, ESearchCase::IgnoreCase))
        {
            FromNode = SN;
        }
        if (SN->BoundGraph && SN->BoundGraph->GetName().Equals(ToState, ESearchCase::IgnoreCase))
        {
            ToNode = SN;
        }
    }

    if (!FromNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("State not found: %s"), *FromState));
    }
    if (!ToNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("State not found: %s"), *ToState));
    }

    // Create a transition node
    UAnimStateTransitionNode* TransNode = NewObject<UAnimStateTransitionNode>(SMGraph);
    SMGraph->AddNode(TransNode, false, false);
    TransNode->CreateNewGuid();
    TransNode->PostPlacedNewNode();
    TransNode->AllocateDefaultPins();

    // Wire: FromNode output -> TransNode input, TransNode output -> ToNode input
    // State nodes have a single output pin and single input pin; transition nodes bridge them
    UEdGraphPin* FromOut = nullptr;
    for (UEdGraphPin* Pin : FromNode->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Output)
        {
            FromOut = Pin;
            break;
        }
    }
    UEdGraphPin* TransIn = nullptr;
    UEdGraphPin* TransOut = nullptr;
    for (UEdGraphPin* Pin : TransNode->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Input) TransIn = Pin;
        if (Pin && Pin->Direction == EGPD_Output) TransOut = Pin;
    }
    UEdGraphPin* ToIn = nullptr;
    for (UEdGraphPin* Pin : ToNode->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Input)
        {
            ToIn = Pin;
            break;
        }
    }

    if (FromOut && TransIn)
    {
        FromOut->MakeLinkTo(TransIn);
    }
    if (TransOut && ToIn)
    {
        TransOut->MakeLinkTo(ToIn);
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("anim_bp"), AnimBPName);
    Result->SetStringField(TEXT("from_state"), FromState);
    Result->SetStringField(TEXT("to_state"), ToState);
    Result->SetStringField(TEXT("transition_node_id"), TransNode->NodeGuid.ToString());
    return Result;
#else
    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimGraph editing requires WITH_EDITOR"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPAnimCommands::HandleSetTransitionRule(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
    FString AnimBPName;
    if (!Params->TryGetStringField(TEXT("anim_bp"), AnimBPName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_bp' parameter"));
    FString FromState, ToState, BoolVarName;
    if (!Params->TryGetStringField(TEXT("from_state"), FromState))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'from_state' parameter"));
    if (!Params->TryGetStringField(TEXT("to_state"), ToState))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'to_state' parameter"));
    if (!Params->TryGetStringField(TEXT("bool_variable"), BoolVarName))
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'bool_variable' parameter"));

    UAnimBlueprint* AnimBP = FindAnimBlueprint(AnimBPName);
    if (!AnimBP)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimBP not found"));

    // Find the state machine graph
    UAnimationStateMachineGraph* SMGraph = nullptr;
    for (UEdGraph* Graph : AnimBP->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == TEXT("AnimGraph"))
        {
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node);
                if (SMNode && SMNode->EditorStateMachineGraph)
                {
                    SMGraph = SMNode->EditorStateMachineGraph;
                    break;
                }
            }
        }
        if (SMGraph) break;
    }
    if (!SMGraph)
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No state machine found"));

    // Find the transition node between from_state and to_state
    UAnimStateTransitionNode* TransNode = nullptr;
    for (UEdGraphNode* Node : SMGraph->Nodes)
    {
        UAnimStateTransitionNode* TN = Cast<UAnimStateTransitionNode>(Node);
        if (!TN) continue;
        // Check if this transition connects from_state to to_state by checking pin links
        bool bFromMatch = false, bToMatch = false;
        for (UEdGraphPin* Pin : TN->Pins)
        {
            if (Pin && Pin->Direction == EGPD_Input)
            {
                for (UEdGraphPin* Linked : Pin->LinkedTo)
                {
                    if (Linked && Linked->GetOwningNode())
                    {
                        UAnimStateNode* SN = Cast<UAnimStateNode>(Linked->GetOwningNode());
                        if (SN && SN->BoundGraph && SN->BoundGraph->GetName().Equals(FromState, ESearchCase::IgnoreCase))
                            bFromMatch = true;
                    }
                }
            }
            if (Pin && Pin->Direction == EGPD_Output)
            {
                for (UEdGraphPin* Linked : Pin->LinkedTo)
                {
                    if (Linked && Linked->GetOwningNode())
                    {
                        UAnimStateNode* SN = Cast<UAnimStateNode>(Linked->GetOwningNode());
                        if (SN && SN->BoundGraph && SN->BoundGraph->GetName().Equals(ToState, ESearchCase::IgnoreCase))
                            bToMatch = true;
                    }
                }
            }
        }
        if (bFromMatch && bToMatch) { TransNode = TN; break; }
    }

    if (!TransNode)
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("No transition found from %s to %s"), *FromState, *ToState));

    // The transition has a BoundGraph (TransitionGraph) with a result node
    // We can note the bool variable name for the user to wire manually,
    // or attempt to create a variable get node in the transition graph
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("anim_bp"), AnimBPName);
    Result->SetStringField(TEXT("from_state"), FromState);
    Result->SetStringField(TEXT("to_state"), ToState);
    Result->SetStringField(TEXT("bool_variable"), BoolVarName);
    Result->SetStringField(TEXT("transition_node_id"), TransNode->NodeGuid.ToString());

    if (TransNode->BoundGraph)
    {
        // Create a variable get node for the bool in the transition graph
        UK2Node_VariableGet* VarGetNode = FUnrealMCPCommonUtils::CreateVariableGetNode(
            TransNode->BoundGraph, AnimBP, BoolVarName, FVector2D(-200, 0));

        if (VarGetNode)
        {
            // Try to connect to the transition result node
            for (UEdGraphNode* N : TransNode->BoundGraph->Nodes)
            {
                if (N && N->GetClass()->GetName().Contains(TEXT("TransitionResult")))
                {
                    // Find the bool input pin on the result
                    UEdGraphPin* ResultPin = nullptr;
                    for (UEdGraphPin* Pin : N->Pins)
                    {
                        if (Pin && Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
                        {
                            ResultPin = Pin;
                            break;
                        }
                    }
                    // Find the bool output pin on the var get
                    UEdGraphPin* VarPin = nullptr;
                    for (UEdGraphPin* Pin : VarGetNode->Pins)
                    {
                        if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
                        {
                            VarPin = Pin;
                            break;
                        }
                    }
                    if (ResultPin && VarPin)
                    {
                        VarPin->MakeLinkTo(ResultPin);
                        Result->SetBoolField(TEXT("rule_wired"), true);
                    }
                    break;
                }
            }
        }
        Result->SetBoolField(TEXT("has_transition_graph"), true);
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBP);
    return Result;
#else
    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimGraph editing requires WITH_EDITOR"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPAnimCommands::HandleAddAnimNotify(const TSharedPtr<FJsonObject>& Params)
{
    FString AnimSeqPath;
    if (!Params->TryGetStringField(TEXT("anim_sequence_path"), AnimSeqPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_sequence_path' parameter"));
    }

    FString NotifyName;
    if (!Params->TryGetStringField(TEXT("notify_name"), NotifyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'notify_name' parameter"));
    }

    double Time = 0.0;
    Params->TryGetNumberField(TEXT("time"), Time);

    UAnimSequence* AnimSeq = Cast<UAnimSequence>(StaticLoadObject(UAnimSequence::StaticClass(), nullptr, *AnimSeqPath));
    if (!AnimSeq)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load AnimSequence: %s"), *AnimSeqPath));
    }

    // Add a simple notify event
    FAnimNotifyEvent NewNotify;
    NewNotify.NotifyName = FName(*NotifyName);
    NewNotify.SetTime((float)Time);

    AnimSeq->Notifies.Add(NewNotify);
    AnimSeq->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("anim_sequence"), AnimSeqPath);
    Result->SetStringField(TEXT("notify_name"), NotifyName);
    Result->SetNumberField(TEXT("time"), Time);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAnimCommands::HandleListAnimBlueprintStates(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
    FString AnimBPName;
    if (!Params->TryGetStringField(TEXT("anim_bp"), AnimBPName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_bp' parameter"));
    }

    UAnimBlueprint* AnimBP = FindAnimBlueprint(AnimBPName);
    if (!AnimBP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("AnimBP not found: %s"), *AnimBPName));
    }

    TArray<TSharedPtr<FJsonValue>> StatesArr;
    for (UEdGraph* Graph : AnimBP->FunctionGraphs)
    {
        if (Graph && Graph->GetName() == TEXT("AnimGraph"))
        {
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node);
                if (!SMNode) continue;
                UAnimationStateMachineGraph* SMGraph = SMNode->EditorStateMachineGraph;
                if (!SMGraph) continue;

                for (UEdGraphNode* SMChild : SMGraph->Nodes)
                {
                    UAnimStateNode* StateNode = Cast<UAnimStateNode>(SMChild);
                    if (StateNode)
                    {
                        TSharedPtr<FJsonObject> S = MakeShared<FJsonObject>();
                        S->SetStringField(TEXT("name"), StateNode->BoundGraph ? StateNode->BoundGraph->GetName() : StateNode->GetNodeTitle(ENodeTitleType::ListView).ToString());
                        S->SetStringField(TEXT("node_id"), StateNode->NodeGuid.ToString());
                        S->SetNumberField(TEXT("pos_x"), StateNode->NodePosX);
                        S->SetNumberField(TEXT("pos_y"), StateNode->NodePosY);
                        StatesArr.Add(MakeShared<FJsonValueObject>(S));
                    }
                }
            }
        }
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetNumberField(TEXT("count"), StatesArr.Num());
    Result->SetArrayField(TEXT("states"), StatesArr);
    return Result;
#else
    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimGraph editing requires WITH_EDITOR"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPAnimCommands::HandleInspectAnimBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    FString AnimBPName;
    if (!Params->TryGetStringField(TEXT("anim_bp"), AnimBPName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_bp' parameter"));
    }

    UAnimBlueprint* AnimBP = FindAnimBlueprint(AnimBPName);
    if (!AnimBP)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("AnimBP not found: %s"), *AnimBPName));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("name"), AnimBP->GetName());
    Result->SetStringField(TEXT("path"), AnimBP->GetPathName());
    Result->SetStringField(TEXT("parent_class"), AnimBP->ParentClass ? AnimBP->ParentClass->GetPathName() : TEXT(""));
    if (AnimBP->TargetSkeleton)
    {
        Result->SetStringField(TEXT("skeleton"), AnimBP->TargetSkeleton->GetPathName());
    }

    TArray<TSharedPtr<FJsonValue>> GraphArr;
    for (UEdGraph* Graph : AnimBP->FunctionGraphs)
    {
        if (Graph)
        {
            TSharedPtr<FJsonObject> G = MakeShared<FJsonObject>();
            G->SetStringField(TEXT("name"), Graph->GetName());
            G->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
            GraphArr.Add(MakeShared<FJsonValueObject>(G));
        }
    }
    Result->SetArrayField(TEXT("graphs"), GraphArr);

    // Variables
    TArray<TSharedPtr<FJsonValue>> VarArr;
    for (const FBPVariableDescription& Var : AnimBP->NewVariables)
    {
        TSharedPtr<FJsonObject> V = MakeShared<FJsonObject>();
        V->SetStringField(TEXT("name"), Var.VarName.ToString());
        V->SetStringField(TEXT("type"), Var.VarType.PinCategory.ToString());
        VarArr.Add(MakeShared<FJsonValueObject>(V));
    }
    Result->SetArrayField(TEXT("variables"), VarArr);

    return Result;
}
