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

#include "core-impl/playlists/types/file/pls/PLSPlaylist.h"

#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "core/capabilities/EditCapability.h"
#include "core/meta/Meta.h"
#include "PlaylistManager.h"
#include "core-impl/meta/proxy/MetaProxy.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"

#include <KMimeType>
#include <KLocale>

#include <QTextStream>
#include <QRegExp>
#include <QString>
#include <QFile>

namespace Playlists {

PLSPlaylist::PLSPlaylist()
    : m_url( Playlists::newPlaylistFilePath( "pls" ) )
    , m_tracksLoaded( true )
{
    m_name = m_url.fileName();
}

PLSPlaylist::PLSPlaylist( Meta::TrackList tracks )
    : m_url( Playlists::newPlaylistFilePath( "pls" ) )
    , m_tracksLoaded( true )
    , m_tracks( tracks )
{
    m_name = m_url.fileName();
}

PLSPlaylist::PLSPlaylist( const KUrl &url )
    : m_url( url )
    , m_tracksLoaded( false )
{
    m_name = m_url.fileName();
}

PLSPlaylist::~PLSPlaylist()
{
}

QString
PLSPlaylist::description() const
{
    KMimeType::Ptr mimeType = KMimeType::mimeType( "audio/x-scpls" );
    return QString( "%1 (%2)").arg( mimeType->name(), "pls" );
}

int
PLSPlaylist::trackCount() const
{
    if( m_tracksLoaded )
        return m_tracks.count();

    //TODO: read NumberOfEntries from footer
    return -1;
}

Meta::TrackList
PLSPlaylist::tracks()
{
    return m_tracks;
}

void
PLSPlaylist::triggerTrackLoad()
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
        loadPls( stream );
        m_tracksLoaded = true;
    }
    else
    {
        The::playlistManager()->downloadPlaylist( m_url, PlaylistFilePtr( this ) );
    }
}

void
PLSPlaylist::addTrack( Meta::TrackPtr track, int position )
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
PLSPlaylist::removeTrack( int position )
{
    if( position < 0 || position >= m_tracks.count() )
        return;
    m_tracks.removeAt( position );

    notifyObserversTrackRemoved( position );

    if( !m_url.isEmpty() )
        saveLater();
}

bool
PLSPlaylist::loadPls( QTextStream &textStream )
{
    MetaProxy::Track *proxyTrack = 0;

    // Counted number of "File#=" lines.
    unsigned int entryCnt = 0;
    // Value of the "NumberOfEntries=#" line.
    unsigned int numberOfEntries = 0;
    // Does the file have a "[playlist]" section? (as it's required by the standard)
    bool havePlaylistSection = false;
    QString tmp;
    QStringList lines;

    const QRegExp regExp_NumberOfEntries("^NumberOfEntries\\s*=\\s*\\d+$");
    const QRegExp regExp_File("^File\\d+\\s*=");
    const QRegExp regExp_Title("^Title\\d+\\s*=");
    const QRegExp regExp_Length("^Length\\d+\\s*=\\s*\\d+$");
    const QRegExp regExp_Version("^Version\\s*=\\s*\\d+$");
    const QString section_playlist("[playlist]");

    /* Preprocess the input data.
    * Read the lines into a buffer; Cleanup the line strings;
    * Count the entries manually and read "NumberOfEntries".
    */
    while( !textStream.atEnd() )
    {
        tmp = textStream.readLine();
        tmp = tmp.trimmed();
        if( tmp.isEmpty() )
            continue;
        lines.append( tmp );

        if( tmp.contains( regExp_File ) )
        {
            entryCnt++;
            continue;
        }
        if( tmp == section_playlist )
        {
            havePlaylistSection = true;
            continue;
        }
        if( tmp.contains( regExp_NumberOfEntries ) )
        {
            numberOfEntries = tmp.section( '=', -1 ).trimmed().toUInt();
            continue;
        }
    }
    if( numberOfEntries != entryCnt )
    {
        warning() << ".pls playlist: Invalid \"NumberOfEntries\" value.  "
                << "NumberOfEntries=" << numberOfEntries << "  counted="
                << entryCnt << endl;
        /* Corrupt file. The "NumberOfEntries" value is
        * not correct. Fix it by setting it to the manually
        * counted number and go on parsing.
        */
        numberOfEntries = entryCnt;
    }
    if( numberOfEntries == 0 )
        return true;

    unsigned int index;
    bool ok = false;
    bool inPlaylistSection = false;

    /* Now iterate through all beautified lines in the buffer
    * and parse the playlist data.
    */
    QStringList::const_iterator i = lines.constBegin(), end = lines.constEnd();
    for( ; i != end; ++i )
    {
        if( !inPlaylistSection && havePlaylistSection )
        {
            /* The playlist begins with the "[playlist]" tag.
            * Skip everything before this.
            */
            if( (*i) == section_playlist )
                inPlaylistSection = true;
            continue;
        }
        if( (*i).contains( regExp_File ) )
        {
            // Have a "File#=XYZ" line.
            index = loadPls_extractIndex( *i );
            if( index > numberOfEntries || index == 0 )
                continue;
            tmp = (*i).section( '=', 1 ).trimmed();
            KUrl url( tmp );
            if( url.isRelative() )
            {
                const QString dir = m_url.directory();
                url = KUrl( dir );
                url.addPath( tmp );
                url.cleanPath();
            }
            proxyTrack = new MetaProxy::Track( url );
            m_tracks << Meta::TrackPtr( proxyTrack );
            continue;
        }
        if( (*i).contains(regExp_Title) )
        {
            // Have a "Title#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if( index > numberOfEntries || index == 0 )
                continue;
            tmp = (*i).section( '=', 1 ).trimmed();
            proxyTrack->setName( tmp );
            continue;
        }
        if( (*i).contains( regExp_Length ) )
        {
            // Have a "Length#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if( index > numberOfEntries || index == 0 )
                continue;
            tmp = (*i).section( '=', 1 ).trimmed();
            bool ok = false;
            int seconds = tmp.toInt( &ok );
            if( ok )
                proxyTrack->setLength( seconds * 1000 ); //length is in milliseconds
            continue;
        }
        if( (*i).contains( regExp_NumberOfEntries ) )
        {
            // Have the "NumberOfEntries=#" line.
            continue;
        }
        if( (*i).contains( regExp_Version ) )
        {
            // Have the "Version=#" line.
            tmp = (*i).section( '=', 1 ).trimmed();
            // We only support Version=2
            if (tmp.toUInt( &ok ) != 2)
                warning() << ".pls playlist: Unsupported version." << endl;
            continue;
        }
        warning() << ".pls playlist: Unrecognized line: \"" << *i << "\"" << endl;
    }

    return true;
}

bool
PLSPlaylist::save( const KUrl &location, bool relative )
{
    Q_UNUSED( relative );

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

    //Format: http://en.wikipedia.org/wiki/PLS_(file_format)
    QTextStream stream( &file );
    //header
    stream << "[Playlist]\n";

    //body
    int i = 1; //PLS starts at File1=
    foreach( Meta::TrackPtr track, m_tracks )
    {
        KUrl playableUrl( track->playableUrl() );
        QString file = playableUrl.url();

        if( playableUrl.isLocalFile() )
        {
            if( relative )
            {
                file = KUrl::relativePath( savePath.directory(),
                                           playableUrl.toLocalFile() );
                if( file.startsWith( "./" ) )
                    file.remove( 0, 2 );
            }
            else
            {
                file = playableUrl.toLocalFile();
            }
        }
        stream << "File" << i << "=" << file;
        stream << "\nTitle" << i << "=";
        stream << track->name();
        stream << "\nLength" << i << "=";
        stream << track->length() / 1000;
        stream << "\n";
        i++;
    }

    //footer
    stream << "NumberOfEntries=" << m_tracks.count() << endl;
    stream << "Version=2\n";
    file.close();
    return true;
}

unsigned int
PLSPlaylist::loadPls_extractIndex( const QString &str ) const
{
    /* Extract the index number out of a .pls line.
     * Example:
    *   loadPls_extractIndex("File2=foobar") == 2
    */
    bool ok = false;
    unsigned int ret;
    QString tmp( str.section( '=', 0, 0 ) );
    tmp.remove( QRegExp( "^\\D*" ) );
    ret = tmp.trimmed().toUInt( &ok );
    Q_ASSERT(ok);
    return ret;
}

bool
PLSPlaylist::isWritable()
{
    if( m_url.isEmpty() )
        return false;

    return QFileInfo( m_url.path() ).isWritable();
}

void
PLSPlaylist::setName( const QString &name )
{
    m_url.setFileName( name );
}

}
