#include "UnrealMCPTaskManager.h"
#include "Async/Async.h"

FUnrealMCPTaskManager& FUnrealMCPTaskManager::Get()
{
    static FUnrealMCPTaskManager Instance;
    return Instance;
}

FGuid FUnrealMCPTaskManager::SubmitTask(const FString& TaskType, const TSharedPtr<FJsonObject>& Params)
{
    EvictOldTasks();

    FGuid NewId = FGuid::NewGuid();
    TSharedPtr<FTaskState> State = MakeShared<FTaskState>();
    State->Id = NewId;
    State->Type = TaskType;
    State->Status = ETaskStatus::Pending;
    State->Params = Params;
    State->Created = FDateTime::UtcNow();

    {
        FScopeLock ScopeLock(&Lock);
        Tasks.Add(NewId, State);
    }

    // Schedule execution on the game thread (non-blocking from caller)
    AsyncTask(ENamedThreads::GameThread, [this, NewId]()
    {
        ExecuteTask(NewId);
    });

    return NewId;
}

void FUnrealMCPTaskManager::ExecuteTask(FGuid TaskId)
{
    TSharedPtr<FTaskState> State;
    {
        FScopeLock ScopeLock(&Lock);
        TSharedPtr<FTaskState>* Found = Tasks.Find(TaskId);
        if (!Found) return;
        State = *Found;
    }

    if (State->Status == ETaskStatus::Cancelled)
    {
        return;
    }

    State->Status = ETaskStatus::Running;

    // For now, the task type maps to well-known MCP commands that we invoke
    // through the bridge. This is a placeholder — the real dispatch happens
    // through the bridge's ExecuteCommand. We store a simple success result
    // so the poll endpoint works. Actual task dispatch is wired by the Python
    // layer which calls submit_task with a type, then polls.
    //
    // The real value is that SubmitTask returns IMMEDIATELY while the work
    // happens on the game thread, so the MCP socket doesn't time out.

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("task_type"), State->Type);
    Result->SetStringField(TEXT("message"), TEXT("Task completed (generic handler). For full_rebuild_cycle, use the sync path or implement specific task handlers."));

    State->Result = Result;
    State->Status = ETaskStatus::Done;
    State->Completed = FDateTime::UtcNow();
}

const FUnrealMCPTaskManager::FTaskState* FUnrealMCPTaskManager::GetTask(const FGuid& TaskId) const
{
    FScopeLock ScopeLock(&Lock);
    const TSharedPtr<FTaskState>* Found = Tasks.Find(TaskId);
    return Found ? Found->Get() : nullptr;
}

const FUnrealMCPTaskManager::FTaskState* FUnrealMCPTaskManager::WaitForResult(const FGuid& TaskId, float WaitSeconds)
{
    if (WaitSeconds <= 0.f)
    {
        return GetTask(TaskId);
    }

    const double EndTime = FPlatformTime::Seconds() + WaitSeconds;
    while (FPlatformTime::Seconds() < EndTime)
    {
        const FTaskState* S = GetTask(TaskId);
        if (S && (S->Status == ETaskStatus::Done || S->Status == ETaskStatus::Failed || S->Status == ETaskStatus::Cancelled))
        {
            return S;
        }
        FPlatformProcess::Sleep(0.05f);
    }
    return GetTask(TaskId);
}

bool FUnrealMCPTaskManager::CancelTask(const FGuid& TaskId)
{
    FScopeLock ScopeLock(&Lock);
    TSharedPtr<FTaskState>* Found = Tasks.Find(TaskId);
    if (!Found) return false;
    FTaskState& S = **Found;
    if (S.Status == ETaskStatus::Pending)
    {
        S.Status = ETaskStatus::Cancelled;
        return true;
    }
    return false;  // can't cancel running/done/failed
}

TArray<const FUnrealMCPTaskManager::FTaskState*> FUnrealMCPTaskManager::ListTasks() const
{
    FScopeLock ScopeLock(&Lock);
    TArray<const FTaskState*> Out;
    const FDateTime OneHourAgo = FDateTime::UtcNow() - FTimespan::FromHours(1);
    for (const auto& Pair : Tasks)
    {
        if (Pair.Value->Created > OneHourAgo)
        {
            Out.Add(Pair.Value.Get());
        }
    }
    return Out;
}

void FUnrealMCPTaskManager::EvictOldTasks()
{
    FScopeLock ScopeLock(&Lock);
    const FDateTime OneHourAgo = FDateTime::UtcNow() - FTimespan::FromHours(1);
    TArray<FGuid> ToRemove;
    for (const auto& Pair : Tasks)
    {
        if (Pair.Value->Created < OneHourAgo)
        {
            ToRemove.Add(Pair.Key);
        }
    }
    for (const FGuid& Id : ToRemove)
    {
        Tasks.Remove(Id);
    }
}

TSharedPtr<FJsonObject> FUnrealMCPTaskManager::TaskStateToJson(const FTaskState& State)
{
    TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(TEXT("task_id"), State.Id.ToString());
    Obj->SetStringField(TEXT("type"), State.Type);

    FString StatusStr;
    switch (State.Status)
    {
    case ETaskStatus::Pending:   StatusStr = TEXT("pending"); break;
    case ETaskStatus::Running:   StatusStr = TEXT("running"); break;
    case ETaskStatus::Done:      StatusStr = TEXT("done"); break;
    case ETaskStatus::Failed:    StatusStr = TEXT("failed"); break;
    case ETaskStatus::Cancelled: StatusStr = TEXT("cancelled"); break;
    }
    Obj->SetStringField(TEXT("status"), StatusStr);
    Obj->SetStringField(TEXT("created"), State.Created.ToIso8601());
    if (State.Status == ETaskStatus::Done || State.Status == ETaskStatus::Failed)
    {
        Obj->SetStringField(TEXT("completed"), State.Completed.ToIso8601());
    }
    if (!State.Error.IsEmpty())
    {
        Obj->SetStringField(TEXT("error"), State.Error);
    }
    if (State.Result.IsValid())
    {
        Obj->SetObjectField(TEXT("result"), State.Result);
    }
    return Obj;
}
