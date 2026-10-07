; Inno Setup script for the SnairCreator Windows installer.
; Built by CI: iscc /DAppVersion=0.0.1 /DSourceDir=<stage> /DOutputDir=<out> packaging\SnairCreator.iss
#ifndef AppVersion
  #define AppVersion "0.0.1"
#endif
#ifndef SourceDir
  #define SourceDir "..\stage"
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif

[Setup]
AppId={{6E0B7C3A-5D7E-4C1F-9C2B-53A1C0D1E201}
AppName=SnairCreator
AppVersion={#AppVersion} beta
AppPublisher=Circuit Drift Labs
AppPublisherURL=https://djshellshoxxx.github.io/circuitdriftlabs/
DefaultDirName={autopf}\Circuit Drift Labs\SnairCreator
DefaultGroupName=Circuit Drift Labs
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename=SnairCreator-v{#AppVersion}-beta-Windows-Installer
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
LicenseFile={#SourceDir}\LICENSE.txt
UninstallDisplayName=SnairCreator {#AppVersion} beta
WizardStyle=modern

[Types]
Name: "full"; Description: "Standalone app, VST3 and CLAP"
Name: "custom"; Description: "Choose components"; Flags: iscustom

[Components]
Name: "standalone"; Description: "SnairCreator standalone application"; Types: full custom
Name: "vst3"; Description: "VST3 plug-in (Common Files\VST3)"; Types: full custom
Name: "clap"; Description: "CLAP plug-in (Common Files\CLAP)"; Types: full custom

[Files]
Source: "{#SourceDir}\SnairCreator.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "{#SourceDir}\README.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme
Source: "{#SourceDir}\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\SnairCreator.vst3\*"; DestDir: "{commoncf64}\VST3\SnairCreator.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\SnairCreator.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap; Flags: ignoreversion

[Icons]
Name: "{group}\SnairCreator"; Filename: "{app}\SnairCreator.exe"; Components: standalone
Name: "{autodesktop}\SnairCreator"; Filename: "{app}\SnairCreator.exe"; Components: standalone; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Components: standalone; Flags: unchecked

[Run]
Filename: "{app}\SnairCreator.exe"; Description: "Launch SnairCreator"; Components: standalone; Flags: nowait postinstall skipifsilent
