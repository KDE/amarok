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
 
/*
This script requires the Unicode NSIS framework http://www.scratchpaper.com/
You will also need to install http://nsis.sourceforge.net/Nsis7z_plug-in
*/

!macro KDE_ADD_LANGUAGE_PACKAGE LANG LANG_SUFFIX
    Section /o "${LANG} (${LANG_SUFFIX})" SECTION_LANGUAGES_${LANG_SUFFIX}
        SetOutPath "$INSTDIR"
        NSISdl::download "http://winkde.org/~pvonreth/downloads/l10n/${kde-version}/kde4-l10n-${LANG_SUFFIX}-${kde-version}.7z" "$TEMP\kde4-l10n-${LANG_SUFFIX}-${kde-version}.7z"
        Nsis7z::ExtractWithDetails "$TEMP\kde4-l10n-${LANG_SUFFIX}-${kde-version}.7z" "Installing language ${LANG}..."
        Delete "$TEMP\kde4-l10n-${LANG_SUFFIX}-${kde-version}.7z"
    SectionEnd
!macroend

!macro KDE_LANGUAGE_PACKAGES
    SubSection $(SECTION_LANGUAGES) SECTION_LANGUAGES
        Section  "Amarican English (en_US)"
            SectionIn RO
        SectionEnd
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "العربية" ar
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "български" bg
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "bosanski" bs
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "català" ca
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "català" ca@valencia
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "čeština" cs
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "dansk" da
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "Deutsch" de
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "Ελληνικά" el
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "English (United Kingdom)" en_GB
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "español" es
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "eesti" et
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "euskara" eu
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "suomi" fi
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "français" fr
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "Gaeilge" ga
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "galego" gl
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "עברית" he
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "hrvatski" hr
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "magyar" hu
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "interlingua" ia
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "Bahasa Indonesia" id
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "íslenska" is
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "italiano" it
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "日本語" ja
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "Қазақ" kk
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "ភាសាខ្មែរ" km
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "한국어" ko
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "lietuvių" lt
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "latviešu" lv
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "norsk bokmål" nb
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "Plattdüütsch" nds
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "Nederlands" nl
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "nynorsk" nn
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "ਪੰਜਾਬੀ" pa
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "polski" pl
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "português" pt
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "português (Brasil)" pt_BR
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "română" ro
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "русский" ru
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "slovenský" sk
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "slovenščina" sl
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "Српски" sr
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "svenska" sv
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "ไทย" th
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "Türkçe" tr
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "None" ug
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "українська" uk
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "Walon" wa
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "中文 (中国)" zh_CN
        !insertmacro KDE_ADD_LANGUAGE_PACKAGE "中文 (台湾)" zh_TW
    SubSectionEnd
!macroend