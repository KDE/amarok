/*
    Copyright (c) 2005 Martin Ehmke <ehmke@gmx.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define DEBUG_PREFIX "CueFile"

#include "cuefile.h"

#include "Debug.h"
#include "EngineController.h"

#include <KGlobal>
#include <KUrl>

#include <QDir>
#include <QFile>
#include <QMap>
#include <QStringList>
#include <QTextStream>


CueFile *CueFile::instance()
{
    static CueFile *s_instance = 0;

    if(!s_instance)
    {
        s_instance = new CueFile(The::engineController());
    }

    return s_instance;
}

CueFile::~CueFile()
{
    debug() << "shmack! destructed";
}


/*
+ - set and continue in the same state
x - cannot happen
track - switch to new state

state/next token >
 v         PERFORMER               TITLE           FILE            TRACK            INDEX          PREGAP
begin         +                      +             file              x                x              x
file          x                      x             file            track              x              x
track         +                      +             file              x                +              +
index         x                      x             file            track              +              x

1. Ignore FILE completely.
2. INDEX 00 is gap abs position in cue formats we care about (use it to calc length of prev track and then drop on the floor).
3. Ignore subsequent INDEX entries (INDEX 02, INDEX 03 etc). - FIXME? this behavior is different from state chart above.
4. For a valid cuefile at least TRACK and INDEX are required.
*/
enum {
    BEGIN = 0,
    TRACK_FOUND, // track found, index not yet found
    INDEX_FOUND
};


/**
* @return true if the cuefile could be successfully loaded
*/
bool CueFile::load(int mediaLength)
{
    clear();
    m_lastSeekPos = -1;

    if( QFile::exists( m_cueFileName ) )
    {
        QFile file( m_cueFileName );
        int track = 0;
        QString defaultArtist = QString();
        QString defaultAlbum = QString();
        QString artist = QString();
        QString title = QString();
        long length = 0;
        long prevIndex = -1;
        bool index00Present = false;
        long index = -1;

        int mode = BEGIN;
        if( file.open( QIODevice::ReadOnly ) )
        {
            QTextStream stream( &file );
            QString line;

            while ( !stream.atEnd() )
            {
                line = stream.readLine().simplified();

                if( line.startsWith( "title", Qt::CaseInsensitive ) )
                {
                    title = line.mid( 6 ).remove( '"' );
                    if( mode == BEGIN )
                    {
                        defaultAlbum = title;
                        title.clear();
                        debug() << "Album: " << defaultAlbum;
                    }
                    else
                        debug() << "Title: " << title;
                }

                else if( line.startsWith( "performer", Qt::CaseInsensitive ))
                {
                    artist = line.mid( 10 ).remove( '"' );
                    if( mode == BEGIN )
                    {
                        defaultArtist = artist;
                        artist.clear();
                        debug() << "Album Artist: " << defaultArtist;
                    }
                    else
                        debug() << "Artist: " << artist;
                }

                else if( line.startsWith( "track", Qt::CaseInsensitive ) )
                {
                    if( mode == TRACK_FOUND )
                    {
                        // not valid, because we have to have an index for the previous track
                        file.close();
                        debug() << "Mode is TRACK_FOUND, abort.";
                        m_valid = false;
                        return false;
                    }
                    if( mode == INDEX_FOUND )
                    {
                        if(artist.isNull())
                            artist = defaultArtist;

                        debug() << "Inserting item: " << title << " - " << artist << " on " << defaultAlbum << " (" << track << ")";
                        // add previous entry to map
                        insert( index, CueFileItem( title, artist, defaultAlbum, track, index ) );
                        prevIndex = index;
                        title.clear();
                        artist.clear();
                        track  = 0;
                    }
                    track = line.section (' ',1,1).toInt();
                    debug() << "Track: " << track;
                    mode = TRACK_FOUND;
                }
                else if( line.startsWith( "index", Qt::CaseInsensitive ) )
                {
                    if( mode == TRACK_FOUND)
                    {
                        int indexNo = line.section(' ',1,1).toInt();

                        if( indexNo == 1 )
                        {
                            QString splitMe = line.section( ' ', -1, -1 );
                            QStringList time = splitMe.split( ':', QString::SkipEmptyParts );

                            index = time[0].toLong()*60*1000 + time[1].toLong()*1000 + time[2].toLong()*1000/75; //75 frames per second

                             if( prevIndex != -1 && !index00Present ) // set the prev track's length if there is INDEX01 present, but no INDEX00
                            {
                            	length = index - prevIndex;
                            	debug() << "Setting length of track " << (*this)[prevIndex].getTitle() << " to " << length << " msecs.";
                            	(*this)[prevIndex].setLength(length);
                            }

                            index00Present = false;
                            mode = INDEX_FOUND;
                            length = 0;
                        }

                        else if( indexNo == 0 ) // gap, use to calc prev track length
                        {
                            QString splitMe = line.section( ' ', -1, -1 );
                            QStringList time = splitMe.split( ':', QString::SkipEmptyParts );

                            length = time[0].toLong()*60*1000 + time[1].toLong()*1000 + time[2].toLong()*1000/75; //75 frames per second

                            if( prevIndex != -1 )
                            {
                            	length -= prevIndex; //this[prevIndex].getIndex();
                            	debug() << "Setting length of track " << (*this)[prevIndex].getTitle() << " to " << length << " msecs.";
                            	(*this)[prevIndex].setLength(length);
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
                        m_valid = false;
                        return false;
                    }
                    debug() << "index: " << index;
                }
            }

            if(artist.isNull())
                artist = defaultArtist;

            debug() << "Inserting item: " << title << " - " << artist << " on " << defaultAlbum << " (" << track << ")";
            // add previous entry to map
            insert( index, CueFileItem( title, artist, defaultAlbum, track, index ) );
            file.close();
        }

        /**
            *  Because there is no way to set the length for the last track in a normal way,
            *  we have to do some magic here. Having the total length of the media file given
            *  we can set the lenth for the last track after all the cue file was loaded into array.
            */

        (*this)[index].setLength(mediaLength*1000 - index);
        debug() << "Setting length of track " << (*this)[index].getTitle() << " to " << mediaLength*1000 - index << " msecs.";
        m_valid = true;
        return true;
    } else {
        m_valid = true;
        return false;
    }
}

void CueFile::engineTrackPositionChanged( long position, bool userSeek )
{
    Q_UNUSED( position ); Q_UNUSED( userSeek );
    //TODO: port 2.0 reimplement this
   /* position /= 1000;
    if(userSeek || position > m_lastSeekPos)
    {
        CueFile::Iterator it = end();
        while( it != begin() )
        {
            --it;
//            debug() << "Checking " << position << " against pos " << it.key()/1000 << " title " << it.data().getTitle();
            if(it.key()/1000 <= position)
            {
                Meta::TrackPtr currenttrack = The::engineController()->currentTrack();

                if(it.value().getTitle() != currenttrack->name()
                   || it.value().getArtist() != currenttrack->artist()->name()
                   || it.value().getAlbum() != currenttrack->album()->name()
                   || it.value().getTrackNumber() != currenttrack->trackNumber())
                {
                    bundle.setTitle(it.value().getTitle());
                    bundle.setArtist(it.value().getArtist());
                    bundle.setAlbum(it.value().getAlbum());
                    bundle.setTrack(it.value().getTrackNumber());
                    emit metaData(bundle);

                    long length = it.value().getLength();
                    if ( length == -1 ) // need to calculate
                    {
                        ++it;
                        long nextKey = it == end() ? bundle.length() * 1000 : it.key();
                        --it;
                        length = qMax( nextKey - it.key(), 0L );
                    }
                    emit newCuePoint( position, it.key() / 1000, ( it.key() + length ) / 1000 );
                }
                break;
            }
        }
    }

    m_lastSeekPos = position;*/
}

CueFile* CueFile::findCueForUrl( const QString &url, int mediaLength )
{
    KUrl kurl( url );
    CueFile *cue = CueFile::instance();
    if ( kurl.isLocalFile() )
        {

            /** The cue file that is provided with the media might have different name than the
             * media file itself, hence simply cutting the media extension and adding ".cue"
             * is not always enough to find the matching cue file. In such cases we have
             * to search for all the cue files in the directory and have a look inside them for
             * the matching FILE="" stanza. However the FILE="" stanza does not always
             * point at the corresponding media file (e.g. it is quite often set to the misleading
             * FILE="audio.wav" WAV). Therfore we also have to check blindly if there is a cue
             * file having the same name as the media file played, as described above.
             */

            // look for the cue file that matches the media file played first
            QString path    = kurl.path();
            QString cueFile = path.left( path.lastIndexOf( '.' ) ) + ".cue";


            cue->setCueFileName( cueFile );

            if( cue->load( mediaLength ) )
                debug() << "[CUEFILE]: " << cueFile << " - Shoot blindly, found and loaded. " << endl;

            // if unlucky, let's have a look inside cue files, if any
            else
            {
                debug() << "[CUEFILE]: " << cueFile << " - Shoot blindly and missed, searching for other cue files." << endl;

                bool foundCueFile = false;
                QDir dir ( kurl.directory() );
                dir.setFilter( QDir::Files ) ;
                QStringList filters;
                filters << "*.cue" << "*.CUE";
                dir.setNameFilters( filters ) ;

                QStringList cueFilesList = dir.entryList();

                if ( !cueFilesList.empty() )
                    for ( QStringList::Iterator it = cueFilesList.begin(); it != cueFilesList.end() && !foundCueFile; ++it )
                    {
                        QFile file ( dir.filePath(*it) );
                        if( file.open( QIODevice::ReadOnly ) )
                        {
                            debug() << "[CUEFILE]: " << *it << " - Opened, looking for the matching FILE stanza." << endl;
                            QTextStream stream( &file );
                            QString line;

                            while ( !stream.atEnd() && !foundCueFile)
                            {
                                line = stream.readLine().simplified();

                                if( line.startsWith( "file", Qt::CaseInsensitive ) )
                                {
                                    line = line.mid( 5 ).remove( '"' );

                                    if ( line.contains( path, Qt::CaseInsensitive ) )
                                    {
                                        cueFile = dir.filePath(*it);
                                        foundCueFile = true;
                                        cue->setCueFileName( cueFile );
                                        if( cue->load( mediaLength ) )
                                            debug() << "[CUEFILE]: " << cueFile << " - Looked inside cue files, found and loaded proper one" << endl;
                                    }
                                }
                            }

                            file.close();
                        }
                    }

                if ( !foundCueFile )
                    debug() << "[CUEFILE]: - Did not find any matching cue file." << endl;
            }
        }
        return cue;
}

bool CueFile::isValid()
{
    return m_valid;
}

#include "cuefile.moc"



