/*
 *  Copyright (c) 2009,2010 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#define DEBUG_PREFIX "ProxyCollectionMeta"

#include "ProxyCollectionMeta.h"

#include "core/meta/support/MetaUtility.h"
#include "core/capabilities/EditCapability.h"
#include "ProxyCollection.h"
#include "SvgHandler.h"

#include "core/support/Debug.h"

#include <QDateTime>
#include <QSet>
#include <QTimer>

#define FORWARD( call ) { foreach( Capabilities::EditCapability *ec, m_ec ) { ec->call; } \
                            if( !m_batchMode ) QTimer::singleShot( 0, m_collection, SLOT( slotUpdated() ) ); }

class ProxyEditCapability : public Capabilities::EditCapability
{
public:
    ProxyEditCapability( ProxyCollection::Collection *coll, const QList<Capabilities::EditCapability*> &ecs )
        : Capabilities::EditCapability()
        , m_batchMode( false )
        , m_collection( coll )
        , m_ec( ecs ) {}
    virtual ~ProxyEditCapability() { qDeleteAll( m_ec ); }

    void beginMetaDataUpdate()
    {
        m_batchMode = true;
        foreach( Capabilities::EditCapability *ec, m_ec ) ec->beginMetaDataUpdate();
    }
    void endMetaDataUpdate()
    {
        foreach( Capabilities::EditCapability *ec, m_ec ) ec->endMetaDataUpdate();
        m_batchMode = false;
        QTimer::singleShot( 0, m_collection, SLOT( slotUpdated() ) );
    }
    void abortMetaDataUpdate()
    {
        foreach( Capabilities::EditCapability *ec, m_ec ) ec->abortMetaDataUpdate();
        m_batchMode = false;
    }
    void setComment( const QString &newComment ) { FORWARD( setComment( newComment ) ) }
    void setTrackNumber( int newTrackNumber ) { FORWARD( setTrackNumber( newTrackNumber ) ) }
    void setDiscNumber( int newDiscNumber ) { FORWARD( setDiscNumber( newDiscNumber ) ) }
    void setBpm( qreal newBpm ) { FORWARD( setBpm( newBpm ) ) }
    void setTitle( const QString &newTitle ) { FORWARD( setTitle( newTitle ) ) }
    void setArtist( const QString &newArtist ) { FORWARD( setArtist( newArtist ) ) }
    void setAlbum( const QString &newAlbum ) { FORWARD( setAlbum( newAlbum ) ) }
    void setGenre( const QString &newGenre ) { FORWARD( setGenre( newGenre ) ) }
    void setComposer( const QString &newComposer ) { FORWARD( setComposer( newComposer ) ) }
    void setYear( const QString &newYear ) { FORWARD( setYear( newYear ) ) }
    bool isEditable() const
    {
        foreach( Capabilities::EditCapability *ec, m_ec )
        {
            if( !ec->isEditable() )
                return false;
        }
        return true;
    }
private:
    bool m_batchMode;
    ProxyCollection::Collection *m_collection;
    QList<Capabilities::EditCapability*> m_ec;
};

#undef FORWARD

ProxyCollection::Track::Track( ProxyCollection::Collection *coll, const Meta::TrackPtr &track )
        : Meta::Track()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( track->name() )
        , m_album( 0 )
        , m_artist( 0 )
        , m_genre( 0 )
        , m_composer( 0 )
        , m_year( 0 )
{
    subscribeTo( track );
    m_tracks.append( track );

    if( track->album() )
        m_album = Meta::AlbumPtr( m_collection->getAlbum( track->album() ) );
    if( track->artist() )
        m_artist = Meta::ArtistPtr( m_collection->getArtist( track->artist() ) );
    if( track->genre() )
        m_genre = Meta::GenrePtr( m_collection->getGenre( track->genre() ) );
    if( track->composer() )
        m_composer = Meta::ComposerPtr( m_collection->getComposer( track->composer() ) );
    if( track->year() )
        m_year = Meta::YearPtr( m_collection->getYear( track->year() ) );
}

ProxyCollection::Track::~Track()
{
}

QString
ProxyCollection::Track::name() const
{
    return m_name;
}

QString
ProxyCollection::Track::prettyName() const
{
    return m_name;
}

QString
ProxyCollection::Track::sortableName() const
{
    if( !m_tracks.isEmpty() )
        return m_tracks.first()->sortableName();

    return m_name;
}

KUrl
ProxyCollection::Track::playableUrl() const
{
    Meta::TrackPtr bestPlayableTrack;
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( track->isPlayable() )
        {
            bool local = track->playableUrl().isLocalFile();
            if( local )
            {
                bestPlayableTrack = track;
                break;
            }
            else
            {
                //we might want to add some more sophisticated logic to figure out
                //the best remote track to play, but this works for now
                bestPlayableTrack = track;
            }
        }
    }
    if( bestPlayableTrack )
        return bestPlayableTrack->playableUrl();

    return KUrl();
}

QString
ProxyCollection::Track::prettyUrl() const
{
    if( m_tracks.count() == 1 )
    {
        return m_tracks.first()->prettyUrl();
    }
    else
    {
        return QString();
    }
}

QString
ProxyCollection::Track::uidUrl() const
{
    //this is where it gets interesting
    //a uidUrl for a proxyTrack probably has to be generated
    //from the parts of the key in ProxyCollection::Collection
    //need to think about this some more
    return QString();
}

bool
ProxyCollection::Track::isPlayable() const
{
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( track->isPlayable() )
            return true;
    }
    return false;
}

Meta::AlbumPtr
ProxyCollection::Track::album() const
{
    return m_album;
}

Meta::ArtistPtr
ProxyCollection::Track::artist() const
{
    return m_artist;
}

Meta::ComposerPtr
ProxyCollection::Track::composer() const
{
    return m_composer;
}

Meta::GenrePtr
ProxyCollection::Track::genre() const
{
    return m_genre;
}

Meta::YearPtr
ProxyCollection::Track::year() const
{
    return m_year;
}

QString
ProxyCollection::Track::comment() const
{
    //try to return something sensible here...
    //do not show a comment if the internal tracks disagree about the comment
    QString comment;
    if( !m_tracks.isEmpty() )
        comment = m_tracks.first()->comment();

    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( track->comment() != comment )
        {
            comment.clear();
            break;
        }
    }
    return comment;
}

qreal
ProxyCollection::Track::bpm() const
{
    //Similar to comment(), try to return something sensible here...
    //do not show a common bpm value if the internal tracks disagree about the bpm
    qreal bpm = -1.0;
    if( !m_tracks.isEmpty() )
        bpm = m_tracks.first()->bpm();

    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( track->bpm() != bpm )
        {
            bpm = -1.0;
            break;
        }
    }
    return bpm;
}

double
ProxyCollection::Track::score() const
{
    //again, multiple ways to implement this method:
    //return the maximum score, the minimum score, the average
    //the score of the track with the maximum play count,
    //or an average weighted by play count. And probably a couple of ways that
    //I cannot think of right now...

    //implementing the weighted average here...
    double weightedSum = 0.0;
    int totalCount = 0;
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        totalCount += track->playCount();
        weightedSum += track->playCount() * track->score();
    }
    if( totalCount )
        return weightedSum / totalCount;

    return 0.0;
}

void
ProxyCollection::Track::setScore( double newScore )
{
    foreach( Meta::TrackPtr track, m_tracks )
    {
        track->setScore( newScore );
    }
}

int
ProxyCollection::Track::rating() const
{
    //yay, multiple options again. As this has to be defined by the user, let's take
    //the maximum here.
    int result = 0;
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( track->rating() > result )
            result = track->rating();
    }
    return result;
}

void
ProxyCollection::Track::setRating( int newRating )
{
    foreach( Meta::TrackPtr track, m_tracks )
    {
        track->setRating( newRating );
    }
}

uint
ProxyCollection::Track::firstPlayed() const
{
    uint result = 0;
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        //use the track's firstPlayed value if it represents an earlier timestamp than
        //the current result, or use it directly if result has not been set yet
        //this should result in the earliest timestamp for first play of all internal
        //tracks being returned
        if( ( track->firstPlayed() && result && track->firstPlayed() < result ) || ( track->firstPlayed() && !result ) )
        {
            result = track->firstPlayed();
        }
    }
    return result;
}

uint
ProxyCollection::Track::lastPlayed() const
{
    uint result = 0;
    //return the latest timestamp. Easier than firstPlayed because we do not have to
    //care about 0.
    //when are we going to perform the refactoring as discussed in Berlin?
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( track->lastPlayed() > result )
        {
            result = track->lastPlayed();
        }
    }
    return result;
}

int
ProxyCollection::Track::playCount() const
{
    //hm, there are two ways to implement this:
    //show the sum of all play counts, or show the maximum of all play counts.
    int result = 0;
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( track->playCount() > result )
        {
            result = track->playCount();
        }
    }
    return result;
}

void
ProxyCollection::Track::finishedPlaying( double playedFraction )
{
    foreach( Meta::TrackPtr track, m_tracks )
    {
        track->finishedPlaying( playedFraction );
    }
}

qint64
ProxyCollection::Track::length() const
{
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( track->length() )
            return track->length();
    }
    return 0;
}

int
ProxyCollection::Track::filesize() const
{
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( track->filesize() )
        {
            return track->filesize();
        }
    }
    return 0;
}

int
ProxyCollection::Track::sampleRate() const
{
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( track->sampleRate() )
            return track->sampleRate();
    }
    return 0;
}

int
ProxyCollection::Track::bitrate() const
{
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( track->bitrate() )
            return track->bitrate();
    }
    return 0;
}

QDateTime
ProxyCollection::Track::createDate() const
{
    QDateTime result;
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        QDateTime dt = track->createDate();
        if( !dt.isValid() )
            continue;
        else
        {
            if( !result.isValid() || dt < result )
                result = dt;
        }
    }
    return result;
}

int
ProxyCollection::Track::trackNumber() const
{
    int result = 0;
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( ( !result && track->trackNumber() ) || ( result && result == track->trackNumber() ) )
        {
            result = track->trackNumber();
        }
        else if( result && result != track->trackNumber() )
        {
            //tracks disagree about the tracknumber
            return 0;
        }
    }
    return result;
}

int
ProxyCollection::Track::discNumber() const
{
    int result = 0;
    foreach( const Meta::TrackPtr &track, m_tracks )
    {
        if( ( !result && track->discNumber() ) || ( result && result == track->discNumber() ) )
        {
            result = track->discNumber();
        }
        else if( result && result != track->discNumber() )
        {
            //tracks disagree about the disc number
            return 0;
        }
    }
    return result;
}

QString
ProxyCollection::Track::type() const
{
    if( m_tracks.size() == 1 )
    {
        return m_tracks.first()->type();
    }
    else
    {
        //TODO: figure something out
        return QString();
    }
}

Amarok::Collection*
ProxyCollection::Track::collection() const
{
    return m_collection;
}

bool
ProxyCollection::Track::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    if( m_tracks.count() == 1 )
    {
        //if we proxy only one track, simply return the tracks capability directly
        return m_tracks.at( 0 )->hasCapabilityInterface( type );
    }
    else
    {
        //if there is more than one track, check all tracks for the given
        //capability if and only if ProxyCollection::Track supports it as well

        //as there are no supported capabilities yet...
        switch( type )
        {
        case Capabilities::Capability::Editable:
            {
                foreach( const Meta::TrackPtr &track, m_tracks )
                {
                    if( !track->hasCapabilityInterface( type ) )
                        return false;
                }
                return true;
            }
        default:
            return false;
        }
    }
}

Capabilities::Capability*
ProxyCollection::Track::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_tracks.count() == 1 )
    {
        Meta::TrackPtr track = m_tracks.at( 0 );
        return track->createCapabilityInterface( type );
    }
    else
    {
        switch( type )
        {
        case Capabilities::Capability::Editable:
            {
                QList<Capabilities::EditCapability*> ecs;
                foreach( Meta::TrackPtr track, m_tracks )
                {
                    Capabilities::EditCapability *ec = track->create<Capabilities::EditCapability>();
                    if( ec )
                        ecs << ec;
                    else
                    {
                        qDeleteAll( ecs );
                        return 0;
                    }
                }
                return new ProxyEditCapability( m_collection, ecs );
            }
        default:
            return 0;
        }
    }
}


void
ProxyCollection::Track::add( const Meta::TrackPtr &track )
{
    if( !track || m_tracks.contains( track ) )
        return;

    m_tracks.append( track );
    subscribeTo( track );

    notifyObservers();
}

void
ProxyCollection::Track::metadataChanged( Meta::TrackPtr track )
{
    if( !track )
        return;

    if( !m_tracks.contains( track ) )
    {
        //why are we subscribed?
        unsubscribeFrom( track );
        return;
    }

    const TrackKey myKey = Meta::keyFromTrack( Meta::TrackPtr( this ) );
    const TrackKey otherKey = Meta::keyFromTrack( track );
    if( myKey == otherKey )
    {
        //no key relevant metadata did change
        notifyObservers();
        return;
    }
    else
    {
        if( m_tracks.size() == 1 )
        {
            if( m_collection->hasTrack( otherKey ) )
            {
                unsubscribeFrom( track );
                m_collection->getTrack( track );
                m_tracks.removeAll( track );
                m_collection->removeTrack( myKey );
                return; //do not notify observers, this track is not valid anymore!
            }
            else
            {
                m_name = track->name();
                if( track->album() )
                     m_album = Meta::AlbumPtr( m_collection->getAlbum( track->album() ) );
                if( track->artist() )
                    m_artist = Meta::ArtistPtr( m_collection->getArtist( track->artist() ) );
                if( track->genre() )
                    m_genre = Meta::GenrePtr( m_collection->getGenre( track->genre() ) );
                if( track->composer() )
                    m_composer = Meta::ComposerPtr( m_collection->getComposer( track->composer() ) );
                if( track->year() )
                    m_year = Meta::YearPtr( m_collection->getYear( track->year() ) );

                m_collection->setTrack( this );
                m_collection->removeTrack( myKey );
            }
        }
        else
        {
            unsubscribeFrom( track );
            m_collection->getTrack( track );
            m_tracks.removeAll( track );
        }
        notifyObservers();
    }
}

ProxyCollection::Album::Album( ProxyCollection::Collection *coll, Meta::AlbumPtr album )
        : Meta::Album()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( album->name() )
{
    m_albums.append( album );
    if( album->hasAlbumArtist() )
        m_albumArtist = Meta::ArtistPtr( m_collection->getArtist( album->albumArtist() ) );
}

ProxyCollection::Album::~Album()
{
}

QString
ProxyCollection::Album::name() const
{
    return m_name;
}

QString
ProxyCollection::Album::prettyName() const
{
    return m_name;
}

QString
ProxyCollection::Album::sortableName() const
{
    if( !m_albums.isEmpty() )
        return m_albums.first()->sortableName();

    return m_name;
}

Meta::TrackList
ProxyCollection::Album::tracks()
{
    QSet<ProxyCollection::Track*> tracks;
    foreach( Meta::AlbumPtr album, m_albums )
    {
        Meta::TrackList tmp = album->tracks();
        foreach( const Meta::TrackPtr &track, tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    foreach( ProxyCollection::Track *track, tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

Meta::ArtistPtr
ProxyCollection::Album::albumArtist() const
{
    return m_albumArtist;
}

bool
ProxyCollection::Album::isCompilation() const
{
    return m_albumArtist.isNull();
}

bool
ProxyCollection::Album::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

bool
ProxyCollection::Album::hasCapabilityInterface(Capabilities::Capability::Type type ) const
{

    if( m_albums.count() == 1 )
    {
        return m_albums.first()->hasCapabilityInterface( type );
    }
    else
    {
        return false;
    }
}

Capabilities::Capability*
ProxyCollection::Album::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_albums.count() == 1 )
    {
        return m_albums.first()->createCapabilityInterface( type );
    }
    else
    {
        return 0;
    }
}

void
ProxyCollection::Album::add( Meta::AlbumPtr album )
{
    if( !album || m_albums.contains( album ) )
        return;

    m_albums.append( album );
    subscribeTo( album );

    notifyObservers();
}

bool
ProxyCollection::Album::hasImage( int size ) const
{
    foreach( const Meta::AlbumPtr &album, m_albums )
    {
        if( album->hasImage( size ) )
            return true;
    }
    return false;
}

QPixmap
ProxyCollection::Album::image( int size )
{
    foreach( Meta::AlbumPtr album, m_albums )
    {
        if( album->hasImage( size ) )
        {
            return album->image( size );
        }
    }
    return QPixmap();
}

KUrl
ProxyCollection::Album::imageLocation( int size )
{
    foreach( Meta::AlbumPtr album, m_albums )
    {
        if( album->hasImage( size ) )
        {
            KUrl url = album->imageLocation( size );
            if( url.isValid() )
            {
                return url;
            }
        }
    }
    return KUrl();
}

QPixmap
ProxyCollection::Album::imageWithBorder( int size, int borderWidth )
{
    foreach( Meta::AlbumPtr album, m_albums )
    {
        if( album->hasImage( size ) )
        {
            return The::svgHandler()->imageWithBorder( album, size, borderWidth );
        }
    }
    return QPixmap();
}

bool
ProxyCollection::Album::canUpdateImage() const
{
    if( m_albums.count() == 0 )
        return false;

    foreach( const Meta::AlbumPtr &album, m_albums )
    {
        //we can only update the image for all albusm at the same time
        if( !album->canUpdateImage() )
            return false;
    }
    return true;
}

void
ProxyCollection::Album::setImage( const QPixmap &pixmap )
{
    foreach( Meta::AlbumPtr album, m_albums )
    {
        album->setImage( pixmap );
    }
}

void
ProxyCollection::Album::removeImage()
{
    foreach( Meta::AlbumPtr album, m_albums )
    {
        album->removeImage();
    }
}

void
ProxyCollection::Album::setSuppressImageAutoFetch( bool suppress )
{
    foreach( Meta::AlbumPtr album, m_albums )
    {
        album->setSuppressImageAutoFetch( suppress );
    }
}

bool
ProxyCollection::Album::suppressImageAutoFetch() const
{
    foreach( const Meta::AlbumPtr &album, m_albums )
    {
        if( !album->suppressImageAutoFetch() )
            return false;
    }
    return true;
}

void
ProxyCollection::Album::metadataChanged( Meta::AlbumPtr album )
{
    if( !album || !m_albums.contains( album ) )
        return;

    if( album->name() != m_name ||
        hasAlbumArtist() != album->hasAlbumArtist() ||
        ( hasAlbumArtist() && m_albumArtist->name() != album->albumArtist()->name() ) )
    {
        if( m_albums.count() > 1 )
        {
            m_collection->getAlbum( album );
            unsubscribeFrom( album );
            m_albums.removeAll( album );
        }
        else
        {
            Meta::ArtistPtr albumartist;
            if( album->hasAlbumArtist() )
                 albumartist = Meta::ArtistPtr( m_collection->getArtist( album->albumArtist() ) );

            QString artistname = m_albumArtist ? m_albumArtist->name() : QString();
            m_collection->removeAlbum( m_name, artistname );
            m_name = album->name();
            m_albumArtist = albumartist;
            m_collection->setAlbum( this );
        }
    }

    notifyObservers();
}

ProxyCollection::Artist::Artist( ProxyCollection::Collection *coll, Meta::ArtistPtr artist )
        : Meta::Artist()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( artist->name() )
{
    m_artists.append( artist );
    subscribeTo( artist );
}

ProxyCollection::Artist::~Artist()
{
}

QString
ProxyCollection::Artist::name() const
{
    return m_name;
}

QString
ProxyCollection::Artist::prettyName() const
{
    return m_name;
}

QString
ProxyCollection::Artist::sortableName() const
{
    if( !m_artists.isEmpty() )
        return m_artists.first()->sortableName();

    return m_name;
}

Meta::TrackList
ProxyCollection::Artist::tracks()
{
    QSet<ProxyCollection::Track*> tracks;
    foreach( Meta::ArtistPtr artist, m_artists )
    {
        Meta::TrackList tmp = artist->tracks();
        foreach( const Meta::TrackPtr &track, tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    foreach( ProxyCollection::Track *track, tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

Meta::AlbumList
ProxyCollection::Artist::albums()
{
    QSet<ProxyCollection::Album*> albums;
    foreach( Meta::ArtistPtr artist, m_artists )
    {
        Meta::AlbumList tmp = artist->albums();
        foreach( const Meta::AlbumPtr &album, tmp )
        {
            albums.insert( m_collection->getAlbum( album ) );
        }
    }

    Meta::AlbumList result;
    foreach( ProxyCollection::Album *album, albums )
    {
        result.append( Meta::AlbumPtr( album ) );
    }
    return result;
}

bool
ProxyCollection::Artist::hasCapabilityInterface(Capabilities::Capability::Type type ) const
{

    if( m_artists.count() == 1 )
    {
        return m_artists.first()->hasCapabilityInterface( type );
    }
    else
    {
        return false;
    }
}

Capabilities::Capability*
ProxyCollection::Artist::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_artists.count() == 1 )
    {
        return m_artists.first()->createCapabilityInterface( type );
    }
    else
    {
        return 0;
    }
}

void
ProxyCollection::Artist::add( Meta::ArtistPtr artist )
{
    if( !artist || m_artists.contains( artist ) )
        return;

    m_artists.append( artist );
    subscribeTo( artist );

    notifyObservers();
}

void
ProxyCollection::Artist::metadataChanged( Meta::ArtistPtr artist )
{
    if( !artist || !m_artists.contains( artist ) )
        return;

    if( artist->name() != m_name )
    {
        if( m_artists.count() > 1 )
        {
            m_collection->getArtist( artist );
            unsubscribeFrom( artist );
            m_artists.removeAll( artist );
        }
        else
        {
            //possible race condition here:
            //if another thread creates an Artist with the new name
            //we will have two instances that have the same name!
            //TODO: figure out a way around that
            //the race condition is a problem for all other metadataChanged methods too
            m_collection->removeArtist( m_name );
            m_name = artist->name();
            m_collection->setArtist( this );

        }
    }

    notifyObservers();
}

ProxyCollection::Genre::Genre( ProxyCollection::Collection *coll, Meta::GenrePtr genre )
        : Meta::Genre()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( genre->name() )
{
    m_genres.append( genre );
    subscribeTo( genre );
}

ProxyCollection::Genre::~Genre()
{
}

QString
ProxyCollection::Genre::name() const
{
    return m_name;
}

QString
ProxyCollection::Genre::prettyName() const
{
    return m_name;
}

QString
ProxyCollection::Genre::sortableName() const
{
    if( !m_genres.isEmpty() )
        return m_genres.first()->sortableName();

    return m_name;
}

Meta::TrackList
ProxyCollection::Genre::tracks()
{
    QSet<ProxyCollection::Track*> tracks;
    foreach( Meta::GenrePtr genre, m_genres )
    {
        Meta::TrackList tmp = genre->tracks();
        foreach( const Meta::TrackPtr &track, tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    foreach( ProxyCollection::Track *track, tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

bool
ProxyCollection::Genre::hasCapabilityInterface(Capabilities::Capability::Type type ) const
{

    if( m_genres.count() == 1 )
    {
        return m_genres.first()->hasCapabilityInterface( type );
    }
    else
    {
        return false;
    }
}

Capabilities::Capability*
ProxyCollection::Genre::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_genres.count() == 1 )
    {
        return m_genres.first()->createCapabilityInterface( type );
    }
    else
    {
        return 0;
    }
}

void
ProxyCollection::Genre::add( Meta::GenrePtr genre )
{
    if( !genre || m_genres.contains( genre ) )
        return;

    m_genres.append( genre );
    subscribeTo( genre );

    notifyObservers();
}

void
ProxyCollection::Genre::metadataChanged( Meta::GenrePtr genre )
{
    if( !genre || !m_genres.contains( genre ) )
        return;

    if( genre->name() != m_name )
    {
        if( m_genres.count() > 1 )
        {
            m_collection->getGenre( genre );
            unsubscribeFrom( genre );
            m_genres.removeAll( genre );
        }
        else
        {
            m_collection->removeGenre( m_name );
            m_collection->setGenre( this );
            m_name = genre->name();
        }
    }

    notifyObservers();
}

ProxyCollection::Composer::Composer( ProxyCollection::Collection *coll, Meta::ComposerPtr composer )
        : Meta::Composer()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( composer->name() )
{
    m_composers.append( composer );
    subscribeTo( composer );
}

ProxyCollection::Composer::~Composer()
{
}

QString
ProxyCollection::Composer::name() const
{
    return m_name;
}

QString
ProxyCollection::Composer::prettyName() const
{
    return m_name;
}

QString
ProxyCollection::Composer::sortableName() const
{
    if( !m_composers.isEmpty() )
        return m_composers.first()->sortableName();

    return m_name;
}

Meta::TrackList
ProxyCollection::Composer::tracks()
{
    QSet<ProxyCollection::Track*> tracks;
    foreach( Meta::ComposerPtr composer, m_composers )
    {
        Meta::TrackList tmp = composer->tracks();
        foreach( const Meta::TrackPtr &track, tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    foreach( ProxyCollection::Track *track, tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

bool
ProxyCollection::Composer::hasCapabilityInterface(Capabilities::Capability::Type type ) const
{

    if( m_composers.count() == 1 )
    {
        return m_composers.first()->hasCapabilityInterface( type );
    }
    else
    {
        return false;
    }
}

Capabilities::Capability*
ProxyCollection::Composer::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_composers.count() == 1 )
    {
        return m_composers.first()->createCapabilityInterface( type );
    }
    else
    {
        return 0;
    }
}

void
ProxyCollection::Composer::add( Meta::ComposerPtr composer )
{
    if( !composer || m_composers.contains( composer ) )
        return;

    m_composers.append( composer );
    subscribeTo( composer );

    notifyObservers();
}

void
ProxyCollection::Composer::metadataChanged( Meta::ComposerPtr composer )
{
    if( !composer || !m_composers.contains( composer ) )
        return;

    if( composer->name() != m_name )
    {
        if( m_composers.count() > 1 )
        {
            m_collection->getComposer( composer );
            unsubscribeFrom( composer );
            m_composers.removeAll( composer );
        }
        else
        {
            m_collection->removeComposer( m_name );
            m_collection->setComposer( this );
            m_name = composer->name();
        }
    }

    notifyObservers();
}

ProxyCollection::Year::Year( ProxyCollection::Collection *coll, Meta::YearPtr year )
        : Meta::Year()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( year->name() )
{
    m_years.append( year );
    subscribeTo( year );
}

ProxyCollection::Year::~Year()
{
    //nothing to do
}

QString
ProxyCollection::Year::name() const
{
    return m_name;
}

QString
ProxyCollection::Year::prettyName() const
{
    return m_name;
}

QString
ProxyCollection::Year::sortableName() const
{
    if( !m_years.isEmpty() )
        return m_years.first()->sortableName();

    return m_name;
}

Meta::TrackList
ProxyCollection::Year::tracks()
{
    QSet<ProxyCollection::Track*> tracks;
    foreach( Meta::YearPtr year, m_years )
    {
        Meta::TrackList tmp = year->tracks();
        foreach( const Meta::TrackPtr &track, tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    foreach( ProxyCollection::Track *track, tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

bool
ProxyCollection::Year::hasCapabilityInterface(Capabilities::Capability::Type type ) const
{

    if( m_years.count() == 1 )
    {
        return m_years.first()->hasCapabilityInterface( type );
    }
    else
    {
        return false;
    }
}

Capabilities::Capability*
ProxyCollection::Year::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_years.count() == 1 )
    {
        return m_years.first()->createCapabilityInterface( type );
    }
    else
    {
        return 0;
    }
}

void
ProxyCollection::Year::add( Meta::YearPtr year )
{
    if( !year || m_years.contains( year ) )
        return;

    m_years.append( year );
    subscribeTo( year );

    notifyObservers();
}

void
ProxyCollection::Year::metadataChanged( Meta::YearPtr year )
{
    if( !year || !m_years.contains( year ) )
        return;

    if( year->name() != m_name )
    {
        if( m_years.count() > 1 )
        {
            m_collection->getYear( year );
            unsubscribeFrom( year );
            m_years.removeAll( year );
        }
        else
        {
            if( m_collection->hasYear( year->name() ) )
            {
                unsubscribeFrom( year );
                m_collection->getYear( year );
                m_years.removeAll( year );
                m_collection->removeYear( m_name );
                return; //do NOT notify observers, the instance is not valid anymore!
            }
            else
            {
                //be careful with the ordering of instructions here
                //ProxyCollection uses KSharedPtr internally
                //so we have to make sure that there is more than one pointer
                //to this instance by registering this instance under the new name
                //before removing the old one. Otherwise kSharedPtr might delete this
                //instance in removeYear()
                QString tmpName = m_name;
                m_name = year->name();
                m_collection->setYear( this );
                m_collection->removeYear( tmpName );
            }
        }
    }

    notifyObservers();
}
