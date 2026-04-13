#pragma once

#include "CoreMinimal.h"
#include "Json.h"
#include "Misc/Guid.h"

/**
 * Manages async MCP tasks so long-running ops don't block the socket.
 * Tasks are submitted, run on the game thread, and results are polled later.
 */
class UNREALMCP_API FUnrealMCPTaskManager
{
public:
    enum class ETaskStatus : uint8
    {
        Pending,
        Running,
        Done,
        Failed,
        Cancelled
    };

    struct FTaskState
    {
        FGuid Id;
        FString Type;
        ETaskStatus Status = ETaskStatus::Pending;
        TSharedPtr<FJsonObject> Params;
        TSharedPtr<FJsonObject> Result;
        FString Error;
        FDateTime Created;
        FDateTime Completed;
    };

    static FUnrealMCPTaskManager& Get();

    /** Submit a task. Returns the task ID immediately. Execution starts on the next game-thread tick. */
    FGuid SubmitTask(const FString& TaskType, const TSharedPtr<FJsonObject>& Params);

    /** Get task state. Returns nullptr if not found. */
    const FTaskState* GetTask(const FGuid& TaskId) const;

    /** Wait up to WaitSeconds for a result. Returns current state. */
    const FTaskState* WaitForResult(const FGuid& TaskId, float WaitSeconds = 0.f);

    /** Cancel a pending task. Running tasks cannot be cancelled. */
    bool CancelTask(const FGuid& TaskId);

    /** List all tasks (last hour). */
    TArray<const FTaskState*> ListTasks() const;

    /** Serialize task state to JSON. */
    static TSharedPtr<FJsonObject> TaskStateToJson(const FTaskState& State);

private:
    FUnrealMCPTaskManager() = default;

    void ExecuteTask(FGuid TaskId);
    void EvictOldTasks();

    mutable FCriticalSection Lock;
    TMap<FGuid, TSharedPtr<FTaskState>> Tasks;
};
