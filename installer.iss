; Nav++ - vrai installer Windows
#define MyAppName "Nav++"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Nav++"
#define MyAppExeName "Nav++.exe"

[Setup]
AppId={{8E2A5F1C-3B4A-4C5D-9E6F-7A8B9C0D1E2F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\Nav++
DefaultGroupName={#MyAppName}
OutputDir=C:\Users\Motata\Downloads\navigateur-vs\installer
OutputBaseFilename=Nav++-Setup-1.0.0
Compression=lzma
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayName={#MyAppName}
VersionInfoVersion=1.0.0.0
VersionInfoDescription=Nav++ installer
SetupIconFile=C:\Users\Motata\Downloads\navigateur-vs\src\app.ico
WizardSmallImageFile=C:\Users\Motata\Downloads\navigateur-vs\src\logo.png

[Languages]
Name: "arabic"; MessagesFile: "compiler:Languages\\Arabic.isl"
Name: "armenian"; MessagesFile: "compiler:Languages\\Armenian.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\\BrazilianPortuguese.isl"
Name: "bulgarian"; MessagesFile: "compiler:Languages\\Bulgarian.isl"
Name: "catalan"; MessagesFile: "compiler:Languages\\Catalan.isl"
Name: "corsican"; MessagesFile: "compiler:Languages\\Corsican.isl"
Name: "czech"; MessagesFile: "compiler:Languages\\Czech.isl"
Name: "danish"; MessagesFile: "compiler:Languages\\Danish.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\\Dutch.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "finnish"; MessagesFile: "compiler:Languages\\Finnish.isl"
Name: "french"; MessagesFile: "compiler:Languages\\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\\German.isl"
Name: "hebrew"; MessagesFile: "compiler:Languages\\Hebrew.isl"
Name: "hungarian"; MessagesFile: "compiler:Languages\\Hungarian.isl"
Name: "italian"; MessagesFile: "compiler:Languages\\Italian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\\Japanese.isl"
Name: "korean"; MessagesFile: "compiler:Languages\\Korean.isl"
Name: "norwegian"; MessagesFile: "compiler:Languages\\Norwegian.isl"
Name: "polish"; MessagesFile: "compiler:Languages\\Polish.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\\Russian.isl"
Name: "slovak"; MessagesFile: "compiler:Languages\\Slovak.isl"
Name: "slovenian"; MessagesFile: "compiler:Languages\\Slovenian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\\Spanish.isl"
Name: "swedish"; MessagesFile: "compiler:Languages\\Swedish.isl"
Name: "tamil"; MessagesFile: "compiler:Languages\\Tamil.isl"
Name: "thai"; MessagesFile: "compiler:Languages\\Thai.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\\Turkish.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\\Ukrainian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "C:\Users\Motata\Downloads\navigateur-vs\x64\Release\Nav++.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\Motata\Downloads\navigateur-vs\x64\Release\WebView2Loader.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\Motata\Downloads\navigateur-vs\src\newtab.html"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKLM; Subkey: "Software\Nav++"; ValueType: string; ValueName: "Lang"; ValueData: "{language}"; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Nav++"; ValueType: string; ValueName: "Lang"; ValueData: "{language}"; Flags: uninsdeletevalue

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
function IsWebView2Installed(): Boolean;
var
  Ver: String;
begin
  Result := RegQueryStringValue(HKLM, 'SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Ver)
    or RegQueryStringValue(HKCU, 'SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Ver);
end;

function InitializeSetup(): Boolean;
begin
  Result := True;
  if not IsWebView2Installed() then
    if MsgBox('WebView2 Runtime non detecte. Continuer quand meme ?', mbConfirmation, MB_YESNO) = IDNO then
      Result := False;
end;
