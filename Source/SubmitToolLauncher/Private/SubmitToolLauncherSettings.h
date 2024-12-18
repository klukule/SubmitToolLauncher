// Copyright (C) 2018-2024 FiolaSoft Studio s.r.o.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SubmitToolLauncherSettings.generated.h"

UCLASS(config=Editor, defaultconfig, meta = (DisplayName = "Submit Tool Settings", ToolTip = "Settings for the submit tool in the editor"))
class SUBMITTOOLLAUNCHER_API USubmitToolLauncherSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(Config, EditAnywhere)
	FString SubmitToolPath;

	UPROPERTY(Config, EditAnywhere)
	FString SubmitToolArguments;

	UPROPERTY(Config, EditAnywhere)
	bool bSubmitToolEnabled;
};
