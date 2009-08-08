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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "M3UPlaylist.h"

#include "Amarok.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "PlaylistManager.h"
#include "PlaylistFileSupport.h"

#include <KUrl>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace Meta {

M3UPlaylist::M3UPlaylist()
    : PlaylistFile()
    , m_url( Meta::newPlaylistFilePath( "m3u" ) )
{
    m_name = m_url.fileName();
}

M3UPlaylist::M3UPlaylist( Meta::TrackList tracks )
    : PlaylistFile()
    , m_url( Meta::newPlaylistFilePath( "m3u" ) )
    , m_tracks( tracks )
{
    m_name = m_url.fileName();
}

M3UPlaylist::M3UPlaylist( const KUrl &url )
    : PlaylistFile( url )
    , m_url( url )
{
    DEBUG_BLOCK
    debug() << "url: " << m_url;
    m_name = m_url.fileName();

    //check if file is local or remote
    if ( m_url.isLocalFile() )
    {
        QFile file( m_url.path() );
        if( !file.open( QIODevice::ReadOnly ) ) {
            debug() << "cannot open file";
            return;
        }

        QString contents( file.readAll() );
        file.close();

        QTextStream stream;
        stream.setString( &contents );
        loadM3u( stream );
    }
    else
    {
        The::playlistManager()->downloadPlaylist( m_url, PlaylistPtr( this ) );
    }
}

M3UPlaylist::~M3UPlaylist()
{
}

bool
M3UPlaylist::loadM3u( QTextStream &stream )
{
    DEBUG_BLOCK

    const QString directory = m_url.directory();

    for( QString line; !stream.atEnd(); )
    {
        line = stream.readLine();
        if( line.startsWith( "#EXTINF" ) )
        {
            //const QString extinf = line.section( ':', 1 );
            //const int length = extinf.section( ',', 0, 0 ).toInt();
        }

        else if( !line.startsWith( '#' ) && !line.isEmpty() )
        {
            line = line.replace( "\\", "/" );

            debug() << "line: " << line;
            
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
                }
            }
            else
            {
                Meta::TrackPtr trackPtr = CollectionManager::instance()->trackForUrl( KUrl( line ) );
                if ( trackPtr ) {
                    m_tracks.append( trackPtr );
                }
            }

            // Ensure that we always have a title: use the URL as fallback
            //if( b.title().isEmpty() )
            //    b.setTitle( url );

        }
    }
    return true;
}

bool
M3UPlaylist::save( const QString &location, bool relative )
{
    QFile file( location );

    if( !file.open( QIODevice::WriteOnly ) )
    {
        debug() << "Unable to write to playlist!";
        return false;
    }

    QTextStream stream( &file );

    stream << "#EXTM3U\n";

    KUrl::List urls;
    QStringList titles;
    QList<int> lengths;
    foreach( Meta::TrackPtr track, m_tracks )
    {
        urls << track->playableUrl();
        titles << track->name();
        lengths << track->length();
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
    m_name = name;
    //TODO: notify observers
}

} //namespace Meta

