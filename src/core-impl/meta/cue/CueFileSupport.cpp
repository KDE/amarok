/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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


#include "CueFileSupport.h"

#include "core/support/Debug.h"
#include "core-impl/meta/timecode/TimecodeMeta.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <KEncodingProber>
#else
#include <QStringConverter>
#endif

#include <QDir>
#include <QFile>
#include <QTextCodec>

using namespace MetaCue;

/**
* Parses a cue sheet file into CueFileItems and inserts them in a QMap
* @return a map of CueFileItems. If the cue file was not successfully loaded
*         the map is empty.
* @author (C) 2005 by Martin Ehmke <ehmke@gmx.de>
*/

CueFileItemMap CueFileSupport::loadCueFile( const QUrl &cuefile, const Meta::TrackPtr &track )
{
    return loadCueFile( cuefile, track->playableUrl(), track->length() );
}


CueFileItemMap CueFileSupport::loadCueFile( const QUrl &cuefile, const QUrl &trackUrl, qint64 trackLen )
{
    DEBUG_BLOCK

    CueFileItemMap cueItems;

    debug() << "CUEFILE: " << cuefile.toDisplayString();
    if ( QFile::exists ( cuefile.toLocalFile() ) )
    {
        debug() << "  EXISTS!";
        QFile file ( cuefile.toLocalFile() );
        int trackNr = 0;
        QString defaultArtist;
        QString defaultAlbum;
        QString artist;
        QString title;
        long length = 0;
        long prevIndex = -1;
        bool index00Present = false;
        long index = -1;
        bool filesSection = false;
        bool fileFound = false;

        int mode = BEGIN;
        if ( file.open ( QIODevice::ReadOnly ) )
        {
            QTextStream stream ( &file );
            QString line;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            KEncodingProber prober;

            KEncodingProber::ProberState result = prober.feed( file.readAll() );
            file.seek( 0 );

            if( result != KEncodingProber::NotMe )
                stream.setCodec( QTextCodec::codecForName( prober.encoding() ) );
            debug() << "Encoding: " << prober.encoding();
#else
            QByteArray data = file.readAll();
            file.seek( 0 );
            auto enc = QStringConverter::encodingForData( data );
            if( enc.has_value() )
                stream.setEncoding( enc.value() );
#endif

            while ( !stream.atEnd() )
            {
                line = stream.readLine().simplified();

                if ( line.startsWith ( QLatin1String("title"), Qt::CaseInsensitive ) )
                {
                    title = line.mid ( 6 ).remove ( QLatin1Char('"') );
                    if ( mode == BEGIN && !filesSection )
                    {
                        defaultAlbum = title;
                        title.clear();
                        debug() << "Album: " << defaultAlbum;
                    }
                    else if( !fileFound )
                    {
                        title.clear();
                        continue;
                    }
                    else
                        debug() << "Title: " << title;
                }

                else if ( line.startsWith ( QLatin1String("performer"), Qt::CaseInsensitive ) )
                {
                    artist = line.mid ( 10 ).remove ( QLatin1Char('"') );
                    if ( mode == BEGIN && !filesSection  )
                    {
                        defaultArtist = artist;
                        artist.clear();
                        debug() << "Album Artist: " << defaultArtist;
                    }
                    else if( !fileFound )
                    {
                        artist.clear();
                        continue;
                    }
                    else
                        debug() << "Artist: " << artist;
                }

                else if ( line.startsWith ( QLatin1String("track"), Qt::CaseInsensitive ) && fileFound )
                {
                    if ( mode == TRACK_FOUND )
                    {
                        // not valid, because we have to have an index for the previous track
                        file.close();
                        debug() << "Mode is TRACK_FOUND, abort.";
                        return CueFileItemMap();
                    }
                    else if ( mode == INDEX_FOUND )
                    {
                        if ( artist.isNull() )
                            artist = defaultArtist;

                        debug() << "Inserting item: " << title << " - " << artist << " on " << defaultAlbum << " (" << trackNr << ")";
                        // add previous entry to map
                        cueItems.insert ( index, CueFileItem ( title, artist, defaultAlbum, trackNr, index ) );
                        prevIndex = index;
                        title.clear();
                        artist.clear();
                        trackNr  = 0;
                    }
                    trackNr = line.section ( QLatin1Char(' '),1,1 ).toInt();
                    debug() << "Track: " << trackNr;
                    mode = TRACK_FOUND;
                }
                else if ( line.startsWith ( QLatin1String("index"), Qt::CaseInsensitive ) && fileFound  )
                {
                    if ( mode == TRACK_FOUND )
                    {
                        int indexNo = line.section ( QLatin1Char(' '),1,1 ).toInt();

                        if ( indexNo == 1 )
                        {
                            QStringList time = line.section ( QLatin1Char(' '), -1, -1 ).split ( QLatin1Char(':') );

                            index = time[0].toLong() *60*1000 + time[1].toLong() *1000 + time[2].toLong() *1000/75; //75 frames per second

                            if ( prevIndex != -1 && !index00Present ) // set the prev track's length if there is INDEX01 present, but no INDEX00
                            {
                                length = index - prevIndex;
                                debug() << "Setting length of track " << cueItems[prevIndex].title() << " to " << length << " msecs.";
                                cueItems[prevIndex].setLength ( length );
                            }

                            index00Present = false;
                            mode = INDEX_FOUND;
                            length = 0;
                        }

                        else if ( indexNo == 0 ) // gap, use to calc prev track length
                        {
                            QStringList time = line.section ( QLatin1Char(' '), -1, -1 ).split ( QLatin1Char(':') );

                            length = time[0].toLong() * 60 * 1000 + time[1].toLong() * 1000 + time[2].toLong() *1000/75; //75 frames per second

                            if ( prevIndex != -1 )
                            {
                                length -= prevIndex; //this[prevIndex].getIndex();
                                debug() << "Setting length of track " << cueItems[prevIndex].title() << " to " << length << " msecs.";
                                cueItems[prevIndex].setLength ( length );
                                index00Present = true;
                            }
                            else
                                length =  0;
                        }
                        else
                        {
                            debug() << "Skipping unsupported INDEX " << indexNo;
                        }
                    }
                    else
                    {
                        // not valid, because we don't have an associated track
                        file.close();
                        debug() << "Mode is not TRACK_FOUND but encountered INDEX, abort.";
                        return CueFileItemMap();
                    }
                    debug() << "index: " << index;
                }
                else if( line.startsWith ( QLatin1String("file"), Qt::CaseInsensitive ) )
                {
                    QString file = line.mid ( 5 ).remove ( QLatin1Char('"') );
                    if( fileFound )
                        break;

                    fileFound = file.contains ( trackUrl.fileName(), Qt::CaseInsensitive );
                    filesSection = true;
                }
            }

            if ( artist.isNull() )
                artist = defaultArtist;

            debug() << "Inserting item: " << title << " - " << artist << " on " << defaultAlbum << " (" << trackNr << ")";
            // add previous entry to map
            cueItems.insert ( index, CueFileItem ( title, artist, defaultAlbum, trackNr, index ) );
            file.close();
        }

        /**
        *  Because there is no way to set the length for the last track in a normal way,
        *  we have to do some magic here. Having the total length of the media file given
        *  we can set the length for the last track after all the cue file was loaded into array.
        */

        cueItems[index].setLength ( trackLen - index );
        debug() << "Setting length of track " << cueItems[index].title() << " to " << trackLen - index << " msecs.";

        return cueItems;
    }
    return CueFileItemMap();
}

QUrl CueFileSupport::locateCueSheet ( const QUrl &trackurl )
{
    if ( !trackurl.isValid() || !trackurl.isLocalFile() )
        return QUrl();
    // look for the cue file that matches the media file
    QString path    = trackurl.path();
    QString cueFile = path.left ( path.lastIndexOf ( QLatin1Char('.') ) ) + QStringLiteral(".cue");

    if ( validateCueSheet ( cueFile ) )
    {
        debug() << "[CUEFILE]: " << cueFile << " - Shoot blindly, found and loaded. ";
        return QUrl::fromLocalFile( cueFile );
    }
    debug() << "[CUEFILE]: " << cueFile << " - Shoot blindly and missed, searching for other cue files.";

    bool foundCueFile = false;
    QDir dir ( trackurl.adjusted(QUrl::RemoveFilename|QUrl::StripTrailingSlash).path() );
    QStringList filters;
    filters << QStringLiteral("*.cue") << QStringLiteral("*.CUE");
    dir.setNameFilters ( filters );

    QStringList cueFilesList = dir.entryList();

    if ( !cueFilesList.empty() )
        for ( QStringList::Iterator it = cueFilesList.begin(); it != cueFilesList.end() && !foundCueFile; ++it )
        {
            QFile file ( dir.filePath ( *it ) );
            if ( file.open ( QIODevice::ReadOnly ) )
            {
                debug() << "[CUEFILE]: " << *it << " - Opened, looking for the matching FILE stanza." << Qt::endl;
                QTextStream stream ( &file );
                QString line;

                while ( !stream.atEnd() && !foundCueFile )
                {
                    line = stream.readLine().simplified();

                    if ( line.startsWith ( QLatin1String("file"), Qt::CaseInsensitive ) )
                    {
                        line = line.mid ( 5 ).remove ( QLatin1Char('"') );

                        if ( line.contains ( trackurl.fileName(), Qt::CaseInsensitive ) )
                        {
                            cueFile = dir.filePath ( *it );

                            if ( validateCueSheet ( cueFile ) )
                            {
                                debug() << "[CUEFILE]: " << cueFile << " - Looked inside cue files, found and loaded proper one" << Qt::endl;
                                foundCueFile = true;
                            }
                        }
                    }
                }

                file.close();
            }
        }

    if ( foundCueFile )
        return QUrl::fromLocalFile( cueFile );
    debug() << "[CUEFILE]: - Didn't find any matching cue file." << Qt::endl;
    return QUrl();
}

bool CueFileSupport::validateCueSheet ( const QString& cuefile )
{
    if ( !QFile::exists ( cuefile ) )
        return false;

    QFile file ( cuefile );
    int track = 0;
    QString defaultArtist;
    QString defaultAlbum;
    QString artist;
    QString title;
    long length = 0;
    long prevIndex = -1;
    bool index00Present = false;
    long index = -1;

    int mode = BEGIN;
    if ( file.open ( QIODevice::ReadOnly ) )
    {
        QTextStream stream ( &file );
        QString line;

        while ( !stream.atEnd() )
        {
            line = stream.readLine().simplified();

            if ( line.startsWith ( QLatin1String("title"), Qt::CaseInsensitive ) )
            {
                title = line.mid ( 6 ).remove ( QLatin1Char('"') );
                if ( mode == BEGIN )
                {
                    defaultAlbum = title;
                    title.clear();
                    debug() << "Album: " << defaultAlbum;
                }
                else
                    debug() << "Title: " << title;
            }

            else if ( line.startsWith ( QLatin1String("performer"), Qt::CaseInsensitive ) )
            {
                artist = line.mid ( 10 ).remove ( QLatin1Char('"') );
                if ( mode == BEGIN )
                {
                    defaultArtist = artist;
                    artist.clear();
                    debug() << "Album Artist: " << defaultArtist;
                }
                else
                    debug() << "Artist: " << artist;
            }

            else if ( line.startsWith ( QLatin1String("track"), Qt::CaseInsensitive ) )
            {
                if ( mode == TRACK_FOUND )
                {
                    // not valid, because we have to have an index for the previous track
                    file.close();
                    debug() << "Mode is TRACK_FOUND, abort.";
                    return false;
                }
                if ( mode == INDEX_FOUND )
                {
                    if ( artist.isNull() )
                        artist = defaultArtist;

                    prevIndex = index;
                    title.clear();
                    artist.clear();
                    track  = 0;
                }
                track = line.section ( QLatin1Char(' '),1,1 ).toInt();
                debug() << "Track: " << track;
                mode = TRACK_FOUND;
            }
            else if ( line.startsWith ( QLatin1String("index"), Qt::CaseInsensitive ) )
            {
                if ( mode == TRACK_FOUND )
                {
                    int indexNo = line.section ( QLatin1Char(' '),1,1 ).toInt();

                    if ( indexNo == 1 )
                    {
                        QStringList time = line.section ( QLatin1Char(' '), -1, -1 ).split ( QLatin1Char(':') );

                        index = time[0].toLong() *60*1000 + time[1].toLong() *1000 + time[2].toLong() *1000/75; //75 frames per second

                        if ( prevIndex != -1 && !index00Present ) // set the prev track's length if there is INDEX01 present, but no INDEX00
                        {
                            length = index - prevIndex;
                        }

                        index00Present = false;
                        mode = INDEX_FOUND;
                        length = 0;
                    }

                    else if ( indexNo == 0 ) // gap, use to calc prev track length
                    {
                        QStringList time = line.section ( QLatin1Char(' '), -1, -1 ).split ( QLatin1Char(':') );

                        length = time[0].toLong() *60*1000 + time[1].toLong() *1000 + time[2].toLong() *1000/75; //75 frames per second

                        if ( prevIndex != -1 )
                        {
                            length -= prevIndex; //this[prevIndex].getIndex();
                            index00Present = true;
                        }
                        else
                            length =  0;
                    }
                    else
                    {
                        debug() << "Skipping unsupported INDEX " << indexNo;
                    }
                }
                else
                {
                    // not valid, because we don't have an associated track
                    file.close();
                    debug() << "Mode is not TRACK_FOUND but encountered INDEX, abort.";
                    return false;
                }
                debug() << "index: " << index;
            }
        }

        if( mode == BEGIN )
        {
            file.close();
            debug() << "Cue file is invalid";
            return false;
        }

        if ( artist.isNull() )
            artist = defaultArtist;

        file.close();
    }
    return true;
}


Meta::TrackList
CueFileSupport::generateTimeCodeTracks( Meta::TrackPtr baseTrack, CueFileItemMap itemMap )
{
    Meta::TrackList trackList;

    for( const CueFileItem &item : itemMap )
    {
        Meta::TimecodeTrack *track = new Meta::TimecodeTrack( item.title(),
                baseTrack->playableUrl(), item.index(), item.index() + item.length() );
        track->beginUpdate();
        track->setArtist( item.artist() );
        track->setAlbum( item.album() );
        track->setTrackNumber( item.trackNumber() );
        track->endUpdate();

        trackList << Meta::TrackPtr( track );
    }

    return trackList;
}
