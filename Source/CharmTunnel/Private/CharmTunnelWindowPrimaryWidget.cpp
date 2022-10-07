#include "CharmTunnelWindowPrimaryWidget.h"
#include <SlateOptMacros.h>

#include "CharmTunnelEditorLibrary.h"
#include "CharmTunnelLog.h"
#include "EditorStyleSet.h"

DEFINE_LOG_CATEGORY(LogCharmTunnel);

#define LOCTEXT_NAMESPACE "SNotificationButtons"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SCharmTunnelWindowPrimaryWidget::Construct(const FArguments& InArgs)
{
	//WidgetName is now of type TAttribute<FName> to resolve, use WidgetName.Get();
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
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		  .FillWidth(1)
		  .Padding(10)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("InfoConfigPathLabel", "Info config file path:"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(SEditableTextBox)
					.HintText(LOCTEXT("InfoConfigPathHint", "C:\\T\\export\\Maps\\B6EAF980_info.cfg"))
				]
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .VAlign(VAlign_Center)
				  .Padding(10, 0, 0, 0)
				[
					SNew(SButton)
					.OnClicked(this, &SCharmTunnelWindowPrimaryWidget::OnSelectInfoConfigPathClicked)
					.Text(LOCTEXT("InfoConfigPathButton", "..."))
					.ToolTipText(LOCTEXT("InfoConfigPathButtonTooltip", "Select info config file, ..._info.cfg"))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("OutputDirectoryLabel", "Output directory within UE:"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(SEditableTextBox)
					.HintText(LOCTEXT("OutputDirectoryHint", "Content\\EDZ\\LostSectors"))
				]
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .VAlign(VAlign_Center)
				  .Padding(10, 0, 0, 0)
				[
					SNew(SButton)
	.OnClicked(this, &SCharmTunnelWindowPrimaryWidget::OnSelectOutputDirectoryClicked)
	.Text(LOCTEXT("OutputDirectoryButton", "..."))
	.ToolTipText(LOCTEXT("OutputDirectoryButtonTooltip", "Must be within the Content directory"))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .VAlign(VAlign_Center)
				  .Padding(10, 0, 0, 0)
				[
					SNew(SButton)
	.OnClicked(this, &SCharmTunnelWindowPrimaryWidget::OnLoadDevMapClicked)
	.Text(LOCTEXT("LoadDevMapButton", "Load dev map"))
				]
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			  .FillHeight(1)
			  .Padding(10)
			[
				LogBox.ToSharedRef()
			]

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
	FString DevMapName = "/CharmTunnel/Dev_P";
	if (FCharmEditorLibrary::DoesAssetExist(DevMapName))
	{
		LOG("Dev map already exists, loading into editor");
		if (FCharmEditorLibrary::LoadLevel(DevMapName))
		{
			LOG("Dev map loaded");
		}
		else
		{
			LOG_ERROR("Dev map could not be loaded, likely need to save the current level first");
		}
	}
	else
	{
		if (FCharmEditorLibrary::CreateLevel(DevMapName))
		{
			LOG("Dev map created");
		}
		else
		{
			LOG_ERROR("Dev map creation failed");
		}
	}
	return FReply::Handled();
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
