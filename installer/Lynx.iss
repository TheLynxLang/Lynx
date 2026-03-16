[Setup]
AppName=Lynx
AppVersion=1.3.3
AppPublisher=justdev-chris
AppPublisherURL=https://github.com/justdev-chris/Lynx
DefaultDirName={pf}\Lynx
DefaultGroupName=Lynx
OutputDir=installer
OutputBaseFilename=Lynx-1.3.3-installer
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "..\lynx.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\std\*.lnx"; DestDir: "{app}\std"; Flags: ignoreversion

[Icons]
Name: "{group}\Lynx"; Filename: "{app}\lynx.exe"
Name: "{group}\Uninstall Lynx"; Filename: "{uninstallexe}"

[Registry]
Root: HKCU; Subkey: "Environment"; ValueType: string; ValueName: "Path"; ValueData: "{app}"; Check: NeedsAddPath('{app}')

[Code]
function NeedsAddPath(Param: string): boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKEY_CURRENT_USER,
    'Environment',
    'Path', OrigPath)
  then begin
    Result := True;
    exit;
  end;
  Result := (Pos(';' + Param + ';', ';' + OrigPath + ';') = 0) and
    (Pos(';' + Param + '\', ';' + OrigPath + '\' ) = 0);
end;

[Run]
Filename: "{app}\lynx.exe"; Description: "Launch Lynx"; Flags: nowait postinstall skipifsilent
