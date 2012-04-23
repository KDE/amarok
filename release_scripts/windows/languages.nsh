
!macro AMAROK_ADD_LANGUAGE_PACKAGE LANG LANG_SUFFIX
    Section /o "${LANG} (${LANG_SUFFIX})" SECTION_LANGUAGES_${LANG_SUFFIX}
        SetOutPath "$INSTDIR"
        NSISdl::download "http://winkde.org/~pvonreth/downloads/l10n/${kde-version}/kde4-l10n-${LANG_SUFFIX}-${kde-version}.7z" "$TEMP\kde4-l10n-${LANG_SUFFIX}-${kde-version}.7z"
        Nsis7z::ExtractWithDetails "$TEMP\kde4-l10n-${LANG_SUFFIX}-${kde-version}.7z" "Installing language ${LANG}..."
        Delete "$TEMP\kde4-l10n-${LANG_SUFFIX}-${kde-version}.7z"
    SectionEnd
!macroend

!macro AMAROK_LANGUAGE_PACKAGES
    SubSection $(SECTION_LANGUAGES) SECTION_LANGUAGES
        Section  "Amarican English (en_US)"
            SectionIn RO
        SectionEnd
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "العربية" ar
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "български" bg
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "bosanski" bs
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "català" ca
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "català" ca@valencia
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "čeština" cs
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "dansk" da
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "Deutsch" de
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "Ελληνικά" el
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "English (United Kingdom)" en_GB
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "español" es
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "eesti" et
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "euskara" eu
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "suomi" fi
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "français" fr
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "Gaeilge" ga
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "galego" gl
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "עברית" he
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "hrvatski" hr
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "magyar" hu
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "interlingua" ia
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "Bahasa Indonesia" id
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "íslenska" is
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "italiano" it
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "日本語" ja
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "Қазақ" kk
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "ភាសាខ្មែរ" km
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "한국어" ko
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "lietuvių" lt
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "latviešu" lv
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "norsk bokmål" nb
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "Plattdüütsch" nds
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "Nederlands" nl
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "nynorsk" nn
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "ਪੰਜਾਬੀ" pa
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "polski" pl
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "português" pt
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "português (Brasil)" pt_BR
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "română" ro
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "русский" ru
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "slovenský" sk
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "slovenščina" sl
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "Српски" sr
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "svenska" sv
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "ไทย" th
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "Türkçe" tr
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "None" ug
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "українська" uk
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "Walon" wa
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "中文 (中国)" zh_CN
        !insertmacro AMAROK_ADD_LANGUAGE_PACKAGE "中文 (台湾)" zh_TW
    SubSectionEnd
!macroend