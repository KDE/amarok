 /****************************************************************************************
 * Copyright (c) 2012 Patrick von Reth <vonreth@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

; registry stuff
!define regkey "Software\${company}\Amarok"
!define uninstkey "Software\Microsoft\Windows\CurrentVersion\Uninstall\Amarok"
 
!define startmenu "$SMPROGRAMS\Amarok"
!define uninstaller "uninstall.exe"
 
 Var StartMenuFolder
 
!define MUI_LANGDLL_ALLLANGUAGES
!define MUI_ICON "amarok.ico"
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\amarok.exe"
;save language
!define MUI_LANGDLL_REGISTRY_ROOT "HKLM" 
!define MUI_LANGDLL_REGISTRY_KEY "${regkey}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;Start Menu Folder Page Configuration
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${regkey}" 
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
;--------------------------------
 
XPStyle on
ShowInstDetails hide
ShowUninstDetails hide

SetCompressor /SOLID lzma


Name "${productname}"
Caption "${productname}"
 
OutFile "${setupname}"
 
!include "MUI2.nsh"

SetDateSave on
SetDatablockOptimize on
CRCCheck on
SilentInstall normal
 
InstallDir "$PROGRAMFILES\Amarok"
InstallDirRegKey HKLM "${regkey}" ""
 
;--------------------------------
 
AutoCloseWindow false
ShowInstDetails hide


!insertmacro MUI_PAGE_WELCOME
!define MUI_PAGE_HEADER_TEXT $(PAGE_LICENSE_HEADER_TEXT)
!define MUI_PAGE_HEADER_SUBTEXT $(PAGE_LICENSE_SUBTEXT)
!define MUI_LICENSEPAGE_TEXT_BOTTOM " "
!define MUI_LICENSEPAGE_BUTTON $(PAGE_LICENSE_BUTTON_TEXT)
!insertmacro MUI_PAGE_LICENSE "..\..\COPYING"
!insertmacro MUI_PAGE_DIRECTORY 
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH


!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
 
 

Section "Amarok" SECTION_AMAROK
  SectionIn RO
  ExecWait '"$INSTDIR\bin\kdeinit4.exe" "--shutdown"'
  WriteRegStr HKLM "${regkey}" "Install_Dir" "$INSTDIR"
  ; write uninstall strings
 
  SetOutPath $INSTDIR
 
 
; package all files, recursively, preserving attributes
; assume files are in the correct places

File /a /r /x "*.nsi" /x "${setupname}" "${srcdir}\*.*" 

WriteUninstaller "${uninstaller}"
  
    ;Create shortcuts

!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    SetShellVarContext all
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Amarok.lnk" "$INSTDIR\bin\Amarok.exe"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Appearance Settings.lnk" "$INSTDIR\bin\kcmshell4.exe" "style" "$INSTDIR\bin\systemsettings.exe"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Snorenotify.lnk" "$INSTDIR\bin\snorenotify.exe"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\uninstall.exe"      
!insertmacro MUI_STARTMENU_WRITE_END

SetOutPath "$INSTDIR"
ExecWait '"$INSTDIR\bin\update-mime-database.exe" "$INSTDIR\share\mime"'
ExecWait '"$INSTDIR\bin\kbuildsycoca4.exe" "--noincremental"'

WriteRegStr HKLM "${uninstkey}" "DisplayName" "Amarok (remove only)"
WriteRegStr HKLM "${uninstkey}" "UninstallString" '"$INSTDIR\${uninstaller}"'
SectionEnd





!macro AMAROK_ADD_LANGUAGE_PACKAGE LANG_SUFFIX
    Section /o "$(SECTION_LANGUAGES_${LANG_SUFFIX}) (${LANG_SUFFIX})" SECTION_LANGUAGES_${LANG_SUFFIX}
        SetOutPath "$INSTDIR"
        NSISdl::download "http://winkde.org/~pvonreth/downloads/l10n/${kde-version}/kde4-l10n-${LANG_SUFFIX}-${kde-version}.7z" "$TEMP\kde4-l10n-${LANG_SUFFIX}-${kde-version}.7z"
        Nsis7z::Extract "$TEMP\kde4-l10n-${LANG_SUFFIX}-${kde-version}.7z" 
        Delete "$TEMP\kde4-l10n-${LANG_SUFFIX}-${kde-version}.7z"
    SectionEnd
!macroend

SubSection $(SECTION_LANGUAGES) SECTION_LANGUAGES
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  ar
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  bg
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  bs
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  ca
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  cs
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  da
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  de
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  el
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  en_GB
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  es
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  et
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  eu
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  fi
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  fr
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  ga
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  gl
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  he
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  hr
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  hu
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  ia
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  id
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  is
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  it
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  ja
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  kk
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  km
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  ko
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  lt
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  lv
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  nb
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  nds
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  nl
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  nn
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  pa
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  pl
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  pt
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  pt_BR
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  ro
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  ru
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  sk
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  sl
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  sr
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  sv
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  th
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  tr
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  ug
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  uk
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  wa
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  zh_CN
    !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE  zh_TW
SubSectionEnd



 
; Uninstaller
; All section names prefixed by "Un" will be in the uninstaller
 
 
Section "Uninstall"
SetShellVarContext all
ExecWait '"$INSTDIR\bin\kdeinit4.exe" "--shutdown"'

DeleteRegKey HKLM "${uninstkey}"
DeleteRegKey HKLM "${regkey}"

!insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder

RMDir /r "$SMPROGRAMS\$StartMenuFolder"
RMDir /r "$INSTDIR"

SectionEnd




!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_AMAROK} $(DESC_SECTION_AMAROK)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES} $(DESC_SECTION_LANGUAGES)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_ar} $(DESC_SECTION_LANGUAGES_ar)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_bg} $(DESC_SECTION_LANGUAGES_bg)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_bs} $(DESC_SECTION_LANGUAGES_bs)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_ca} $(DESC_SECTION_LANGUAGES_ca)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_cs} $(DESC_SECTION_LANGUAGES_cs)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_da} $(DESC_SECTION_LANGUAGES_da)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_de} $(DESC_SECTION_LANGUAGES_de)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_el} $(DESC_SECTION_LANGUAGES_el)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_en_GB} $(DESC_SECTION_LANGUAGES_en_GB)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_es} $(DESC_SECTION_LANGUAGES_es)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_et} $(DESC_SECTION_LANGUAGES_et)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_eu} $(DESC_SECTION_LANGUAGES_eu)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_fi} $(DESC_SECTION_LANGUAGES_fi)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_fr} $(DESC_SECTION_LANGUAGES_fr)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_ga} $(DESC_SECTION_LANGUAGES_ga)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_gl} $(DESC_SECTION_LANGUAGES_gl)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_he} $(DESC_SECTION_LANGUAGES_he)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_hr} $(DESC_SECTION_LANGUAGES_hr)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_hu} $(DESC_SECTION_LANGUAGES_hu)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_ia} $(DESC_SECTION_LANGUAGES_ia)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_id} $(DESC_SECTION_LANGUAGES_id)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_is} $(DESC_SECTION_LANGUAGES_is)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_it} $(DESC_SECTION_LANGUAGES_it)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_ja} $(DESC_SECTION_LANGUAGES_ja)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_kk} $(DESC_SECTION_LANGUAGES_kk)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_km} $(DESC_SECTION_LANGUAGES_km)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_ko} $(DESC_SECTION_LANGUAGES_ko)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_lt} $(DESC_SECTION_LANGUAGES_lt)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_lv} $(DESC_SECTION_LANGUAGES_lv)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_nb} $(DESC_SECTION_LANGUAGES_nb)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_nds} $(DESC_SECTION_LANGUAGES_nds)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_nl} $(DESC_SECTION_LANGUAGES_nl)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_nn} $(DESC_SECTION_LANGUAGES_nn)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_pa} $(DESC_SECTION_LANGUAGES_pa)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_pl} $(DESC_SECTION_LANGUAGES_pl)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_pt} $(DESC_SECTION_LANGUAGES_pt)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_pt_BR} $(DESC_SECTION_LANGUAGES_pt_BR)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_ro} $(DESC_SECTION_LANGUAGES_ro)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_ru} $(DESC_SECTION_LANGUAGES_ru)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_sk} $(DESC_SECTION_LANGUAGES_sk)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_sl} $(DESC_SECTION_LANGUAGES_sl)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_sr} $(DESC_SECTION_LANGUAGES_sr)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_sv} $(DESC_SECTION_LANGUAGES_sv)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_th} $(DESC_SECTION_LANGUAGES_th)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_tr} $(DESC_SECTION_LANGUAGES_tr)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_ug} $(DESC_SECTION_LANGUAGES_ug)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_uk} $(DESC_SECTION_LANGUAGES_uk)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_wa} $(DESC_SECTION_LANGUAGES_wa)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_zh_CN} $(DESC_SECTION_LANGUAGES_zh_CN)
    !insertmacro MUI_DESCRIPTION_TEXT ${SECTION_LANGUAGES_zh_TW} $(DESC_SECTION_LANGUAGES_zh_TW)  
!insertmacro MUI_FUNCTION_DESCRIPTION_END



  ;initialize the translations
!include "amarok_translation.nsh"


;installer Fcuntion
Function .onInit

  !insertmacro MUI_LANGDLL_DISPLAY

FunctionEnd

; Uninstaller Functions

Function un.onInit

  !insertmacro MUI_UNGETLANGUAGE
  
FunctionEnd

  
 



