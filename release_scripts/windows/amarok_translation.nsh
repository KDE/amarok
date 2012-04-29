!macro AMAROK_TRANSLATIONS
    ;Languages
    ; uncomment supported languages
    !insertmacro MUI_LANGUAGE "English" ;first language is the default language
    !insertmacro MUI_LANGUAGE "French"
    !insertmacro MUI_LANGUAGE "German"
    ; !insertmacro MUI_LANGUAGE "Spanish"
    ; !insertmacro MUI_LANGUAGE "SpanishInternational"
    ; !insertmacro MUI_LANGUAGE "SimpChinese"
    ; !insertmacro MUI_LANGUAGE "TradChinese"
    ; !insertmacro MUI_LANGUAGE "Japanese"
    ; !insertmacro MUI_LANGUAGE "Korean"
    !insertmacro MUI_LANGUAGE "Italian"
    !insertmacro MUI_LANGUAGE "Dutch"
    !insertmacro MUI_LANGUAGE "Danish"
    ; !insertmacro MUI_LANGUAGE "Swedish"
    ; !insertmacro MUI_LANGUAGE "Norwegian"
    ; !insertmacro MUI_LANGUAGE "NorwegianNynorsk"
    !insertmacro MUI_LANGUAGE "Finnish"
    ; !insertmacro MUI_LANGUAGE "Greek"
    ; !insertmacro MUI_LANGUAGE "Russian"
    ; !insertmacro MUI_LANGUAGE "Portuguese"
    ; !insertmacro MUI_LANGUAGE "PortugueseBR"
    ; !insertmacro MUI_LANGUAGE "Polish"
    !insertmacro MUI_LANGUAGE "Ukrainian"
    ; !insertmacro MUI_LANGUAGE "Czech"
    ; !insertmacro MUI_LANGUAGE "Slovak"
    ; !insertmacro MUI_LANGUAGE "Croatian"
    ; !insertmacro MUI_LANGUAGE "Bulgarian"
    !insertmacro MUI_LANGUAGE "Hungarian"
    ; !insertmacro MUI_LANGUAGE "Thai"
    ; !insertmacro MUI_LANGUAGE "Romanian"
    ; !insertmacro MUI_LANGUAGE "Latvian"
    ; !insertmacro MUI_LANGUAGE "Macedonian"
    !insertmacro MUI_LANGUAGE "Estonian"
    ; !insertmacro MUI_LANGUAGE "Turkish"
    ; !insertmacro MUI_LANGUAGE "Lithuanian"
    ; !insertmacro MUI_LANGUAGE "Slovenian"
    ; !insertmacro MUI_LANGUAGE "Serbian"
    ; !insertmacro MUI_LANGUAGE "SerbianLatin"
    ; !insertmacro MUI_LANGUAGE "Arabic"
    ; !insertmacro MUI_LANGUAGE "Farsi"
    !insertmacro MUI_LANGUAGE "Hebrew"
    ; !insertmacro MUI_LANGUAGE "Indonesian"
    ; !insertmacro MUI_LANGUAGE "Mongolian"
    ; !insertmacro MUI_LANGUAGE "Luxembourgish"
    ; !insertmacro MUI_LANGUAGE "Albanian"
    ; !insertmacro MUI_LANGUAGE "Breton"
    ; !insertmacro MUI_LANGUAGE "Belarusian"
    ; !insertmacro MUI_LANGUAGE "Icelandic"
    ; !insertmacro MUI_LANGUAGE "Malay"
    ; !insertmacro MUI_LANGUAGE "Bosnian"
    ; !insertmacro MUI_LANGUAGE "Kurdish"
    ; !insertmacro MUI_LANGUAGE "Irish"
    ; !insertmacro MUI_LANGUAGE "Uzbek"
    !insertmacro MUI_LANGUAGE "Galician"
    ; !insertmacro MUI_LANGUAGE "Afrikaans"
    !insertmacro MUI_LANGUAGE "Catalan"
    ; !insertmacro MUI_LANGUAGE "Esperanto"

    !insertmacro MUI_RESERVEFILE_LANGDLL
      

    ;English
    LangString PAGE_LICENSE_HEADER_TEXT ${LANG_ENGLISH} "License Review"
    LangString PAGE_LICENSE_SUBTEXT ${LANG_ENGLISH} "Please review the license to know your rights before installing Amarok"
    LangString SECTION_SNORE ${LANG_ENGLISH} "Snore (OSD notifications)"
    LangString SECTION_LANGUAGES ${LANG_ENGLISH} "Languages";(internet connection required)
    LangString APPEARANCE_SETTINGS ${LANG_ENGLISH} "Appearance Settings"
    LangString LANGUAGE_SETTINGS ${LANG_ENGLISH} "Language Settings"

    ;German
    LangString PAGE_LICENSE_HEADER_TEXT ${LANG_GERMAN} "Lizenz-Hinweis"
    LangString PAGE_LICENSE_SUBTEXT ${LANG_GERMAN} "Bitte lesen Sie den Lizenz-Text bevor Sie Amarok installieren, um ihre Rechte zu kennen";ouch needs some improvement
    LangString SECTION_SNORE ${LANG_GERMAN} "Snore (OSD notifications)"
    LangString SECTION_LANGUAGES ${LANG_GERMAN} "Sprachen"; (Internet-Verbindung benötigt)"
    LangString APPEARANCE_SETTINGS ${LANG_GERMAN} "Einstellungen zum Erscheinungsbild"
    LangString LANGUAGE_SETTINGS ${LANG_GERMAN} "Einstellungen zur Sprache"

    ;Italian
    LangString PAGE_LICENSE_HEADER_TEXT ${LANG_ITALIAN} "Lettura della licenza"
    LangString PAGE_LICENSE_SUBTEXT ${LANG_ITALIAN} "Leggi la licenza per conoscere i tuoi diritti prima di installare Amarok"
    LangString SECTION_SNORE ${LANG_ITALIAN} "Snore (OSD notifications)"
    LangString SECTION_LANGUAGES ${LANG_ITALIAN} "Lingue"; (richiesta la connessione a Internet)"
    LangString APPEARANCE_SETTINGS ${LANG_ITALIAN} "Impostazioni dell'aspetto"
    LangString LANGUAGE_SETTINGS ${LANG_ITALIAN} "Impostazioni della lingua"
    
    ;Galician
    LangString PAGE_LICENSE_HEADER_TEXT ${LANG_GALICIAN} "Revisión da licenza"
    LangString PAGE_LICENSE_SUBTEXT ${LANG_GALICIAN} "Antes de instalar o Amarok revise a licenza para coñecer os seus dereitos"
    LangString SECTION_SNORE ${LANG_GALICIAN} "Snore (OSD notifications)"
    LangString SECTION_LANGUAGES ${LANG_GALICIAN} "Linguas"; (require de conexión á internet)"
    LangString APPEARANCE_SETTINGS ${LANG_GALICIAN} "Configuranción da aparencia"
    LangString LANGUAGE_SETTINGS ${LANG_GALICIAN} "Configuración da lingua"
    
    ;Hungarian
    LangString PAGE_LICENSE_HEADER_TEXT ${LANG_HUNGARIAN} "Licencáttekintés"
    LangString PAGE_LICENSE_SUBTEXT ${LANG_HUNGARIAN} "Kérjük, hogy olvassa át a licencfeltételeket az Amarok telepítése előtt"
    LangString SECTION_SNORE ${LANG_HUNGARUAN} "Snore (OSD notifications)"
    LangString SECTION_LANGUAGES ${LANG_HUNGARIAN} "Nyelvek"; (internetkapcsolat szükséges)"
    LangString APPEARANCE_SETTINGS ${LANG_HUNGARIAN} "Megjelenési beállítások"
    LangString LANGUAGE_SETTINGS ${LANG_HUNGARIAN} "Nyelvi beállítások"
    
    ;Ukrainian
    LangString PAGE_LICENSE_HEADER_TEXT ${LANG_UKRAINIAN} "Перегляд умов ліцензування"
    LangString PAGE_LICENSE_SUBTEXT ${LANG_UKRAINIAN} "Будь ласка, прочитайте умови ліцензування, щоб ознайомитися зі своїми правами до встановлення Amarok"
    LangString SECTION_SNORE ${LANG_UKRAIN} "Snore (OSD notifications)"
    LangString SECTION_LANGUAGES ${LANG_UKRAINIAN} "Мови"; (потрібне інтернет-з’єднання)"
    LangString APPEARANCE_SETTINGS ${LANG_UKRAINIAN} "Параметри вигляду"
    LangString LANGUAGE_SETTINGS ${LANG_UKRAINIAN} "Параметри мови"
    
    ;Danish
    LangString PAGE_LICENSE_HEADER_TEXT ${LANG_DANISH} "Læs licensen"
    LangString PAGE_LICENSE_SUBTEXT ${LANG_DANISH} "Læs licensen for at kende dine rettigheder før du installerer Amarok"
    LangString SECTION_SNORE ${LANG_DANISH} "Snore (OSD notifications)"
    LangString SECTION_LANGUAGES ${LANG_DANISH} "Sprog"; (internetforbindelse kræves)"
    LangString APPEARANCE_SETTINGS ${LANG_DANISH} "Indstilling af udseende"
    LangString LANGUAGE_SETTINGS ${LANG_DANISH} "Indstilling af sprog"
    
    ;Dutch
    LangString PAGE_LICENSE_HEADER_TEXT ${LANG_DUTCH} "Licentie bekijken"
    LangString PAGE_LICENSE_SUBTEXT ${LANG_DUTCH} "Bekijk aub de licentie om uw rechten en plichten te weten alvorens Amarok te installeren"
    LangString SECTION_SNORE ${LANG_DUTCH} "Snore (OSD notifications)"
    LangString SECTION_LANGUAGES ${LANG_DUTCH} "Talen"; (verbinding met het internet vereist)"
    LangString APPEARANCE_SETTINGS ${LANG_DUTCH} "Instellingen voor het uiterlijk"
    LangString LANGUAGE_SETTINGS ${LANG_DUTCH} "Instellingen voor de taal"
    
    ;French
    LangString PAGE_LICENSE_HEADER_TEXT ${LANG_FRENCH} "Consultation de la licence"
    LangString PAGE_LICENSE_SUBTEXT ${LANG_FRENCH} "Veuillez consulter la licence pour connaître vos droits avant d'installer Amarok"
    LangString SECTION_SNORE ${LANG_FRENCH} "Snore (OSD notifications)"
    LangString SECTION_LANGUAGES ${LANG_FRENCH} "Langues"; (connexion internet requise)"
    LangString APPEARANCE_SETTINGS ${LANG_FRENCH} "Configuration de l'apparence"
    LangString LANGUAGE_SETTINGS ${LANG_FRENCH} "Configuration de la langue"
    
    ;Estonian
    LangString PAGE_LICENSE_HEADER_TEXT ${LANG_ESTONIAN} "Teave litsentsi kohta"
    LangString PAGE_LICENSE_SUBTEXT ${LANG_ESTONIAN} "Palun loe litsents hoolikalt läbi, et teaksid enne Amaroki paigaldamist täpselt, millised on sinu õigused"
    LangString SECTION_SNORE ${LANG_ESTONIA} "Snore (OSD notifications)"
    LangString SECTION_LANGUAGES ${LANG_ESTONIAN} "Keeled"; (vajalik on internetiühendus)"
    LangString APPEARANCE_SETTINGS ${LANG_ESTONIAN} "Välimuse seadistused"
    LangString LANGUAGE_SETTINGS ${LANG_ESTONIAN} "Keeleseadistused"
    
    ;Hebrew
    LangString PAGE_LICENSE_HEADER_TEXT ${LANG_HEBREW} "קרא רישיון"
    LangString PAGE_LICENSE_SUBTEXT ${LANG_HEBREW} "אנא קרא את הרישיון כדי לדעת את זכויותיך לפני ההתקנה של Amarok"
    LangString SECTION_SNORE ${LANG_HEBREW} "Snore (OSD notifications)"
    LangString SECTION_LANGUAGES ${LANG_HEBREW} "שפות" ;"שפות (יש צורך בחיבור אינטרנט)"
    LangString APPEARANCE_SETTINGS ${LANG_HEBREW} "הגדרות מראה"
    LangString LANGUAGE_SETTINGS ${LANG_HEBREW} "הגדרות שפה"
    
    ;Finnish
    LangString PAGE_LICENSE_HEADER_TEXT ${LANG_FINNISH} "Lisenssin luku"
    LangString PAGE_LICENSE_SUBTEXT ${LANG_FINNISH} "Lue lisenssi, jotta tiedät oikeutesi ennen Amarokin asentamista"
    LangString SECTION_SNORE ${LANG_FINISH} "Snore (OSD notifications)"
    LangString SECTION_LANGUAGES ${LANG_FINNISH} "Kielet"; (Internet-yhteys tarvitaan)"
    LangString APPEARANCE_SETTINGS ${LANG_FINNISH} "Ulkoasuasetukset"
    LangString LANGUAGE_SETTINGS ${LANG_FINNISH} "Kieliasetukset"
    
    ;Catalan"
    LangString PAGE_LICENSE_HEADER_TEXT ${LANG_CATALAN} "Revisió de la llicència"
    LangString PAGE_LICENSE_SUBTEXT ${LANG_CATALAN} "Reviseu la llicència per conèixer els vostres drets abans d'instal·lar l'Amarok"
    LangString SECTION_SNORE ${LANG_CATALAN} "Snore (OSD notifications)"
    LangString SECTION_LANGUAGES ${LANG_CATALAN} "Idiomes"; (Es requereix connexió a Internet)"
    LangString APPEARANCE_SETTINGS ${LANG_CATALAN} "Configuració de l'aparença"
    LangString LANGUAGE_SETTINGS ${LANG_CATALAN} "Configuració de l'idioma"

!macroend

