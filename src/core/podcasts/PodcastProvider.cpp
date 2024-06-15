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

#include "core/podcasts/PodcastProvider.h"

#include "core/support/Debug.h"

#include <QRegularExpression>
#include <QUrl>

using namespace Podcasts;

bool
PodcastProvider::couldBeFeed( const QString &urlString )
{
    DEBUG_BLOCK

    QStringList feedProtocols;
    feedProtocols << QStringLiteral("itpc");
    feedProtocols << QStringLiteral("pcast");
    feedProtocols << QStringLiteral("feed");

    QString matchString = QStringLiteral( "^(%1)" ).arg( feedProtocols.join( QStringLiteral("|") ) );
    QRegularExpression rx( matchString );
    int pos = urlString.trimmed().indexOf( rx );

    return pos != -1;
}

QUrl
PodcastProvider::toFeedUrl( const QString &urlString )
{
    DEBUG_BLOCK
    debug() << urlString;

    QUrl kurl( urlString.trimmed() );

    if( kurl.scheme() == QLatin1String("itpc") )
    {
        debug() << "itpc:// url.";
        kurl.setScheme( QStringLiteral("http") );
    }
    else if( kurl.scheme() == QLatin1String("pcast") )
    {
        debug() << "pcast:// url.";
        kurl.setScheme( QStringLiteral("http") );
    }
    else if( kurl.scheme() == QLatin1String("feed") )
    {
        //TODO: also handle the case feed:https://example.com/entries.atom
        debug() << "feed:// url.";
        kurl.setScheme( QStringLiteral("http") );
    }

    return kurl;
}

Playlists::PlaylistPtr
PodcastProvider::addPlaylist(Playlists::PlaylistPtr playlist )
{
    PodcastChannelPtr channel = PodcastChannelPtr::dynamicCast( playlist );
    if( channel.isNull() )
        return Playlists::PlaylistPtr();

    return Playlists::PlaylistPtr::dynamicCast( addChannel( channel ) );
}

Meta::TrackPtr
PodcastProvider::addTrack( const Meta::TrackPtr &track )
{
    PodcastEpisodePtr episode = PodcastEpisodePtr::dynamicCast( track );
    if( episode.isNull() )
        return Meta::TrackPtr();

    return Meta::TrackPtr::dynamicCast( addEpisode( episode ) );
}
