# SubmitTool Launcher

Simple editor plugin allowing the execution of SubmitTool directly from within Unreal Engine editor. 

This plugin also includes option for running Data Validation before launching SubmitTool even while using the "Submit Content" option which normally doesn't do such checks.

## Configuration

Visual configuration options are provided in `Project Settings -> Editor -> Submit Tool Settings`, configuration is stored inside Editor.ini files.

| Variable | Description |
|---|---|
| SubmitToolPath | Path to the SubmitTool executable |
| SubmitToolArguments | Arguments passed onto the SubmitTool executable when launching |
| bSubmitToolEnabled | Whether or not the SubmitTool is executed, if false regular changelist submission flow is used |
| bEnforceDataValidation | If set to true, content validation is ran before launching SubmitTool |

### SubmitToolPath Tokens

SubmitToolPath variable can contain few dynamically substituted tokens:

| Variable | Description |
|---|---|
| {LocalAppData} | Absolute path to the LocalAppData directory |
| {EngineDir} | Absolute path to the core engine directory |
| {ProjectDir} | Absolute path to the project directory |
| {RootDir} | Absolute path to the engine root directory |

Default value is set up for use with UnrealGameSync. 

It is also recommended to configure this variable per-platform inside the `{Platform}/{Platform}Editor.ini` files - although this plugin has currently been tested only on Windows.

### SubmitToolArguments Tokens

| Variable | Description |
|---|---|
| {Port} | Server and port of the currently used perforce server inside the editor |
| {User} | Currently configured perforce user |
| {Client} | Currently configured perforce workspace |
| {Changelist} | Changelist number currently being submitted |
| {RootDir} | Absolute path to the engine root directory |