# UE5 Replication / Multiplayer Cheatsheet

## Authority model
- **Server is authoritative** for all gameplay state
- Clients see a replicated copy + run client-side prediction for their own pawn
- `HasAuthority()` returns true on server (and standalone) — gate server-only logic with this

## Enabling replication on an Actor
```cpp
AMyActor::AMyActor()
{
    bReplicates = true;
    SetReplicateMovement(true);  // optional: auto-replicate transform
}
```

## Replicated properties
```cpp
// Header
UPROPERTY(Replicated)
int32 Health;

UPROPERTY(ReplicatedUsing=OnRep_Health)
int32 Health;  // alternative: triggers OnRep_Health() on clients when changed

UFUNCTION()
void OnRep_Health();

// Cpp
void AMyActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
{
    Super::GetLifetimeReplicatedProps(Out);
    DOREPLIFETIME(AMyActor, Health);
}
```

## RPCs
| Specifier | Direction | Caller | Runs on |
|---|---|---|---|
| `Server` | Client → Server | Client | Server |
| `Client` | Server → Owning client | Server | Owning client |
| `NetMulticast` | Server → All clients (+ server) | Server | Everyone |

```cpp
UFUNCTION(Server, Reliable, WithValidation)
void Server_TakeDamage(float Amount);
```
- `Reliable` — TCP-like guarantee. Use sparingly
- `Unreliable` — fire and forget. Use for cosmetic updates
- `WithValidation` — implement `Server_TakeDamage_Validate` to reject malicious input

## Common pitfalls
- **Forgetting `GetLifetimeReplicatedProps`** — property is marked `Replicated` but never actually replicates
- **Calling `Server_*` from server itself** — runs locally, not over network. Always check `HasAuthority()` first if you need conditional logic
- **Replicating large structs every frame** — bandwidth killer. Use `COND_Initial`, `COND_OwnerOnly`, etc. with `DOREPLIFETIME_CONDITION`
- **Spawning actors on clients** — only the server should `SpawnActor` for replicated actors. Clients receive replicated copies automatically
- **Testing in PIE**: enable "Number of Players" > 1 in PIE settings, OR use "Run Dedicated Server" toggle

## Validation
```cpp
bool AMyActor::Server_TakeDamage_Validate(float Amount)
{
    return Amount >= 0 && Amount <= 1000;  // reject impossible damage values
}
```
Returning false disconnects the client as a cheat suspect.

## Key UE source paths
- `Engine/Source/Runtime/Engine/Classes/Engine/NetSerialization.h`
- `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h` (search for `bReplicates`)
- `Engine/Source/Runtime/Engine/Classes/Net/UnrealNetwork.h` (DOREPLIFETIME macros)
