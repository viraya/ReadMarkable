; ReadMarkable Installer  -  NSIS script
;
; Invoked from package-windows.ps1 after windeployqt has staged Qt DLLs
; alongside readmarkable-installer.exe in installer/build/deploy-win/.
;
; Produces ReadMarkableInstaller-setup.exe at the installer/ root.
;
; Per-user install (no admin required, no HKLM writes). Install dir is
; %LOCALAPPDATA%\Programs\ReadMarkable Installer\.
;
; SmartScreen: the exe is unsigned by design (see installer/README.md
; "Why does Windows warn me?"). sha256 is computed by the caller and
; published on the GitHub Release page so users can verify.

!include "MUI2.nsh"

!define APP_NAME    "ReadMarkable Installer"
!define APP_EXE     "readmarkable-installer.exe"
!define COMPANY     "ReadMarkable"

Name            "${APP_NAME}"
OutFile         "..\ReadMarkableInstaller-setup.exe"
InstallDir      "$LOCALAPPDATA\Programs\${APP_NAME}"
InstallDirRegKey HKCU "Software\${COMPANY}\${APP_NAME}" "InstallDir"
RequestExecutionLevel user
SetCompressor /SOLID lzma

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${APP_NAME}"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "Install"
    SetOutPath "$INSTDIR"
    ; Include everything windeployqt staged under deploy-win/.
    File /r "..\build\deploy-win\*.*"

    WriteRegStr HKCU "Software\${COMPANY}\${APP_NAME}" "InstallDir" "$INSTDIR"
    WriteUninstaller "$INSTDIR\uninstall.exe"

    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortCut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
    CreateShortCut  "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe"

    ; Add/Remove Programs entry
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName"     "${APP_NAME}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher"       "${COMPANY}"
SectionEnd

Section "Uninstall"
    RMDir /r "$INSTDIR"
    RMDir /r "$SMPROGRAMS\${APP_NAME}"
    DeleteRegKey HKCU "Software\${COMPANY}\${APP_NAME}"
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
SectionEnd
