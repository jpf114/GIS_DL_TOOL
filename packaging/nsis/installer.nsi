!define PRODUCT_NAME "GIS AI TOOLKIT"
!define PRODUCT_VERSION "0.1.0"
!define PRODUCT_PUBLISHER "GIS_AI"
!define PRODUCT_WEB_SITE "https://github.com/gis-ai/toolkit"
!define PRODUCT_DIR_REGKEY "Software\GIS_AI\GIS AI TOOLKIT"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_INSTALL_DIR "$PROGRAMFILES64\${PRODUCT_NAME}"

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"

Var StartMenuFolder

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "GIS_AI_TOOLKIT-${PRODUCT_VERSION}-setup.exe"
InstallDir "${PRODUCT_INSTALL_DIR}"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" "InstallDir"
RequestExecutionLevel admin

!define MUI_ICON "..\..\resources\app.ico"
!define MUI_UNICON "..\..\resources\app.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "..\..\resources\installer-welcome.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "..\..\resources\installer-header.bmp"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY

!define MUI_STARTMENUPAGE_DEFAULTFOLDER "${PRODUCT_NAME}"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT HKLM
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_DIR_REGKEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "StartMenuFolder"
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\gis-ai-gui.exe"
!define MUI_FINISHPAGE_RUN_TEXT "$(LaunchApp)"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "English"

LangString LaunchApp ${LANG_SIMPCHINESE} "运行 GIS AI TOOLKIT"
LangString LaunchApp ${LANG_ENGLISH} "Launch GIS AI TOOLKIT"
LangString AppDesc ${LANG_SIMPCHINESE} "GIS AI 遥感智能处理工具箱"
LangString AppDesc ${LANG_ENGLISH} "GIS AI Remote Sensing Intelligent Processing Toolkit"

Section "!$(AppDesc)" SEC_MAIN
    SetOutPath "$INSTDIR"
    SetOverwrite on

    File /r "..\..\install\bin\*.*"
    File /r "..\..\install\share\*.*"

    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${PRODUCT_NAME}.lnk" "$INSTDIR\bin\gis-ai-gui.exe"
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\GIS AI CLI.lnk" "$INSTDIR\bin\gis_ai_cli.exe"
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(UninstallLink).lnk" "$INSTDIR\uninst.exe"
        CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\bin\gis-ai-gui.exe"
    !insertmacro MUI_STARTMENU_WRITE_END

    WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "Version" "${PRODUCT_VERSION}"

    WriteUninstaller "$INSTDIR\uninst.exe"

    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\bin\gis-ai-gui.exe"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair" 1

    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
SectionEnd

Section "Uninstall"
    !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder

    Delete "$SMPROGRAMS\$StartMenuFolder\${PRODUCT_NAME}.lnk"
    Delete "$SMPROGRAMS\$StartMenuFolder\GIS AI CLI.lnk"
    Delete "$SMPROGRAMS\$StartMenuFolder\$(UninstallLink).lnk"
    RMDir "$SMPROGRAMS\$StartMenuFolder"
    Delete "$DESKTOP\${PRODUCT_NAME}.lnk"

    RMDir /r "$INSTDIR\bin"
    RMDir /r "$INSTDIR\share"
    Delete "$INSTDIR\uninst.exe"
    RMDir "$INSTDIR"

    DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"

    SetAutoClose true
SectionEnd

LangString UninstallLink ${LANG_SIMPCHINESE} "卸载"
LangString UninstallLink ${LANG_ENGLISH} "Uninstall"

Function .onInit
    !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd
