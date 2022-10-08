// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CharmTunnelStyle.h"
#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FCharmTunnelCommands : public TCommands<FCharmTunnelCommands>
{
public:
    FCharmTunnelCommands()
        : TCommands<FCharmTunnelCommands>(TEXT("CharmTunnel"), NSLOCTEXT("Contexts", "CharmTunnel", "CharmTunnel Plugin"), NAME_None,
              FCharmTunnelStyle::GetStyleSetName())
    {
    }

    // TCommands<> interface
    virtual void RegisterCommands() override;

public:
    TSharedPtr<FUICommandInfo> OpenCharmTunnelPluginWindow;
};
