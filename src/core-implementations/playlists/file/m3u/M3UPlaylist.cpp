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

#include "core-implementations/playlists/file/m3u/M3UPlaylist.h"

#define DEBUG_PREFIX "M3UPlaylist"

#include "Amarok.h"
#include "CollectionManager.h"
#include "core/support/Debug.h"
#include "PlaylistManager.h"
#include "core-implementations/playlists/file/PlaylistFileSupport.h"

#include <KMimeType>
#include <KUrl>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace Playlists {

M3UPlaylist::M3UPlaylist()
    : m_url( Playlists::newPlaylistFilePath( "m3u" ) )
    , m_tracksLoaded( false )
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
    //DEBUG_BLOCK
    //debug() << "url: " << m_url;
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

Meta::TrackList
M3UPlaylist::tracks()
{
    if( m_tracksLoaded )
        return m_tracks;

    //check if file is local or remote
    if ( m_url.isLocalFile() )
    {
        QFile file( m_url.toLocalFile() );
        if( !file.open( QIODevice::ReadOnly ) ) {
            debug() << "cannot open file";
            return m_tracks;
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
    return m_tracks;
}

bool
M3UPlaylist::loadM3u( QTextStream &stream )
{
    DEBUG_BLOCK

    const QString directory = m_url.directory();
    bool hasTracks( false );
    m_tracksLoaded = false;

    do
    {
        QString line = stream.readLine();
        if( line.startsWith( "#EXTINF" ) )
        {
            //const QString extinf = line.section( ':', 1 );
            //const int length = extinf.section( ',', 0, 0 ).toInt();
        }
        else if( !line.startsWith( '#' ) && !line.isEmpty() )
        {
            line = line.replace( "\\", "/" );

            debug() << "line:" << line;

            // KUrl::isRelativeUrl() expects absolute URLs to start with a protocol, so prepend it if missing
            QString url = line;
            if( url.startsWith( '/' ) )
                url.prepend( "file://" );
            // Won't be relative if it begins with a /
            // Also won't be windows url, so no need to worry about swapping \ for /
            if( KUrl::isRelativeUrl( url ) )
            {
                debug() << "relative url";
                //Replace \ with / for windows playlists
                line.replace('\\','/');
                KUrl kurl( directory );
                kurl.addPath( line ); // adds directory separator if required
                kurl.cleanPath();
                Meta::TrackPtr trackPtr = CollectionManager::instance()->trackForUrl( kurl );

                if ( trackPtr ) {
                    debug() << "track url: " << trackPtr->prettyUrl();
                    m_tracks.append( trackPtr );
                    hasTracks = true;
                    m_tracksLoaded = true;
                }
            }
            else
            {
                Meta::TrackPtr trackPtr = CollectionManager::instance()->trackForUrl( KUrl( line ) );
                if ( trackPtr ) {
                    m_tracks.append( trackPtr );
                    hasTracks = true;
                    m_tracksLoaded = true;
                }
            }
        }
    } while( !stream.atEnd() );
    return hasTracks;
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
        debug() << "Unable to write to playlist " << savePath.path();
        return false;
    }

    QTextStream stream( &file );

    stream << "#EXTM3U\n";

    KUrl::List urls;
    QStringList titles;
    QList<int> lengths;
    foreach( Meta::TrackPtr track, m_tracks )
    {
        if( track )
        {
            urls << track->playableUrl();
            titles << track->name();
            lengths << track->length();
        }
    }

    for( int i = 0, n = urls.count(); i < n; ++i )
    {
        const KUrl &url = urls[i];

        if( !titles.isEmpty() && !lengths.isEmpty() )
        {
            stream << "#EXTINF:";
            stream << QString::number( lengths[i] );
            stream << ',';
            stream << titles[i];
            stream << '\n';
        }
        if (url.protocol() == "file" ) {
            if ( relative ) {
                const QFileInfo fi(file);
                stream << KUrl::relativePath(fi.path(), url.path());
            } else
                stream << url.path();
        } else {
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

