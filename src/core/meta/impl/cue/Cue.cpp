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
#include "core/meta/Meta.h"
#include "core/capabilities/TimecodeLoadCapability.h"
#include "amarokurls/PlayUrlRunner.h"
#include "amarokurls/PlayUrlGenerator.h"
#include "amarokurls/BookmarkMetaActions.h"

#include <KEncodingProber>

#include <QAction>
#include <QDir>
#include <QFile>
#include <QMapIterator>
#include <QPointer>
#include <QTextCodec>

using namespace MetaCue;

namespace MetaCue {

class TimecodeLoadCapabilityImpl : public Capabilities::TimecodeLoadCapability
{
    public:
        TimecodeLoadCapabilityImpl( Track *track )
        : Capabilities::TimecodeLoadCapability()
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

            while ( it.hasNext() )
            {
                it.next();
                debug() << " seconds : " << it.key();

                AmarokUrl aUrl;
                rUrl = PlayUrlGenerator::instance()->createTrackBookmark( Meta::TrackPtr( m_track.data() ), it.key(), it.value().getTitle() );
                AmarokUrlPtr url( new AmarokUrl( aUrl.url() ) );
                url->setName( aUrl.name() ); // TODO AmarokUrl should really have a copy constructor

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

    m_cueitems = CueFileSupport::loadCueFile( this );
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
Track::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    return type == Capabilities::Capability::LoadTimecode;
}

Capabilities::Capability*
Track::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::LoadTimecode:
            return new MetaCue::TimecodeLoadCapabilityImpl( this );
        default:
            return 0;
    }
}

