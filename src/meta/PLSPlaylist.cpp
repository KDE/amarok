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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PLSPlaylist.h"

#include "CollectionManager.h"
#include "Debug.h"
#include "EditCapability.h"
#include "Meta.h"
#include "PlaylistManager.h"
#include "PlaylistFileSupport.h"

#include <KMimeType>
#include <KLocale>

#include <QTextStream>
#include <QRegExp>
#include <QString>
#include <QFile>

namespace Meta {

PLSPlaylist::PLSPlaylist()
    : PlaylistFile()
    , m_url( Meta::newPlaylistFilePath( "pls" ) )
{
    m_name = m_url.fileName();
}

PLSPlaylist::PLSPlaylist( TrackList tracks )
    : PlaylistFile()
    , m_tracks( tracks )
    , m_url( Meta::newPlaylistFilePath( "pls" ) )
{
    m_name = m_url.fileName();
}

PLSPlaylist::PLSPlaylist( const KUrl &url )
    : PlaylistFile( url )
    , m_url( url )
{
    DEBUG_BLOCK
    debug() << "url: " << m_url;

    m_name = m_url.fileName();

    //check if file is local or remote
    if ( m_url.isLocalFile() )
    {
        QFile file( m_url.toLocalFile() );
        if( !file.open( QIODevice::ReadOnly ) ) {
            debug() << "cannot open file";
            return;
        }

        QString contents = QString( file.readAll() );
        file.close();

        QTextStream stream;
        stream.setString( &contents );
        loadPls( stream );
    }
    else
    {
        The::playlistManager()->downloadPlaylist( m_url, PlaylistFilePtr( this ) );
    }
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

bool
PLSPlaylist::loadPls( QTextStream &stream )
{
    DEBUG_BLOCK

    Meta::TrackPtr currentTrack;

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
    while (!stream.atEnd()) {
        tmp = stream.readLine();
        tmp = tmp.trimmed();
        if (tmp.isEmpty())
            continue;
        lines.append(tmp);

        if (tmp.contains(regExp_File)) {
            entryCnt++;
            continue;
        }
        if (tmp == section_playlist) {
            havePlaylistSection = true;
            continue;
        }
        if (tmp.contains(regExp_NumberOfEntries)) {
            numberOfEntries = tmp.section('=', -1).trimmed().toUInt();
            continue;
        }
    }
    if (numberOfEntries != entryCnt) {
        warning() << ".pls playlist: Invalid \"NumberOfEntries\" value.  "
                << "NumberOfEntries=" << numberOfEntries << "  counted="
                << entryCnt << endl;
        /* Corrupt file. The "NumberOfEntries" value is
        * not correct. Fix it by setting it to the manually
        * counted number and go on parsing.
        */
        numberOfEntries = entryCnt;
    }
    if (!numberOfEntries)
        return true;

    unsigned int index;
    bool ok = false;
    bool inPlaylistSection = false;

    /* Now iterate through all beautified lines in the buffer
    * and parse the playlist data.
    */
    QStringList::const_iterator i = lines.constBegin(), end = lines.constEnd();
    for ( ; i != end; ++i) {
        if (!inPlaylistSection && havePlaylistSection) {
            /* The playlist begins with the "[playlist]" tag.
            * Skip everything before this.
            */
            if ((*i) == section_playlist)
                inPlaylistSection = true;
            continue;
        }
        if ((*i).contains(regExp_File)) {
            // Have a "File#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if (index > numberOfEntries || index == 0)
                continue;
            tmp = (*i).section('=', 1).trimmed();
            currentTrack = CollectionManager::instance()->trackForUrl( tmp );
            m_tracks.append( currentTrack );
            continue;
        }
        if ((*i).contains(regExp_Title)) {
            // Have a "Title#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if (index > numberOfEntries || index == 0)
                continue;
            tmp = (*i).section('=', 1).trimmed();

            if ( currentTrack.data() != 0 && currentTrack->is<Meta::EditCapability>() )
            {
                Meta::EditCapability *ec = currentTrack->create<Meta::EditCapability>();
                if( ec )
                    ec->setTitle( tmp );
                delete ec;
            }
            continue;
        }
        if ((*i).contains(regExp_Length)) {
            // Have a "Length#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if (index > numberOfEntries || index == 0)
                continue;
            tmp = (*i).section('=', 1).trimmed();
            //tracks.append( KUrl(tmp) );
//             Q_ASSERT(ok);
            continue;
        }
        if ((*i).contains(regExp_NumberOfEntries)) {
            // Have the "NumberOfEntries=#" line.
            continue;
        }
        if ((*i).contains(regExp_Version)) {
            // Have the "Version=#" line.
            tmp = (*i).section('=', 1).trimmed();
            // We only support Version=2
            if (tmp.toUInt(&ok) != 2)
                warning() << ".pls playlist: Unsupported version." << endl;
//             Q_ASSERT(ok);
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

    QTextStream stream( &file );
    stream << "[Playlist]\n";
    stream << "NumberOfEntries=" << m_tracks.count() << endl;
    int i = 0;
    foreach( Meta::TrackPtr track, m_tracks )
    {
        stream << "File" << i << "=";
        stream << KUrl( track->playableUrl() ).path();
        stream << "\nTitle" << i << "=";
        stream << track->name();
        stream << "\nLength" << i << "=";
        stream << track->length();
        stream << "\n";
        i++;
    }

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
    QString tmp(str.section('=', 0, 0));
    tmp.remove(QRegExp("^\\D*"));
    ret = tmp.trimmed().toUInt(&ok);
    Q_ASSERT(ok);
    return ret;
}

bool
PLSPlaylist::isWritable()
{
    return QFile( m_url.path() ).isWritable();
}

void
PLSPlaylist::setName( const QString &name )
{
    m_url.setFileName( name );
}

}
