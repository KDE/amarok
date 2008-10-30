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

#include "CoreUrl.h"
#include "CoreLocale.h"
#include "CoreSettings.h"
#include <QRegExp>
#include <QStringList>


QString //static
CoreUrl::encode( QString s )
{
    s.replace( "&", "%26" );
    s.replace( "/", "%2F" );
    s.replace( ";", "%3B" );
    s.replace( "+", "%2B" );
    s.replace( "#", "%23" );
    return QString::fromAscii( QUrl::toPercentEncoding( s ) );
}


QString //static
CoreUrl::decode( QString s )
{
    s = QUrl::fromPercentEncoding( s.toAscii() );
    s.replace( "%26", "&" );
    s.replace( "%2F", "/" );
    s.replace( "%3B", ";" );
    s.replace( "%2B", "+" );
    s.replace( "%23", "#" );
    s.replace( '+', ' ' );
    return s;
}


QString //static
CoreUrl::localisedHostName( const CoreLocale& locale )
{
	QString const code = locale.code();
	
    if (code == "en") return "www.last.fm"; //first as optimisation
    if (code == "pt") return "www.lastfm.com.br";
    if (code == "tr") return "www.lastfm.com.tr";
    if (code == "cn") return "cn.last.fm";
    if (code == "sv") return "www.lastfm.se";
	
    QStringList const simple_hosts = QStringList()
		<< "fr" << "it" << "de" << "es" << "pl"
		<< "ru" << "jp" << "se";
	
    if (simple_hosts.contains( code ))
        return "www.lastfm." + code;
	
    // else default to english site
    return "www.last.fm";
}


CoreUrl
CoreUrl::localised() const
{
	CoreUrl url = *this;
	url.setHost( host().replace( QRegExp("^(www.)?last.fm"), localisedHostName( CoreSettings().locale() ) ) );
	return url;
}


CoreUrl
CoreUrl::mobilised() const
{
	CoreUrl url = *this;
	url.setHost( host().replace( QRegExp("^(www.)?last"), "tim.m.dev.last" ) );
	url.setPort( 8090 );
	url.setEncodedQuery( "mobilesafari" );
	return url;
}
