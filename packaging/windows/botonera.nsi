Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
!include "StrFunc.nsh"
${StrStr}
${UnStrStr}

!ifndef VERSION
  !error "VERSION is required"
!endif
!ifndef PAYLOAD
  !error "PAYLOAD is required"
!endif
!ifndef OUTPUT
  !error "OUTPUT is required"
!endif

Name "Botonera per a OBS ${VERSION}"
OutFile "${OUTPUT}"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
ShowInstDetails show
ShowUninstDetails show
VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=1027 "ProductName" "Botonera per a OBS"
VIAddVersionKey /LANG=1027 "FileDescription" "Instal·lador de la botonera per a OBS Studio (x64)"
VIAddVersionKey /LANG=1027 "FileVersion" "${VERSION}"
VIAddVersionKey /LANG=1027 "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=1027 "LegalCopyright" "GPL-2.0-or-later; vegeu LICENSE.txt"

!define REGKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\OBSControlDeck"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TITLE "Botonera per a OBS"
!define MUI_WELCOMEPAGE_TEXT "Instal·la la botonera ${VERSION} per a OBS Studio 33 de 64 bits, a Windows 10/11 x64.$\r$\n$\r$\nTanca OBS abans de continuar. No es modifiquen les escenes ni els perfils.$\r$\n$\r$\nLa integració DSK necessita Downstream Keyer instal·lat per separat. Montserrat és opcional.$\r$\n$\r$\nAquesta compilació de Windows està pendent de la primera prova real en un PC."
!define MUI_FINISHPAGE_TITLE "Botonera instal·lada"
!define MUI_FINISHPAGE_TEXT "Obre OBS i ves a Acobladors (Docks) > Botonera.$\r$\n$\r$\nLa instal·lació queda a ProgramData per a tots els usuaris. Pots desinstal·lar-la des d'Aplicacions instal·lades de Windows.$\r$\n$\r$\nPrimera prova: prepara una escena, executa TRANSICIÓ i prem dues vegades una composició DSK per mostrar-la i amagar-la."
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Catalan"

!macro CHECK_OBS PREFIX SEARCH
Function ${PREFIX}CheckOBSClosed
  check_again:
    nsExec::ExecToStack '"$SYSDIR\tasklist.exe" /FI "IMAGENAME eq obs64.exe" /NH'
    Pop $0
    Pop $1
    ${If} $0 != 0
      MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "No s'ha pogut comprovar si OBS està tancat. Tanca OBS i torna-ho a provar." /SD IDCANCEL IDRETRY check_again
      Abort
    ${EndIf}
    ${${SEARCH}} $2 $1 "obs64.exe"
    ${If} $2 != ""
      MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "OBS està obert. Tanca'l i prem Reintenta. No s'aturarà cap gravació automàticament." /SD IDCANCEL IDRETRY check_again
      Abort
    ${EndIf}
FunctionEnd
!macroend
!insertmacro CHECK_OBS "" StrStr
!insertmacro CHECK_OBS "un." UnStrStr

Function .onInit
  ${IfNot} ${AtLeastWin10}
    MessageBox MB_OK|MB_ICONSTOP "Cal Windows 10 o posterior." /SD IDOK
    Abort
  ${EndIf}
  ${IfNot} ${IsNativeAMD64}
    MessageBox MB_OK|MB_ICONSTOP "Aquest instal·lador és per a PC Intel/AMD de 64 bits (x64)." /SD IDOK
    Abort
  ${EndIf}
  SetRegView 64
  SetShellVarContext all
  ; Fixed destination; /D cannot redirect the uninstaller to an unrelated folder.
  StrCpy $INSTDIR "$APPDATA\obs-studio\plugins\obs-control-deck"
  Call CheckOBSClosed
FunctionEnd

Section "Botonera" Main
  SetOutPath "$INSTDIR"
  File "${PAYLOAD}/obs-control-deck.dll"
  File "${PAYLOAD}/LLEGEIX-ME.txt"
  File "${PAYLOAD}/LICENSE.txt"
  SetOutPath "$INSTDIR\data\locale"
  File "${PAYLOAD}/data/locale/ca-ES.ini"
  File "${PAYLOAD}/data/locale/en-US.ini"
  SetOutPath "$INSTDIR"
  WriteUninstaller "$INSTDIR\Desinstal·lar Botonera.exe"
  WriteRegStr HKLM "${REGKEY}" "DisplayName" "Botonera per a OBS"
  WriteRegStr HKLM "${REGKEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "${REGKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${REGKEY}" "UninstallString" '$\"$INSTDIR\Desinstal·lar Botonera.exe$\"'
  WriteRegStr HKLM "${REGKEY}" "QuietUninstallString" '$\"$INSTDIR\Desinstal·lar Botonera.exe$\" /S'
  WriteRegDWORD HKLM "${REGKEY}" "NoModify" 1
  WriteRegDWORD HKLM "${REGKEY}" "NoRepair" 1
  WriteRegDWORD HKLM "${REGKEY}" "EstimatedSize" ${SIZE_KB}
SectionEnd

Function un.onInit
  SetRegView 64
  SetShellVarContext all
  StrCpy $INSTDIR "$APPDATA\obs-studio\plugins\obs-control-deck"
  Call un.CheckOBSClosed
FunctionEnd

Section "Uninstall"
  ; Remove only owned files. OBS configuration and scene collections are elsewhere.
  Delete "$INSTDIR\obs-control-deck.dll"
  Delete "$INSTDIR\LLEGEIX-ME.txt"
  Delete "$INSTDIR\LICENSE.txt"
  Delete "$INSTDIR\data\locale\ca-ES.ini"
  Delete "$INSTDIR\data\locale\en-US.ini"
  Delete "$INSTDIR\Desinstal·lar Botonera.exe"
  RMDir "$INSTDIR\data\locale"
  RMDir "$INSTDIR\data"
  RMDir "$INSTDIR"
  DeleteRegKey HKLM "${REGKEY}"
SectionEnd
