// Copyright Epic Games, Inc. All Rights Reserved.

#include "CharmTunnelCommands.h"

#define LOCTEXT_NAMESPACE "FCharmTunnelModule"

void FCharmTunnelCommands::RegisterCommands()
{
    UI_COMMAND(OpenCharmTunnelPluginWindow, "CharmTunnel", "Open Charm Tunnel window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
