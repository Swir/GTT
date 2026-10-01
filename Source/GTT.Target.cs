using UnrealBuildTool;
using System.Collections.Generic;

public class GTTTarget : TargetRules
{
    public GTTTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        bOverrideBuildEnvironment = true;
        bUseLoggingInShipping = true;
        ExtraModuleNames.Add("GTT");
    }
}
