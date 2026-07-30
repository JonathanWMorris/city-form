// Copyright Jonathan Morris. All Rights Reserved.

using UnrealBuildTool;

public class CitySimulation : ModuleRules
{
	public CitySimulation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.Add("Core");
	}
}
