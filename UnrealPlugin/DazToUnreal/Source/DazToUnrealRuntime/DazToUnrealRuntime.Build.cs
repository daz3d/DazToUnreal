// Copyright 2018-2019 David Vodhanel. All Rights Reserved.

using UnrealBuildTool;

public class DazToUnrealRuntime : ModuleRules
{
	public DazToUnrealRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
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
				"AnimGraphRuntime",
				// ... add private dependencies that you statically link with here ...	
			}
			);

		PrecompileForTargets = PrecompileTargetsType.Any;
	}
}
