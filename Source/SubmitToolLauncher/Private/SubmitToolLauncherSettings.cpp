// Copyright (C) 2018-2024 FiolaSoft Studio s.r.o.


#include "SubmitToolLauncherSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SubmitToolLauncherSettings)

USubmitToolLauncherSettings::USubmitToolLauncherSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, SubmitToolPath(TEXT("{LocalAppData}/UnrealGameSync/Tools/SubmitTool/Current/Windows/Engine/Binaries/Win64/SubmitTool.exe"))
	, SubmitToolArguments(TEXT("-server {Port} -user {User} -client {Client} -cl {Changelist} -root-dir \\\\\\\"{RootDir}\\\\\\\""))
	, bSubmitToolEnabled(false)
{
}
