// Copyright Epic Games, Inc. All Rights Reserved.

#include "CharmTunnel.h"

#include "CT_WindowPrimaryWidget.h"
#include "CharmTunnelCommands.h"
#include "CharmTunnelStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"

static const FName CharmTunnelTabName("CharmTunnel");

#define LOCTEXT_NAMESPACE "FCharmTunnelModule"

void FCharmTunnelModule::StartupModule()
{
    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

    FCharmTunnelStyle::Initialize();
    FCharmTunnelStyle::ReloadTextures();

    FCharmTunnelCommands::Register();

    PluginCommands = MakeShareable(new FUICommandList);

    PluginCommands->MapAction(FCharmTunnelCommands::Get().OpenCharmTunnelPluginWindow,
        FExecuteAction::CreateRaw(this, &FCharmTunnelModule::PluginButtonClicked), FCanExecuteAction());

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FCharmTunnelModule::RegisterMenus));

    FGlobalTabmanager::Get()
        ->RegisterNomadTabSpawner(CharmTunnelTabName, FOnSpawnTab::CreateRaw(this, &FCharmTunnelModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("FCharmTunnelTabTitle", "Charm Tunnel"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("CharmTunnel"))->GetBaseDir(), TEXT("Shaders"));
    AddShaderSourceDirectoryMapping(TEXT("/Plugin/CharmTunnel"), PluginShaderDir);
}

void FCharmTunnelModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.

    UToolMenus::UnRegisterStartupCallback(this);

    UToolMenus::UnregisterOwner(this);

    FCharmTunnelStyle::Shutdown();

    FCharmTunnelCommands::Unregister();
}

void FCharmTunnelModule::PluginButtonClicked()
{
    FGlobalTabmanager::Get()->TryInvokeTab(CharmTunnelTabName);
}

TSharedRef<SDockTab> FCharmTunnelModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
    // Put your "OnButtonClicked" stuff here
    FText DialogText = FText::Format(LOCTEXT("PluginButtonDialogText", "Add code to {0} in {1} to override this button's actions"),
        FText::FromString(TEXT("FCharmTunnelModule::PluginButtonClicked()")), FText::FromString(TEXT("CharmTunnel.cpp")));

    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SCharmTunnelWindowPrimaryWidget)];
}

void FCharmTunnelModule::RegisterMenus()
{
    // Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
    FToolMenuOwnerScoped OwnerScoped(this);

    {
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
        {
            FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
            Section.AddMenuEntryWithCommandList(FCharmTunnelCommands::Get().OpenCharmTunnelPluginWindow, PluginCommands);
        }
    }

    {
        UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
        {
            FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
            {
                FToolMenuEntry& Entry =
                    Section.AddEntry(FToolMenuEntry::InitToolBarButton(FCharmTunnelCommands::Get().OpenCharmTunnelPluginWindow));
                Entry.SetCommandList(PluginCommands);
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCharmTunnelModule, CharmTunnel)
