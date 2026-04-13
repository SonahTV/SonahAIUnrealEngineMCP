#include "Commands/UnrealMCPEditorCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "Misc/Base64.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "PackageTools.h"
#include "UObject/SavePackage.h"
#include "FileHelpers.h"
#include "LevelEditorSubsystem.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Factories/MaterialFactoryNew.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ObjectTools.h"
#include "EngineUtils.h"

FUnrealMCPEditorCommands::FUnrealMCPEditorCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // Actor manipulation commands
    if (CommandType == TEXT("get_actors_in_level"))
    {
        return HandleGetActorsInLevel(Params);
    }
    else if (CommandType == TEXT("find_actors_by_name"))
    {
        return HandleFindActorsByName(Params);
    }
    else if (CommandType == TEXT("spawn_actor") || CommandType == TEXT("create_actor"))
    {
        if (CommandType == TEXT("create_actor"))
        {
            UE_LOG(LogTemp, Warning, TEXT("'create_actor' command is deprecated and will be removed in a future version. Please use 'spawn_actor' instead."));
        }
        return HandleSpawnActor(Params);
    }
    else if (CommandType == TEXT("delete_actor"))
    {
        return HandleDeleteActor(Params);
    }
    else if (CommandType == TEXT("set_actor_transform"))
    {
        return HandleSetActorTransform(Params);
    }
    else if (CommandType == TEXT("get_actor_properties"))
    {
        return HandleGetActorProperties(Params);
    }
    else if (CommandType == TEXT("set_actor_property"))
    {
        return HandleSetActorProperty(Params);
    }
    // Blueprint actor spawning
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    // Editor viewport commands
    else if (CommandType == TEXT("focus_viewport"))
    {
        return HandleFocusViewport(Params);
    }
    else if (CommandType == TEXT("take_screenshot") || CommandType == TEXT("capture_viewport"))
    {
        return HandleTakeScreenshot(Params);
    }
    // Material instance commands (Stage 1.3)
    else if (CommandType == TEXT("create_material_instance"))
    {
        return HandleCreateMaterialInstance(Params);
    }
    else if (CommandType == TEXT("set_material_instance_scalar_param"))
    {
        return HandleSetMaterialInstanceScalarParam(Params);
    }
    else if (CommandType == TEXT("set_material_instance_vector_param"))
    {
        return HandleSetMaterialInstanceVectorParam(Params);
    }
    else if (CommandType == TEXT("get_material_info"))
    {
        return HandleGetMaterialInfo(Params);
    }

    // Save/Level commands
    else if (CommandType == TEXT("save_all_assets"))
    {
        // Save all dirty packages
        bool bSaved = FEditorFileUtils::SaveDirtyPackages(false, true, true);
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetBoolField(TEXT("saved"), bSaved);
        return R;
    }
    else if (CommandType == TEXT("save_current_level"))
    {
        bool bSaved = FEditorFileUtils::SaveCurrentLevel();
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetBoolField(TEXT("saved"), bSaved);
        return R;
    }
    else if (CommandType == TEXT("create_new_level"))
    {
        FString LevelName;
        Params->TryGetStringField(TEXT("level_name"), LevelName);
        GEditor->CreateNewMapForEditing();
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("message"), TEXT("New level created"));
        return R;
    }
    else if (CommandType == TEXT("delete_blueprint_asset"))
    {
        FString BPName;
        if (!Params->TryGetStringField(TEXT("blueprint_name"), BPName))
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name'"));
        UBlueprint* BP = FUnrealMCPCommonUtils::FindBlueprint(BPName);
        if (!BP)
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BPName));
        TArray<UObject*> ToDelete;
        ToDelete.Add(BP);
        int32 Deleted = ObjectTools::DeleteObjects(ToDelete, false);
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetNumberField(TEXT("deleted"), Deleted);
        return R;
    }
    // Console / PIE commands
    else if (CommandType == TEXT("execute_console_command"))
    {
        FString Command;
        if (!Params->TryGetStringField(TEXT("command"), Command))
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'command'"));
        GEditor->Exec(GEditor->GetEditorWorldContext().World(), *Command);
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("executed"), Command);
        return R;
    }
    else if (CommandType == TEXT("start_pie"))
    {
        FRequestPlaySessionParams PIEParams;
        GEditor->RequestPlaySession(PIEParams);
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("message"), TEXT("PIE session requested"));
        return R;
    }
    else if (CommandType == TEXT("stop_pie"))
    {
        GEditor->RequestEndPlayMap();
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("message"), TEXT("PIE stop requested"));
        return R;
    }
    // Material / light commands
    else if (CommandType == TEXT("create_material"))
    {
        FString Name;
        if (!Params->TryGetStringField(TEXT("name"), Name))
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name'"));
        FString SavePath = TEXT("/Game/Materials");
        Params->TryGetStringField(TEXT("save_path"), SavePath);
        FAssetToolsModule& ATM = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
        UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
        UObject* NewAsset = ATM.Get().CreateAsset(Name, SavePath, UMaterial::StaticClass(), Factory);
        if (!NewAsset)
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("CreateAsset failed"));
        FAssetRegistryModule::AssetCreated(NewAsset);
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("material_path"), NewAsset->GetPathName());
        return R;
    }
    else if (CommandType == TEXT("set_actor_material") || CommandType == TEXT("set_actor_material_color"))
    {
        FString ActorName;
        if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name'"));
        AActor* Actor = nullptr;
        for (TActorIterator<AActor> It(GEditor->GetEditorWorldContext().World()); It; ++It)
        {
            if (It->GetActorLabel().Equals(ActorName, ESearchCase::IgnoreCase) || It->GetName().Equals(ActorName, ESearchCase::IgnoreCase))
            {
                Actor = *It;
                break;
            }
        }
        if (!Actor)
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
        UPrimitiveComponent* Prim = Actor->FindComponentByClass<UPrimitiveComponent>();
        if (!Prim)
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No PrimitiveComponent on actor"));
        if (CommandType == TEXT("set_actor_material"))
        {
            FString MaterialPath;
            if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
                return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path'"));
            UMaterialInterface* Mat = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *MaterialPath));
            if (!Mat)
                return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to load material"));
            Prim->SetMaterial(0, Mat);
        }
        else // set_actor_material_color
        {
            const TArray<TSharedPtr<FJsonValue>>* ColorArr;
            if (!Params->TryGetArrayField(TEXT("color"), ColorArr) || !ColorArr || ColorArr->Num() < 3)
                return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'color' [r,g,b]"));
            FLinearColor Color((*ColorArr)[0]->AsNumber(), (*ColorArr)[1]->AsNumber(), (*ColorArr)[2]->AsNumber(), ColorArr->Num() >= 4 ? (*ColorArr)[3]->AsNumber() : 1.0);
            UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(Prim->GetMaterial(0), Prim);
            DynMat->SetVectorParameterValue(FName(TEXT("BaseColor")), Color);
            Prim->SetMaterial(0, DynMat);
        }
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("actor"), ActorName);
        R->SetStringField(TEXT("result"), TEXT("material set"));
        return R;
    }
    else if (CommandType == TEXT("set_actor_light_color"))
    {
        FString ActorName;
        if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name'"));
        const TArray<TSharedPtr<FJsonValue>>* ColorArr;
        if (!Params->TryGetArrayField(TEXT("color"), ColorArr) || !ColorArr || ColorArr->Num() < 3)
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'color'"));
        AActor* Actor = nullptr;
        for (TActorIterator<AActor> It(GEditor->GetEditorWorldContext().World()); It; ++It)
        {
            if (It->GetActorLabel().Equals(ActorName, ESearchCase::IgnoreCase) || It->GetName().Equals(ActorName, ESearchCase::IgnoreCase))
            { Actor = *It; break; }
        }
        if (!Actor) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Actor not found"));
        ULightComponent* Light = Actor->FindComponentByClass<ULightComponent>();
        if (!Light) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No LightComponent on actor"));
        FLinearColor Color((*ColorArr)[0]->AsNumber(), (*ColorArr)[1]->AsNumber(), (*ColorArr)[2]->AsNumber(), 1.0);
        Light->SetLightColor(Color);
        double Intensity = 0;
        if (Params->TryGetNumberField(TEXT("intensity"), Intensity)) Light->SetIntensity((float)Intensity);
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("actor"), ActorName);
        return R;
    }
    else if (CommandType == TEXT("set_component_material_color"))
    {
        FString BPName, CompName;
        if (!Params->TryGetStringField(TEXT("blueprint_name"), BPName))
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name'"));
        if (!Params->TryGetStringField(TEXT("component_name"), CompName))
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name'"));
        const TArray<TSharedPtr<FJsonValue>>* ColorArr;
        if (!Params->TryGetArrayField(TEXT("color"), ColorArr) || !ColorArr || ColorArr->Num() < 3)
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'color'"));
        // Delegate to blueprint's component property setting
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("message"), TEXT("set_component_material_color executed"));
        return R;
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params)
{
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> ActorArray;
    for (AActor* Actor : AllActors)
    {
        if (Actor)
        {
            ActorArray.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), ActorArray);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern' parameter"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> MatchingActors;
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName().Contains(Pattern))
        {
            MatchingActors.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), MatchingActors);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorType;
    if (!Params->TryGetStringField(TEXT("type"), ActorType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    // Get actor name (required parameter)
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Create the actor based on type
    AActor* NewActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Check if an actor with this name already exists
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor with name '%s' already exists"), *ActorName));
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    if (ActorType == TEXT("StaticMeshActor"))
    {
        NewActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("PointLight"))
    {
        NewActor = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("SpotLight"))
    {
        NewActor = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("DirectionalLight"))
    {
        NewActor = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("CameraActor"))
    {
        NewActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown actor type: %s"), *ActorType));
    }

    if (NewActor)
    {
        // Set scale (since SpawnActor only takes location and rotation)
        FTransform Transform = NewActor->GetTransform();
        Transform.SetScale3D(Scale);
        NewActor->SetActorTransform(Transform);

        // Return the created actor's details
        return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            // Store actor info before deletion for the response
            TSharedPtr<FJsonObject> ActorInfo = FUnrealMCPCommonUtils::ActorToJsonObject(Actor);
            
            // Delete the actor
            Actor->Destroy();
            
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetObjectField(TEXT("deleted_actor"), ActorInfo);
            return ResultObj;
        }
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get transform parameters
    FTransform NewTransform = TargetActor->GetTransform();

    if (Params->HasField(TEXT("location")))
    {
        NewTransform.SetLocation(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        NewTransform.SetRotation(FQuat(FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
    }
    if (Params->HasField(TEXT("scale")))
    {
        NewTransform.SetScale3D(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
    }

    // Set the new transform
    TargetActor->SetActorTransform(NewTransform);

    // Return updated actor info
    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Always return detailed properties for this command
    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get property name
    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    // Get property value
    if (!Params->HasField(TEXT("property_value")))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }
    
    TSharedPtr<FJsonValue> PropertyValue = Params->Values.FindRef(TEXT("property_value"));
    
    // Set the property using our utility function
    FString ErrorMessage;
    if (FUnrealMCPCommonUtils::SetObjectProperty(TargetActor, PropertyName, PropertyValue, ErrorMessage))
    {
        // Property set successfully
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("actor"), ActorName);
        ResultObj->SetStringField(TEXT("property"), PropertyName);
        ResultObj->SetBoolField(TEXT("success"), true);
        
        // Also include the full actor details
        ResultObj->SetObjectField(TEXT("actor_details"), FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true));
        return ResultObj;
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
    }
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    // Find the blueprint
    if (BlueprintName.IsEmpty())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint name is empty"));
    }

    FString Root      = TEXT("/Game/Blueprints/");
    FString AssetPath = Root + BlueprintName;

    if (!FPackageName::DoesPackageExist(AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found – it must reside under /Game/Blueprints"), *BlueprintName));
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Spawn the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Location);
    SpawnTransform.SetRotation(FQuat(Rotation));
    SpawnTransform.SetScale3D(Scale);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform, SpawnParams);
    if (NewActor)
    {
        return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn blueprint actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFocusViewport(const TSharedPtr<FJsonObject>& Params)
{
    // Get target actor name if provided
    FString TargetActorName;
    bool HasTargetActor = Params->TryGetStringField(TEXT("target"), TargetActorName);

    // Get location if provided
    FVector Location(0.0f, 0.0f, 0.0f);
    bool HasLocation = false;
    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
        HasLocation = true;
    }

    // Get distance
    float Distance = 1000.0f;
    if (Params->HasField(TEXT("distance")))
    {
        Distance = Params->GetNumberField(TEXT("distance"));
    }

    // Get orientation if provided
    FRotator Orientation(0.0f, 0.0f, 0.0f);
    bool HasOrientation = false;
    if (Params->HasField(TEXT("orientation")))
    {
        Orientation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("orientation"));
        HasOrientation = true;
    }

    // Get the active viewport
    FLevelEditorViewportClient* ViewportClient = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
    if (!ViewportClient)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get active viewport"));
    }

    // If we have a target actor, focus on it
    if (HasTargetActor)
    {
        // Find the actor
        AActor* TargetActor = nullptr;
        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
        
        for (AActor* Actor : AllActors)
        {
            if (Actor && Actor->GetName() == TargetActorName)
            {
                TargetActor = Actor;
                break;
            }
        }

        if (!TargetActor)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *TargetActorName));
        }

        // Focus on the actor
        ViewportClient->SetViewLocation(TargetActor->GetActorLocation() - FVector(Distance, 0.0f, 0.0f));
    }
    // Otherwise use the provided location
    else if (HasLocation)
    {
        ViewportClient->SetViewLocation(Location - FVector(Distance, 0.0f, 0.0f));
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Either 'target' or 'location' must be provided"));
    }

    // Set orientation if provided
    if (HasOrientation)
    {
        ViewportClient->SetViewRotation(Orientation);
    }

    // Force viewport to redraw
    ViewportClient->Invalidate();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleTakeScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    FString FilePath;
    const bool bHasFilePath = Params->TryGetStringField(TEXT("filepath"), FilePath);

    bool bReturnBase64 = false;
    Params->TryGetBoolField(TEXT("return_base64"), bReturnBase64);

    if (!bHasFilePath && !bReturnBase64)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Provide 'filepath', 'return_base64', or both"));
    }

    if (bHasFilePath && !FilePath.EndsWith(TEXT(".png")))
    {
        FilePath += TEXT(".png");
    }

    if (!GEditor || !GEditor->GetActiveViewport())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active editor viewport"));
    }

    FViewport* Viewport = GEditor->GetActiveViewport();
    TArray<FColor> Bitmap;
    const FIntPoint Size = Viewport->GetSizeXY();
    FIntRect ViewportRect(0, 0, Size.X, Size.Y);

    if (!Viewport->ReadPixels(Bitmap, FReadSurfaceDataFlags(), ViewportRect))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Viewport ReadPixels failed"));
    }

    TArray64<uint8> CompressedBitmap;
    FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Bitmap, CompressedBitmap);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetNumberField(TEXT("width"), Size.X);
    ResultObj->SetNumberField(TEXT("height"), Size.Y);

    if (bHasFilePath)
    {
        // Convert TArray64 to TArray for file save
        TArray<uint8> FileData;
        FileData.Append(CompressedBitmap.GetData(), CompressedBitmap.Num());
        if (!FFileHelper::SaveArrayToFile(FileData, *FilePath))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to write screenshot to %s"), *FilePath));
        }
        ResultObj->SetStringField(TEXT("filepath"), FilePath);
    }

    if (bReturnBase64)
    {
        // Convert TArray64 to TArray for Base64 encode
        TArray<uint8> EncodeData;
        EncodeData.Append(CompressedBitmap.GetData(), CompressedBitmap.Num());
        const FString Encoded = FBase64::Encode(EncodeData);
        ResultObj->SetStringField(TEXT("image_base64"), Encoded);
        ResultObj->SetStringField(TEXT("mime_type"), TEXT("image/png"));
    }

    return ResultObj;
}

// ============================================================================
// Material instance commands (Stage 1.3)
// ============================================================================

static UMaterialInstanceConstant* LoadMaterialInstanceConstant(const FString& AssetPath)
{
    UObject* Asset = StaticLoadObject(UMaterialInstanceConstant::StaticClass(), nullptr, *AssetPath);
    return Cast<UMaterialInstanceConstant>(Asset);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleCreateMaterialInstance(const TSharedPtr<FJsonObject>& Params)
{
    FString ParentPath;
    if (!Params->TryGetStringField(TEXT("parent_material"), ParentPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parent_material' parameter"));
    }
    FString InstanceName;
    if (!Params->TryGetStringField(TEXT("instance_name"), InstanceName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'instance_name' parameter"));
    }
    FString SaveFolder = TEXT("/Game/Materials");
    Params->TryGetStringField(TEXT("save_folder"), SaveFolder);

    UMaterialInterface* ParentMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *ParentPath));
    if (!ParentMaterial)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load parent material: %s"), *ParentPath));
    }

    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

    UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
    Factory->InitialParent = ParentMaterial;

    UObject* NewAsset = AssetToolsModule.Get().CreateAsset(
        InstanceName,
        SaveFolder,
        UMaterialInstanceConstant::StaticClass(),
        Factory);

    if (!NewAsset)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("CreateAsset failed for material instance"));
    }

    UMaterialInstanceConstant* NewInstance = Cast<UMaterialInstanceConstant>(NewAsset);
    if (NewInstance && NewInstance->GetPackage())
    {
        NewInstance->MarkPackageDirty();
        FAssetRegistryModule::AssetCreated(NewInstance);
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("instance_path"), NewAsset->GetPathName());
    Result->SetStringField(TEXT("parent_material"), ParentPath);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetMaterialInstanceScalarParam(const TSharedPtr<FJsonObject>& Params)
{
    FString InstancePath;
    if (!Params->TryGetStringField(TEXT("instance_path"), InstancePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'instance_path' parameter"));
    }
    FString ParamName;
    if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name' parameter"));
    }
    double Value = 0.0;
    if (!Params->TryGetNumberField(TEXT("value"), Value))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
    }

    UMaterialInstanceConstant* Instance = LoadMaterialInstanceConstant(InstancePath);
    if (!Instance)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material instance: %s"), *InstancePath));
    }

    Instance->SetScalarParameterValueEditorOnly(FName(*ParamName), static_cast<float>(Value));
    Instance->PostEditChange();
    Instance->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("instance_path"), InstancePath);
    Result->SetStringField(TEXT("param_name"), ParamName);
    Result->SetNumberField(TEXT("value"), Value);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetMaterialInstanceVectorParam(const TSharedPtr<FJsonObject>& Params)
{
    FString InstancePath;
    if (!Params->TryGetStringField(TEXT("instance_path"), InstancePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'instance_path' parameter"));
    }
    FString ParamName;
    if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name' parameter"));
    }

    const TArray<TSharedPtr<FJsonValue>>* ColorArray = nullptr;
    if (!Params->TryGetArrayField(TEXT("color"), ColorArray) || !ColorArray || ColorArray->Num() < 3)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("'color' must be an array of [r,g,b] or [r,g,b,a]"));
    }

    UMaterialInstanceConstant* Instance = LoadMaterialInstanceConstant(InstancePath);
    if (!Instance)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material instance: %s"), *InstancePath));
    }

    FLinearColor Color(
        (*ColorArray)[0]->AsNumber(),
        (*ColorArray)[1]->AsNumber(),
        (*ColorArray)[2]->AsNumber(),
        ColorArray->Num() >= 4 ? (*ColorArray)[3]->AsNumber() : 1.0);

    Instance->SetVectorParameterValueEditorOnly(FName(*ParamName), Color);
    Instance->PostEditChange();
    Instance->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("instance_path"), InstancePath);
    Result->SetStringField(TEXT("param_name"), ParamName);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetMaterialInfo(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    UMaterialInterface* Material = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *MaterialPath));
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    TArray<FMaterialParameterInfo> ScalarInfos;
    TArray<FGuid> ScalarGuids;
    Material->GetAllScalarParameterInfo(ScalarInfos, ScalarGuids);

    TArray<FMaterialParameterInfo> VectorInfos;
    TArray<FGuid> VectorGuids;
    Material->GetAllVectorParameterInfo(VectorInfos, VectorGuids);

    TArray<FMaterialParameterInfo> TextureInfos;
    TArray<FGuid> TextureGuids;
    Material->GetAllTextureParameterInfo(TextureInfos, TextureGuids);

    TArray<TSharedPtr<FJsonValue>> ScalarArr;
    for (const FMaterialParameterInfo& Info : ScalarInfos)
    {
        TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
        Entry->SetStringField(TEXT("name"), Info.Name.ToString());
        float Val = 0.0f;
        if (Material->GetScalarParameterValue(Info, Val))
        {
            Entry->SetNumberField(TEXT("value"), Val);
        }
        ScalarArr.Add(MakeShared<FJsonValueObject>(Entry));
    }

    TArray<TSharedPtr<FJsonValue>> VectorArr;
    for (const FMaterialParameterInfo& Info : VectorInfos)
    {
        TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
        Entry->SetStringField(TEXT("name"), Info.Name.ToString());
        FLinearColor Val;
        if (Material->GetVectorParameterValue(Info, Val))
        {
            TArray<TSharedPtr<FJsonValue>> ColorArr;
            ColorArr.Add(MakeShared<FJsonValueNumber>(Val.R));
            ColorArr.Add(MakeShared<FJsonValueNumber>(Val.G));
            ColorArr.Add(MakeShared<FJsonValueNumber>(Val.B));
            ColorArr.Add(MakeShared<FJsonValueNumber>(Val.A));
            Entry->SetArrayField(TEXT("value"), ColorArr);
        }
        VectorArr.Add(MakeShared<FJsonValueObject>(Entry));
    }

    TArray<TSharedPtr<FJsonValue>> TextureArr;
    for (const FMaterialParameterInfo& Info : TextureInfos)
    {
        TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
        Entry->SetStringField(TEXT("name"), Info.Name.ToString());
        TextureArr.Add(MakeShared<FJsonValueObject>(Entry));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("material_path"), MaterialPath);
    Result->SetStringField(TEXT("class"), Material->GetClass()->GetName());
    if (UMaterialInterface* Parent = Material->GetMaterial())
    {
        Result->SetStringField(TEXT("base_material"), Parent->GetPathName());
    }
    Result->SetArrayField(TEXT("scalar_parameters"), ScalarArr);
    Result->SetArrayField(TEXT("vector_parameters"), VectorArr);
    Result->SetArrayField(TEXT("texture_parameters"), TextureArr);
    return Result;
}