; MyTel distribution of linphone-desktop. Upstream legal notices remain in the payload.
Unicode true
ManifestDPIAware true
RequestExecutionLevel user
SetCompressor /SOLID lzma
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

Name "MyTel"
OutFile "${MYTEL_OUTPUT}"
InstallDir "$LOCALAPPDATA\Programs\MyTel"
InstallDirRegKey HKCU "Software\MyTel" "InstallDir"
BrandingText "MyTel"
Icon "${MYTEL_ICON}"
UninstallIcon "${MYTEL_ICON}"
VIProductVersion "${MYTEL_VERSION}.0"
VIAddVersionKey /LANG=1033 "ProductName" "MyTel"
VIAddVersionKey /LANG=1033 "CompanyName" "MyTel"
VIAddVersionKey /LANG=1033 "FileDescription" "MyTel Setup"
VIAddVersionKey /LANG=1033 "FileVersion" "${MYTEL_VERSION}"
VIAddVersionKey /LANG=1033 "ProductVersion" "${MYTEL_VERSION}"
VIAddVersionKey /LANG=1033 "LegalCopyright" "MyTel distribution. Includes GPL software by Belledonne Communications and other contributors."

!define MUI_ICON "${MYTEL_ICON}"
!define MUI_UNICON "${MYTEL_ICON}"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${MYTEL_HEADER}"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${MYTEL_WELCOME}"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${MYTEL_WELCOME}"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TITLE "Welcome to MyTel ${MYTEL_VERSION} Setup"
!define MUI_WELCOMEPAGE_TEXT "Install MyTel for your Windows account.$\r$\n$\r$\nClose MyTel before continuing. Your SIP accounts, contacts and history are kept when updating."
!define MUI_FINISHPAGE_RUN "$INSTDIR\mytellphone.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Open MyTel"
!define MUI_UNCONFIRMPAGE_TEXT_TOP "Remove MyTel from this computer? Your accounts, settings, contacts and history will be kept in your Windows profile."
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${MYTEL_LICENSE}"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "English"

Function .onInit
    ${IfNot} ${RunningX64}
        MessageBox MB_ICONSTOP "MyTel requires 64-bit Windows."
        Abort
    ${EndIf}
    SetShellVarContext current
    SetRegView 64
FunctionEnd

!macro CheckMyTelClosed PREFIX
Function ${PREFIX}CheckMyTelClosed
    retry:
    IfFileExists "$INSTDIR\mytellphone.exe" 0 done
    System::Call 'kernel32::CreateFileW(w "$INSTDIR\mytellphone.exe", i 0x40000000, i 0, p 0, i 3, i 0, p 0) p.r0'
    ${If} $0 == -1
        MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Close MyTel before continuing." IDRETRY retry
        Abort
    ${EndIf}
    System::Call 'kernel32::CloseHandle(p r0)'
    done:
FunctionEnd
!macroend
!insertmacro CheckMyTelClosed ""
!insertmacro CheckMyTelClosed "un."

Section "MyTel (required)" SEC_MYTEL
    SectionIn RO
    Call CheckMyTelClosed
    SetOutPath "$INSTDIR"
    !include "${MYTEL_FILES}"
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    CreateDirectory "$SMPROGRAMS\MyTel"
    CreateShortcut "$SMPROGRAMS\MyTel\MyTel.lnk" "$INSTDIR\mytellphone.exe" "" "$INSTDIR\mytellphone.exe" 0
    CreateShortcut "$SMPROGRAMS\MyTel\Uninstall MyTel.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0
    WriteRegStr HKCU "Software\MyTel" "InstallDir" "$INSTDIR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyTel" "DisplayName" "MyTel"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyTel" "DisplayVersion" "${MYTEL_VERSION}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyTel" "Publisher" "MyTel"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyTel" "DisplayIcon" "$INSTDIR\mytellphone.exe,0"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyTel" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyTel" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyTel" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyTel" "URLInfoAbout" "https://github.com/DashSaman/MyTelSoftPhone"
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyTel" "NoModify" 1
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyTel" "NoRepair" 1
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyTel" "EstimatedSize" ${MYTEL_SIZE_KB}
SectionEnd

Section "Desktop shortcut" SEC_DESKTOP
    CreateShortcut "$DESKTOP\MyTel.lnk" "$INSTDIR\mytellphone.exe" "" "$INSTDIR\mytellphone.exe" 0
SectionEnd

Function un.onInit
    SetShellVarContext current
    SetRegView 64
FunctionEnd

Section "Uninstall"
    Call un.CheckMyTelClosed
    !include "${MYTEL_UNINSTALL_FILES}"
    Delete "$DESKTOP\MyTel.lnk"
    Delete "$SMPROGRAMS\MyTel\MyTel.lnk"
    Delete "$SMPROGRAMS\MyTel\Uninstall MyTel.lnk"
    RMDir "$SMPROGRAMS\MyTel"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyTel"
    DeleteRegKey HKCU "Software\MyTel"
SectionEnd
