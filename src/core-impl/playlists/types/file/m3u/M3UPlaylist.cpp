/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "M3UPlaylist.h"

#include "core/support/Debug.h"

#include <QFile>

using namespace Playlists;

M3UPlaylist::M3UPlaylist( const QUrl &url, PlaylistProvider *provider )
    : PlaylistFile( url, provider )
{
}

bool
M3UPlaylist::loadM3u( QTextStream &stream )
{
    if( m_tracksLoaded )
        return true;
    const QString directory = m_url.adjusted(QUrl::RemoveFilename).path();
    m_tracksLoaded = true;

    int length = -1;
    QString extinfTitle;
    do
    {
        QString line = stream.readLine();
        if( line.startsWith( "#EXTINF" ) )
        {
            const QString extinf = line.section( ':', 1 );
            bool ok;
            length = extinf.section( ',', 0, 0 ).toInt( &ok );
            if( !ok )
                length = -1;
            extinfTitle = extinf.section( ',', 1 );
        }
        else if( !line.startsWith( '#' ) && !line.isEmpty() )
        {
            line = line.replace( "\\", "/" );

            QUrl url = getAbsolutePath( QUrl( line ) );

            MetaProxy::TrackPtr proxyTrack( new MetaProxy::Track( url ) );
            QString artist = extinfTitle.section( " - ", 0, 0 );
            QString title = extinfTitle.section( " - ", 1, 1 );
            //if title and artist are saved such as in M3UPlaylist::save()
            if( !title.isEmpty() && !artist.isEmpty() )
            {
                proxyTrack->setTitle( title );
                proxyTrack->setArtist( artist );
            }
            else
            {
                proxyTrack->setTitle( extinfTitle );
            }
            proxyTrack->setLength( length );
            Meta::TrackPtr track( proxyTrack.data() );
            addProxyTrack( track );
        }
    } while( !stream.atEnd() );

    //TODO: return false if stream is not readable, empty or has errors
    return true;
}

void
M3UPlaylist::savePlaylist( QFile &file )
{
    QTextStream stream( &file );

    stream << "#EXTM3U\n";
    QList<QUrl> urls;
    QStringList titles;
    QList<int> lengths;

    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( !track ) // see BUG: 303056
            continue;

        const QUrl &url = track->playableUrl();
        int length = track->length() / 1000;
        const QString &title = track->name();
        const QString &artist = track->artist()->name();

        if( !title.isEmpty() && !artist.isEmpty() && length )
        {
            stream << "#EXTINF:";
            stream << QString::number( length );
            stream << ',';
            stream << artist << " - " << title;
            stream << '\n';
        }

        if( url.scheme() == "file" )
            stream << trackLocation( track );
        else
            stream << url.url();
        stream << "\n";
    }
}
