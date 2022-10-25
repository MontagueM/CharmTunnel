// Fill out your copyright notice in the Description page of Project Settings.

#include "CharmSceneViewExtension.h"

#include "Async/ParallelFor.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/Texture2DArray.h"
#include "EngineModule.h"
#include "GlobalShader.h"
#include "HAL/LowLevelMemTracker.h"
#include "MeshBatch.h"
#include "MeshMaterialShader.h"
#include "MeshMaterialShaderType.h"
#include "MeshPassProcessor.h"
#include "MeshPassProcessor.inl"
#include "NaniteSceneProxy.h"
#include "OneColorShader.h"
#include "PipelineStateCache.h"
#include "PostProcess/PostProcessMaterial.h"
#include "PrimitiveSceneInfo.h"
#include "PrimitiveSceneProxy.h"
#include "ProfilingDebugging/ExternalProfiler.h"
#include "RHI.h"
#include "RHIResources.h"
#include "RayTracingDefinitions.h"
#include "RenderCore.h"
#include "RenderGraphBuilder.h"
#include "RenderUtils.h"
#include "Renderer/Private/ScenePrivate.h"
#include "Renderer/Public/MeshPassProcessor.h"
#include "RendererInterface.h"
#include "Rendering/NaniteResources.h"
#include "SceneManagement.h"
#include "SceneRelativeViewMatrices.h"
#include "SceneUtils.h"
#include "Shader.h"
#include "ShaderParameterUtils.h"
#include "ShaderParameters.h"
#include "SimpleElementShaders.h"
#include "Texture2DPreview.h"
#include "TextureResource.h"
#include "VirtualTexturing.h"

class FCharmData : public FMeshMaterialShaderElementData
{
public:
    FMatrix44f LocalToWorld;
};

class FCharmTestVS : public FMeshMaterialShader
{
    DECLARE_SHADER_TYPE(FCharmTestVS, MeshMaterial);

    LAYOUT_FIELD(FShaderParameter, CharmPrimitiveIdParameter)

public:
    FCharmTestVS() = default;

    FCharmTestVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) : FMeshMaterialShader(Initializer)
    {
        CharmPrimitiveIdParameter.Bind(Initializer.ParameterMap, TEXT("CharmPrimitiveId"));
    }

    // It tries to compile for every single permutation of a shader, but we can restrict its use cases
    static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
    {
        // return EnumHasAllFlags(Parameters.Flags, EShaderPermutationFlags::HasEditorOnlyData)
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
               IsSupportedVertexFactoryType(Parameters.VertexFactoryType);
    }

    static bool IsSupportedVertexFactoryType(const FVertexFactoryType* VertexFactoryType)
    {
        static FName LocalVfFName = FName(TEXT("FLocalVertexFactory"), FNAME_Find);
        return VertexFactoryType == FindVertexFactoryType(LocalVfFName);
    }

    static void ModifyCompilationEnvironment(
        const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    {
        FMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
    }

    void SetParameters(FRHICommandList& RHICmdList, const FSceneView View, uint32 CharmPrimitiveId)
    {
        FRHIVertexShader* ShaderRHI = RHICmdList.GetBoundVertexShader();
        SetViewParameters(RHICmdList, ShaderRHI, View, View.ViewUniformBuffer);
        SetShaderValue(RHICmdList, ShaderRHI, CharmPrimitiveIdParameter, CharmPrimitiveId);
        // SetShaderValue(InRHICmdList, ShaderRHI, LocalToWorld, InLocalToWorld);
    }
};

class FCharmTestPS : public FMeshMaterialShader
{
    DECLARE_SHADER_TYPE(FCharmTestPS, MeshMaterial);

    LAYOUT_FIELD(FShaderResourceParameter, InputTexture)
    LAYOUT_FIELD(FShaderResourceParameter, InputTextureSampler)
public:
    FCharmTestPS() = default;

    FCharmTestPS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) : FMeshMaterialShader(Initializer)
    {
        InputTexture.Bind(Initializer.ParameterMap, TEXT("InputTexture"));
        InputTextureSampler.Bind(Initializer.ParameterMap, TEXT("InputTextureSampler"));
    }

    static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
    {
        // return EnumHasAllFlags(Parameters.Flags, EShaderPermutationFlags::HasEditorOnlyData)
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }

    void SetParameters(
        FRHICommandList& RHICmdList, const FMaterialRenderProxy* MaterialProxy, const FMaterial& Material, const FSceneView View)
    {
        FRHIVertexShader* ShaderRHI = RHICmdList.GetBoundVertexShader();
        SetViewParameters(RHICmdList, ShaderRHI, View, View.ViewUniformBuffer);
        FMeshMaterialShader::SetParameters(RHICmdList, ShaderRHI, MaterialProxy, Material, View);
        // SetShaderValue(InRHICmdList, ShaderRHI, LocalToWorld, InLocalToWorld);
    }

    void SetTexture(FRHICommandList& RHICmdList, FTextureRHIRef InInputTexture)
    {
        FRHIPixelShader* ShaderRHI = RHICmdList.GetBoundPixelShader();
        SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), InputTexture, InputTextureSampler,
            TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), InInputTexture);
    }
};

IMPLEMENT_SHADER_TYPE(, FCharmTestVS, TEXT("/Plugin/CharmTunnel/Private/CharmTestVS.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FCharmTestPS, TEXT("/Plugin/CharmTunnel/Private/CharmTestPS.usf"), TEXT("MainPS"), SF_Pixel);

// BEGIN_SHADER_PARAMETER_STRUCT(FCharmShaderParameters, )
// SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
// SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FLocalVertexFactoryShaderParameters, LocalVF)
// SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FInstanceCullingGlobalUniforms, InstanceCulling)
// RENDER_TARGET_BINDING_SLOTS()
// END_SHADER_PARAMETER_STRUCT()

FCharmSceneViewExtension::FCharmSceneViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister)
{
}

void FCharmSceneViewExtension::PostRenderBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
    FSceneView* View = &InView;
    FScene* Scene = View->Family->Scene->GetRenderScene();
    auto x = Scene->Primitives;
    auto y = Scene->PrimitiveSceneProxies;
    // we can set "Rendering -> Advanced -> Uncheck 'Render in Main Pass'" to override it with our own render?
    // might also have to do depth pass given transparency, unsure
    // but we want shadows etc so keep that stuff
    // https://github.com/donaldwuid/unreal_source_explained/blob/master/main/rendering.md <-- using old system

    int PrimitiveId = 0;
    for (auto& PrimitiveSceneProxy : Scene->PrimitiveSceneProxies)
    {
        // FStaticMeshSceneProxy* a = static_cast<FStaticMeshSceneProxy*>(Scene->PrimitiveSceneProxies[PrimitiveId]);
        // We have to dynamic cast to ensure it is a valid FStaticMeshSceneProxy - could try GetTypeHash but unsure
        // auto q = FStaticMeshSceneProxy;

        // if (PrimitiveSceneProxy->ShouldRenderInMainPass() || PrimitiveSceneProxy->ShouldRenderInDepthPass() ||
        //     PrimitiveSceneProxy->GetPrimitiveSceneInfo()->StaticMeshes.Num() == 0)
        // {
        //     continue;
        // }
        FStaticMeshSceneProxy* StaticMeshSceneProxy = static_cast<FStaticMeshSceneProxy*>(PrimitiveSceneProxy);
        RenderStaticMesh(RHICmdList, InView, StaticMeshSceneProxy, PrimitiveId);
        PrimitiveId++;
    }
    bool a = 0;
}

void FCharmSceneViewExtension::DoWorkLambda(FScene* Scene, FCachedPassMeshDrawListContext& DrawListContext, int32 BatchIndex)
{
    const auto PassType = EMeshPass::BasePass;
    // PassProcessorCreateFunction CreateFunction = FPassProcessorManager::GetCreateFunction(Scene->GetShadingPath(), PassType);
    // FMeshPassProcessor* PassMeshProcessor = CreateFunction(Scene, nullptr, &DrawListContext);
    // TArray<FPrimitiveSceneInfo*> SceneInfos = Scene->Primitives;
    // if (PassMeshProcessor != nullptr)
    // {
    //     struct FMeshInfoAndIndex
    //     {
    //         int32 InfoIndex;
    //         int32 MeshIndex;
    //     };
    //     FPrimitiveSceneInfo* SceneInfo = SceneInfos[BatchIndex];
    //     TArray<FMeshInfoAndIndex, TMemStackAllocator<>> MeshBatches;
    //     check(SceneInfo->StaticMeshCommandInfos.Num() == 0);
    //     SceneInfo->StaticMeshCommandInfos.AddDefaulted(EMeshPass::Num * SceneInfo->StaticMeshes.Num());
    //     FPrimitiveSceneProxy* SceneProxy = SceneInfo->Proxy;
    //     // Volumetric self shadow mesh commands need to be generated every frame, as they depend on single frame uniform buffers with
    //     self
    //     // shadow data.
    //     if (!SceneProxy->CastsVolumetricTranslucentShadow())
    //     {
    //         for (int32 MeshIndex = 0; MeshIndex < SceneInfo->StaticMeshes.Num(); MeshIndex++)
    //         {
    //             FStaticMeshBatch& Mesh = SceneInfo->StaticMeshes[MeshIndex];
    //             if (SupportsCachingMeshDrawCommands(Mesh))
    //             {
    //                 MeshBatches.Add(FMeshInfoAndIndex{BatchIndex, MeshIndex});
    //             }
    //         }
    //     }
    //
    //     for (const FMeshInfoAndIndex& MeshAndInfo : MeshBatches)
    //     {
    //         FPrimitiveSceneInfo* SceneInfo2 = Scene->GetPrimitiveSceneInfo(MeshAndInfo.InfoIndex);
    //         FStaticMeshBatch& Mesh = SceneInfo2->StaticMeshes[MeshAndInfo.MeshIndex];
    //
    //         FStaticMeshBatchRelevance& MeshRelevance = SceneInfo2->StaticMeshRelevances[MeshAndInfo.MeshIndex];
    //
    //         check(!MeshRelevance.CommandInfosMask.Get(PassType));
    //
    //         uint64 BatchElementMask = ~0ull;
    //         // NOTE: AddMeshBatch calls FCachedPassMeshDrawListContext::FinalizeCommand
    //         PassMeshProcessor->AddMeshBatch(Mesh, BatchElementMask, SceneInfo2->Proxy);
    //
    //         FCachedMeshDrawCommandInfo CommandInfo = DrawListContext.GetCommandInfoAndReset();
    //         if (CommandInfo.CommandIndex != -1 || CommandInfo.StateBucketId != -1)
    //         {
    //             static_assert(sizeof(MeshRelevance.CommandInfosMask) * 8 >= EMeshPass::Num,
    //                 "CommandInfosMask is too small to contain all mesh passes.");
    //             MeshRelevance.CommandInfosMask.Set(PassType);
    //             MeshRelevance.CommandInfosBase++;
    //
    //             int CommandInfoIndex = MeshAndInfo.MeshIndex * EMeshPass::Num + PassType;
    //             check(SceneInfo2->StaticMeshCommandInfos[CommandInfoIndex].MeshPass == EMeshPass::Num);
    //             SceneInfo2->StaticMeshCommandInfos[CommandInfoIndex] = CommandInfo;
    //         }
    //     }
    //
    //     PassMeshProcessor->~FMeshPassProcessor();
    // }
}

void FCharmSceneViewExtension::RenderStaticMesh(
    FRHICommandListImmediate& RHICmdList, FSceneView& InView, FStaticMeshSceneProxy* StaticMeshSceneProxy, uint32 PrimitiveId)
{
    FSceneView* View = &InView;
    // BuildMeshDrawCommands -> DrawListContext->AddCommand + DrawListContext->FinalizeCommand
    // FMeshDrawCommand::SubmitDraw seems to be the key here
    // q: do we submit our draw ourselves, or somehow add to a list?
    // a: quite sure we submit it ourselves, one draw per mesh.
    // FMeshPassProcessor::BuildMeshDrawCommands
    // a: then we just need MeshDrawCommand which is an FMeshDrawCommand, and submit it!
    // -> FMeshDrawCommand::SubmitDrawEnd -> RHICmdList.DrawIndexedPrimitive
    if (false)
    {
        // RHICmdList.DrawIndexedPrimitive()
        // FMeshDrawCommand& MeshDrawCommand = DrawListContext->AddCommand(SharedMeshDrawCommand, NumElements);
        // DrawListContext->FinalizeCommand(MeshBatch, BatchElementIndex, IdInfo, MeshFillMode, MeshCullMode, SortKey, Flags, PipelineState,
        //     &ShadersForDebugging, MeshDrawCommand);
        // FScene* Scene = InView.Family->Scene->GetRenderScene();
        // FMeshPassDrawListContext DrawListContext(*Scene);
        // for (int Idx = 0; Idx < StaticMeshSceneProxy->GetNumMeshBatches(); Idx++)
        // {
        // DoWorkLambda(Scene, DrawListContext, Idx);
        // }
        // bool bAnyLooseParameterBuffers = DrawListContext.HasAnyLooseParameterBuffers();
        return;
    }

    // if (!OverlayVertexBufferRHI)
    // {
    //     // Setup vertex buffer
    //     const FVector4f Positions[] = {FVector4f(0.0f, 1.0f, 0.0f, 1.0f), FVector4f(0.0f, 0.0f, 0.0f, 1.0f),
    //         FVector4f(1.0f, 1.0f, 0.0f, 1.0f), FVector4f(1.0f, 0.0f, 0.0f, 1.0f)};
    //
    //     TResourceArray<FFilterVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
    //     Vertices.SetNumUninitialized(4);
    //
    //     for (auto Index = 0; Index < UE_ARRAY_COUNT(Positions); ++Index)
    //     {
    //         const auto& Position = Positions[Index];
    //         Vertices[Index].Position = Position;
    //         Vertices[Index].UV = FVector2f(Position.X, Position.Y);
    //     }
    //
    //     FRHIResourceCreateInfo CreateInfoVB(TEXT("VideoOverlayVertexBuffer"), &Vertices);
    //     OverlayVertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfoVB);
    //
    //     // Cache UVOffsets
    //     const FVector2D ViewSize(InView.UnconstrainedViewRect.Max.X, InView.UnconstrainedViewRect.Max.Y);
    //     const FVector2D CameraSize = Frame.Camera.ImageResolution;
    //     for (auto Index = 0; Index < UE_ARRAY_COUNT(UVOffsets); ++Index)
    //     {
    //         if (Index == VERTEX_BUFFER_INDEX_LANDSCAPE)
    //         {
    //             // Landscape
    //             const auto ViewSizeLandscape = ViewSize.X > ViewSize.Y ? ViewSize : FVector2D(ViewSize.Y, ViewSize.X);
    //             const auto TextureSizeLandscape = CameraSize.X > CameraSize.Y ? CameraSize : FVector2D(CameraSize.Y, CameraSize.X);
    //             UVOffsets[Index] = UARUtilitiesFunctionLibrary::GetUVOffset(ViewSizeLandscape, TextureSizeLandscape);
    //         }
    //         else
    //         {
    //             // Portrait
    //             const auto ViewSizePortrait = ViewSize.X < ViewSize.Y ? ViewSize : FVector2D(ViewSize.Y, ViewSize.X);
    //             const auto TextureSizePortrait = CameraSize.X < CameraSize.Y ? CameraSize : FVector2D(CameraSize.Y, CameraSize.X);
    //             UVOffsets[Index] = UARUtilitiesFunctionLibrary::GetUVOffset(ViewSizePortrait, TextureSizePortrait);
    //         }
    //     }
    // }
    //
    // if (!IndexBufferRHI)
    // {
    //     // Setup index buffer
    //     const uint16 Indices[] = {0, 1, 2, 2, 1, 3};
    //
    //     TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> IndexBuffer;
    //     const uint32 NumIndices = UE_ARRAY_COUNT(Indices);
    //     IndexBuffer.AddUninitialized(NumIndices);
    //     FMemory::Memcpy(IndexBuffer.GetData(), Indices, NumIndices * sizeof(uint16));
    //
    //     FRHIResourceCreateInfo CreateInfoIB(TEXT("VideoOverlayIndexBuffer"), &IndexBuffer);
    //     IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), IndexBuffer.GetResourceDataSize(), BUF_Static, CreateInfoIB);
    // }

    // void FPrimitiveSceneInfo::AddStaticMeshes
    auto PSI = StaticMeshSceneProxy->GetPrimitiveSceneInfo();
    if (PSI->StaticMeshes.Num() == 0)
    {
        return;
    }
    // SCOPED_DRAW_EVENTF(RHICmdList, RenderStaticMesh,
    // *FString::Printf(
    //     TEXT("CHARM %ls %ls"), *StaticMeshSceneProxy->GetOwnerName().ToString(), *StaticMeshSceneProxy->GetLevelName().ToString()));
    SCOPED_DRAW_EVENTF(RHICmdList, RenderStaticMesh, TEXT("CT SM"));
    for (FStaticMeshBatch& StaticMesh : PSI->StaticMeshes)
    {
        if (!StaticMesh.bUseForDepthPass)
        {
            const auto FeatureLevel = InView.GetFeatureLevel();
            IRendererModule& RendererModule = GetRendererModule();

            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            const FVertexFactory* VertexFactory = StaticMesh.VertexFactory;

            const FMaterialRenderProxy* MaterialProxy = StaticMesh.MaterialRenderProxy;
            const FMaterial& Material = MaterialProxy->GetMaterialWithFallback(FeatureLevel, MaterialProxy);
            const FMaterialShaderMap* const MaterialShaderMap = Material.GetRenderingThreadShaderMap();
            TShaderRef<FCharmTestVS> VertexShader;
            TShaderRef<FCharmTestPS> PixelShader;

            FMaterialShaderTypes ShaderTypes;
            ShaderTypes.AddShaderType<FCharmTestVS>();
            ShaderTypes.AddShaderType<FCharmTestPS>();

            FMaterialShaders Shaders;
            if (!Material.TryGetShaders(ShaderTypes, StaticMesh.VertexFactory->GetType(), Shaders))
            {
                continue;
            }
            auto SMElement = StaticMesh.Elements[0];

            Shaders.TryGetVertexShader(VertexShader);
            Shaders.TryGetPixelShader(PixelShader);
            check(VertexShader.IsValid());
            check(PixelShader.IsValid());
            // const TUniformBuffer<FPrimitiveUniformShaderParameters>* PUBR = SMElement.PrimitiveUniformBufferResource;
            // FRHIUniformBuffer* PUB = PUBR->GetUniformBufferRHI();

            // VertexShader->SetViewParameters(RHICmdList, VertexShader.GetVertexShader(), InView, InView.ViewUniformBuffer);

            // SetUniformBufferParameter(
            // RHICmdList, VertexShader.GetVertexShader(), MaterialUniformBuffer, UniformExpressionCache->UniformBuffer);
            // VertexShader->SetViewParameters();
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

            // CreateLocalVFUniformBuffer(LocalVF, LODIndex, VertexBuffer, 0, 0)

            // THIS IS WHERE THE ATTRIBUTE SET IS DECLARED!!!!
            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexFactory->GetDeclaration(EVertexInputStreamType::Default);

            // Enable depth test
            GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_DepthNearOrEqual>::GetRHI();
            GraphicsPSOInit.PrimitiveType = PT_TriangleList;
            GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
            GraphicsPSOInit.BlendState = TStaticBlendStateWriteMask<>::GetRHI();
            SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

            // assume 1 element
            // auto q = SMElement.PrimitiveUniformBuffer;
            // FRHIUniformBuffer* VFUserData =
            // static_cast<FRHIUniformBuffer*>(SMElement.VertexFactoryUserData);    // <--- FLocalVertexFactoryUniformShaderParameters
            FVertexInputStreamArray VertexStreams;
            VertexFactory->GetStreams(ERHIFeatureLevel::ES3_1, EVertexInputStreamType::Default,
                VertexStreams);    // ES3_1 to force it giving us manual fetch streams
            for (auto& VertexStream : VertexStreams)
            {
                RHICmdList.SetStreamSource(VertexStream.StreamIndex, VertexStream.VertexBuffer, VertexStream.Offset);
            }

            // TResourceArray<UINT32, VERTEXBUFFER_ALIGNMENT> Verts;
            // Verts.Add(12);
            // FRHIResourceCreateInfo CreateInfo(TEXT("CharmPrimitiveId"), &Verts);
            // FBufferRHIRef VertexBufferRHI = RHICreateVertexBuffer(Verts.GetResourceDataSize(), BUF_Static, CreateInfo);
            // RHICmdList.SetStreamSource(13, VertexBufferRHI.GetReference(), 0);
            // auto VertexStream = VertexStreams[0];
            // RHICmdList.SetStreamSource(0, SMElement.VertexBufferRHI, 0);

            // const FLocalVertexFactory* LocalVF = static_cast<const FLocalVertexFactory*>(VertexFactory);
            // TUniformBufferRef<FLocalVertexFactoryUniformShaderParameters> VFUniformBufferRef =
            // CreateLocalVFUniformBuffer(LocalVF, 0, nullptr, SMElement.BaseVertexIndex, SMElement.BaseVertexIndex);
            // FLocalVertexFactoryUniformShaderParameters VFUniformBuffer;
            // VFUniformBufferRef.CreateUniformBufferImmediate(VFUniformBuffer, UniformBuffer_SingleFrame);
            // SetUniformBufferParameterImmediate(RHICmdList, VertexShader.GetVertexShader(),
            // VertexShader->GetUniformBufferParameter<FLocalVertexFactoryUniformShaderParameters>(), VFUserData);

            // FInstanceCullingGlobalUniforms InstanceCullingGlobalUniforms;
            // InstanceCullingGlobalUniforms.InstanceIdsBuffer
            // CreateUniformBufferImmediate<FInstanceCullingGlobalUniforms>(
            // InstanceCullingGlobalUniforms, UniformBuffer_SingleFrame);
            // SetUniformBufferParameterImmediate(RHICmdList, VertexShader.GetVertexShader(),
            // VertexShader->GetUniformBufferParameter<FInstanceCullingGlobalUniforms>(), InstanceCullingGlobalUniforms);
            // GetLocalToWorld()
            // RHICmdList

            // FCharmData ShaderElementData;
            // ShaderElementData.InitializeMeshMaterialData(View, StaticMeshSceneProxy, StaticMesh, -1, true);
            // ShaderElementData.LocalToWorld = FMatrix44f(StaticMeshSceneProxy->GetLocalToWorld());
            VertexShader->SetParameters(RHICmdList, InView, PrimitiveId);
            PixelShader->SetParameters(RHICmdList, MaterialProxy, Material, InView);
            // VertexShader->SetCustomParameters(RHICmdList, VertexShader.GetVertexShader(), 12);
            // FCharmTestVS* const VertexShaderPtr = static_cast<FCharmTestVS*>(VertexShader.GetShader());
            // check(VertexShaderPtr != nullptr);
            // SetUniformBufferParameterImmediate(RHICmdList, VertexShader.GetVertexShader(),
            // VertexShaderPtr->GetUniformBufferParameter<FDrawRectangleParameters>(), Parameters);

            // UTexture2D* Texture = NewObject<UTexture2D>(Font, *TextureString);
            bool bSet = false;
            FMaterialRenderContext MaterialRenderContext(MaterialProxy, Material, View);
            const FMaterialResource* MatInt = Material.GetMaterialInterface()->GetMaterialResource(FeatureLevel);
            if (MatInt)
            {
                const FUniformExpressionSet& UniformExpressions = MatInt->GetUniformExpressions();
                EMaterialTextureParameterType TextureTypes[] = {
                    EMaterialTextureParameterType::Standard2D, EMaterialTextureParameterType::Virtual};
                for (EMaterialTextureParameterType TextureType : TextureTypes)
                {
                    for (int32 i = 0; i < UniformExpressions.GetNumTextures(TextureType); ++i)
                    {
                        const UTexture* Texture;
                        UniformExpressions.GetTextureValue(TextureType, i, MaterialRenderContext, Material, Texture);

                        const UTexture2D* Texture2D = Cast<UTexture2D>(Texture);

                        if (!bSet)
                        {
                            bSet = true;
                            PixelShader->SetTexture(RHICmdList, Texture2D->GetResource()->GetTexture2DRHI());
                        }
                        bool b = 0;
                    }
                }
            }

            RHICmdList.DrawIndexedPrimitive(SMElement.IndexBuffer->IndexBufferRHI, SMElement.BaseVertexIndex, 0,
                SMElement.MaxVertexIndex - SMElement.MinVertexIndex, SMElement.FirstIndex, SMElement.NumPrimitives, SMElement.NumInstances);
        }
    }

    //

    //
    // if (OverlayVertexBufferRHI && IndexBufferRHI)
    // {
    //     RHICmdList.SetStreamSource(0, OverlayVertexBufferRHI, 0);
    //     RHICmdList.DrawIndexedPrimitive(IndexBufferRHI,
    //         /*BaseVertexIndex=*/0,
    //         /*MinIndex=*/0,
    //         /*NumVertices=*/4,
    //         /*StartIndex=*/0,
    //         /*NumPrimitives=*/2,
    //         /*NumInstances=*/1);
    // }
}

/* void FColorCorrectRegionsSceneViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const
 * FPostProcessingInputs& Inputs) take a look at this!!!! it seems to do everything I want, like adding a new render pass for an iteration
 * over meshes it also takes into account the viewport etc
 * however, we still cant get scene proxy info here which is what we really need
 * i need a method where i can get scene proxy but ALSO do it via a plugin
 */

void FCharmSceneViewExtension::PrePostProcessPass_RenderThread(
    FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
    //     GraphBuilder.AddPass(Forward<FRDGEventName>(PassName), PassParameters, ERDGPassFlags::Raster,
    //         [PassParameters, GlobalShaderMap, Viewport, PixelShader, BlendState, RasterizerState, DepthStencilState, StencilRef](
    //             FRHICommandList& RHICmdList)
    //         {
    //             RHICmdList.SetViewport(Viewport.Min.X, Viewport.Min.Y, 0.0f, Viewport.Max.X, Viewport.Max.Y, 1.0f);
    //
    //             FGraphicsPipelineStateInitializer GraphicsPSOInit;
    //             FPixelShaderUtils::InitFullscreenPipelineState(RHICmdList, GlobalShaderMap, PixelShader, GraphicsPSOInit);
    //
    //             GraphicsPSOInit.BlendState = BlendState ? BlendState : GraphicsPSOInit.BlendState;
    //             GraphicsPSOInit.RasterizerState = RasterizerState ? RasterizerState : GraphicsPSOInit.RasterizerState;
    //             GraphicsPSOInit.DepthStencilState = DepthStencilState ? DepthStencilState : GraphicsPSOInit.DepthStencilState;
    //
    //             SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, StencilRef);
    //
    //             SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PassParameters->PS);
    //
    //             FPixelShaderUtils::DrawFullscreenTriangle(RHICmdList);
    //         });
}
