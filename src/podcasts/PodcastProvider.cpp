/****************************************************************************************
 * Copyright (c) 2009 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "PodcastProvider.h"

#include "core/support/Debug.h"

#include <KUrl>

bool
PodcastProvider::couldBeFeed( const QString &urlString )
{
    DEBUG_BLOCK

    QStringList feedProtocols;
    feedProtocols << "itpc";
    feedProtocols << "pcast";
    feedProtocols << "feed";

    QString matchString = QString( "^(%1)" ).arg( feedProtocols.join( "|" ) );
    qDebug() << "matchString = " << matchString;

    QRegExp rx( matchString );
    int pos = rx.indexIn( urlString.trimmed() );
    qDebug() << "found at " << pos;

    return pos != -1;
}

KUrl
PodcastProvider::toFeedUrl( const QString &urlString )
{
    DEBUG_BLOCK
    debug() << urlString;

    KUrl kurl( urlString.trimmed() );

    if( kurl.protocol() == "itpc" )
    {
        debug() << "itpc:// url.";
        kurl.setProtocol( "http" );
    }
    else if( kurl.protocol() == "pcast" )
    {
        debug() << "pcast:// url.";
        kurl.setProtocol( "http" );
    }
    else if( kurl.protocol() == "feed" )
    {
        //TODO: also handle the case feed:https://example.com/entries.atom
        debug() << "feed:// url.";
        kurl.setProtocol( "http" );
    }

    return kurl;
}
