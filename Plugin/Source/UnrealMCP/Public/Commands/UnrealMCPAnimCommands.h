#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Animation Blueprint MCP commands (Stage 6).
 * State machine creation, states, transitions, transition rules, blend spaces, notifies, slots.
 */
class UNREALMCP_API FUnrealMCPAnimCommands
{
public:
    FUnrealMCPAnimCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleCreateAnimBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateStateMachine(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddState(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetStateAnimation(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateTransition(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddAnimNotify(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetTransitionRule(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListAnimBlueprintStates(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleInspectAnimBlueprint(const TSharedPtr<FJsonObject>& Params);
};
