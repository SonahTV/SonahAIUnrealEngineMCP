# UE5 Asset & Content Browser Cheatsheet

## Asset Registry — querying
```cpp
FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
IAssetRegistry& AR = ARM.Get();

// All assets of a class
TArray<FAssetData> Assets;
AR.GetAssetsByClass(UMaterial::StaticClass()->GetClassPathName(), Assets);

// Filter
FARFilter Filter;
Filter.PackagePaths.Add(FName("/Game/Materials"));
Filter.ClassPaths.Add(UMaterialInstanceConstant::StaticClass()->GetClassPathName());
Filter.bRecursivePaths = true;
AR.GetAssets(Filter, Assets);
```

## Asset dependencies / referencers
```cpp
TArray<FName> Deps;
AR.GetDependencies(FName("/Game/MyMaterial"), Deps);

TArray<FName> Refs;
AR.GetReferencers(FName("/Game/MyMaterial"), Refs);
```

## Asset Tools — duplicate, rename, move
```cpp
FAssetToolsModule& ATM = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

// Duplicate
UObject* Duplicated = ATM.Get().DuplicateAsset(NewName, NewPath, ExistingObject);

// Rename
TArray<FAssetRenameData> Renames;
Renames.Add(FAssetRenameData(Asset, NewPath, NewName));
ATM.Get().RenameAssets(Renames);
```

## Reimport
```cpp
FReimportManager::Instance()->Reimport(Asset, /*bAskForNewFile*/ false);
```

## Loading an asset by path
```cpp
UObject* Asset = StaticLoadObject(UMaterial::StaticClass(), nullptr, TEXT("/Game/MyMaterial.MyMaterial"));
// Note the .AssetName suffix — that's the object name inside the package
```

For soft references (preferred):
```cpp
TSoftObjectPtr<UMaterial> SoftRef(FSoftObjectPath("/Game/MyMaterial.MyMaterial"));
UMaterial* Loaded = SoftRef.LoadSynchronous();
```

## Saving packages
```cpp
UPackage* Package = Asset->GetPackage();
Package->MarkPackageDirty();
FAssetRegistryModule::AssetCreated(Asset);  // tell registry
FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
FSavePackageArgs Args;
Args.TopLevelFlags = RF_Public | RF_Standalone;
UPackage::SavePackage(Package, Asset, *Filename, Args);
```

## Material instances
- `UMaterialInstanceConstant` — editor-time, baked, lives as an asset
- `UMaterialInstanceDynamic` — runtime, created per-instance via `UMaterialInstanceDynamic::Create(Parent, Outer)`
- Create constant via `UMaterialInstanceConstantFactoryNew`, set `InitialParent`, call `IAssetTools::CreateAsset`
- Set params editor-only: `SetScalarParameterValueEditorOnly`, `SetVectorParameterValueEditorOnly`

## Common pitfalls
- **Asset path with `.AssetName` suffix** is required by `StaticLoadObject` — `/Game/Foo.Foo` not `/Game/Foo`
- **`GetAssetsByClass` is fast**, but `GetAssets(FARFilter)` is the proper way for any non-trivial filter
- **Renaming an asset breaks references** unless you use `IAssetTools::RenameAssets` which fixes redirects
- **Don't call `LoadObject` from a worker thread** — use the async loader (`StreamableManager`)

## Key UE source paths
- `Engine/Source/Runtime/AssetRegistry/Public/AssetRegistry/IAssetRegistry.h`
- `Engine/Source/Developer/AssetTools/Public/IAssetTools.h`
- `Engine/Source/Runtime/Engine/Classes/Materials/MaterialInstanceConstant.h`
