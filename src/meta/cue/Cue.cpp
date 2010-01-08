/****************************************************************************************
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "Cue.h"
#include "Cue_p.h"
#include "Cue_p.moc"

#include "Debug.h"
#include "Meta.h"
#include "meta/capabilities/TimecodeLoadCapability.h"
#include "amarokurls/PlayUrlRunner.h"
#include "amarokurls/PlayUrlGenerator.h"
#include "amarokurls/BookmarkMetaActions.h"

#include <KEncodingProber>
#include <KSharedPtr>

#include <QAction>
#include <QDir>
#include <QFile>
#include <QMapIterator>
#include <QPointer>
#include <QTextCodec>

using namespace MetaCue;
namespace MetaCue {
class TimecodeLoadCapabilityImpl : public Meta::TimecodeLoadCapability
{
    public:
        TimecodeLoadCapabilityImpl( Track *track )
        : Meta::TimecodeLoadCapability()
        , m_track( track )
        {}

        virtual bool hasTimecodes()
        {
            if ( loadTimecodes().size() > 0 )
                return true;
            return false;
        }

        virtual BookmarkList loadTimecodes()
        {
            DEBUG_BLOCK
            CueFileItemMap map = m_track->cueItems();
            debug() << " cue has " << map.size() << " entries";
            QMapIterator<long, CueFileItem> it( map );
            BookmarkList list;
            while ( it.hasNext() ) {
                it.next();
                debug() << " seconds : " << it.key();
                AmarokUrl aurl = PlayUrlGenerator::instance()->createTrackBookmark( Meta::TrackPtr( m_track.data() ), it.key(), it.value().getTitle() );
                AmarokUrlPtr url( new AmarokUrl( aurl.url() ) );
                url->setName( aurl.name() ); // TODO AmarokUrl should really have a copy constructor
                list << url;
            }
            return list;
        }

    private:
        KSharedPtr<Track> m_track;
};
}
Track::Track ( const KUrl &url, const KUrl &cuefile )
        : MetaFile::Track ( url )
        , EngineObserver ( The::engineController() )
        , m_cuefile ( cuefile )
        , m_lastSeekPos ( -1 )
        , m_cueitems()
        , d ( new Track::Private ( this ) )
{
    DEBUG_BLOCK

    d->url = url;
    d->artistPtr = Meta::ArtistPtr ( new CueArtist ( QPointer<Track::Private> ( d ) ) );
    d->albumPtr = Meta::AlbumPtr ( new CueAlbum ( QPointer<Track::Private> ( d ) ) );

    setTitle ( MetaFile::Track::name() );
    setArtist ( MetaFile::Track::artist()->name() );
    setAlbum ( MetaFile::Track::album()->name() );
    setTrackNumber ( MetaFile::Track::trackNumber() );

    load ( MetaFile::Track::length() );
}

Track::~Track()
{
    delete d;
}

CueFileItemMap Track::cueItems() const
{
    return m_cueitems;
}

void Track::subscribe ( Meta::Observer *observer )
{
    DEBUG_BLOCK
    debug() << "Adding observer: " << observer;
    m_observers.insert ( observer );
}

void Track::unsubscribe ( Meta::Observer *observer )
{
    DEBUG_BLOCK
    debug() << "Removing observer: " << observer;
    m_observers.remove ( observer );
}

void Track::notify() const
{
    foreach ( Meta::Observer *observer, m_observers )
    {
//         debug() << "Notifying observer: " << observer;
        observer->metadataChanged ( Meta::TrackPtr ( Meta::TrackPtr ( const_cast<MetaCue::Track*> ( d->track ) ) ) );
    }
}

void Track::engineTrackPositionChanged( qint64 position, bool userSeek )
{
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if ( !currentTrack || currentTrack->playableUrl().url() != MetaFile::Track::playableUrl().url() )
        return;
//     debug() << "userSeek: " << userSeek << " pos: " << position << " lastseekPos: " << m_lastSeekPos;
    if ( userSeek || position > m_lastSeekPos )
    {
        CueFileItemMap::Iterator it = m_cueitems.end();
        while ( it != m_cueitems.begin() )
        {
            --it;
//             debug() << "Checking " << position << " against pos " << it.key()/1000 << " title " << (*it).getTitle() << endl;
            if ( it.key() <= position )
            {
//                 debug() << "\tcurr: " << currentTrack->artist()->name() << " " << currentTrack->album()->name() << " " <<  currentTrack->name() << " "<< currentTrack->trackNumber();
//                 debug() << "\titer: " << (*it).getArtist() << " " << (*it).getAlbum() << " " << (*it).getTitle()<< " " << (*it).getTrackNumber();
                bool artistCheck = currentTrack->artist() && currentTrack->artist()->name() != ( *it ).getArtist();
                bool albumCheck = currentTrack->album() && currentTrack->album()->name() != ( *it ).getAlbum();
                bool titleCheck = ( *it ).getTitle() != currentTrack->name();
                bool trackNumCheck = ( *it ).getTrackNumber() != currentTrack->trackNumber();
                if ( artistCheck || albumCheck || titleCheck || trackNumCheck )
                {
                    setTitle ( ( *it ).getTitle() );
                    setArtist ( ( *it ).getArtist() );
                    setAlbum ( ( *it ).getAlbum() );
                    setTrackNumber ( ( *it ).getTrackNumber() );

                    long length = ( *it ).getLength();
                    if ( length == -1 ) // need to calculate
                    {
                        ++it;
                        long nextKey = it == m_cueitems.end() ? currentTrack->length() : it.key();
                        --it;
                        length = qMax ( nextKey - it.key(), 0L );
                    }
                    //emit newCuePoint( position, it.key() / 1000, ( it.key() + length ) / 1000 );
                    notify();
                }
                break;
            }
        }
    }

    m_lastSeekPos = position;
}

/**
* Parses a cue sheet file into CueFileItems and inserts them in a QMap
* @return true if the cuefile could be successfully loaded
* @author (C) 2005 by Martin Ehmke <ehmke@gmx.de>
*/
bool Track::load ( qint64 mediaLength )
{
    DEBUG_BLOCK
    m_cueitems.clear();
    m_lastSeekPos = -1;

    debug() << "CUEFILE: " << m_cuefile.pathOrUrl();
    if ( QFile::exists ( m_cuefile.pathOrUrl() ) )
    {
        debug() << "  EXISTS!";
        QFile file ( m_cuefile.pathOrUrl() );
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
            KEncodingProber prober;

            KEncodingProber::ProberState result = prober.feed( file.readAll() );
            file.seek( 0 );

            if( result != KEncodingProber::NotMe )
                stream.setCodec( QTextCodec::codecForName( prober.encoding() ) );

            debug() << "Encoding: " << prober.encoding();

            while ( !stream.atEnd() )
            {
                line = stream.readLine().simplified();

                if ( line.startsWith ( "title", Qt::CaseInsensitive ) )
                {
                    title = line.mid ( 6 ).remove ( '"' );
                    if ( mode == BEGIN )
                    {
                        defaultAlbum = title;
                        title.clear();
                        debug() << "Album: " << defaultAlbum;
                    }
                    else
                        debug() << "Title: " << title;
                }

                else if ( line.startsWith ( "performer", Qt::CaseInsensitive ) )
                {
                    artist = line.mid ( 10 ).remove ( '"' );
                    if ( mode == BEGIN )
                    {
                        defaultArtist = artist;
                        artist.clear();
                        debug() << "Album Artist: " << defaultArtist;
                    }
                    else
                        debug() << "Artist: " << artist;
                }

                else if ( line.startsWith ( "track", Qt::CaseInsensitive ) )
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

                        debug() << "Inserting item: " << title << " - " << artist << " on " << defaultAlbum << " (" << track << ")";
                        // add previous entry to map
                        m_cueitems.insert ( index, CueFileItem ( title, artist, defaultAlbum, track, index ) );
                        prevIndex = index;
                        title.clear();
                        artist.clear();
                        track  = 0;
                    }
                    track = line.section ( ' ',1,1 ).toInt();
                    debug() << "Track: " << track;
                    mode = TRACK_FOUND;
                }
                else if ( line.startsWith ( "index", Qt::CaseInsensitive ) )
                {
                    if ( mode == TRACK_FOUND )
                    {
                        int indexNo = line.section ( ' ',1,1 ).toInt();

                        if ( indexNo == 1 )
                        {
                            QStringList time = line.section ( ' ', -1, -1 ).split ( ':' );

                            index = time[0].toLong() *60*1000 + time[1].toLong() *1000 + time[2].toLong() *1000/75; //75 frames per second

                            if ( prevIndex != -1 && !index00Present ) // set the prev track's length if there is INDEX01 present, but no INDEX00
                            {
                                length = index - prevIndex;
                                debug() << "Setting length of track " << m_cueitems[prevIndex].getTitle() << " to " << length << " msecs.";
                                m_cueitems[prevIndex].setLength ( length );
                            }

                            index00Present = false;
                            mode = INDEX_FOUND;
                            length = 0;
                        }

                        else if ( indexNo == 0 ) // gap, use to calc prev track length
                        {
                            QStringList time = line.section ( ' ', -1, -1 ).split ( ':' );

                            length = time[0].toLong() * 60 * 1000 + time[1].toLong() * 1000 + time[2].toLong() *1000/75; //75 frames per second

                            if ( prevIndex != -1 )
                            {
                                length -= prevIndex; //this[prevIndex].getIndex();
                                debug() << "Setting length of track " << m_cueitems[prevIndex].getTitle() << " to " << length << " msecs.";
                                m_cueitems[prevIndex].setLength ( length );
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

            if ( artist.isNull() )
                artist = defaultArtist;

            debug() << "Inserting item: " << title << " - " << artist << " on " << defaultAlbum << " (" << track << ")";
            // add previous entry to map
            m_cueitems.insert ( index, CueFileItem ( title, artist, defaultAlbum, track, index ) );
            file.close();
        }

        /**
        *  Because there is no way to set the length for the last track in a normal way,
        *  we have to do some magic here. Having the total length of the media file given
        *  we can set the lenth for the last track after all the cue file was loaded into array.
        */

        m_cueitems[index].setLength ( mediaLength - index );
        debug() << "Setting length of track " << m_cueitems[index].getTitle() << " to " << mediaLength - index << " msecs.";

        return true;
    }
    return false;
}

KUrl Track::locateCueSheet ( const KUrl &trackurl )
{
    if ( !trackurl.isValid() || !trackurl.isLocalFile() )
        return KUrl();
    // look for the cue file that matches the media file
    QString path    = trackurl.path();
    QString cueFile = path.left ( path.lastIndexOf ( '.' ) ) + ".cue";

    if ( Track::validateCueSheet ( cueFile ) )
    {
        debug() << "[CUEFILE]: " << cueFile << " - Shoot blindly, found and loaded. ";
        return KUrl ( cueFile );
    }
    debug() << "[CUEFILE]: " << cueFile << " - Shoot blindly and missed, searching for other cue files.";

    bool foundCueFile = false;
    QDir dir ( trackurl.directory() );
    QStringList filters;
    filters << "*.cue" << "*.CUE";
    dir.setNameFilters ( filters );

    QStringList cueFilesList = dir.entryList();

    if ( !cueFilesList.empty() )
        for ( QStringList::Iterator it = cueFilesList.begin(); it != cueFilesList.end() && !foundCueFile; ++it )
        {
            QFile file ( dir.filePath ( *it ) );
            if ( file.open ( QIODevice::ReadOnly ) )
            {
                debug() << "[CUEFILE]: " << *it << " - Opened, looking for the matching FILE stanza." << endl;
                QTextStream stream ( &file );
                QString line;

                while ( !stream.atEnd() && !foundCueFile )
                {
                    line = stream.readLine().simplified();

                    if ( line.startsWith ( "file", Qt::CaseInsensitive ) )
                    {
                        line = line.mid ( 5 ).remove ( '"' );

                        if ( line.contains ( trackurl.fileName(), Qt::CaseInsensitive ) )
                        {
                            cueFile = dir.filePath ( *it );

                            if ( Track::validateCueSheet ( cueFile ) )
                            {
                                debug() << "[CUEFILE]: " << cueFile << " - Looked inside cue files, found and loaded proper one" << endl;
                                foundCueFile = true;
                            }
                        }
                    }
                }

                file.close();
            }
        }

    if ( foundCueFile )
        return KUrl ( cueFile );
    debug() << "[CUEFILE]: - Didn't find any matching cue file." << endl;
    return KUrl();
}

bool Track::validateCueSheet ( const QString& cuefile )
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

    int mode = Track::BEGIN;
    if ( file.open ( QIODevice::ReadOnly ) )
    {
        QTextStream stream ( &file );
        QString line;

        while ( !stream.atEnd() )
        {
            line = stream.readLine().simplified();

            if ( line.startsWith ( "title", Qt::CaseInsensitive ) )
            {
                title = line.mid ( 6 ).remove ( '"' );
                if ( mode == Track::BEGIN )
                {
                    defaultAlbum = title;
                    title.clear();
                    debug() << "Album: " << defaultAlbum;
                }
                else
                    debug() << "Title: " << title;
            }

            else if ( line.startsWith ( "performer", Qt::CaseInsensitive ) )
            {
                artist = line.mid ( 10 ).remove ( '"' );
                if ( mode == Track::BEGIN )
                {
                    defaultArtist = artist;
                    artist.clear();
                    debug() << "Album Artist: " << defaultArtist;
                }
                else
                    debug() << "Artist: " << artist;
            }

            else if ( line.startsWith ( "track", Qt::CaseInsensitive ) )
            {
                if ( mode == Track::TRACK_FOUND )
                {
                    // not valid, because we have to have an index for the previous track
                    file.close();
                    debug() << "Mode is TRACK_FOUND, abort.";
                    return false;
                }
                if ( mode == Track::INDEX_FOUND )
                {
                    if ( artist.isNull() )
                        artist = defaultArtist;

                    prevIndex = index;
                    title.clear();
                    artist.clear();
                    track  = 0;
                }
                track = line.section ( ' ',1,1 ).toInt();
                debug() << "Track: " << track;
                mode = Track::TRACK_FOUND;
            }
            else if ( line.startsWith ( "index", Qt::CaseInsensitive ) )
            {
                if ( mode == Track::TRACK_FOUND )
                {
                    int indexNo = line.section ( ' ',1,1 ).toInt();

                    if ( indexNo == 1 )
                    {
                        QStringList time = line.section ( ' ', -1, -1 ).split ( ':' );

                        index = time[0].toLong() *60*1000 + time[1].toLong() *1000 + time[2].toLong() *1000/75; //75 frames per second

                        if ( prevIndex != -1 && !index00Present ) // set the prev track's length if there is INDEX01 present, but no INDEX00
                        {
                            length = index - prevIndex;
                        }

                        index00Present = false;
                        mode = Track::INDEX_FOUND;
                        length = 0;
                    }

                    else if ( indexNo == 0 ) // gap, use to calc prev track length
                    {
                        QStringList time = line.section ( ' ', -1, -1 ).split ( ':' );

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

        if ( artist.isNull() )
            artist = defaultArtist;

        file.close();
    }
    return true;
}

QString
Track::name() const
{
    return d->title;
}

QString
Track::prettyName() const
{
    return name();
}

QString
Track::fullPrettyName() const
{
    return name();
}

QString
Track::sortableName() const
{
    return name();
}

Meta::AlbumPtr
Track::album() const
{
    return d->albumPtr;
}

Meta::ArtistPtr
Track::artist() const
{
    return d->artistPtr;
}


void
Track::setAlbum ( const QString &newAlbum )
{
    d->album = newAlbum;
}

void
Track::setArtist ( const QString& newArtist )
{
    d->artist = newArtist;
}

void
Track::setTitle ( const QString &newTitle )
{
    d->title = newTitle;
}

int
Track::trackNumber() const
{
    return d->tracknumber;
}

void
Track::setTrackNumber ( int newTrackNumber )
{
    d->tracknumber = newTrackNumber;
}

qint64
Track::length() const
{
    return d->length;
}

bool
Track::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    return type == Meta::Capability::LoadTimecode;
}

Meta::Capability*
Track::createCapabilityInterface( Meta::Capability::Type type )
{
    switch( type )
    {
        case Meta::Capability::LoadTimecode:
            return new MetaCue::TimecodeLoadCapabilityImpl( this );
        default:
            return 0;
    }
}

