#pragma once
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"

/**
 *
 */

DECLARE_LOG_CATEGORY_EXTERN(LogCTUsfConverter, Log, All);
inline DEFINE_LOG_CATEGORY(LogCTUsfConverter);

#define LOG(x, ...) UE_LOG(LogCTUsfConverter, Log, TEXT(x), __VA_ARGS__)
#define LOG_VERBOSE(x, ...) UE_LOG(LogCTUsfConverter, Verbose, TEXT(x), __VA_ARGS__)
#define LOG_WARNING(x, ...) UE_LOG(LogCTUsfConverter, Warning, TEXT(x), __VA_ARGS__)
#define LOG_ERROR(x, ...) UE_LOG(LogCTUsfConverter, Error, TEXT(x), __VA_ARGS__)

#define T(x) TEXT(x)

struct UsfTexture
{
    FString Dimension;
    FString Type;
    FString Variable;
    int Index;
};

struct UsfConstantBuffer
{
    FString Variable;
    FString Type;
    int Count;
    int Index;
};

struct UsfInput
{
    FString Variable;
    FString Type;
    int Index;
    FString Semantic;
};

struct UsfOutput
{
    FString Variable;
    FString Type;
    int Index;
    FString Semantic;
};

enum EShaderType
{
    PixelShader,
    VertexShader,
};

struct UsfShader
{
    EShaderType Type;
    TArray<UsfTexture> Textures;
    TArray<UsfConstantBuffer> ConstantBuffers;
    TArray<UsfInput> Inputs;
    TArray<UsfOutput> Outputs;
    TArray<int> Samplers;
    bool bHasOpacityMasked;

    FString HlslPath;
    TArray<FString> HlslLines;
    TArray<FString> UsfLines;
    FString UsfContents;

    UsfShader(FString InHlslPath, EShaderType InType) : Type(InType), bHasOpacityMasked(false), HlslPath(InHlslPath) {}
};

struct CT_UsfConverter
{
public:
    static TSharedRef<UsfShader> ConvertFromHlsl(
        TSharedPtr<FJsonObject> MaterialInfo, FString HlslPath, EShaderType ShaderType, bool& bOutSuccess)
    {
        bOutSuccess = false;
        TSharedRef<UsfShader> Shader = MakeShareable(new UsfShader(HlslPath, ShaderType));
        if (!ProcessHlslText(Shader))
        {
            LOG_ERROR("Failed to process hlsl text");
            return Shader;
        }
        if (!WriteConstantBuffers(MaterialInfo, Shader))
        {
            LOG_ERROR("Failed to write constant buffers");
            return Shader;
        }
        WriteFunctionDefinition(Shader);
        if (!ConvertInstructions(Shader))
        {
            LOG_ERROR("Failed to convert HLSL instructions to USF");
            return Shader;
        }
        if (!WriteOutputs(Shader))
        {
            LOG_ERROR("Failed to write outputs");
            return Shader;
        }
        WriteFooter(Shader);

        FString UsfString;
        for (FString Line : Shader->UsfLines)
        {
            UsfString += Line + "\r\n";
        }
        // FString UsfPath = "C:/T/export/test.usf";
        // FFileHelper::SaveStringToFile(UsfString, *UsfPath);
        Shader->UsfContents = UsfString;
        bOutSuccess = true;
        return Shader;
    }

private:
    static bool WriteConstantBuffers(const TSharedPtr<FJsonObject> MaterialInfo, const TSharedRef<UsfShader>& Shader)
    {
        for (auto& ConstantBuffer : Shader->ConstantBuffers)
        {
            Shader->UsfLines.Add(
                FString::Printf(T("static %ls %ls[%d] = \r\n{"), *ConstantBuffer.Type, *ConstantBuffer.Variable, ConstantBuffer.Count));
            if (MaterialInfo->GetObjectField("ConstantBuffers")->HasField(FString::FromInt(ConstantBuffer.Count)))
            {
                const TArray<TSharedPtr<FJsonValue>>& Data =
                    MaterialInfo->GetObjectField("ConstantBuffers")->GetArrayField(FString::FromInt(ConstantBuffer.Count));
                for (auto JsonValue : Data)
                {
                    Shader->UsfLines.Add(FString::Printf(T("float4(%f, %f, %f, %f),"), JsonValue->AsObject()->GetNumberField("X"),
                        JsonValue->AsObject()->GetNumberField("Y"), JsonValue->AsObject()->GetNumberField("Z"),
                        JsonValue->AsObject()->GetNumberField("W")));
                }
            }
            else
            {
                // Doesn't exist, fill in with garbage for now
                for (int i = 0; i < ConstantBuffer.Count; i++)
                {
                    Shader->UsfLines.Add(FString::Printf(T("float4(1, 1, 1, 1),")));
                }
            }

            Shader->UsfLines.Add("};\r\n");
        }

        return true;
    }

    static bool ProcessHlslText(const TSharedRef<UsfShader>& Shader)
    {
        FString HlslText;
        if (!FFileHelper::LoadFileToString(HlslText, *Shader->HlslPath))
        {
            LOG_ERROR("Failed to load hlsl file %s.", *Shader->HlslPath);
            return false;
        }

        int32 LineCount = HlslText.ParseIntoArray(Shader->HlslLines, TEXT("\n"), true);
        bool bFindOpacity = false;
        int i = 0;
        while (++i < LineCount)
        {
            FString Line = Shader->HlslLines[i];

            if (Line.Contains("r0,r1"))    // at end of function definition
            {
                bFindOpacity = true;
            }

            if (bFindOpacity)
            {
                if (Line.Contains("discard"))
                {
                    Shader->bHasOpacityMasked = true;
                    break;
                }
                continue;
            }
            if (Line.Contains("Texture"))
            {
                UsfTexture Texture;
                Line.Split(T("<"), &Texture.Dimension, nullptr);

                Line.Split(T("<"), nullptr, &Texture.Type);
                Texture.Type.Split(T(">"), &Texture.Type, nullptr);

                Line.Split(T("> "), nullptr, &Texture.Variable);
                Texture.Variable.Split(T(" : "), &Texture.Variable, nullptr);

                Texture.Index = FCString::Atoi(*Texture.Variable.RightChop(1));

                Shader->Textures.Add(Texture);
            }
            else if (Line.Contains("SamplerState"))
            {
                FString TempState;
                Line.Split(T("("), nullptr, &TempState);
                TempState.Split(T(")"), &TempState, nullptr);
                Shader->Samplers.Add(FCString::Atoi(*TempState.Right(1)));
            }
            else if (Line.Contains("cbuffer"))
            {
                ++i;
                Line = Shader->HlslLines[++i];
                UsfConstantBuffer ConstantBuffer;

                Line.Split(T("["), &ConstantBuffer.Variable, nullptr);
                ConstantBuffer.Variable.Split(T(" cb"), nullptr, &ConstantBuffer.Variable);
                ConstantBuffer.Variable = "cb" + ConstantBuffer.Variable;

                ConstantBuffer.Index = FCString::Atoi(*ConstantBuffer.Variable.RightChop(2));

                FString TempCount;
                Line.Split(T("["), nullptr, &TempCount);
                TempCount.Split(T("]"), &TempCount, nullptr);
                ConstantBuffer.Count = FCString::Atoi(*TempCount);

                Line.Split("cb", &ConstantBuffer.Type, nullptr);
                ConstantBuffer.Type = ConstantBuffer.Type.TrimStartAndEnd();

                Shader->ConstantBuffers.Add(ConstantBuffer);
            }
            else if (Line.Contains(" v") && Line.Contains(" : ") && !Line.Contains("?"))
            {
                UsfInput Input;

                Line.Split("v", nullptr, &Input.Variable);
                Input.Variable.Split(" : ", &Input.Variable, nullptr);
                Input.Variable = "v" + Input.Variable;

                Input.Index = FCString::Atoi(*Input.Variable.RightChop(1));

                Line.Split(" : ", nullptr, &Input.Semantic);
                Input.Semantic.Split(",", &Input.Semantic, nullptr);

                Line.Split(" v", &Input.Type, nullptr);
                Input.Type = Input.Type.TrimStartAndEnd();

                Shader->Inputs.Add(Input);
            }
            else if (Line.Contains("out") && Line.Contains(" : "))
            {
                UsfOutput Output;

                FString TempOutput;
                Line.Split("out ", nullptr, &TempOutput);

                TempOutput.Split(" o", nullptr, &Output.Variable);
                Output.Variable.Split(" : ", &Output.Variable, nullptr);
                Output.Variable = "o" + Output.Variable;

                Output.Index = FCString::Atoi(*Output.Variable.RightChop(1));

                Line.Split(" : ", nullptr, &Output.Semantic);
                Output.Semantic = Output.Semantic.LeftChop(1);

                TempOutput.Split(" o", &Output.Type, nullptr);

                Shader->Outputs.Add(Output);
            }
        }
        return true;
    }

    static bool WriteFunctionDefinition(const TSharedRef<UsfShader>& Shader)
    {
        if (Shader->Type == PixelShader)
        {
            for (auto& Input : Shader->Inputs)
            {
                if (Input.Type == "float4")
                {
                    Shader->UsfLines.Add(FString::Printf(T("static %ls %ls = {1, 1, 1, 1};"), *Input.Type, *Input.Variable));
                }
                else if (Input.Type == "float3")
                {
                    Shader->UsfLines.Add(FString::Printf(T("static %ls %ls = {1, 1, 1};"), *Input.Type, *Input.Variable));
                }
                else if (Input.Type == "uint")
                {
                    Shader->UsfLines.Add(FString::Printf(T("static %ls %ls = 1;"), *Input.Type, *Input.Variable));
                }
            }
        }
        Shader->UsfLines.Add("#define cmp -");
        Shader->UsfLines.Add("struct shader {");
        if (Shader->Type == VertexShader)
        {
            for (auto& Output : Shader->Outputs)
            {
                Shader->UsfLines.Add(FString::Printf(T("%ls %ls;"), *Output.Type, *Output.Variable));
            }
            Shader->UsfLines.Add("");
            Shader->UsfLines.Add("void main(");
            for (auto& Texture : Shader->Textures)
            {
                Shader->UsfLines.Add(FString::Printf(T("   %ls %ls;"), *Texture.Type, *Texture.Variable));
            }

            for (int i = 0; i < Shader->Inputs.Num(); i++)
            {
                auto& Input = Shader->Inputs[i];
                if (i != Shader->Inputs.Num() - 1)
                {
                    Shader->UsfLines.Add(FString::Printf(T("   %ls %ls, // %ls"), *Input.Type, *Input.Variable, *Input.Semantic));
                }
                else
                {
                    Shader->UsfLines.Add(FString::Printf(T("   %ls %ls) // %ls"), *Input.Type, *Input.Variable, *Input.Semantic));
                }
            }
        }
        else
        {
            Shader->UsfLines.Add("FMaterialAttributes main(");

            for (auto& Texture : Shader->Textures)
            {
                Shader->UsfLines.Add(FString::Printf(T("   %ls %ls,"), *Texture.Type, *Texture.Variable));
            }

            Shader->UsfLines.Add("   float2 tx)");

            Shader->UsfLines.Add("{");
            Shader->UsfLines.Add("  FMaterialAttributes output;");
            // Output render targets, todo support vertex shader, todo account for non-3 component outputs (error)
            Shader->UsfLines.Add("  float4 o0,o1,o2;");
            for (auto& Input : Shader->Inputs)
            {
                if (Input.Type == "float4")
                {
                    Shader->UsfLines.Add(FString::Printf(T("  %ls.xyzw = %ls.xyzw * tx.xyxy;"), *Input.Variable, *Input.Variable));
                }
                else if (Input.Type == "float3")
                {
                    Shader->UsfLines.Add(FString::Printf(T("  %ls.xyz = %ls.xyz * tx.xyx;"), *Input.Variable, *Input.Variable));
                }
                else if (Input.Type == "uint")
                {
                    Shader->UsfLines.Add(FString::Printf(T("  %ls.x = %ls.x * tx.x;"), *Input.Variable, *Input.Variable));
                }
                // usf.Replace("v0.xyzw = v0.xyzw * tx.xyxy;", "v0.xyzw = v0.xyzw;");
            }
        }
        return true;
    }

    static bool ConvertInstructions(const TSharedRef<UsfShader>& Shader)
    {
        TMap<int, UsfTexture> TextureMap;
        for (auto& Texture : Shader->Textures)
        {
            TextureMap.Add(Texture.Index, Texture);
        }
        TArray<int> SortedIndices;
        TextureMap.GetKeys(SortedIndices);
        SortedIndices.Sort();

        int i = 0;
        bool bPastHeader = false;
        while (++i < Shader->HlslLines.Num())
        {
            FString Line = Shader->HlslLines[i];
            if (!bPastHeader)
            {
                if (Line.Contains("float4 r0,r1"))
                {
                    bPastHeader = true;
                }
                else
                {
                    continue;
                }
            }

            if (Line.Contains("return;"))
            {
                break;
            }

            // Replace texture samples todo add dimension
            if (Line.Contains("Sample"))
            {
                FString TempString;

                FString EqualLeft;
                Line.Split("=", &EqualLeft, nullptr);

                Line.Split(".Sample", &TempString, nullptr);
                TempString.Split("t", nullptr, &TempString);
                int TextureIndex = FCString::Atoi(*TempString);

                Line.Split("(s", nullptr, &TempString);
                TempString.Split("_s", &TempString, nullptr);
                int SampleIndex = FCString::Atoi(*TempString);

                Line.Split(", ", nullptr, &TempString);
                FString SampleTx;
                TempString.Split(")", &SampleTx, nullptr);

                FString DotAfter;
                Line.Split(").", nullptr, &DotAfter);

                Shader->UsfLines.Add(FString::Printf(T("%ls= Material_Texture2D_%d.SampleLevel(Material_Texture2D_%dSampler, %ls,0).%ls"),
                    *EqualLeft, SortedIndices.IndexOfByKey(TextureIndex), SampleIndex - 1, *SampleTx, *DotAfter));
            }
            // Replace discard
            else if (Line.Contains("discard"))
            {
                Shader->UsfLines.Add(Line.Replace(T("discard"), T("{ output.OpacityMask = 0; return output; }")));
            }
            else
            {
                Shader->UsfLines.Add(Line);
            }

            // todo add load, levelofdetail
        }

        return true;
    }

    static bool WriteOutputs(const TSharedRef<UsfShader>& Shader)
    {
        FString OutputString;
        FString UsfRTOutputConversion = FPaths::ProjectPluginsDir() / T("CharmTunnel/Resources/UsfRTOutputConversion.usf");
        if (!FFileHelper::LoadFileToString(OutputString, *UsfRTOutputConversion))
        {
            LOG_ERROR("Failed to load output conversion file");
            return false;
        }
        TArray<FString> OutputLines;
        OutputString.ParseIntoArray(OutputLines, TEXT("\r\n"), true);
        for (auto& OutputLine : OutputLines)
        {
            Shader->UsfLines.Add("  " + OutputLine);
        }
        return true;
    }

    static void WriteFooter(const TSharedRef<UsfShader>& Shader)
    {
        Shader->UsfLines.Add("}");
        Shader->UsfLines.Add("};");
        if (Shader->Type == PixelShader)
        {
            Shader->UsfLines.Add("shader s;");
            FString OutputString = "return s.main(";
            for (auto& Texture : Shader->Textures)
            {
                OutputString += Texture.Variable + ",";
            }
            OutputString += "tx);";
            Shader->UsfLines.Add(OutputString);
        }
    }
};
