# UE5 Actor / Pawn / Component Cheatsheet

## Actor lifecycle
1. `PostInitializeComponents` — components constructed
2. `BeginPlay` — world is ready, all actors spawned
3. `Tick(DeltaSeconds)` — every frame (if `PrimaryActorTick.bCanEverTick = true`)
4. `EndPlay(EEndPlayReason)` — destruction or level unload

## Disable tick by default
```cpp
PrimaryActorTick.bCanEverTick = false;  // in constructor
PrimaryActorTick.bStartWithTickEnabled = false;
```
Re-enable per-instance with `SetActorTickEnabled(true)`.

## Spawning
```cpp
FActorSpawnParameters Params;
Params.Owner = this;
Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
AMyActor* New = GetWorld()->SpawnActor<AMyActor>(AMyActor::StaticClass(), Location, Rotation, Params);
```

## Components
- `UActorComponent` — pure logic, no transform
- `USceneComponent` — has transform, can attach to other scene components
- `UPrimitiveComponent` — has collision/rendering (StaticMesh, Skeletal, etc.)
- Create in constructor: `MyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"))`
- Set as root: `SetRootComponent(MyMesh)` or attach: `MyMesh->SetupAttachment(RootComponent)`

## Pawn vs Character
- `APawn` — base for anything possessable. You implement movement
- `ACharacter` — APawn + CapsuleComponent (root), CharacterMovementComponent, SkeletalMeshComponent. 95% of the time use Character
- Possess: `PlayerController->Possess(MyPawn)`

## CharacterMovementComponent key params
- `MaxWalkSpeed` (default 600)
- `JumpZVelocity` (default 420)
- `AirControl` (0..1, default 0.05)
- `GravityScale` (default 1.0)
- `MaxAcceleration` (default 2048)
- `BrakingDecelerationWalking` (default 2048)

## Input (Enhanced Input — UE5 default)
1. Create `UInputAction` assets in Content Browser
2. Create `UInputMappingContext` and add `(InputAction, Key)` mappings
3. In `APlayerController::BeginPlay`:
```cpp
if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
{
    Subsystem->AddMappingContext(IMC_Default, /*Priority*/ 0);
}
```
4. In `APawn::SetupPlayerInputComponent`:
```cpp
if (auto* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
{
    EIC->BindAction(IA_Jump, ETriggerEvent::Triggered, this, &AMyChar::Jump);
}
```

## Common pitfalls
- **Holding raw pointers to UObjects across frames** — they can get GC'd. Use `TWeakObjectPtr<>` or a `UPROPERTY()` member
- **Tick is expensive at scale** — disable in constructor, opt in per actor
- **Constructors run on the CDO** (Class Default Object) AND every spawned instance. Don't put world-querying logic in constructors
- **`GetWorld()` is null in constructors** — use `BeginPlay` or `PostInitializeComponents`
- **Replication needs `bReplicates = true`** in constructor + `UPROPERTY(Replicated)` + `GetLifetimeReplicatedProps`

## Key UE source paths
- `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h`
- `Engine/Source/Runtime/Engine/Classes/GameFramework/Character.h`
- `Engine/Source/Runtime/Engine/Classes/GameFramework/CharacterMovementComponent.h`
