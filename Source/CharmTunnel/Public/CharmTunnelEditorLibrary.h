#pragma once

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "CoreMinimal.h"
#include "LevelEditorSubsystem.h"
#include "ObjectTools.h"

#include "CharmTunnelEditorLibrary.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCharmTunnel, Log, All);
inline DEFINE_LOG_CATEGORY(LogCharmTunnel);

#define LOG(x, ...) UE_LOG(LogCharmTunnel, Log, TEXT(x), __VA_ARGS__)
#define LOG_VERBOSE(x, ...) UE_LOG(LogCharmTunnel, Verbose, TEXT(x), __VA_ARGS__)
#define LOG_WARNING(x, ...) UE_LOG(LogCharmTunnel, Warning, TEXT(x), __VA_ARGS__)
#define LOG_ERROR(x, ...) UE_LOG(LogCharmTunnel, Error, TEXT(x), __VA_ARGS__)

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
     * @param OutLevel The created level if successful.
     * @return true if the level was created successfully.
     */
    static bool CreateLevel(const FString LevelAssetPath, OUT ULevel*& OutLevel)
    {
        ULevelEditorSubsystem* LevelEditorSubsystem = GetLevelEditorSubsystem();
        if (LevelEditorSubsystem && LevelEditorSubsystem->NewLevel(LevelAssetPath))
        {
            OutLevel = LevelEditorSubsystem->GetCurrentLevel();
            return true;
        }
        return false;
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
     * Deletes the given asset.
     *
     * @param AssetPath The asset path to delete.
     * @return true if the asset is deleted.
     */
    static bool DeleteAsset(const FString AssetPath)
    {
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        TArray<FAssetData> AssetDataList;
        AssetRegistryModule.Get().GetAssetsByPackageName(*AssetPath, AssetDataList);
        if (AssetDataList.Num() > 1)
        {
            LOG_ERROR("More than one asset found with the given asset path.");
            return false;
        }
        else if (AssetDataList.Num() == 1)
        {
            UObject* Asset = AssetDataList[0].GetAsset();
            TArray<UObject*> AssetsToDelete;
            AssetsToDelete.Add(Asset);
            return ObjectTools::ForceDeleteObjects(AssetsToDelete, false) == AssetsToDelete.Num();
        }
        return false;
    }

    /**
     * Load a given level into the editor. The current level will be saved before loading the new level.
     *
     * @param LevelToLoad The level asset path to load into the editor.
     * @return true if the level is loaded.
     */
    static bool LoadLevelIntoEditor(const FString LevelToLoad)
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

    /**
     * Load a given asset to be used by reference.
     *
     * @param AssetPath The asset to load.
     * @return UObject* reference if the asset exists, otherwise nullptr.
     */
    static UObject* LoadAsset(const FString AssetPath)
    {
        if (!DoesAssetExist(AssetPath))
        {
            LOG_ERROR("Asset does not exist, cannot load %s.", *AssetPath);
            return nullptr;
        }
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        TArray<FAssetData> AssetDataList;
        AssetRegistryModule.Get().GetAssetsByPackageName(*AssetPath, AssetDataList);
        if (AssetDataList.Num() > 1)
        {
            LOG_ERROR("More than one asset found with the given asset path.");
            return nullptr;
        }
        else if (AssetDataList.Num() == 1)
        {
            return AssetDataList[0].GetAsset();
        }
        return nullptr;
    }

    /**
     * Get a list of all files in a given folder, recursive.
     *
     * @param SearchDirectory The directory to start searching from.
     * @return TArray<FString> of all files found.
     */
    static TArray<FString> GetFilesInDirectory(const FString SearchDirectory, const FString FileSelector = TEXT("*.*"))
    {
        TArray<FString> Files;
        IFileManager::Get().FindFilesRecursive(Files, *SearchDirectory, *FileSelector, true, false);
        return Files;
    }

    /**
     * Import an asset using a Charm config file.
     *
     * @param ConfigFilePath The directory to start searching from.
     * @param ConfigFilePath The directory to start searching from.
     * @return true if the asset is imported.
     */
    static bool ImportAssetFromConfigFile(const FString& ConfigFilePath, const FString& TargetDirectory) { return false; }

private:
    /**
     * Return a reference to the level editor subsystem.
     *
     * @return the level editor subsystem if valid, otherwise nullptr.
     */
    static ULevelEditorSubsystem* GetLevelEditorSubsystem() { return GEditor->GetEditorSubsystem<ULevelEditorSubsystem>(); }
};
