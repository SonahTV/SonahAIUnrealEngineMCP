#include "Commands/UnrealMCPProjectCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "GameFramework/InputSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "EditorReimportHandler.h"
#include "UObject/UObjectIterator.h"

FUnrealMCPProjectCommands::FUnrealMCPProjectCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_input_mapping"))
    {
        return HandleCreateInputMapping(Params);
    }
    // Stage 5: Enhanced Input
    else if (CommandType == TEXT("list_input_actions"))
    {
        return HandleListInputActions(Params);
    }
    else if (CommandType == TEXT("remove_input_mapping"))
    {
        return HandleRemoveInputMapping(Params);
    }
    // Stage 3: Asset operations
    else if (CommandType == TEXT("asset_search"))
    {
        return HandleAssetSearch(Params);
    }
    else if (CommandType == TEXT("asset_dependencies"))
    {
        return HandleAssetDependencies(Params);
    }
    else if (CommandType == TEXT("asset_referencers"))
    {
        return HandleAssetReferencers(Params);
    }
    else if (CommandType == TEXT("duplicate_asset"))
    {
        return HandleDuplicateAsset(Params);
    }
    else if (CommandType == TEXT("rename_asset"))
    {
        return HandleRenameAsset(Params);
    }
    else if (CommandType == TEXT("move_asset"))
    {
        return HandleMoveAsset(Params);
    }
    else if (CommandType == TEXT("reimport_asset"))
    {
        return HandleReimportAsset(Params);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown project command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCreateInputMapping(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    FString Key;
    if (!Params->TryGetStringField(TEXT("key"), Key))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
    }

    // Get the input settings
    UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
    if (!InputSettings)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get input settings"));
    }

    // Create the input action mapping
    FInputActionKeyMapping ActionMapping;
    ActionMapping.ActionName = FName(*ActionName);
    ActionMapping.Key = FKey(*Key);

    // Add modifiers if provided
    if (Params->HasField(TEXT("shift")))
    {
        ActionMapping.bShift = Params->GetBoolField(TEXT("shift"));
    }
    if (Params->HasField(TEXT("ctrl")))
    {
        ActionMapping.bCtrl = Params->GetBoolField(TEXT("ctrl"));
    }
    if (Params->HasField(TEXT("alt")))
    {
        ActionMapping.bAlt = Params->GetBoolField(TEXT("alt"));
    }
    if (Params->HasField(TEXT("cmd")))
    {
        ActionMapping.bCmd = Params->GetBoolField(TEXT("cmd"));
    }

    // Add the mapping
    InputSettings->AddActionMapping(ActionMapping);
    InputSettings->SaveConfig();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("action_name"), ActionName);
    ResultObj->SetStringField(TEXT("key"), Key);
    return ResultObj;
}

// ============================================================================
// Stage 3: Asset operations
// ============================================================================

static TSharedPtr<FJsonObject> AssetDataToJson(const FAssetData& AssetData)
{
    TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
    Obj->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
    Obj->SetStringField(TEXT("package"), AssetData.PackageName.ToString());
    Obj->SetStringField(TEXT("class"), AssetData.AssetClassPath.GetAssetName().ToString());
    return Obj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleAssetSearch(const TSharedPtr<FJsonObject>& Params)
{
    IAssetRegistry& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

    FARFilter Filter;

    FString PathFilter;
    if (Params->TryGetStringField(TEXT("path_filter"), PathFilter))
    {
        Filter.PackagePaths.Add(FName(*PathFilter));
        Filter.bRecursivePaths = true;
    }

    FString ClassFilter;
    if (Params->TryGetStringField(TEXT("class_filter"), ClassFilter))
    {
        Filter.ClassPaths.Add(FTopLevelAssetPath(FName(TEXT("/Script/Engine")), FName(*ClassFilter)));
    }

    double Limit = 100;
    Params->TryGetNumberField(TEXT("limit"), Limit);
    double Offset = 0;
    Params->TryGetNumberField(TEXT("offset"), Offset);

    FString Query;
    Params->TryGetStringField(TEXT("query"), Query);

    TArray<FAssetData> AllAssets;
    AR.GetAssets(Filter, AllAssets);

    // Client-side substring filter on asset name
    TArray<FAssetData> Filtered;
    for (const FAssetData& A : AllAssets)
    {
        if (Query.IsEmpty() || A.AssetName.ToString().Contains(Query, ESearchCase::IgnoreCase))
        {
            Filtered.Add(A);
        }
    }

    int32 Start = FMath::Clamp((int32)Offset, 0, Filtered.Num());
    int32 End = FMath::Clamp(Start + (int32)Limit, Start, Filtered.Num());

    TArray<TSharedPtr<FJsonValue>> Arr;
    for (int32 i = Start; i < End; ++i)
    {
        Arr.Add(MakeShared<FJsonValueObject>(AssetDataToJson(Filtered[i])));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetNumberField(TEXT("total"), Filtered.Num());
    Result->SetNumberField(TEXT("offset"), Offset);
    Result->SetNumberField(TEXT("count"), Arr.Num());
    Result->SetArrayField(TEXT("assets"), Arr);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleAssetDependencies(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    IAssetRegistry& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

    TArray<FName> Deps;
    AR.GetDependencies(FName(*AssetPath), Deps);

    TArray<TSharedPtr<FJsonValue>> Arr;
    for (const FName& Dep : Deps)
    {
        Arr.Add(MakeShared<FJsonValueString>(Dep.ToString()));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("asset_path"), AssetPath);
    Result->SetNumberField(TEXT("count"), Arr.Num());
    Result->SetArrayField(TEXT("dependencies"), Arr);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleAssetReferencers(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    IAssetRegistry& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

    TArray<FName> Refs;
    AR.GetReferencers(FName(*AssetPath), Refs);

    TArray<TSharedPtr<FJsonValue>> Arr;
    for (const FName& Ref : Refs)
    {
        Arr.Add(MakeShared<FJsonValueString>(Ref.ToString()));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("asset_path"), AssetPath);
    Result->SetNumberField(TEXT("count"), Arr.Num());
    Result->SetArrayField(TEXT("referencers"), Arr);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleDuplicateAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString SourcePath;
    if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
    }

    FString DestName;
    if (!Params->TryGetStringField(TEXT("dest_name"), DestName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'dest_name' parameter"));
    }

    FString DestFolder;
    if (!Params->TryGetStringField(TEXT("dest_folder"), DestFolder))
    {
        // Default to same folder as source
        DestFolder = FPackageName::GetLongPackagePath(SourcePath);
    }

    UObject* Source = StaticLoadObject(UObject::StaticClass(), nullptr, *SourcePath);
    if (!Source)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load source asset: %s"), *SourcePath));
    }

    FAssetToolsModule& ATM = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    UObject* Dup = ATM.Get().DuplicateAsset(DestName, DestFolder, Source);
    if (!Dup)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("DuplicateAsset failed"));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("source"), SourcePath);
    Result->SetStringField(TEXT("duplicate_path"), Dup->GetPathName());
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleRenameAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString SourcePath;
    if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
    }

    FString NewName;
    if (!Params->TryGetStringField(TEXT("new_name"), NewName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'new_name' parameter"));
    }

    UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, *SourcePath);
    if (!Asset)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load asset: %s"), *SourcePath));
    }

    FAssetToolsModule& ATM = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    TArray<FAssetRenameData> Renames;
    FString NewPath = FPackageName::GetLongPackagePath(SourcePath) / NewName;
    Renames.Add(FAssetRenameData(Asset, FPackageName::GetLongPackagePath(SourcePath), NewName));
    bool bSuccess = ATM.Get().RenameAssets(Renames);

    if (!bSuccess)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("RenameAssets returned false"));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("old_path"), SourcePath);
    Result->SetStringField(TEXT("new_path"), Asset->GetPathName());
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleMoveAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString SourcePath;
    if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
    }

    FString DestFolder;
    if (!Params->TryGetStringField(TEXT("dest_folder"), DestFolder))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'dest_folder' parameter"));
    }

    UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, *SourcePath);
    if (!Asset)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load asset: %s"), *SourcePath));
    }

    FString AssetName = Asset->GetName();

    FAssetToolsModule& ATM = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    TArray<FAssetRenameData> Renames;
    Renames.Add(FAssetRenameData(Asset, DestFolder, AssetName));
    bool bSuccess = ATM.Get().RenameAssets(Renames);

    if (!bSuccess)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("MoveAsset (RenameAssets) returned false"));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("old_path"), SourcePath);
    Result->SetStringField(TEXT("new_path"), Asset->GetPathName());
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleReimportAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath);
    if (!Asset)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load asset: %s"), *AssetPath));
    }

    bool bSuccess = FReimportManager::Instance()->Reimport(Asset, /*bAskForNewFile*/ false);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("asset_path"), AssetPath);
    Result->SetBoolField(TEXT("reimport_success"), bSuccess);
    return Result;
}

// ============================================================================
// Stage 5: Enhanced Input richer
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleListInputActions(const TSharedPtr<FJsonObject>& Params)
{
    UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
    if (!InputSettings)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get input settings"));
    }

    // Get all action mappings
    const TArray<FInputActionKeyMapping>& Actions = InputSettings->GetActionMappings();
    TArray<TSharedPtr<FJsonValue>> Arr;
    for (const FInputActionKeyMapping& Mapping : Actions)
    {
        TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
        Entry->SetStringField(TEXT("action_name"), Mapping.ActionName.ToString());
        Entry->SetStringField(TEXT("key"), Mapping.Key.ToString());
        Entry->SetBoolField(TEXT("shift"), Mapping.bShift);
        Entry->SetBoolField(TEXT("ctrl"), Mapping.bCtrl);
        Entry->SetBoolField(TEXT("alt"), Mapping.bAlt);
        Entry->SetBoolField(TEXT("cmd"), Mapping.bCmd);
        Arr.Add(MakeShared<FJsonValueObject>(Entry));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetNumberField(TEXT("count"), Arr.Num());
    Result->SetArrayField(TEXT("actions"), Arr);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleRemoveInputMapping(const TSharedPtr<FJsonObject>& Params)
{
    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    FString Key;
    Params->TryGetStringField(TEXT("key"), Key);

    UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
    if (!InputSettings)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get input settings"));
    }

    if (Key.IsEmpty())
    {
        // Remove all mappings for this action
        InputSettings->RemoveActionMapping(FInputActionKeyMapping(FName(*ActionName)));
    }
    else
    {
        FInputActionKeyMapping ToRemove;
        ToRemove.ActionName = FName(*ActionName);
        ToRemove.Key = FKey(*Key);
        InputSettings->RemoveActionMapping(ToRemove);
    }
    InputSettings->SaveConfig();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("action_name"), ActionName);
    Result->SetStringField(TEXT("removed_key"), Key.IsEmpty() ? TEXT("all") : Key);
    return Result;
}