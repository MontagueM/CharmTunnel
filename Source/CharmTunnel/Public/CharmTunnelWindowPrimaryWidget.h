#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCharmTunnel, Log, All);

#define LOG(x, ...) UE_LOG(LogCharmTunnel, Log, TEXT(x), __VA_ARGS__)
#define LOG_VERBOSE(x, ...) UE_LOG(LogCharmTunnel, Verbose, TEXT(x), __VA_ARGS__)
#define LOG_WARNING(x, ...) UE_LOG(LogCharmTunnel, Warning, TEXT(x), __VA_ARGS__)
#define LOG_ERROR(x, ...) UE_LOG(LogCharmTunnel, Error, TEXT(x), __VA_ARGS__)


/**
 * A CompoundWidget is the base from which most non-primitive widgets should be built.
 * CompoundWidgets have a protected member named ChildSlot.
 */
class SCharmTunnelWindowPrimaryWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCharmTunnelWindowPrimaryWidget)
			: _WidgetTitle(FName())
		{
		}

		//Example argument for slate, follows (Type, Name)
		//See construct for accessing.
		SLATE_ARGUMENT(FName, WidgetTitle)

	SLATE_END_ARGS()

	FReply OnSelectInfoConfigPathClicked();
	FReply OnLoadDevMapClicked();
	FReply OnSelectOutputDirectoryClicked();
	/**
	 * Construct this widget. Called by the SNew() Slate macro.
	 *
	 * @param  InArgs	Declaration used by the SNew() macro to construct this widget
	 */
	void Construct(const FArguments& InArgs);

	TSharedPtr<class SCharmLog> LogBox;

private:
	//An example property to set in Construct
	TAttribute<FName> WidgetName;
};
