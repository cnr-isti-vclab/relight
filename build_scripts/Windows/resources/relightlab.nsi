!define PRODUCT_NAME "RelightLab"
!define PRODUCT_VERSION "RELIGHT_VERSION"
!define PRODUCT_PUBLISHER "Visual Computing Lab - ISTI - CNR"
!define PRODUCT_WEB_SITE "https://github.com/cnr-isti-vclab/relight"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\relightlab.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define DISTRIB_FOLDER "DISTRIB_PATH"

RequestExecutionLevel admin

!include "MUI2.nsh"

!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"
!define MUI_FINISHPAGE_RUN "$INSTDIR\\relightlab.exe"

SetCompressor /SOLID lzma

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${DISTRIB_FOLDER}\\LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "RelightLab${PRODUCT_VERSION}-windows.exe"
InstallDir "$PROGRAMFILES64\\RelightLab"
ShowInstDetails show
ShowUnInstDetails show

Section "RelightLab" SEC01
    SetShellVarContext all
    SetOverwrite on

    ; Remove leftovers from previous installs
    RMDir /r "$INSTDIR"
    RMDir /r "$SMPROGRAMS\\RelightLab"
    Delete "$DESKTOP\\RelightLab.lnk"

    ; Copy payload
    SetOutPath "$INSTDIR"
    File /nonfatal /a /r "${DISTRIB_FOLDER}\\*"

    ; Shortcuts
    CreateDirectory "$SMPROGRAMS\\RelightLab"
    CreateShortCut "$SMPROGRAMS\\RelightLab\\RelightLab.lnk" "$INSTDIR\\relightlab.exe"
    CreateShortCut "$DESKTOP\\RelightLab.lnk" "$INSTDIR\\relightlab.exe"
SectionEnd

Section "Visual C++ Runtime" SEC_VCRT
    IfFileExists "$INSTDIR\\vc_redist.x64.exe" 0 +2
    ExecWait '"$INSTDIR\\vc_redist.x64.exe" /passive /norestart'
SectionEnd

Section -Post
    WriteUninstaller "$INSTDIR\\uninstall.exe"
    WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\\relightlab.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\\uninstall.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\\uninstall.exe" /S'
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\\relightlab.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
SectionEnd

Function un.onInit
    MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name)?" IDYES +2
    Abort
FunctionEnd

Section Uninstall
    SetShellVarContext all

    Delete "$DESKTOP\\RelightLab.lnk"
    RMDir /r "$SMPROGRAMS\\RelightLab"
    RMDir /r "$INSTDIR"

    DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
SectionEnd

Function un.onUninstSuccess
    HideWindow
    MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd
