// Copyright (C) 2018-2024 FiolaSoft Studio s.r.o.

#include "EditorValidatorSubsystem.h"
#include "ISourceControlWindowsModule.h"
#include "SubmitToolLauncherSettings.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlOperations.h"

#define LOCTEXT_NAMESPACE "FSubmitToolLauncherModule"

DEFINE_LOG_CATEGORY_STATIC(LogSubmitToolLauncher, Log, All)

class FSubmitToolLauncherModule final : public IModuleInterface
{
	virtual void StartupModule() override
	{
		ISourceControlWindowsModule& SourceControlWindowsModule = ISourceControlWindowsModule::Get();

		if (!SourceControlWindowsModule.SubmitOverrideDelegate.IsBound())
		{
			UE_LOG(LogSubmitToolLauncher, Display, TEXT("Registering ISourceControlWindowsModule SubmitOverrideDelegate for SubmitTool to handle submissions from the editor"));
			SourceControlWindowsModule.SubmitOverrideDelegate.BindStatic(&FSubmitToolLauncherModule::OnSubmitOverride);
		}
	}

	virtual void ShutdownModule() override
	{
	}

	/** Evaluates configured submit tool path tokens */
	static bool EvaluateSubmitToolPath(const FString& InPath, OUT FString& ExecutablePath)
	{
		FStringFormatNamedArguments Args;
		Args.Add(TEXT("EngineDir"), FPaths::EngineDir());
		Args.Add(TEXT("ProjectDir"), FPaths::ProjectDir());
		Args.Add(TEXT("RootDir"), FPaths::RootDir());
		Args.Add(TEXT("LocalAppData"), FPlatformMisc::GetEnvironmentVariable(TEXT("LOCALAPPDATA")));

		ExecutablePath = FString::Format(*InPath, Args);

		return IFileManager::Get().FileExists(*ExecutablePath);
	}


	/** Runs pre-submit validation if requested */
	// Pretty much SSourceControlChangelists.cpp GetChangelistValidationResult, but we first look up the changelist by it's identifier
	static bool RunPreSubmitValidation(FString Identifier)
	{
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();

		// Find the changelist by the identifier
		TArray<FSourceControlChangelistRef> Changelists = SourceControlProvider.GetChangelists(EStateCacheUsage::Use);
		const FSourceControlChangelistRef* Changelist = Changelists.FindByPredicate([&Identifier](const FSourceControlChangelistRef& InChangelist)
		{
			return InChangelist->GetIdentifier() == Identifier;
		});

		if (!Changelist)
		{
			return false;
		}

		FSourceControlPreSubmitDataValidationDelegate ValidationDelegate = ISourceControlModule::Get().GetRegisteredPreSubmitDataValidation();

		EDataValidationResult ValidationResult = EDataValidationResult::NotValidated;
		TArray<FText> ValidationErrors;
		TArray<FText> ValidationWarnings;

		if (ValidationDelegate.ExecuteIfBound(*Changelist, ValidationResult, ValidationErrors, ValidationWarnings))
		{
			// NOTE: Logging is already done internally by the delegate
			return ValidationResult == EDataValidationResult::Valid;
		}

		// Changelist is considered valid if we don't have a validation delegate registered
		return true;
	}

	/** Builds a new changelist from a list of files */
	static bool BuildChangelistFromFiles(const FString& InDescription, const TArray<FString>& InFiles, OUT FString& OutIdentifier)
	{
		const TSharedRef<FNewChangelist> NewChangelistOperation = ISourceControlOperation::Create<FNewChangelist>();
		NewChangelistOperation->SetDescription(FText::FromString(InDescription));

		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		const ECommandResult::Type Result = SourceControlProvider.Execute(NewChangelistOperation, InFiles);

		if (Result == ECommandResult::Succeeded)
		{
			if (const FSourceControlChangelistPtr NewChangelist = NewChangelistOperation->GetNewChangelist())
			{
				OutIdentifier = NewChangelist->GetIdentifier();
				return true;
			}
		}

		return false;
	}

	/** Submit override handler */
	static FSubmitOverrideReply OnSubmitOverride(SSubmitOverrideParameters InParameters)
	{
		const USubmitToolLauncherSettings* SubmitToolLauncherSettings = GetDefault<USubmitToolLauncherSettings>();

		// Provider Not Supported continues with default flow
		if (!SubmitToolLauncherSettings->bSubmitToolEnabled)
		{
			return FSubmitOverrideReply::ProviderNotSupported;
		}

		// We only support Perforce
		const ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		if (!SourceControlProvider.IsAvailable() || SourceControlProvider.GetName() != TEXT("Perforce"))
		{
			return FSubmitOverrideReply::ProviderNotSupported;
		}

		// Check if the submit tool is present at the configured path
		FString ExecutablePath;
		if (!EvaluateSubmitToolPath(SubmitToolLauncherSettings->SubmitToolPath, ExecutablePath))
		{
			// TODO: lukas.jech - Consider ProviderNotSupported instead - would return us to default flow rather than hard failing
			FMessageLog("SourceControl").Error(FText::Format(LOCTEXT("SubmitToolNotFound", "SubmitTool executable not found at '{0}'"), FText::FromString(ExecutablePath)));
			return FSubmitOverrideReply::Error;
		}

		FString Identifier = "";

		// Build a new changelist from the files and get the identifier
		if (InParameters.ToSubmit.HasSubtype<TArray<FString>>())
		{
			if (!BuildChangelistFromFiles(InParameters.Description, InParameters.ToSubmit.GetSubtype<TArray<FString>>(), Identifier))
			{
				FMessageLog("SourceControl").Error(LOCTEXT("FailedToCreateChangelist", "Failed to create new changelist"));
				return FSubmitOverrideReply::Error;
			}
		}
		// Use the provided identifier
		else if (InParameters.ToSubmit.HasSubtype<FString>())
		{
			Identifier = InParameters.ToSubmit.GetSubtype<FString>();
		}
		else
		{
			UE_LOG(LogSubmitToolLauncher, Error, TEXT("Invalid ToSubmit subtype"));
			return FSubmitOverrideReply::Error;
		}

		// Run data validation before submitting if enforced - because override hook bypasses built-in validation flows
		if (SubmitToolLauncherSettings->bEnforceDataValidation && !RunPreSubmitValidation(Identifier))
		{
			return FSubmitOverrideReply::Error;
		}

		TMap<ISourceControlProvider::EStatus, FString> PerforceStatus = SourceControlProvider.GetStatus();
		if (PerforceStatus.IsEmpty())
		{
			FMessageLog("SourceControl").Error(LOCTEXT("PerforceStatusEmpty", "Perforce status is empty"));
			return FSubmitOverrideReply::Error;
		}

		const FString* HostAndPort = PerforceStatus.Find(ISourceControlProvider::EStatus::Port);
		const FString* Username = PerforceStatus.Find(ISourceControlProvider::EStatus::User);
		const FString* Client = PerforceStatus.Find(ISourceControlProvider::EStatus::Client);
		const FString& RootDir = FPaths::RootDir();

		FStringFormatNamedArguments Args;
		Args.Add(TEXT("Port"), HostAndPort != nullptr ? *HostAndPort : TEXT("null"));
		Args.Add(TEXT("User"), Username != nullptr ? *Username : TEXT("null"));
		Args.Add(TEXT("Client"), Client != nullptr ? *Client : TEXT("null"));
		Args.Add(TEXT("Changelist"), Identifier);
		Args.Add(TEXT("RootDir"), RootDir);

		const FString Parameters = FString::Format(*SubmitToolLauncherSettings->SubmitToolArguments, Args);

		UE_LOG(LogSubmitToolLauncher, Display, TEXT("Invoking SubmitTool: '%s %s'"), *ExecutablePath, *Parameters);
		const FProcHandle SubmitToolProcess = FPlatformProcess::CreateProc(*ExecutablePath, *Parameters, /* bLaunchDetached */ true, /* bLaunchHidden */ false, /* bLaunchReallyHidden */ false, nullptr, 0, nullptr, nullptr);

		if (SubmitToolProcess.IsValid())
		{
			return FSubmitOverrideReply::Handled;
		}

		FMessageLog("SourceControl").Error(LOCTEXT("SubmitToolFailed", "Failed to launch SubmitTool executable"));
		return FSubmitOverrideReply::Error;
	}
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSubmitToolLauncherModule, SubmitToolLauncher)
