/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "CoreLocale.h"
#include "mac/CFStringToQString.h"
#include <QStringList>
#include <QVariant>


QString
CoreLocale::code() const
{
    switch (m_language)
    {
        case QLocale::English:    return "en";
        case QLocale::French:     return "fr";
        case QLocale::Italian:    return "it";
        case QLocale::German:     return "de";
        case QLocale::Spanish:    return "es";
        case QLocale::Portuguese: return "pt";
        case QLocale::Polish:     return "pl";
        case QLocale::Russian:    return "ru";
        case QLocale::Japanese:   return "jp";
        case QLocale::Chinese:    return "cn";
        case QLocale::Swedish:    return "sv";
        case QLocale::Turkish:    return "tr";
        default:                  return "en";
    }
}


QString
CoreLocale::iso639() const
{
	QString const code = this->code();
    if (code == "jp") return "ja";
    if (code == "cn") return "zh";
	return code;
}


CoreLocale //static
CoreLocale::system()
{
	#ifdef Q_WS_MAC
		//TODO see what Qt's version does
		CFArrayRef languages = (CFArrayRef) CFPreferencesCopyValue( 
			    CFSTR( "AppleLanguages" ),
				kCFPreferencesAnyApplication,
				kCFPreferencesCurrentUser,
				kCFPreferencesAnyHost );
		
		if (languages == NULL)
			return CoreLocale( QLocale::system() );

		CFStringRef uxstylelangs = CFStringCreateByCombiningStrings( kCFAllocatorDefault, languages, CFSTR( ":" ) );

		QString const s = CFStringToQString( uxstylelangs ).split( ':' ).value( 0 );
		return QLocale( s ).language();
	#else
		return QLocale::system().language();
	#endif
}
