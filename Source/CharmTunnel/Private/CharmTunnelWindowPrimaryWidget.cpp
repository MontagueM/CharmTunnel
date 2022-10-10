#include "CharmTunnelWindowPrimaryWidget.h"

#include "CharmTunnelEditorLibrary.h"
#include "CharmTunnelLog.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "EditorStyleSet.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Subsystems/EditorActorSubsystem.h"

#include <SlateOptMacros.h>

#define LOCTEXT_NAMESPACE "SNotificationButtons"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SCharmTunnelWindowPrimaryWidget::Construct(const FArguments& InArgs)
{
    // WidgetName is now of type TAttribute<FName> to resolve, use WidgetName.Get();
    WidgetName = InArgs._WidgetTitle;

    LogBox = SNew(SCharmLog)
                 // .Style(FEditorStyle::Get(), "Log.TextBox")
                 // .TextStyle(FEditorStyle::Get(), "Log.Normal")
                 // .Marshaller(MessagesTextMarshaller)
                 .IsReadOnly(true)
                 .Font(FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("Log.Normal").Font)
                 .AlwaysShowScrollbars(true);
    // .AutoWrapText(this, &SOutputLog::IsWordWrapEnabled)
    // .OnVScrollBarUserScrolled(this, &SOutputLog::OnUserScrolled)
    // .ContextMenuExtender(this, &SOutputLog::ExtendTextBoxMenu);

    ChildSlot
        [SNew(SHorizontalBox) +
            SHorizontalBox::Slot().FillWidth(1).Padding(
                10)[SNew(SVerticalBox) +
                    SVerticalBox::Slot().AutoHeight()
                        [SNew(SHorizontalBox) +
                            SHorizontalBox::Slot().FillWidth(
                                1)[SNew(STextBlock).Text(LOCTEXT("InfoConfigPathLabel", "Info config file path:"))] +
                            SHorizontalBox::Slot().FillWidth(1)
                                [SNew(SEditableTextBox).HintText(LOCTEXT("InfoConfigPathHint", "C:\\T\\export\\Maps\\B6EAF980_info.cfg"))] +
                            SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(10, 0, 0,
                                    0)[SNew(SButton)
                                           .OnClicked(this, &SCharmTunnelWindowPrimaryWidget::OnSelectInfoConfigPathClicked)
                                           .Text(LOCTEXT("InfoConfigPathButton", "..."))
                                           .ToolTipText(LOCTEXT("InfoConfigPathButtonTooltip", "Select info config file, ..._info.cfg"))]] +
                    SVerticalBox::Slot().AutoHeight()
                        [SNew(SHorizontalBox) +
                            SHorizontalBox::Slot().FillWidth(
                                1)[SNew(STextBlock).Text(LOCTEXT("OutputDirectoryLabel", "Output directory within UE:"))] +
                            SHorizontalBox::Slot().FillWidth(
                                1)[SNew(SEditableTextBox).HintText(LOCTEXT("OutputDirectoryHint", "Content\\EDZ\\LostSectors"))] +
                            SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(10, 0, 0,
                                    0)[SNew(SButton)
                                           .OnClicked(this, &SCharmTunnelWindowPrimaryWidget::OnSelectOutputDirectoryClicked)
                                           .Text(LOCTEXT("OutputDirectoryButton", "..."))
                                           .ToolTipText(LOCTEXT("OutputDirectoryButtonTooltip", "Must be within the Content directory"))]] +
                    SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox) +
                                                      SHorizontalBox::Slot()
                                                          .AutoWidth()
                                                          .VAlign(VAlign_Center)
                                                          .Padding(10, 0, 0,
                                                              0)[SNew(SButton)
                                                                     .OnClicked(this, &SCharmTunnelWindowPrimaryWidget::OnLoadDevMapClicked)
                                                                     .Text(LOCTEXT("LoadDevMapButton", "Load dev map"))]]] +
            SHorizontalBox::Slot().FillWidth(1)[SNew(SVerticalBox) + SVerticalBox::Slot().FillHeight(1).Padding(10)[LogBox.ToSharedRef()]

    ]

    ];

    LOG("Charm Tunnel loaded");
    LOG("Charm Tunnel v0.0.1-alpha");
}

FReply SCharmTunnelWindowPrimaryWidget::OnSelectInfoConfigPathClicked()
{
    return FReply::Handled();
}

FReply SCharmTunnelWindowPrimaryWidget::OnSelectOutputDirectoryClicked()
{
    return FReply::Handled();
}

FReply SCharmTunnelWindowPrimaryWidget::OnLoadDevMapClicked()
{
    FString DevMapName = "/CharmTunnel/Dev/Dev_P";
    if (FCharmEditorLibrary::DoesAssetExist(DevMapName))
    {
        LOG("Dev map already exists, deleting and reloading");
        if (FCharmEditorLibrary::DeleteAsset(DevMapName))
        {
            LOG("Dev map deleted");
        }
        else
        {
            LOG_ERROR("Failed to delete dev map");
        }
    }
    ULevel* DevLevel;
    if (FCharmEditorLibrary::CreateLevel(DevMapName, DevLevel))
    {
        LOG("Dev map created");
        PopulateDevMap(DevLevel);
    }
    else
    {
        LOG_ERROR("Dev map creation failed");
    }
    return FReply::Handled();
}

void SCharmTunnelWindowPrimaryWidget::PopulateDevMap(ULevel* DevLevel)
{
    // Load all assets in dev map directory and add to level
    FString DebugStaticSourcePath = "C:/T/export/devmap/";
    FString DebugStaticDestPath = "/CharmTunnel/Dev/";

    TArray<FString> Files = FCharmEditorLibrary::GetFilesInDirectory(DebugStaticSourcePath, "*_info.cfg");
    for (auto& File : Files)
    {
        FCharmEditorLibrary::ImportAssetFromConfigFile(File, DebugStaticDestPath / "Data/");
    }

    if (UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>())
    {
        EditorActorSubsystem->SpawnActorFromObject(ASkyLight::StaticClass(), FVector::Zero());
        EditorActorSubsystem->SpawnActorFromObject(AVolumetricCloud::StaticClass(), FVector::Zero());
        EditorActorSubsystem->SpawnActorFromObject(ASkyAtmosphere::StaticClass(), FVector::Zero());
        EditorActorSubsystem->SpawnActorFromObject(AExponentialHeightFog::StaticClass(), FVector::Zero());
        EditorActorSubsystem->SpawnActorFromObject(ADirectionalLight::StaticClass(), FVector::Zero(), FRotator(-105, -31, -14));
        EditorActorSubsystem->SpawnActorFromObject(ADirectionalLight::StaticClass(), FVector::Zero(), FRotator(-105, -31, -14));
        UStaticMesh* StaticMeshPlane = FCharmEditorLibrary::LoadAsset<UStaticMesh>("/Engine/BasicShapes/Plane");
        AActor* Plane = EditorActorSubsystem->SpawnActorFromObject(StaticMeshPlane, FVector::Zero(), FRotator(), false);
        Plane->SetActorScale3D(FVector(100, 100, 1));
        UStaticMesh* SM0F3CBE80 = FCharmEditorLibrary::LoadAsset<UStaticMesh>(DebugStaticDestPath / "SM/0F3CBE80");
        EditorActorSubsystem->SpawnActorFromObject(SM0F3CBE80, FVector::Zero());
        UStaticMesh* SM68A8B480 = FCharmEditorLibrary::LoadAsset<UStaticMesh>(DebugStaticDestPath / "SM/68A8B480");
        EditorActorSubsystem->SpawnActorFromObject(SM68A8B480, FVector(1000, 0, 0));
        UStaticMesh* SM6C24BB80 = FCharmEditorLibrary::LoadAsset<UStaticMesh>(DebugStaticDestPath / "SM/6C24BB80");
        EditorActorSubsystem->SpawnActorFromObject(SM6C24BB80, FVector(2000, 0, 0));
        UStaticMesh* SMA229BE80 = FCharmEditorLibrary::LoadAsset<UStaticMesh>(DebugStaticDestPath / "SM/A229BE80");
        EditorActorSubsystem->SpawnActorFromObject(SMA229BE80, FVector(3000, 0, 0));
        UStaticMesh* SMA237BE80 = FCharmEditorLibrary::LoadAsset<UStaticMesh>(DebugStaticDestPath / "SM/A237BE80");
        EditorActorSubsystem->SpawnActorFromObject(SMA237BE80, FVector(4000, 0, 0));
        UStaticMesh* SMB540BE80 = FCharmEditorLibrary::LoadAsset<UStaticMesh>(DebugStaticDestPath / "SM/B540BE80");
        EditorActorSubsystem->SpawnActorFromObject(SMB540BE80, FVector(-1000, 0, 0));
        UStaticMesh* SMB63BBE80 = FCharmEditorLibrary::LoadAsset<UStaticMesh>(DebugStaticDestPath / "SM/B63BBE80");
        EditorActorSubsystem->SpawnActorFromObject(SMB63BBE80, FVector(-2000, 0, 0));
        UStaticMesh* SMCB32BE80 = FCharmEditorLibrary::LoadAsset<UStaticMesh>(DebugStaticDestPath / "SM/CB32BE80");
        EditorActorSubsystem->SpawnActorFromObject(SMCB32BE80, FVector(-3000, 0, 0));
        UStaticMesh* SME1C5B280 = FCharmEditorLibrary::LoadAsset<UStaticMesh>(DebugStaticDestPath / "SM/E1C5B280");
        EditorActorSubsystem->SpawnActorFromObject(SME1C5B280, FVector(-4000, 0, 0));
        UStaticMesh* SMFBA4B480 = FCharmEditorLibrary::LoadAsset<UStaticMesh>(DebugStaticDestPath / "SM/FBA4B480");
        EditorActorSubsystem->SpawnActorFromObject(SMFBA4B480, FVector(0, 1000, 0));
    }

    auto a = 0;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
