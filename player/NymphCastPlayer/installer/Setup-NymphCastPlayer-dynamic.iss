; Inno Setup script for NymphCast GUI Player.
;
; Created 22 January 2022.
; Copyright (c) 2021 Nyanko.ws
;

#define MyAppName           "NymphCast Player"
#define MyAppNameNoSpace    "NymphCastPlayer"
#define MyAppPublisher      "Nyanko"
#define MyAppPublisherURL   "http://www.nyanko.ws/"
#define MyAppContact        "info@nyanko.ws"
#define MyAppCopyright      "Copyright (C) 2019-2022, Nyanko"
#define MyAppURL            "http://nyanko.ws/nymphcast.php"
#define MyAppSupportURL     "https://github.com/MayaPosch/NymphCast/issues"
#define MyAppUpdatesURL     "https://github.com/MayaPosch/NymphCast/releases"
#define MyAppComments       "Audio and video casting system with support for custom applications."

#define MyAppBinFolder      "../build/x86_64-w64-msvc/release/"

#define MyAppBaseName        MyAppNameNoSpace
#define MyAppExeName         MyAppBaseName + ".exe"
#define MyAppIconName        MyAppBaseName + ".ico"
#define MyAppHomeShortcut    MyAppBaseName + ".url"
#define MyAppExeSrcPath      MyAppBinFolder + MyAppExeName
#define MyAppExeDestName     MyAppNameNoSpace + ".exe"

#define MyAppReadme         "Readme.txt"
#define MyAppChanges        "Changes.txt"
#define MyAppLicenseFile    "Copying.txt"
#define MyAppInfoBeforeFile "InfoBefore.txt"
#define MyAppInfoAfterFile  "InfoAfter.txt"

; Product version string is expected from a definition on
; the iscc commandline like: `-DMyAppVersion="v0.1[.2][-rc0-yyyymmdd]"`.

#ifndef MyAppVersion
#define MyAppVersion         "vx.x.x"
#endif

; Dependencies:

#define NC_CONFIG_VCPKG_QT5  1

; Paths for DLLs of dependencies to include:

#define VcpkgRoot            GetEnv('VCPKG_ROOT')
#define VcpkgDllFolder      "installed\x64-windows\bin"
#define VcpkgPlgFolder      "installed\x64-windows\plugins"

; Paths to download and install VC redistributable if required:

#define VcRedistFile        "vc_redist.x64.exe"
#define VcRedistUrl         "https://aka.ms/vs/17/release/" + VcRedistFile
#define VcRedistMsgDl       "Downloading Microsoft Visual C++ 14.1 RunTime..."
#define VcRedistMsgIn       "Installing Microsoft Visual C++ 14.1 RunTime..."

; Tools 7z, wget Expected in {NymphCast}/tools/
; wget.exe: https://eternallybored.org/misc/wget/
; Expected in {NymphCast}/tools/

#define ToolPath            "../../../tools/"

#define Wget                "wget.exe"
#define WgetPath             ToolPath + Wget

; Workaround for vcpkg Qt5 not ready:

#define Qt5Root             "D:\Libraries\Qt\5.15.2\msvc2015_64"
#define Qt5DllFolder        "bin"
#define Qt5PlgFolder        "plugins"

[Setup]

; Require Windows 7 or newer:

MinVersion = 0,6.1

; The source paths are relative to the directory with this script.
; Place Setup-NymphCast-{version}.exe in directory with this script.
; Note: outputdir is relative to sourcedir:

SourceDir          = ./
OutputDir          = ./

; Application information:

; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)

AppId              = {{F70A7EE2-FDDA-4B1E-B79D-3599126E467A}
AppName            = {#MyAppName}
AppVersion         = {#MyAppVersion}
;AppVerName        = {#MyAppName} {#MyAppVersion}
AppPublisher       = {#MyAppPublisher}
AppPublisherURL    = {#MyAppPublisherURL}
AppSupportURL      = {#MyAppSupportURL}
AppUpdatesURL      = {#MyAppUpdatesURL}
AppCopyright       = {#MyAppCopyright}

DefaultDirName     = {pf}\{#MyAppBaseName}
DefaultGroupName   = {#MyAppBaseName}
OutputBaseFilename = Setup-{#MyAppBaseName}-{#MyAppVersion}-dll

; Show welcome page [, license, pre- and postinstall information]:

DisableWelcomePage  = no
;LicenseFile        = {#MyAppLicenseFile}
;InfoBeforeFile     = {#MyAppInfoBeforeFile}
;InfoAfterFile      = {#MyAppInfoAfterFile}

; Other:

DirExistsWarning    = yes
PrivilegesRequired  = none
Compression         = lzma
SolidCompression    = yes
ChangesAssociations = no
ChangesEnvironment  = no
; No update of other applications (explorer) needed

; Ensure 64-bit install:

ArchitecturesAllowed            = x64
ArchitecturesInstallIn64BitMode = x64

; Cosmetic:

;SetupIconFile        = {#MyAppIconName}
;UninstallDisplayIcon = {#MyAppIconName}
ShowComponentSizes    = yes
;ShowTasksTreeLines   = yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"    ; Description: "{cm:CreateDesktopIcon}"    ; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Dirs]
Name: "{app}/bin"

[Files]
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

; Tools:
Source: "{#WgetPath}"; DestDir: "{tmp}"; Flags: deleteafterinstall;

; Program and DLLs of dependencies:

Source: "{#MyAppExeSrcPath}" ; DestDir: "{app}/bin"; DestName: "{#MyAppExeDestName}"; Flags: ignoreversion

Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/pcre.dll"          ; DestDir: "{app}/bin"   ; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/PocoFoundation.dll"; DestDir: "{app}/bin"   ; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/PocoNet.dll"       ; DestDir: "{app}/bin"   ; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/PocoUtil.dll"      ; DestDir: "{app}/bin"   ; Flags: ignoreversion

Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/brotlicommon.dll"  ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/brotlidec.dll"     ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/bz2.dll"           ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/freetype.dll"      ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/harfbuzz.dll"      ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/icudt69.dll"       ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/icuin69.dll"       ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/icutu69.dll"       ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/icuuc69.dll"       ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/libpng16.dll"      ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/pcre2-16.dll"      ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/zlib1.dll"         ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/zstd.dll"          ; DestDir: "{app}/bin"; Flags: ignoreversion

#if NC_CONFIG_VCPKG_QT5

Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/Qt5Core.dll"       ; DestDir: "{app}/bin"   ; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/Qt5Gui.dll"        ; DestDir: "{app}/bin"   ; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/Qt5Widgets.dll"    ; DestDir: "{app}/bin"   ; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/Qt5Svg.dll"        ; DestDir: "{app}/bin"   ; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgPlgFolder}/platforms/qwindows.dll"; DestDir: "{app}/bin/platforms"; Flags: ignoreversion

#else

Source: "{#Qt5Root}/{#Qt5DllFolder}/Qt5Core.dll"           ; DestDir: "{app}/bin"   ; Flags: ignoreversion
Source: "{#Qt5Root}/{#Qt5DllFolder}/Qt5Gui.dll"            ; DestDir: "{app}/bin"   ; Flags: ignoreversion
Source: "{#Qt5Root}/{#Qt5DllFolder}/Qt5Widgets.dll"        ; DestDir: "{app}/bin"   ; Flags: ignoreversion
Source: "{#Qt5Root}/{#Qt5DllFolder}/Qt5Svg.dll"            ; DestDir: "{app}/bin"   ; Flags: ignoreversion
Source: "{#Qt5Root}/{#Qt5PlgFolder}/platforms/qwindows.dll"; DestDir: "{app}/bin/platforms"; Flags: ignoreversion

#endif

[Icons]
Name: "{group}\{#MyAppName}"        ; Filename: "{app}\bin\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\bin\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\bin\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]

; If needed, download and install the Visual C++ runtime:
Filename: "{tmp}/{#Wget}"        ; Parameters: """{#VcRedistUrl}"""; WorkingDir: "{tmp}"; StatusMsg: "{#VcRedistMsgDl}"; Check: IsWin64 and not VCinstalled
Filename: "{tmp}/{#VcRedistFile}"; Parameters: "/install /passive" ; WorkingDir: "{tmp}"; StatusMsg: "{#VcRedistMsgIn}"; Check: IsWin64 and not VCinstalled

; If requested, run NymphCast Player:
Filename: "{app}\bin\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

; Code to determine if installation of VC14.1 (VS2017) runtime is needed.
; From: http://stackoverflow.com/questions/11137424/how-to-make-vcredist-x86-reinstall-only-if-not-yet-installed/11172939#11172939
;       https://stackoverflow.com/a/45979466/437272

[Code]

function VCinstalled: Boolean;
 // Function for Inno Setup Compiler
 // Returns True if same or later Microsoft Visual C++ 2017 Redistributable is installed, otherwise False.
 var
  major: Cardinal;
  minor: Cardinal;
  bld: Cardinal;
  rbld: Cardinal;
  key: String;
 begin
  Result := False;
  key := 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64';
  if RegQueryDWordValue(HKEY_LOCAL_MACHINE, key, 'Major', major) then begin
    if RegQueryDWordValue(HKEY_LOCAL_MACHINE, key, 'Minor', minor) then begin
      if RegQueryDWordValue(HKEY_LOCAL_MACHINE, key, 'Bld', bld) then begin
        if RegQueryDWordValue(HKEY_LOCAL_MACHINE, key, 'RBld', rbld) then begin
            Log('VC 2017 Redist Major is: ' + IntToStr(major) + ' Minor is: ' + IntToStr(minor) + ' Bld is: ' + IntToStr(bld) + ' Rbld is: ' + IntToStr(rbld));
            // Version info was found. Return true if later or equal to our 14.31.30818.00 redistributable
            // Note brackets required because of weird operator precendence
            Result := (major >= 14) and (minor >= 31) and (bld >= 30818) and (rbld >= 0)
        end;
      end;
    end;
  end;
 end;

(* End of file *)
