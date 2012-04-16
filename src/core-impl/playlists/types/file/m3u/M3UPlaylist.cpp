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

#include "core-impl/playlists/types/file/m3u/M3UPlaylist.h"

#define _PREFIX "M3UPlaylist"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/meta/proxy/MetaProxy.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "playlistmanager/file/PlaylistFileProvider.h"
#include "PlaylistManager.h"

#include <KMimeType>
#include <KUrl>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace Playlists {

M3UPlaylist::M3UPlaylist()
    : m_url( Playlists::newPlaylistFilePath( "m3u" ) )
    , m_tracksLoaded( true )
{
    m_name = m_url.fileName();
}

M3UPlaylist::M3UPlaylist( Meta::TrackList tracks )
    : m_url( Playlists::newPlaylistFilePath( "m3u" ) )
    , m_tracksLoaded( true )
    , m_tracks( tracks )
{
    m_name = m_url.fileName();
}

M3UPlaylist::M3UPlaylist( const KUrl &url )
    : m_url( url )
    , m_tracksLoaded( false )
{
    m_name = m_url.fileName();
}

M3UPlaylist::~M3UPlaylist()
{
}

QString
M3UPlaylist::description() const
{
    KMimeType::Ptr mimeType = KMimeType::mimeType( "audio/x-mpegurl" );
    return QString( "%1 (%2)").arg( mimeType->name(), "m3u" );
}

int
M3UPlaylist::trackCount() const
{
    if( m_tracksLoaded )
        return m_tracks.count();

    //TODO: count the number of lines starting with #
    return -1;
}

Meta::TrackList
M3UPlaylist::tracks()
{
    return m_tracks;
}

void
M3UPlaylist::triggerTrackLoad()
{
    //TODO make sure we've got all tracks first.
    if( m_tracksLoaded )
        return;

    //check if file is local or remote
    if( m_url.isLocalFile() )
    {
        QFile file( m_url.toLocalFile() );
        if( !file.open( QIODevice::ReadOnly ) )
        {
            error() << "cannot open file";
            return;
        }

        QString contents( file.readAll() );
        file.close();

        QTextStream stream;
        stream.setString( &contents );
        loadM3u( stream );
        m_tracksLoaded = true;
    }
    else
    {
        The::playlistManager()->downloadPlaylist( m_url, PlaylistFilePtr( this ) );
    }
}

void
M3UPlaylist::addTrack( Meta::TrackPtr track, int position )
{
    if( !m_tracksLoaded )
        triggerTrackLoad();

    int trackPos = position < 0 ? m_tracks.count() : position;
    if( trackPos > m_tracks.count() )
        trackPos = m_tracks.count();
    m_tracks.insert( trackPos, track );
    //set in case no track was in the playlist before
    m_tracksLoaded = true;

    notifyObserversTrackAdded( track, trackPos );

    if( !m_url.isEmpty() )
        saveLater();
}

void
M3UPlaylist::removeTrack( int position )
{
    if( position < 0 || position >= m_tracks.count() )
        return;
    m_tracks.removeAt( position );

    notifyObserversTrackRemoved( position );

    if( !m_url.isEmpty() )
        saveLater();
}

bool
M3UPlaylist::loadM3u( QTextStream &stream )
{
    const QString directory = m_url.directory();
    m_tracksLoaded = false;

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
            MetaProxy::Track *proxyTrack;
            line = line.replace( "\\", "/" );

            // KUrl's constructor handles detection of local file paths without
            // file:// etc for us
            KUrl url ( line );
            if( url.isRelative() )
            {
                url = KUrl( directory );
                url.addPath( line ); // adds directory separator if required
                url.cleanPath();
            }

            proxyTrack = new MetaProxy::Track( url );
            QString artist = extinfTitle.section( " - ", 0, 0 );
            QString title = extinfTitle.section( " - ", 1, 1 );
            //if title and artist are saved such as in M3UPlaylist::save()
            if( !title.isEmpty() && !artist.isEmpty() )
            {
                proxyTrack->setName( title );
                proxyTrack->setArtist( artist );
            }
            else
            {
                proxyTrack->setName( extinfTitle );
            }
            proxyTrack->setLength( length );
            m_tracks << Meta::TrackPtr( proxyTrack );
            m_tracksLoaded = true;
        }
    } while( !stream.atEnd() );

    //TODO: return false if stream is not readable, empty or has errors
    return true;
}

bool
M3UPlaylist::save( const KUrl &location, bool relative )
{
    KUrl savePath = location;
    //if the location is a directory append the name of this playlist.
    if( savePath.fileName().isNull() )
        savePath.setFileName( name() );

    QFile file( savePath.path() );

    if( !file.open( QIODevice::WriteOnly ) )
    {
        error() << "Unable to write to playlist " << savePath.path();
        return false;
    }

    QTextStream stream( &file );

    stream << "#EXTM3U\n";

    KUrl::List urls;
    QStringList titles;
    QList<int> lengths;
    foreach( Meta::TrackPtr track, m_tracks )
    {
        Q_ASSERT(track);

        const KUrl &url = track->playableUrl();
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
        if( url.protocol() == "file" )
        {
            if( relative )
            {
                const QFileInfo fi( file );
                QString relativePath = KUrl::relativePath( fi.path(), url.path() );
                if( relativePath.startsWith( "./" ) )
                    relativePath.remove( 0, 2 );
                stream << relativePath;
            }
            else
            {
                stream << url.path();
            }
        }
        else
        {
            stream << url.url();
        }
        stream << "\n";
    }

    return true;
}

bool
M3UPlaylist::isWritable()
{
    if( m_url.isEmpty() )
        return false;

    return QFileInfo( m_url.path() ).isWritable();
}

void
M3UPlaylist::setName( const QString &name )
{
    m_url.setFileName( name );
}

} //namespace Playlists

