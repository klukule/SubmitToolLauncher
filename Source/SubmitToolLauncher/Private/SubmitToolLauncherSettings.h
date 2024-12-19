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
	/** Path to the submit tool executable
	 * - Available tokens:
	 *	  {LocalAppData} - Path to LocalAppData directory
	 *	  {EngineDir}    - Path to the engine directory
	 *	  {ProjectDir}   - Path to the project directory
	 *	  {RootDir}      - Path to the workspace root directory
	 */
	UPROPERTY(Config, EditAnywhere)
	FString SubmitToolPath;

	/** Arguments to pass to the submit tool
	 * - Available tokens:
	 *	  {Port}       - Configured perforce server
	 *	  {User}       - Configured perforce user
	 *	  {Client}     - Configured perforce workspace
	 *	  {Changelist} - Changelist number to submit
	 *	  {RootDir}    - Root directory of the workspace
	 */
	UPROPERTY(Config, EditAnywhere)
	FString SubmitToolArguments;

	/** Enables the submit tool override hook */
	UPROPERTY(Config, EditAnywhere)
	bool bSubmitToolEnabled;

	/** Forces data validation to run before calling the submit tool - only executes if submit tool hook is enabled */
	UPROPERTY(Config, EditAnywhere)
	bool bEnforceDataValidation;
};
