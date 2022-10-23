// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MeshPassProcessor.h"
#include "SceneViewExtension.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FCharmUniformBufferParameters, ENGINE_API)
SHADER_PARAMETER_SRV(Buffer<float>, Position)
SHADER_PARAMETER_SRV(Buffer<float>, MotionBlurData)
SHADER_PARAMETER_SRV(Buffer<half4>, TangentX)
SHADER_PARAMETER_SRV(Buffer<half4>, TangentZ)
SHADER_PARAMETER_SRV(Buffer<float4>, Color)
SHADER_PARAMETER_SRV(Buffer<float>, TexCoords)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

typedef TUniformBufferRef<FCharmUniformBufferParameters> FCharmUniformBufferParametersRef;

/**
 *
 */
class FCharmSceneViewExtension : public FSceneViewExtensionBase
{
public:
    FCharmSceneViewExtension(const FAutoRegister& AutoRegister);

    virtual void PostRenderBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
    void DoWorkLambda(FScene* Scene, FCachedPassMeshDrawListContext& DrawListContext, int32 BatchIndex);
    void RenderStaticMesh(FRHICommandListImmediate& RHICmdList, FSceneView& InView, FStaticMeshSceneProxy* StaticMeshSceneProxy);

    //~ Begin FSceneViewExtensionBase Interface
    virtual int32 GetPriority() const override { return 100; }
    virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override{};
    virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override{};
    virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override{};
    virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override{};
    virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override{};
    virtual void PrePostProcessPass_RenderThread(
        FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override;
    //~ End FSceneViewExtensionBase Interface

public:
};
