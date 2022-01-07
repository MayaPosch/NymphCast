;NSIS Modern User Interface
;NymphCast Player install script

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"

;--------------------------------
;General

	;Name and file
	Name "NymphCastPlayer"
	OutFile "NymphCast_Player_installer_0.1_RC1.exe"

	;Default installation folder
	InstallDir "$PROGRAMFILES\NymphCastPlayer"

	;Get installation folder from registry if available
	InstallDirRegKey HKLM "Software\NymphCastPlayer" ""

	;Request application privileges for Windows Vista+
	RequestExecutionLevel admin

;--------------------------------
;Interface Settings

	!define MUI_ABORTWARNING

;--------------------------------
;Pages

	;!insertmacro MUI_PAGE_LICENSE "LICENSE"
	;!insertmacro MUI_PAGE_COMPONENTS
	!insertmacro MUI_PAGE_DIRECTORY
	!insertmacro MUI_PAGE_INSTFILES

	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages

	!insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "NymphCastPlayer"
	SetOutPath $INSTDIR

	File "NymphCastPlayer.exe"
	File /r *.dll
	
	; Request access to the 64-bit registry.
	SetRegView 64

	; Store installation folder
	WriteRegStr HKLM "Software\NymphCastPlayer" "" $INSTDIR

	; Create uninstaller
	WriteUninstaller "$INSTDIR\uninstall.exe"
	
	; Register uninstaller
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NymphCastPlayer"	"DisplayName" "NymphCastPlayer" ;The Name shown in the dialog
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NymphCastPlayer" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NymphCastPlayer" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NymphCastPlayer" "Publisher" "Maya Posch/Nyanko"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NymphCastPlayer" "HelpLink" "http://www.mayaposch.com/mqttcute.php"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NymphCastPlayer" "DisplayVersion" "0.3-alpha"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NymphCastPlayer" "NoModify" 1 ; The installers does not offer a possibility to modify the installation
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NymphCastPlayer" "NoRepair" 1 ; The installers does not offer a possibility to repair the installation
  
	; Create a shortcuts in the start menu programs directory.
	CreateDirectory "$SMPROGRAMS\NymphCastPlayer"
    CreateShortCut "$SMPROGRAMS\NymphCastPlayer\NymphCastPlayer.lnk" "$INSTDIR\NymphCastPlayer.exe"
    ;CreateShortCut "$SMPROGRAMS\MQTTCute\Uninstall NymphCastPlayer.lnk" "$INSTDIR\uninstall.exe"
SectionEnd

;--------------------------------
;Descriptions

	;Language strings
	;LangString DESC_SecDummy ${LANG_ENGLISH} "A test section."

	;Assign language strings to sections
	;!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	; !insertmacro MUI_DESCRIPTION_TEXT ${SecDummy} $(DESC_SecDummy)
	;!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "un.Uninstall Section"
	Delete "NymphCastPlayer.exe"
	Delete $INSTDIR\*.dll
	Delete $INSTDIR\iconengines\*.dll
	Delete $INSTDIR\imageformats\*.dll
	Delete $INSTDIR\platforms\*.dll
	Delete $INSTDIR\styles\*.dll
	Delete $INSTDIR\translations\*.qm

	Delete "$INSTDIR\uninstall.exe"

	RMDir /r "$INSTDIR"
	
	; Request access to the 64-bit registry.
	SetRegView 64

	DeleteRegKey /ifempty HKLM "Software\NymphCastPlayer"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NymphCastPlayer"
  
	Delete "$SMPROGRAMS\NymphCastPlayer\NymphCastPlayer.lnk"
    Delete "$SMPROGRAMS\NymphCastPlayer\Uninstall NymphCastPlayer.lnk"

SectionEnd