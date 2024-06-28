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

#include "PLSPlaylist.h"

#include "core/support/Debug.h"

#include <QFile>

using namespace Playlists;

PLSPlaylist::PLSPlaylist( const QUrl &url, PlaylistProvider *provider )
    : PlaylistFile( url, provider )
{
}

bool
PLSPlaylist::loadPls( QTextStream &textStream )
{
    if( m_tracksLoaded )
        return true;
    m_tracksLoaded = true;

    // Counted number of "File#=" lines.
    unsigned int entryCnt = 0;
    // Value of the "NumberOfEntries=#" line.
    unsigned int numberOfEntries = 0;
    // Does the file have a "[playlist]" section? (as it's required by the standard)
    bool havePlaylistSection = false;
    QString tmp;
    QStringList lines;

    // Case insensitive, as it seems many playlists use numberofentries
    QRegularExpression regExp_NumberOfEntries( QStringLiteral("^NumberOfEntries\\s*=\\s*\\d+$"), QRegularExpression::CaseInsensitiveOption );
    const QRegularExpression regExp_File( QStringLiteral("^File\\d+\\s*=") );
    const QRegularExpression regExp_Title( QStringLiteral("^Title\\d+\\s*=") );
    const QRegularExpression regExp_Length( QStringLiteral("^Length\\d+\\s*=\\s*-?\\d+$") ); // Length Can be -1
    const QRegularExpression regExp_Version( QStringLiteral("^Version\\s*=\\s*\\d+$") );
    const QString section_playlist(QStringLiteral("[playlist]"));



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
            numberOfEntries = tmp.section( QLatin1Char('='), -1 ).trimmed().toUInt();
            continue;
        }
    }
    if( numberOfEntries != entryCnt )
    {
        warning() << ".pls playlist: Invalid \"NumberOfEntries\" value.  "
                << "NumberOfEntries=" << numberOfEntries << "  counted="
                << entryCnt << Qt::endl;
        /* Corrupt file. The "NumberOfEntries" value is
        * not correct. Fix it by setting it to the manually
        * counted number and go on parsing.
        */
        numberOfEntries = entryCnt;
    }
    if( numberOfEntries == 0 )
    {
        return true;
    }

    unsigned int index;
    bool ok = false;
    bool inPlaylistSection = false;

    MetaProxy::TrackPtr proxyTrack;
    /* Now iterate through all beautified lines in the buffer
     * and parse the playlist data. */
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
            tmp = (*i).section( QLatin1Char('='), 1 ).trimmed();
            QUrl url = getAbsolutePath( QUrl( tmp ) );
            proxyTrack = new MetaProxy::Track( url );
            Meta::TrackPtr track( proxyTrack.data() );
            addProxyTrack( track );
            continue;
        }
        if( (*i).contains(regExp_Title) && proxyTrack )
        {
            // Have a "Title#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if( index > numberOfEntries || index == 0 )
                continue;
            tmp = (*i).section( QLatin1Char('='), 1 ).trimmed();
            proxyTrack->setTitle( tmp );
            continue;
        }
        if( (*i).contains( regExp_Length ) && proxyTrack )
        {
            // Have a "Length#=XYZ" line.
            index = loadPls_extractIndex(*i);
            if( index > numberOfEntries || index == 0 )
                continue;
            tmp = (*i).section( QLatin1Char('='), 1 ).trimmed();
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
            tmp = (*i).section( QLatin1Char('='), 1 ).trimmed();
            // We only support Version=2
            if (tmp.toUInt( &ok ) != 2)
                warning() << ".pls playlist: Unsupported version." << Qt::endl;
            continue;
        }
        warning() << ".pls playlist: Unrecognized line: \"" << *i << "\"" << Qt::endl;
    }
    return true;
}

unsigned int
PLSPlaylist::loadPls_extractIndex( const QString &str ) const
{
    /* Extract the index number out of a .pls line.
     * Example:
     *   loadPls_extractIndex("File2=foobar") == 2 */
    bool ok = false;
    unsigned int ret;
    QString tmp( str.section( QLatin1Char('='), 0, 0 ) );
    tmp.remove( QRegularExpression( QLatin1String("^\\D*") ) );
    ret = tmp.trimmed().toUInt( &ok );
    Q_ASSERT(ok);
    return ret;
}

void
PLSPlaylist::savePlaylist( QFile &file )
{
    //Format: http://en.wikipedia.org/wiki/PLS_(file_format)
    QTextStream stream( &file );
    //header
    stream << "[Playlist]\n";

    //body
    int i = 1; //PLS starts at File1=
    for( Meta::TrackPtr track : m_tracks )
    {
        if( !track ) // see BUG: 303056
            continue;

        stream << "File" << i << "=" << trackLocation( track );
        stream << "\nTitle" << i << "=";
        stream << track->name();
        stream << "\nLength" << i << "=";
        stream << track->length() / 1000;
        stream << "\n";
        i++;
   }

   //footer
   stream << "NumberOfEntries=" << m_tracks.count() << Qt::endl;
   stream << "Version=2\n";
}
