// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

/**
 * A log box used to display LogCharmTunnel-scope messages.
 */
class CHARMTUNNEL_API SCharmLog : public FOutputDevice, public SMultiLineEditableTextBox
{
public:
    SCharmLog()
    {
        check(GLog);
        GLog->AddOutputDevice(this);
    }

    ~SCharmLog()
    {
        if (GLog != nullptr)
        {
            GLog->RemoveOutputDevice(this);
        }
    }

protected:
    virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
    {
        if (!(Category.ToString().Contains("LogCharmTunnel") || Category.ToString().Contains("LogCT")))
            return;

        FString VerbosityString;
        switch (Verbosity)
        {
            case ELogVerbosity::Error:
                VerbosityString = TEXT("ERROR: ");
                break;
            case ELogVerbosity::Warning:
                VerbosityString = TEXT("Warning: ");
                break;
            default:
                break;
        }
        LogText += TEXT("\n") + VerbosityString + Message;
        SetText(FText::FromString(LogText));
    }

private:
    FString LogText;
};
