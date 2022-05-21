// Copyright 2018-2019 David Vodhanel. All Rights Reserved.

using UnrealBuildTool;

public class DazToUnrealDeveloper : ModuleRules
{
	public DazToUnrealDeveloper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"Slate",
				"SlateCore",
				"AnimGraph",
				"AnimGraphRuntime",
				"BlueprintGraph",
				"EditorStyle",
				"DazToUnrealRuntime",
				// ... add private dependencies that you statically link with here ...	
			}
			);
	}
}
