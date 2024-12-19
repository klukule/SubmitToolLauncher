// Copyright (C) 2018-2024 FiolaSoft Studio s.r.o.

using UnrealBuildTool;

public class SubmitToolLauncher : ModuleRules
{
	public SubmitToolLauncher(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"SourceControlWindows",
				"SourceControl",
				"DeveloperSettings",
				"DataValidation"
			}
		);
	}
}
