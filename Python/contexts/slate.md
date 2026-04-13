# UE5 Slate / Editor UI Cheatsheet

## Slate vs UMG
- **Slate** = C++-only declarative UI for editor and engine internals
- **UMG** = Unreal Motion Graphics — designer-friendly Blueprint-facing wrapper around Slate, used for in-game UI

## Compound widget pattern
```cpp
// Header
class SMyWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMyWidget) {}
        SLATE_ARGUMENT(FString, Title)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
};

// Cpp
void SMyWidget::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(STextBlock).Text(FText::FromString(InArgs._Title))
        ]
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(SButton)
            .Text(FText::FromString("Click me"))
            .OnClicked(this, &SMyWidget::OnClicked)
        ]
    ];
}
```

## Common widgets
- `STextBlock` — single-line text
- `SButton` — clickable button (provide `.OnClicked`)
- `SImage` — image
- `SBox` — sized container
- `SHorizontalBox` / `SVerticalBox` — layout
- `SBorder` — bordered container
- `SCheckBox`, `SEditableTextBox`, `SComboBox<>` — input widgets
- `SListView<TItemPtr>` — virtualized list
- `SDockTab` — for editor docking panels

## Tool menus (`UToolMenus`)
```cpp
UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
FToolMenuSection& Section = Menu->FindOrAddSection("MyTools");
Section.AddMenuEntry(
    "MyTool",
    FText::FromString("Open My Tool"),
    FText::FromString("Tooltip"),
    FSlateIcon(),
    FUIAction(FExecuteAction::CreateLambda([](){ /* open */ }))
);
```

## Editor utility widgets
- `UEditorUtilityWidget` — UMG widget used as an editor tool (no game required)
- Right-click in Content Browser → Editor Utilities → Editor Utility Widget
- Run via `Run Editor Utility Widget` menu option

## Common pitfalls
- **Slate operator overloading is unusual** — `+` adds slots, `[]` adds content. Don't mix them up
- **Lifetime**: Slate widgets are `TSharedRef`/`TSharedPtr` — never raw pointers
- **Lambdas capturing `this`** in Slate: use `SLATE_BEGIN_ARGS`/`SP(...)` delegates instead of raw lambdas to avoid dangling references when the widget dies
- **Delegate types**: use `FOnClicked` not `FOnClickedDelegate` — Slate has its own naming

## Key UE source paths
- `Engine/Source/Runtime/Slate/Public/Widgets/SCompoundWidget.h`
- `Engine/Source/Runtime/SlateCore/Public/Widgets/SWidget.h`
- `Engine/Source/Editor/UnrealEd/Public/Toolkits/AssetEditorToolkit.h`
