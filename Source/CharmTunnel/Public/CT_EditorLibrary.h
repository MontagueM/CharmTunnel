#pragma once

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "CT_UsfConverter.h"
#include "CoreMinimal.h"
#include "EditorAssetLibrary.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/TextureFactory.h"
#include "LevelEditorSubsystem.h"
#include "MaterialEditingLibrary.h"
#include "Materials/MaterialExpressionBreakMaterialAttributes.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Misc/FileHelper.h"
#include "ObjectTools.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "CT_EditorLibrary.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCharmTunnel, Log, All);
inline DEFINE_LOG_CATEGORY(LogCharmTunnel);

#define LOG(x, ...) UE_LOG(LogCharmTunnel, Log, TEXT(x), __VA_ARGS__)
#define LOG_VERBOSE(x, ...) UE_LOG(LogCharmTunnel, Verbose, TEXT(x), __VA_ARGS__)
#define LOG_WARNING(x, ...) UE_LOG(LogCharmTunnel, Warning, TEXT(x), __VA_ARGS__)
#define LOG_ERROR(x, ...) UE_LOG(LogCharmTunnel, Error, TEXT(x), __VA_ARGS__)

/*

Todo:

Move USF converter into the plugin from Charm

*/

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
     * Deletes the given directory.
     *
     * @param DirectoryPath The directory to delete.
     * @return true if the directory is deleted.
     */
    static bool DeleteDirectory(const FString DirectoryPath)
    {
        TArray<FString> FilesToDelete = GetFilesInDirectory(DirectoryPath);
        if (FilesToDelete.Num() > 0)
        {
            TArray<UObject*> AssetsToDelete;
            for (const auto& FileToDelete : FilesToDelete)
            {
                AssetsToDelete.Add(LoadAsset<UObject>(FileToDelete));
            }
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
    template <typename T>
    static T* LoadAsset(const FString AssetPath)
    {
        if (!DoesAssetExist(AssetPath))
        {
            LOG_ERROR("Asset does not exist, cannot load %s.", *AssetPath);
            return nullptr;
        }
        return Cast<T>(LoadObject<T>(nullptr, *AssetPath));
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

    static bool LoadConfigFile(const FString& ConfigFilePath, OUT TSharedPtr<FJsonObject>& JsonObject)
    {
        FString FileContents;
        if (!FFileHelper::LoadFileToString(FileContents, *ConfigFilePath))
        {
            LOG_ERROR("Failed to load config file %s.", *ConfigFilePath);
            return false;
        }

        const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(FileContents);

        if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
        {
            LOG_ERROR("Unable to parse config json=[%s]", *FileContents);
            return false;
        }

        return true;
    }

    /**
     * Import an asset using a Charm config file.
     *
     * @param ConfigFilePath The path of the *_info.cfg file.
     * @param TargetDirectory The directory to import the asset into.
     * @return true if the asset is imported.
     */
    static bool ImportAssetFromConfigFile(const FString& ConfigFilePath, const FString& TargetDirectory)
    {
        // Read the config file to determine how to load the file.
        TSharedPtr<FJsonObject> JsonObject;
        if (!LoadConfigFile(ConfigFilePath, JsonObject))
        {
            return false;
        }

        const FString ParentDirectory = FPaths::GetPath(ConfigFilePath);

        // Identify how to import this asset fully depending on the type.
        // TODO account for entities
        if (JsonObject->GetObjectField("Instances")->Values.Num() == 0)
        {
            return ImportStaticFromConfigFile(JsonObject, ParentDirectory, TargetDirectory);
        }
        else
        {
            // todo implement + maybe rename info.cfg to metadata
            // ImportMapFromConfigFile(JsonObject, TargetDirectory);
        }
        return true;
    }

    /**
     * Import an static mesh asset using a Charm config file.
     *
     * @param JsonObject Json object of the info config file.
     * @param SourceDirectory The directory to import the asset from.
     * @param TargetDirectory The directory to import the asset into.
     * @return true if the asset is imported.
     */
    static bool ImportStaticFromConfigFile(
        const TSharedPtr<FJsonObject> JsonObject, const FString& SourceDirectory, const FString& TargetDirectory)
    {
        // Import mesh using FBX factory
        const FString MeshName = JsonObject->GetStringField("MeshName");
        const FString MeshPath = SourceDirectory / MeshName + ".fbx";
        const TArray<UObject*> ImportedObjects = ImportFbxAsStaticMesh(MeshPath, TargetDirectory);
        UStaticMesh* ImportedMesh = Cast<UStaticMesh>(ImportedObjects[0]);

        // Make materials
        const TSharedPtr<FJsonObject> Materials = JsonObject->GetObjectField("Materials");
        const TSharedPtr<FJsonObject> Parts = JsonObject->GetObjectField("Parts");
        TSet<FString> TextureHashesToImport;
        for (auto& StaticMaterial : ImportedMesh->GetStaticMaterials())
        {
            FString MaterialHash = StaticMaterial.MaterialSlotName.ToString();
            if (MaterialHash == "2029BE80")
            {
                auto a = 0;
            }
            const TSharedPtr<FJsonObject> MaterialInfo = Materials->GetObjectField(MaterialHash);
            const TSharedPtr<FJsonObject> PSTexturesInfo = MaterialInfo->GetObjectField("PS")->GetObjectField("Textures");
            for (auto& Value : PSTexturesInfo->Values)
            {
                const TSharedPtr<FJsonObject>* TextureMap;
                Value.Value->TryGetObject(TextureMap);
                const FString TextureHash = (*TextureMap)->GetStringField("Hash");
                TextureHashesToImport.Add(TextureHash);
            }
        }

        // Make materials - first import every texture (faster to do all in one go than stop-start)
        ImportTextures(TextureHashesToImport.Array(), SourceDirectory / "Textures", TargetDirectory);

        // Make materials - then create the materials one by one, but only if the material does not already exist
        for (auto& StaticMaterial : ImportedMesh->GetStaticMaterials())
        {
            FString MaterialHash = StaticMaterial.MaterialSlotName.ToString();
            const TSharedPtr<FJsonObject> MaterialInfo = Materials->GetObjectField(MaterialHash);
            // const TSharedPtr<FJsonObject> PSTexturesInfo = MaterialInfo->GetObjectField("PS");
            UMaterial* Material;
            if (!DoesAssetExist(TargetDirectory / "Materials" / MaterialHash))
            {
                Material = CreateMaterialFromConfigFile(MaterialHash, MaterialInfo, SourceDirectory, TargetDirectory);
            }
            else
            {
                Material = LoadAsset<UMaterial>(TargetDirectory / "Materials" / MaterialHash);
            }

            // Assign material to mesh
            ImportedMesh->SetMaterial(ImportedMesh->GetMaterialIndex(StaticMaterial.MaterialSlotName), Material);
        }

        return true;
    }

    static UMaterial* CreateMaterialFromConfigFile(
        const FString& MaterialName, TSharedPtr<FJsonObject> MaterialInfo, const FString& SourceDirectory, const FString& TargetDirectory)
    {
        // Make material object
        const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
        UMaterialFactoryNew* MaterialFactory = UMaterialFactoryNew::StaticClass()->GetDefaultObject<UMaterialFactoryNew>();
        UObject* MaterialObject =
            AssetToolsModule.Get().CreateAsset(MaterialName, TargetDirectory / "Materials", UMaterial::StaticClass(), MaterialFactory);
        UMaterial* Material = Cast<UMaterial>(MaterialObject);

        // Configure material

        // Add custom nodes
        UMaterialExpressionCustom* CustomVSNode = Cast<UMaterialExpressionCustom>(
            UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionCustom::StaticClass(), -1500, 0));
        // CustomVSNode->Code = "";
        // CustomVSNode->OutputType = ECustomMaterialOutputType::CMOT_MaterialAttributes;
        UMaterialExpressionCustom* CustomPSNode = Cast<UMaterialExpressionCustom>(
            UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionCustom::StaticClass(), -500, 0));
        // FString UsfContents;
        bool bOutSuccess;
        TSharedRef<UsfShader> Shader = CT_UsfConverter::ConvertFromHlsl(MaterialInfo->GetObjectField("PS"),
            SourceDirectory / "Shaders" / "PS_" + MaterialName + ".hlsl", EShaderType::PixelShader, bOutSuccess);

        // FString PsUsfFilePath = SourceDirectory / "Shaders" / "PS_" + MaterialName + ".usf";
        // if (!FFileHelper::LoadFileToString(UsfContents, *PsUsfFilePath))
        // {
        // LOG_ERROR("Failed to load usf file %s.", *PsUsfFilePath);
        // }
        CustomPSNode->Code = Shader->UsfContents;
        if (Shader->UsfContents.Contains("// masked"))
        {
            Material->BlendMode = BLEND_Masked;
            Material->TwoSided = true;
        }
        CustomPSNode->OutputType = CMOT_MaterialAttributes;
        CustomPSNode->Inputs.Empty();
        const TSharedPtr<FJsonObject> PSTexturesInfo = MaterialInfo->GetObjectField("PS")->GetObjectField("Textures");
        int i = 0;
        for (auto& TextureInfo : PSTexturesInfo->Values)
        {
            // In here also add texture samples and connect to custom nodes
            UMaterialExpressionTextureSample* TextureNode =
                Cast<UMaterialExpressionTextureSample>(UMaterialEditingLibrary::CreateMaterialExpression(
                    Material, UMaterialExpressionTextureSample::StaticClass(), -1000, -500 + 250 * i++));
            const TSharedPtr<FJsonObject>* TextureMap;
            // todo we really should pull all this out into separate classes, DestinyMaterial etc
            TextureInfo.Value->TryGetObject(TextureMap);
            const FString TextureHash = (*TextureMap)->GetStringField("Hash");
            const bool bTextureIsSrgb = (*TextureMap)->GetBoolField("SRGB");
            UTexture* Texture = LoadAsset<UTexture>(TargetDirectory / "Textures" / TextureHash);
            if (!Texture)
            {
                LOG_ERROR("Failed to load texture %s.", *TextureHash);
                continue;
            }
            TextureNode->Texture = Texture;
            Texture->PreEditChange(nullptr);
            Texture->SRGB = bTextureIsSrgb;
            // Texture->CompressionNone = true;
            Texture->CompressionSettings = bTextureIsSrgb ? TC_Default : TC_VectorDisplacementmap;
            TextureNode->SamplerType = bTextureIsSrgb ? SAMPLERTYPE_Color : SAMPLERTYPE_LinearColor;
            // Texture->MipGenSettings = TMGS_NoMipmaps;
            // Texture->Filter = TF_Nearest;
            // Texture->AssetImportData->Update(TargetTextureName.ToString());
            Texture->PostEditChange();
            FCustomInput Input;
            Input.InputName = FName("t" + TextureInfo.Key);
            FExpressionInput ExpressionInput;
            ExpressionInput.Expression = TextureNode;
            Input.Input = ExpressionInput;
            CustomPSNode->Inputs.Add(Input);
        }
        UMaterialExpressionTextureCoordinate* TextureCoordinateNode = Cast<UMaterialExpressionTextureCoordinate>(
            UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionTextureCoordinate::StaticClass(), -500, 300));

        FCustomInput Input;
        Input.InputName = "tx";
        FExpressionInput ExpressionInput;
        ExpressionInput.Expression = TextureCoordinateNode;
        Input.Input = ExpressionInput;
        CustomPSNode->Inputs.Add(Input);

        // Connect to output
        UMaterialExpressionBreakMaterialAttributes* MatAttrNode =
            Cast<UMaterialExpressionBreakMaterialAttributes>(UMaterialEditingLibrary::CreateMaterialExpression(
                Material, UMaterialExpressionBreakMaterialAttributes::StaticClass(), -300, 0));
        UMaterialEditingLibrary::ConnectMaterialExpressions(CustomPSNode, "", MatAttrNode, "Attr");
        UMaterialEditingLibrary::ConnectMaterialProperty(MatAttrNode, "BaseColor", MP_BaseColor);
        UMaterialEditingLibrary::ConnectMaterialProperty(MatAttrNode, "Metallic", MP_Metallic);
        UMaterialEditingLibrary::ConnectMaterialProperty(MatAttrNode, "Roughness", MP_Roughness);
        UMaterialEditingLibrary::ConnectMaterialProperty(MatAttrNode, "EmissiveColor", MP_EmissiveColor);
        UMaterialEditingLibrary::ConnectMaterialProperty(MatAttrNode, "OpacityMask", MP_OpacityMask);
        UMaterialEditingLibrary::ConnectMaterialProperty(MatAttrNode, "Normal", MP_Normal);
        UMaterialEditingLibrary::ConnectMaterialProperty(MatAttrNode, "AmbientOcclusion", MP_AmbientOcclusion);

        UMaterialEditingLibrary::RecompileMaterial(Material);

        return Material;
    }

    static TArray<UTexture*> ImportTextures(
        const TArray<FString>& TexturePaths, const FString& SourceDirectory, const FString& TargetDirectory)
    {
        UTextureFactory* TextureFactory = UTextureFactory::StaticClass()->GetDefaultObject<UTextureFactory>();
        TextureFactory->SuppressImportOverwriteDialog();

        UAutomatedAssetImportData* ImportData =
            NewObject<UAutomatedAssetImportData>(TextureFactory, UAutomatedAssetImportData::StaticClass());
        ImportData->Factory = TextureFactory;
        ImportData->bReplaceExisting =
            true;    // required when using batch import, if one exists it stops the entire import for some reason
        ImportData->DestinationPath = TargetDirectory / "Textures";
        for (auto& TexturePath : TexturePaths)
        {
            ImportData->Filenames.Add(SourceDirectory / TexturePath + ".dds");
        }
        TextureFactory->SetAutomatedAssetImportData(ImportData);
        const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
        TArray<UObject*> ImportedObjects = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);

        TArray<UTexture*> ImportedTextures;
        // todo dont correct textures here, correct them in the material code
        for (const auto& ImportedObject : ImportedObjects)
        {
            if (UTexture* Texture = Cast<UTexture>(ImportedObject))
            {
                // todo correct these settings
                ImportedTextures.Add(Texture);
            }
        }

        UEditorAssetLibrary::SaveLoadedAssets(ImportedObjects, true);

        return ImportedTextures;
    }

    static TArray<UObject*> ImportFbxAsStaticMesh(const FString& FbxPath, const FString& TargetDirectory)
    {
        UFbxFactory* FbxFactory = NewObject<UFbxFactory>(UFbxFactory::StaticClass());
        FbxFactory->AddToRoot();

        UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>(FbxFactory, UAutomatedAssetImportData::StaticClass());
        ImportData->Factory = FbxFactory;
        ImportData->bReplaceExisting = true;
        ImportData->DestinationPath = "/CharmTunnel/Dev/SM";
        ImportData->Filenames.Add(FbxPath);

        FbxFactory->SetAutomatedAssetImportData(ImportData);
        UFbxImportUI* ImportUI = NewObject<UFbxImportUI>(UFbxImportUI::StaticClass());
        FbxFactory->ImportUI = ImportUI;
        ImportUI->StaticMeshImportData->bCombineMeshes = true;
        ImportUI->StaticMeshImportData->bBuildNanite = false;
        ImportUI->StaticMeshImportData->bGenerateLightmapUVs = false;
        ImportUI->StaticMeshImportData->bAutoGenerateCollision = false;
        ImportUI->StaticMeshImportData->ImportUniformScale = 100.0f;
        ImportUI->StaticMeshImportData->bConvertSceneUnit = true;
        ImportUI->StaticMeshImportData->bConvertScene = true;
        // ImportUI->StaticMeshImportData->ImportRotation = FRotator(0.0f, 0.0f, -90.0f);
        ImportUI->StaticMeshImportData->NormalImportMethod = FBXNIM_ImportNormals;
        ImportUI->StaticMeshImportData->VertexColorImportOption = EVertexColorImportOption::Replace;
        ImportUI->bImportMaterials = false;
        ImportUI->bImportTextures = false;
        ImportUI->bImportAnimations = false;
        ImportUI->bImportAsSkeletal = false;
        ImportUI->bImportMesh = true;
        const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
        TArray<UObject*> ImportedObjects = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);

        return ImportedObjects;
    }

private:
    /**
     * Return a reference to the level editor subsystem.
     *
     * @return the level editor subsystem if valid, otherwise nullptr.
     */
    static ULevelEditorSubsystem* GetLevelEditorSubsystem() { return GEditor->GetEditorSubsystem<ULevelEditorSubsystem>(); }
};