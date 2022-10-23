// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CharmTunnel : ModuleRules
{
	public CharmTunnel(ReadOnlyTargetRules Target) : base(Target)
	{
		OptimizeCode = CodeOptimization.Never;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
				"../../../UnrealEngine/Engine/Source/Runtime/Renderer/Public"
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				"../../../../Engine/Source/Runtime/Renderer/Private"
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", "Slate", "EditorScriptingUtilities", "Renderer", "RenderCore",				"RHI",
				"RHICore", "TargetPlatform", "Engine"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"TargetPlatform",
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
				"LevelEditor",
				"AssetRegistry",
				"AssetTools",
				"Json",
				"MaterialEditor",
				"Renderer", 
				"RenderCore",
				"RHI",
				"RHICore",
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
