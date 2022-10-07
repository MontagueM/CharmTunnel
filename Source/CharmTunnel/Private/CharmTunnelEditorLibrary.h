#pragma once

#include "CoreMinimal.h"

#include "LevelEditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "CharmTunnelEditorLibrary.generated.h"

class ULevelEditorSubsystem;

USTRUCT()
struct CHARMTUNNEL_API FCharmEditorLibrary
{
	GENERATED_BODY()
public:
	/**
	* Creates a level.
	*
	* @param LevelAssetPath The asset path of the level to create.
	* @return true if the level was created successfully.
	*/
	static bool CreateLevel(const FString LevelAssetPath)
	{
		ULevelEditorSubsystem* LevelEditorSubsystem = GetLevelEditorSubsystem();
		return LevelEditorSubsystem ? LevelEditorSubsystem->NewLevel(LevelAssetPath) : false;
	}

	/**
	* Check if the given asset path exists.
	*
	* @param AssetPath The asset path to check.
	* @return true if the asset exists.
	*/
	static bool DoesAssetExist(const FString AssetPath)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetDataList;
		AssetRegistryModule.Get().GetAssetsByPackageName(*AssetPath, AssetDataList);
		return AssetDataList.Num() > 0;
	}

	/**
	* Load a given level into the editor. The current level will be saved before loading the new level.
	*
	* @param LevelToLoad The level asset path to load into the editor.
	* @return true if the level is loaded.
	*/
	static bool LoadLevel(const FString LevelToLoad)
	{
		if (ULevelEditorSubsystem* LevelEditorSubsystem = GetLevelEditorSubsystem())
		{
			if (LevelEditorSubsystem->SaveCurrentLevel())
			{
				return LevelEditorSubsystem->LoadLevel(LevelToLoad);
			}
		}
		return false;
	}
private:
	/**
	* Return a reference to the level editor subsystem.
	*
	* @return the level editor subsystem if valid, otherwise nullptr.
	*/
	static ULevelEditorSubsystem* GetLevelEditorSubsystem()
	{
		return GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
	}
};
