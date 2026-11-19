; Liberty Music Software BOOM: Windows x64 VST3 installer.
#define ProductVersion GetEnv("BOOM_RELEASE_VERSION")
#define PluginSource GetEnv("BOOM_VST3_BUNDLE")
#define InstallerOutput GetEnv("BOOM_INSTALLER_OUTPUT")

[Setup]
AppId=LibertyMusicSoftware.BOOM.VST3
AppName=BOOM
AppVersion={#ProductVersion}
AppVerName=BOOM {#ProductVersion}
AppPublisher=Liberty Music Software
AppPublisherURL=https://libertymusicsoftware.com
AppSupportURL=https://libertymusicsoftware.com
DefaultDirName={autopf}\Liberty Music Software\BOOM
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
OutputDir={#InstallerOutput}
OutputBaseFilename=BOOM-v{#ProductVersion}-Windows-x64-VST3-Installer-unsigned
Compression=lzma2
SolidCompression=yes
Uninstallable=yes
UsePreviousAppDir=no
SetupLogging=yes

[Files]
Source: "{#PluginSource}\*"; DestDir: "{commoncf64}\VST3\BOOM.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"
