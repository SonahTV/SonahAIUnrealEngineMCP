# UE5 Blueprint Cheatsheet

## Blueprint anatomy
- `UBlueprint` is the editor-time asset; at cook time it compiles into a `UBlueprintGeneratedClass`
- Key collections on `UBlueprint`:
  - `UbergraphPages` — main event graph(s)
  - `FunctionGraphs` — user-defined functions
  - `MacroGraphs` — user-defined macros
  - `NewVariables` (`TArray<FBPVariableDescription>`) — declared variables
  - `SimpleConstructionScript` — the components tree (Mesh, Capsule, etc.)
  - `ParentClass` — what the BP inherits from

## Nodes (`UEdGraphNode` subclasses)
- `UK2Node_Event` — event entry points (BeginPlay, Tick, custom events)
- `UK2Node_CallFunction` — calls a UFUNCTION
- `UK2Node_VariableGet` / `UK2Node_VariableSet` — variable accessors
- `UK2Node_IfThenElse` — branch
- `UK2Node_Self` — `Self` reference
- `UK2Node_DynamicCast` — Cast To node
- `UK2Node_Knot` — reroute
- All nodes have `Pins` (`TArray<UEdGraphPin*>`) — input/output, exec/data

## Pin types (`FEdGraphPinType`)
- `PinCategory`: `exec`, `bool`, `int`, `float`, `name`, `string`, `text`, `object`, `class`, `struct`, `enum`, `interface`, `delegate`
- `PinSubCategoryObject`: for object/struct/enum pins, the actual UClass/UScriptStruct/UEnum

## Compiling
```cpp
FKismetEditorUtilities::CompileBlueprint(BP);
FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
```
- Always compile after structural changes (variables added, components added, graphs edited)
- `MarkBlueprintAsModified` makes sure the editor knows to save

## Creating a new BP via C++
```cpp
FKismetEditorUtilities::CreateBlueprint(
    ParentClass,
    Package,
    *Name,
    EBlueprintType::BPTYPE_Normal,
    UBlueprint::StaticClass(),
    UBlueprintGeneratedClass::StaticClass());
```

## Variables
- Add via `FBlueprintEditorUtils::AddMemberVariable(BP, Name, PinType)`
- Get default value via `FBlueprintEditorUtils::GetBlueprintVariableDefaultValue(BP, Name)`
- Set default via `FBlueprintEditorUtils::PropertyValueFromString(Property, ValueString, ...)`

## Components (SimpleConstructionScript)
- `BP->SimpleConstructionScript->FindSCSNodeByVariableName(Name)` returns a `USCS_Node`
- `USCS_Node::ComponentTemplate` is the actual component instance you edit defaults on
- Add new component: `USCS_Node* NewNode = BP->SimpleConstructionScript->CreateNode(ComponentClass, Name)` then `BP->SimpleConstructionScript->AddNode(NewNode)`

## Common pitfalls
- **Memory-only blueprints**: BPs created via `CreateBlueprint` are not persisted until you save the package (`UPackage::SavePackage` or `UEditorAssetLibrary::SaveAsset`)
- **Changing parent class** at runtime: use `FBlueprintEditorUtils::ReparentBlueprint`, then recompile
- **Don't edit `GeneratedClass` directly** — always edit the source `UBlueprint` and recompile
- **Component types in tools**: when adding components, use the full path `/Script/Engine.StaticMeshComponent`, not just `StaticMeshComponent`

## Key UE source paths
- `Engine/Source/Editor/Kismet/Public/BlueprintEditorUtils.h`
- `Engine/Source/Editor/UnrealEd/Public/Kismet2/KismetEditorUtilities.h`
- `Engine/Source/Runtime/Engine/Classes/Engine/Blueprint.h`
