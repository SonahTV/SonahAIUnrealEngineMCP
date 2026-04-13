# UE5 Animation Cheatsheet

## Animation Blueprint anatomy
- `UAnimBlueprint` (asset) → compiles to `UAnimBlueprintGeneratedClass`
- Two main graphs:
  - **EventGraph** — like a normal BP event graph; use it to update bool/float vars driving the AnimGraph (e.g. `Speed`, `bIsInAir`)
  - **AnimGraph** — special graph that produces a `FPoseLink` each frame. Final node is `UAnimGraphNode_Root`
- `UAnimInstance` is the C++ runtime counterpart. Override `NativeUpdateAnimation(DeltaSeconds)` to compute variables in C++ instead of BP

## State machines
- `UAnimGraphNode_StateMachine` lives in the AnimGraph
- Each state machine has its own sub-graph of `UAnimStateNode`s (states) connected by `UAnimStateTransitionNode`s (transitions)
- Each `UAnimStateNode` itself contains a sub-AnimGraph that produces a pose (typically a `UAnimGraphNode_SequencePlayer` or `UAnimGraphNode_BlendSpacePlayer`)
- Each transition has a `TransitionGraph` — a tiny graph whose result pin is a bool driving "should transition fire"

## Transition rules (the most error-prone part)
- The transition graph's final node is `UAnimGraphNode_TransitionResult` with one bool input pin
- Wire a `UK2Node_VariableGet` of an AnimBP bool variable directly into that pin for the simplest case
- For float comparisons, use `UK2Node_CallFunction` referencing `UKismetMathLibrary::Greater_FloatFloat` or similar
- Both states must be in the same parent state machine — you cannot transition across state machines directly

## Sequence player vs blend space player
- `UAnimGraphNode_SequencePlayer` — plays a single `UAnimSequence`. Set `Node.Sequence` and `bLoopAnimation`
- `UAnimGraphNode_BlendSpacePlayer` — plays a `UBlendSpace`/`UBlendSpace1D`, blended by 1 or 2 input variables (X, Y axes). Best for locomotion (Speed → Idle/Walk/Run blend)

## Notifies
- `UAnimSequence::Notifies` is a `TArray<FAnimNotifyEvent>`
- `FAnimNotifyEvent::SetTime(Time)` and `Notify` (a `UAnimNotify*` instance) — useful for footstep/sfx triggers
- `UAnimNotifyState` for ranged events (start + end)

## Slot nodes
- `UAnimGraphNode_Slot` — lets you play a `UAnimMontage` via slot name from the AnimInstance
- Default slot name is `DefaultSlot`; create others via the AnimBP's `AnimSlotManager`

## Common pitfalls
- **Compile after every structural change.** AnimBP changes don't take effect until `FKismetEditorUtilities::CompileBlueprint(AnimBP)` runs
- **State machine creation requires editor APIs.** You need `AnimGraph` + `AnimGraphRuntime` modules in your Build.cs
- **Transition rule wires must use AnimBP variables**, not Pawn variables. To drive transitions from gameplay state, copy gameplay vars into AnimBP vars in `NativeUpdateAnimation`
- **Setting `Mesh->SetAnimInstanceClass(AnimBP->GeneratedClass)`** is how you assign an AnimBP to a SkeletalMeshComponent at runtime; for default, set `AnimClass` on the component template in the character BP

## Key UE source paths
- `Engine/Source/Editor/AnimGraph/Public/AnimStateNode.h`
- `Engine/Source/Editor/AnimGraph/Public/AnimGraphNode_StateMachine.h`
- `Engine/Source/Runtime/Engine/Classes/Animation/AnimInstance.h`
