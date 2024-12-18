// Copyright (C) 2018-2024 FiolaSoft Studio s.r.o.

#include "ISourceControlWindowsModule.h"
#include "SubmitToolLauncherSettings.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"

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
			SourceControlWindowsModule.SubmitOverrideDelegate.BindRaw(this, &FSubmitToolLauncherModule::OnSubmitOverride);
		}
	}

	virtual void ShutdownModule() override
	{
		// ISourceControlWindowsModule& SourceControlWindowsModule = ISourceControlWindowsModule::Get();
		// SourceControlWindowsModule.SubmitOverrideDelegate.Unbind();
	}

	static bool EvaluateSubmitToolPath(const FString& InPath, OUT FString& ExecutablePath)
	{
		if (InPath.IsEmpty()) return false;

		FStringFormatNamedArguments Args;
		Args.Add(TEXT("EngineDir"), FPaths::EngineDir());
		Args.Add(TEXT("ProjectDir"), FPaths::ProjectDir());
		Args.Add(TEXT("RootDir"), FPaths::RootDir());
		Args.Add(TEXT("LocalAppData"), FPlatformMisc::GetEnvironmentVariable(TEXT("LOCALAPPDATA")));

		ExecutablePath = FString::Format(*InPath, Args);

		return IFileManager::Get().FileExists(*ExecutablePath);
	}

	FSubmitOverrideReply OnSubmitOverride(SSubmitOverrideParameters InParameters) const
	{
		const USubmitToolLauncherSettings* SubmitToolLauncherSettings = GetDefault<USubmitToolLauncherSettings>();

		// Provider Not Supported continues with default flow
		if (!SubmitToolLauncherSettings->bSubmitToolEnabled)
		{
			return FSubmitOverrideReply::ProviderNotSupported;
		}

		// We only handle the case where we get sent CL instead of file list
		// SSourceControlChangelists.cpp:2347	-> HANDLED
		// SourceControlWindows.cpp:436			-> NOT HANDLED
		if (!InParameters.ToSubmit.HasSubtype<FString>())
		{
			return FSubmitOverrideReply::ProviderNotSupported;
		}

		const FString& Identifier = InParameters.ToSubmit.GetSubtype<FString>();

		FString ExecutablePath;
		if (!EvaluateSubmitToolPath(SubmitToolLauncherSettings->SubmitToolPath, ExecutablePath))
		{
			return FSubmitOverrideReply::Error;
		}

		const ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		TMap<ISourceControlProvider::EStatus, FString> PerforceStatus = SourceControlProvider.GetStatus();
		if (PerforceStatus.IsEmpty())
		{
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

		const FProcHandle SubmitToolProcess = FPlatformProcess::CreateProc(*ExecutablePath, *Parameters, /* bLaunchDetached */ true, /* bLaunchHidden */ false, /* bLaunchReallyHidden */ false, nullptr, 0, nullptr, nullptr);

		if (SubmitToolProcess.IsValid())
		{
			return FSubmitOverrideReply::Handled;
		}
		return FSubmitOverrideReply::Error;
	}
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSubmitToolLauncherModule, SubmitToolLauncher)
