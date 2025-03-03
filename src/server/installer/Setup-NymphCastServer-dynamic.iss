; Inno Setup script for NymphCast Server (dynamic).
;
; Created 31 January 2022.
; Copyright (c) 2021 Nyanko.ws
;

#define MyAppName           "NymphCast Server"
#define MyAppNameNoSpace    "NymphCastServer"
#define MyAppPublisher      "Nyanko"
#define MyAppPublisherURL   "http://www.nyanko.ws/"
#define MyAppContact        "info@nyanko.ws"
#define MyAppCopyright      "Copyright (C) 2019-2022, Nyanko"
#define MyAppURL            "http://nyanko.ws/nymphcast.php"
#define MyAppSupportURL     "https://github.com/MayaPosch/NymphCast/issues"
#define MyAppUpdatesURL     "https://github.com/MayaPosch/NymphCast/releases"
#define MyAppComments       "Audio and video casting system with support for custom applications."

#define MyAppBinFolder      "../bin/x86_64-w64-msvc/release/"

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

; Tasks:

#define NcConfigTaskGroup   "NymphCast Server Configuration"
#define NcAudioConfig       "nymphcast_audio_config.ini"
#define NcVideoConfig       "nymphcast_video_config.ini"
#define NcScrSvrConfig      "nymphcast_screensaver_config.ini"
#define NcGuiConfig         "nymphcast_gui_config.ini"
#define NcDefaultConfig     "nymphcast_configuration.ini"

#define NcConfigAutorunTask "Autorun NymphCast Server with Default Configuration"

#define NcWallPrTaskGroup   "Screen saver Wallpaper Downloads"

#define NcScenicFile        "wallpapers.zip"
#define NcScenicUrl         "https://github.com/MayaPosch/NymphCast/releases/download/v0.1-rc0/" + NcScenicFile
#define NcScenicMsg         "Extracting scenic wallpapers..."
; Note: with non-pre-releases, above URL can be changed to use '.../releases/latest/download/...'.

; Paths for DLLs of dependencies to include:

#define VcpkgRoot            GetEnv('VCPKG_ROOT')
#define VcpkgDllFolder      "installed\x64-windows\bin"

; Paths to download and install VC redistributable if required:

#define VcRedistFile        "vc_redist.x64.exe"
#define VcRedistUrl         "https://aka.ms/vs/17/release/" + VcRedistFile
#define VcRedistMsg         "Installing Microsoft Visual C++ 14.1 RunTime..."

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

AppId              = {{E12E1557-AD80-4CF8-BDCB-B7F441C56539}
AppName            = {#MyAppName}
AppVersion         = {#MyAppVersion}
;AppVerName        = {#MyAppName} {#MyAppVersion}
AppPublisher       = {#MyAppPublisher}
AppPublisherURL    = {#MyAppPublisherURL}
AppSupportURL      = {#MyAppSupportURL}
AppUpdatesURL      = {#MyAppUpdatesURL}
AppCopyright       = {#MyAppCopyright}

DefaultDirName     = {pf}\{#MyAppNameNoSpace}
DefaultGroupName   = {#MyAppNameNoSpace}
OutputBaseFilename = Setup-{#MyAppNameNoSpace}-{#MyAppVersion}-dll

; Show welcome page [, license, pre- and postinstall information]:

DisableWelcomePage = no
;LicenseFile       = {#MyAppLicenseFile}
InfoBeforeFile     = {#MyAppInfoBeforeFile}
InfoAfterFile      = {#MyAppInfoAfterFile}

; Other:

DirExistsWarning    = yes
PrivilegesRequired  = none
Compression         = lzma
SolidCompression    = yes
ChangesAssociations = no
ChangesEnvironment  = SetEnvSdlAudioDriver
; Update of other applications (explorer) needed: for Win7 (onlybelow Win8)

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
Name: "Audio"          ; Description: "Audio profile";       GroupDescription: "{#NcConfigTaskGroup}"; Flags: exclusive
Name: "Video"          ; Description: "Video profile";       GroupDescription: "{#NcConfigTaskGroup}"; Flags: exclusive unchecked
Name: "Screensaver"    ; Description: "Screensaver profile"; GroupDescription: "{#NcConfigTaskGroup}"; Flags: exclusive unchecked
Name: "GUI"            ; Description: "GUI profile";         GroupDescription: "{#NcConfigTaskGroup}"; Flags: exclusive unchecked

Name: "Autorun"        ; Description: "Autorun server";      GroupDescription: "{#NcConfigAutorunTask}"; Flags: unchecked

Name: "Standard"       ; Description: "Standard wallpapers"         ; GroupDescription: "{#NcWallprTaskGroup}"; Flags: exclusive
Name: "Scenic"         ; Description: "Scenic wallpapers (170MByte)"; GroupDescription: "{#NcWallprTaskGroup}"; Flags: exclusive unchecked

Name: "desktopicon"    ; Description: "{cm:CreateDesktopIcon}"    ; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Dirs]

; Program:
Name: "{app}/bin"
Name: "{app}/apps"
Name: "{app}/apps/hellocast"
Name: "{app}/apps/soundcloud"
Name: "{app}/config"
Name: "{app}/assets"
Name: "{app}/wallpapers"

; User data NCS uses {%HOMEPATH} (or {%USERPROFILE}):
Name: "{%HOMEPATH}/.emulationstation"
Name: "{%HOMEPATH}/.emulationstation/collections"
Name: "{%HOMEPATH}/.emulationstation/resources"
Name: "{%HOMEPATH}/.emulationstation/themes"
Name: "{%HOMEPATH}/.emulationstation/tmp"

[Files]
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

; Downloaded items: Visual C++ runtime, wallpapers:
Source: "{tmp}/{#VcRedistFile}"   ; DestDir: "{tmp}"; Flags: external; Check: not VCinstalled

Source: "../apps/*.*"             ; DestDir: "{app}/apps"                   ; Flags: ignoreversion
Source: "../apps/hellocast/*.*"   ; DestDir: "{app}/apps/hellocast"         ; Flags: ignoreversion
Source: "../apps/soundcloud/*.*"  ; DestDir: "{app}/apps/soundcloud"        ; Flags: ignoreversion
Source: "../*.ini"                ; DestDir: "{app}/config"                 ; Flags: ignoreversion
Source: "../assets/*.*"           ; DestDir: "{app}/assets"                 ; Flags: ignoreversion
Source: "../.emulationstation/*.*"; DestDir: "{%HOMEPATH}/.emulationstation"; Flags: ignoreversion recursesubdirs

; Standard wallpapers:
;Source: "../wallpapers/*.*"                ; DestDir: "{app}/wallpapers"; Flags: ignoreversion
Source: "../wallpapers/fetch_wallpapers.sh"; DestDir: "{app}/wallpapers"; Tasks: Standard
Source: "../wallpapers/forest_brook.jpg"   ; DestDir: "{app}/wallpapers"; Tasks: Standard
Source: "../wallpapers/green.jpg"          ; DestDir: "{app}/wallpapers"; Tasks: Standard
Source: "../wallpapers/urls.txt"           ; DestDir: "{app}/wallpapers"; Tasks: Standard

; Setup chosen default configuration file:
Source: "../{#NcAudioConfig}" ; DestDir: "{app}/config"; DestName: "{#NcDefaultConfig}"; Tasks: Audio
Source: "../{#NcVideoConfig}" ; DestDir: "{app}/config"; DestName: "{#NcDefaultConfig}"; Tasks: Video
Source: "../{#NcScrSvrConfig}"; DestDir: "{app}/config"; DestName: "{#NcDefaultConfig}"; Tasks: Screensaver
Source: "../{#NcGuiConfig}"   ; DestDir: "{app}/config"; DestName: "{#NcDefaultConfig}"; Tasks: GUI

; Program and DLLs of dependencies:

Source: "{#MyAppExeSrcPath}"; DestDir: "{app}/bin"; DestName: "{#MyAppExeDestName}"; Flags: ignoreversion

Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/avcodec-61.dll"     ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/avdevice-61.dll"    ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/avfilter-10.dll"     ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/avformat-61.dll"    ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/avutil-59.dll"      ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/brotlicommon.dll"   ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/brotlidec.dll"      ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/bz2.dll"            ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/deflate.dll"        ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/FreeImage.dll"      ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/freetype.dll"       ; DestDir: "{app}/bin"; Flags: ignoreversion
;Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/Half-2_5.dll"       ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/Iex-3_*.dll"        ; DestDir: "{app}/bin"; Flags: ignoreversion
;Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/IlmImf-2_5.dll"     ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/IlmThread-3_*.dll"  ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/Imath-3_1.dll"      ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/jasper.dll"         ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/jpeg62.dll"         ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/lcms2.dll"          ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/libcrypto-3-x64.dll"; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/libcurl.dll"        ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/libexpat.dll"       ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/liblzma.dll"        ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/libpng16.dll"       ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/libsharpyuv.dll"    ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/libssl-3-x64.dll"   ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/libwebp.dll"        ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/libwebpdecoder.dll" ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/libwebpdemux.dll"   ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/libwebpmux.dll"     ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/OpenEXR-3_*.dll"    ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/OpenEXRCore-3_*.dll"; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/openjp2.dll"        ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/pcre2-8.dll"        ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/PocoCrypto.dll"     ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/PocoData.dll"       ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/PocoDataSQLite.dll" ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/PocoFoundation.dll" ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/PocoJSON.dll"       ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/PocoNet.dll"        ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/PocoNetSSL.dll"     ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/PocoUtil.dll"       ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/PocoXML.dll"        ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/postproc-58.dll"    ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/pthreadVC3.dll"     ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/raw.dll"            ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/SDL2.dll"           ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/SDL2_image.dll"     ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/sqlite3.dll"        ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/swresample-5.dll"   ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/swscale-8.dll"      ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/tiff.dll"           ; DestDir: "{app}/bin"; Flags: ignoreversion
Source: "{#VcpkgRoot}/{#VcpkgDllFolder}/zlib1.dll"          ; DestDir: "{app}/bin"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName} - Audio"       ; Filename: "{%COMSPEC}"; Parameters: "/k """"{app}\bin\{#MyAppExeDestName}"" -c ""{app}/config/{#NcAudioConfig}"" -a ""{app}/apps""" ; WorkingDir: "{autodocs}"; Comment: "Run NymphCast Server with audio configuration.";
Name: "{group}\{#MyAppName} - Video"       ; Filename: "{%COMSPEC}"; Parameters: "/k """"{app}\bin\{#MyAppExeDestName}"" -c ""{app}/config/{#NcVideoConfig}"" -a ""{app}/apps""" ; WorkingDir: "{autodocs}"; Comment: "Run NymphCast Server with video configuration.";
Name: "{group}\{#MyAppName} - GUI"         ; Filename: "{%COMSPEC}"; Parameters: "/k """"{app}\bin\{#MyAppExeDestName}"" -c ""{app}/config/{#NcGuiConfig}"" -a ""{app}/apps"" -r ""{app}/assets"""       ; WorkingDir: "{autodocs}"; Comment: "Run NymphCast Server with GUI configuration.";
Name: "{group}\{#MyAppName} - Screen saver"; Filename: "{%COMSPEC}"; Parameters: "/k """"{app}\bin\{#MyAppExeDestName}"" -c ""{app}/config/{#NcScrSvrConfig}"" -a ""{app}/apps"" -w ""{app}/wallpapers"""; WorkingDir: "{autodocs}"; Comment: "Run NymphCast Server with screen saver configuration.";

; Note: backslash required in the following folder path:
Name: "{group}\Reveal config folder in Explorer"; Filename: "{app}\config"
;Name: "{group}\{#NcAudioConfig}"               ; Filename: "{app}/config/{#NcAudioConfig}"
;Name: "{group}\{#NcVideoConfig}"               ; Filename: "{app}/config/{#NcVideoConfig}"
;Name: "{group}\{#NcGuiConfig}"                 ; Filename: "{app}/config/{#NcGuiConfig}"
;Name: "{group}\{#NcScrSvrConfig}"              ; Filename: "{app}/config/{#NcScrSvrConfig}"

; {userstartup}, or {commonstartup}:
Name: "{userstartup}\{#MyAppName} - Default"; Filename: "{%COMSPEC}"; Parameters: "/k """"{app}\bin\{#MyAppExeDestName}"" -c ""{app}/config/{#NcDefaultConfig}"" -a ""{app}/apps"" -r ""{app}/assets"" -w ""{app}/wallpapers"""; WorkingDir: "{autodocs}"; Comment: "Run NymphCast Server with default configuration."; Tasks: Autorun
Name: "{commondesktop}\{#MyAppName}"        ; Filename: "{%COMSPEC}"; Parameters: "/k """"{app}\bin\{#MyAppExeDestName}"" -c ""{app}/config/{#NcDefaultConfig}"" -a ""{app}/apps"" -r ""{app}/assets"" -w ""{app}/wallpapers"""; WorkingDir: "{autodocs}"; Comment: "Run NymphCast Server with default configuration."; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{%COMSPEC}"; Parameters: "/k """"{app}\bin\{#MyAppExeDestName}"" -c ""{app}/config/{#NcDefaultConfig}"" -a ""{app}/apps"" -r ""{app}/assets"" -w ""{app}/wallpapers"""; WorkingDir: "{autodocs}"; Comment: "Run NymphCast Server with default configuration."; Tasks: quicklaunchicon

; Post-install actions:
; - Download and install VC redistributable, if needed.
; - Optionally download and extract wallpapers.
; - Optionally run NymphCast Server with selected default configuration.

[Run]

; If needed, unzip the wallpaper archive to {app}/wallpapers; print.exe figures as a no-op here:
Filename: "print.exe"; AfterInstall: UnzipScenicWallpapers; StatusMsg: "{#NcScenicMsg}"; Tasks: Scenic

; If needed, download (see TDownloadWizardPage) and install the Visual C++ runtime:
Filename: "{tmp}/{#VcRedistFile}"; Parameters: "/install /passive" ; WorkingDir: "{tmp}"; StatusMsg: "{#VcRedistMsg}"; Check: not VCinstalled

; If requested, run NymphCast Server with default configuration:
Filename: "{%COMSPEC}"; Parameters: "/k """"{app}\bin\{#MyAppExeDestName}"" -c ""{app}/config/{#NcDefaultConfig}"" -a ""{app}/apps"" -r ""{app}/assets"" -w ""{app}/wallpapers"""; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

; Code to determine if installation of VC14.1 (VS2017) runtime is needed.
; From: http://stackoverflow.com/questions/11137424/how-to-make-vcredist-x86-reinstall-only-if-not-yet-installed/11172939#11172939
;       https://stackoverflow.com/a/45979466/437272

[UninstallDelete]
Type: files; Name: "{app}/wallpapers/*.jpg" ; Tasks: Scenic
Type: files; Name: "{app}/wallpapers/*.png" ; Tasks: Scenic
Type: files; Name: "{app}/wallpapers/*.jpeg"; Tasks: Scenic

[Registry]
; `Set SDL_AUDIODRIVER=directsound` on Win7:
Check: SetEnvSdlAudioDriver; Root: "HKCU"; Subkey: "Environment"; ValueType: string; ValueName: "SDL_AUDIODRIVER"; ValueData: "directsound"; Flags: preservestringtype

[Code]

function SetEnvSdlAudioDriver: Boolean;
// Return True if below Win8:
begin
  Result := (GetWindowsVersion < $06020000);
end;

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

var
  DownloadPage: TDownloadWizardPage;

function OnDownloadProgress(const Url, FileName: String; const Progress, ProgressMax: Int64): Boolean;
begin
  if Progress = ProgressMax then
    Log(Format('Successfully downloaded file to {tmp}: %s', [FileName]));
  Result := True;
end;

procedure InitializeWizard;
begin
  DownloadPage := CreateDownloadPage(SetupMessage(msgWizardPreparing), SetupMessage(msgPreparingDesc), @OnDownloadProgress);
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  if (CurPageID = wpReady) and ((not VCinstalled) or WizardIsTaskSelected('Scenic')) then begin
    DownloadPage.Clear;
    if not VCinstalled then begin
      DownloadPage.Add('{#VcRedistUrl}', '{#VcRedistFile}', '');
    end;
    if WizardIsTaskSelected('Scenic') then begin
      DownloadPage.Add('{#NcScenicUrl}', '{#NcScenicFile}', '');
    end;
    DownloadPage.Show;
    try
      try
        DownloadPage.Download; // This downloads the files to {tmp}
        Result := True;
      except
        if DownloadPage.AbortedByUser then
          Log('Aborted by user.')
        else
          SuppressibleMsgBox(AddPeriod(GetExceptionMessage), mbCriticalError, MB_OK, IDOK);
        Result := False;
      end;
    finally
      DownloadPage.Hide;
    end;
  end else
    Result := True;
end;

const
  SHCONTCH_NOPROGRESSBOX = 4;
  SHCONTCH_RESPONDYESTOALL = 16;

procedure UnZip(ZipPath, TargetPath: string);
var
  Shell: Variant;
  ZipFile: Variant;
  TargetFolder: Variant;
begin
  Shell := CreateOleObject('Shell.Application');

  ZipFile := Shell.NameSpace(ZipPath);
  if VarIsClear(ZipFile) then
    RaiseException(
      Format('ZIP file "%s" does not exist or cannot be opened', [ZipPath]));

  TargetFolder := Shell.NameSpace(TargetPath);
  if VarIsClear(TargetFolder) then
    RaiseException(Format('Target path "%s" does not exist', [TargetPath]));

  TargetFolder.CopyHere(
    ZipFile.Items, SHCONTCH_NOPROGRESSBOX or SHCONTCH_RESPONDYESTOALL);
end;

procedure UnzipScenicWallpapers;
begin
  Unzip(ExpandConstant('{tmp}') + '\{#NcScenicFile}', ExpandConstant('{app}') + '\wallpapers');
end;

(* End of file *)
