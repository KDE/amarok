/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "PlaylistFileSupport.h"
#include "debug.h"
#include "collection/CollectionManager.h"
#include "ContextStatusBar.h"
#include "meta/XSPFPlaylist.h"
#include "meta/PLSPlaylist.h"
#include "meta/M3UPlaylist.h"


#include <KLocale>

#include <QDir>
#include <QFile>

namespace Meta {

bool
saveM3u( const TrackList &tracks, const KUrl &path, bool relative )
{
    if( path.isEmpty() )
        return false;

    QFile file( path.url() );

    if( !file.open( QIODevice::WriteOnly ) )
    {
        Amarok::ContextStatusBar::instance()->longMessageThreadSafe( i18n( "Cannot write playlist (%1).", path.url() ) );
        return false;
    }

    QTextStream stream( &file );
    stream << "#EXTM3U\n";

//     KUrl::List urls;
//     for( int i = 0, n = in_urls.count(); i < n; ++i )
//     {
//         const KUrl &url = in_urls[i];
//         if( url.isLocalFile() && QFileInfo( url.path() ).isDir() )
//             urls += recurse( url );
//         else
//             urls += url;
//     }

    foreach( TrackPtr track, tracks )
    {
        const KUrl &url = track->playableUrl();

        stream << "#EXTINF:";
        stream << QString::number( track->length() );
        stream << ',';
        stream << track->fullPrettyName();
        stream << '\n';

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

    file.close(); // Flushes the file, before we read it
    //PlaylistBrowser::instance()->addPlaylist( path, 0, true );

    return true;
}

TrackList
loadM3u( QTextStream &stream, const QString &playlistDir )
{
    TrackList tracks;
    for( QString line; !stream.atEnd(); )
    {
        line = stream.readLine();

        //we are ignoring the additional meta information at the moment
        if( line.startsWith( "#EXTINF" ) ) {
            const QString extinf = line.section( ':', 1 );
            const int length = extinf.section( ',', 0, 0 ).toInt();
            Q_UNUSED( length );
            //b.setTitle( extinf.section( ',', 1 ) );
            //b.setLength( length <= 0 ? /*MetaBundle::Undetermined HACK*/ -2 : length );
        }
        else if( !line.startsWith( '#' ) && !line.isEmpty() )
        {
            // KUrl::isRelativeUrl() expects absolute URLs to start with a protocol, so prepend it if missing
            QString url = line;
            //TODO: Unix specific
            if( url.startsWith( '/' ) )
                url.prepend( "file://" );

            TrackPtr track;
            if( KUrl::isRelativeUrl( url ) )
            {
                KUrl kurl( KUrl( playlistDir + line ) );
                kurl.cleanPath();
                track = CollectionManager::instance()->trackForUrl( kurl );
            }
            else
            {
                track = CollectionManager::instance()->trackForUrl( KUrl( line ) );
            }
            if( track )
                tracks.append( track );
        }
    }

    return tracks;
}

bool
saveXSPF( const TrackList &tracks, const KUrl &path, bool relative )
{
    Q_UNUSED( relative );

    Meta::XSPFPlaylist playlist;

    playlist.setCreator( "Amarok" );
    //playlist.setTitle( item->text(0) );

    playlist.setTrackList( tracks, false );
    QFile file( path.url() );
    file.open( QIODevice::WriteOnly );

    QTextStream stream ( &file );

    playlist.save( stream, 2 );

    file.close();
    return true;
}

Meta::Format
getFormat( const QString &filename )
{

    const QString ext = Amarok::extension( filename );

    if( ext == "m3u" ) return M3U;
    if( ext == "pls" ) return PLS;
    if( ext == "ram" ) return RAM;
    if( ext == "smil") return SMIL;
    if( ext == "asx" || ext == "wax" ) return ASX;
    if( ext == "xml" ) return XML;
    if( ext == "xspf" ) return XSPF;

    return Unknown;
}

PlaylistPtr
loadPlaylist( QFile &file )
{
    DEBUG_BLOCK

    Format format = getFormat( file.fileName() );

    QTextStream stream( new QString( file.readAll() ) );
    PlaylistPtr playlist;
    switch( format ) {
        case PLS:
            playlist = new PLSPlaylist( stream );
            break;
        case M3U:
            playlist = new M3UPlaylist( stream );
            break;
//         case RAM:
//             playlist = loadRealAudioRam( stream );
//             break;
//         case ASX:
//             playlist = loadASX( stream );
//             break;
//         case SMIL:
//             playlist = loadSMIL( stream );
//             break;
        case XSPF:
            playlist = new XSPFPlaylist( stream );
            break;

        default:
            debug() << "unknown type!";
            break;
    }

    return playlist;

}

}
