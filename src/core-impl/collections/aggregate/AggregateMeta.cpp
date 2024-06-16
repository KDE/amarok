/****************************************************************************************
 * Copyright (c) 2009,2010 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#define DEBUG_PREFIX "AggregateMeta"

#include "AggregateMeta.h"

#include "SvgHandler.h"
#include "core/meta/TrackEditor.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Debug.h"
#include "core-impl/collections/aggregate/AggregateCollection.h"

#include <QDateTime>
#include <QSet>
#include <QTimer>

namespace Meta
{

#define FORWARD( call ) { for( TrackEditorPtr e : m_editors ) { e->call; } \
                            if( !m_batchMode ) QTimer::singleShot( 0, m_collection, &Collections::AggregateCollection::slotUpdated ); }

class AggregateTrackEditor : public TrackEditor
{
public:
    AggregateTrackEditor( Collections::AggregateCollection *coll, const QList<TrackEditorPtr> &editors )
        : TrackEditor()
        , m_batchMode( false )
        , m_collection( coll )
        , m_editors( editors )
    {}

    void beginUpdate() override
    {
        m_batchMode = true;
        for( TrackEditorPtr ec : m_editors ) ec->beginUpdate();
    }
    void endUpdate() override
    {
        for( TrackEditorPtr ec : m_editors ) ec->endUpdate();
        m_batchMode = false;
        QTimer::singleShot( 0, m_collection, &Collections::AggregateCollection::slotUpdated );
    }
    void setComment( const QString &newComment ) override { FORWARD( setComment( newComment ) ) }
    void setTrackNumber( int newTrackNumber ) override { FORWARD( setTrackNumber( newTrackNumber ) ) }
    void setDiscNumber( int newDiscNumber ) override { FORWARD( setDiscNumber( newDiscNumber ) ) }
    void setBpm( const qreal newBpm ) override { FORWARD( setBpm( newBpm ) ) }
    void setTitle( const QString &newTitle ) override { FORWARD( setTitle( newTitle ) ) }
    void setArtist( const QString &newArtist ) override { FORWARD( setArtist( newArtist ) ) }
    void setAlbum( const QString &newAlbum ) override { FORWARD( setAlbum( newAlbum ) ) }
    void setAlbumArtist( const QString &newAlbumArtist ) override { FORWARD( setAlbumArtist ( newAlbumArtist ) ) }
    void setGenre( const QString &newGenre ) override { FORWARD( setGenre( newGenre ) ) }
    void setComposer( const QString &newComposer ) override { FORWARD( setComposer( newComposer ) ) }
    void setYear( int newYear ) override { FORWARD( setYear( newYear ) ) }
private:
    bool m_batchMode;
    Collections::AggregateCollection *m_collection;
    QList<TrackEditorPtr> m_editors;
};

#undef FORWARD

AggregateTrack::AggregateTrack( Collections::AggregateCollection *coll, const TrackPtr &track )
        : Track()
        , Observer()
        , m_collection( coll )
        , m_name( track->name() )
        , m_album( nullptr )
        , m_artist( nullptr )
        , m_genre( nullptr )
        , m_composer( nullptr )
        , m_year( nullptr )
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

AggregateTrack::~AggregateTrack()
{
}

QString
AggregateTrack::name() const
{
    return m_name;
}

QString
AggregateTrack::prettyName() const
{
    return m_name;
}

QString
AggregateTrack::sortableName() const
{
    if( !m_tracks.isEmpty() )
        return m_tracks.first()->sortableName();

    return m_name;
}

QUrl
AggregateTrack::playableUrl() const
{
    Meta::TrackPtr bestPlayableTrack;
    for( const Meta::TrackPtr &track : m_tracks )
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

    return QUrl();
}

QString
AggregateTrack::prettyUrl() const
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
AggregateTrack::uidUrl() const
{
    // this is where it gets interesting
    // a uidUrl for a AggregateTrack probably has to be generated
    // from the parts of the key in AggregateCollection
    // need to think about this some more
    return QString();
}

QString
AggregateTrack::notPlayableReason() const
{
    QStringList reasons;
    for( const Meta::TrackPtr &track : m_tracks )
    {
        if( !track->isPlayable() )
            reasons.append( track->notPlayableReason() );
        else
            return QString(); // no reason if at least one playable
    }
    return reasons.join( QStringLiteral( ", " ) );
}

Meta::AlbumPtr
AggregateTrack::album() const
{
    return m_album;
}

Meta::ArtistPtr
AggregateTrack::artist() const
{
    return m_artist;
}

Meta::ComposerPtr
AggregateTrack::composer() const
{
    return m_composer;
}

Meta::GenrePtr
AggregateTrack::genre() const
{
    return m_genre;
}

Meta::YearPtr
AggregateTrack::year() const
{
    return m_year;
}

QString
AggregateTrack::comment() const
{
    //try to return something sensible here...
    //do not show a comment if the internal tracks disagree about the comment
    QString comment;
    if( !m_tracks.isEmpty() )
        comment = m_tracks.first()->comment();

    for( const Meta::TrackPtr &track : m_tracks )
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
AggregateTrack::bpm() const
{
    //Similar to comment(), try to return something sensible here...
    //do not show a common bpm value if the internal tracks disagree about the bpm
    qreal bpm = -1.0;
    if( !m_tracks.isEmpty() )
        bpm = m_tracks.first()->bpm();

    for( const Meta::TrackPtr &track : m_tracks )
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
AggregateTrack::score() const
{
    //again, multiple ways to implement this method:
    //return the maximum score, the minimum score, the average
    //the score of the track with the maximum play count,
    //or an average weighted by play count. And probably a couple of ways that
    //I cannot think of right now...

    //implementing the weighted average here...
    double weightedSum = 0.0;
    int totalCount = 0;
    for( const Meta::TrackPtr &track : m_tracks )
    {
        ConstStatisticsPtr statistics = track->statistics();
        totalCount += statistics->playCount();
        weightedSum += statistics->playCount() * statistics->score();
    }
    if( totalCount )
        return weightedSum / totalCount;

    return 0.0;
}

void
AggregateTrack::setScore( double newScore )
{
    for( Meta::TrackPtr track : m_tracks )
    {
        track->statistics()->setScore( newScore );
    }
}

int
AggregateTrack::rating() const
{
    //yay, multiple options again. As this has to be defined by the user, let's take
    //the maximum here.
    int result = 0;
    for( const Meta::TrackPtr &track : m_tracks )
    {
        if( track->statistics()->rating() > result )
            result = track->statistics()->rating();
    }
    return result;
}

void
AggregateTrack::setRating( int newRating )
{
    for( Meta::TrackPtr track : m_tracks )
    {
        track->statistics()->setRating( newRating );
    }
}

QDateTime
AggregateTrack::firstPlayed() const
{
    QDateTime result;
    for( const Meta::TrackPtr &track : m_tracks )
    {
        ConstStatisticsPtr statistics = track->statistics();
        //use the track's firstPlayed value if it represents an earlier timestamp than
        //the current result, or use it directly if result has not been set yet
        //this should result in the earliest timestamp for first play of all internal
        //tracks being returned
        if( ( statistics->firstPlayed().isValid() && result.isValid() && statistics->firstPlayed() < result ) ||
            ( statistics->firstPlayed().isValid() && !result.isValid() ) )
        {
            result = statistics->firstPlayed();
        }
    }
    return result;
}

void
AggregateTrack::setFirstPlayed( const QDateTime &date )
{
    for( Meta::TrackPtr track : m_tracks )
    {
        // only "lower" the first played
        Meta::StatisticsPtr trackStats = track->statistics();
        if( !trackStats->firstPlayed().isValid() ||
            trackStats->firstPlayed() > date )
        {
            trackStats->setFirstPlayed( date );
        }
    }
}

QDateTime
AggregateTrack::lastPlayed() const
{
    QDateTime result;
    //return the latest timestamp. Easier than firstPlayed because we do not have to
    //care about 0.
    //when are we going to perform the refactoring as discussed in Berlin?
    for( const Meta::TrackPtr &track : m_tracks )
    {
        if( track->statistics()->lastPlayed() > result )
        {
            result = track->statistics()->lastPlayed();
        }
    }
    return result;
}

void
AggregateTrack::setLastPlayed(const QDateTime& date)
{
    for( Meta::TrackPtr track : m_tracks )
    {
        // only "raise" the last played
        Meta::StatisticsPtr trackStats = track->statistics();
        if( !trackStats->lastPlayed().isValid() ||
            trackStats->lastPlayed() < date )
        {
            trackStats->setLastPlayed( date );
        }
    }
}

int
AggregateTrack::playCount() const
{
    // show the maximum of all play counts.
    int result = 0;
    for( const Meta::TrackPtr &track : m_tracks )
    {
        if( track->statistics()->playCount() > result )
        {
            result = track->statistics()->playCount();
        }
    }
    return result;
}

void
AggregateTrack::setPlayCount( int newPlayCount )
{
    Q_UNUSED( newPlayCount )
    // no safe thing to do here. Notice we override finishedPlaying()
}

void
AggregateTrack::finishedPlaying( double playedFraction )
{
    for( Meta::TrackPtr track : m_tracks )
    {
        track->finishedPlaying( playedFraction );
    }
}

qint64
AggregateTrack::length() const
{
    for( const Meta::TrackPtr &track : m_tracks )
    {
        if( track->length() )
            return track->length();
    }
    return 0;
}

int
AggregateTrack::filesize() const
{
    for( const Meta::TrackPtr &track : m_tracks )
    {
        if( track->filesize() )
        {
            return track->filesize();
        }
    }
    return 0;
}

int
AggregateTrack::sampleRate() const
{
    for( const Meta::TrackPtr &track : m_tracks )
    {
        if( track->sampleRate() )
            return track->sampleRate();
    }
    return 0;
}

int
AggregateTrack::bitrate() const
{
    for( const Meta::TrackPtr &track : m_tracks )
    {
        if( track->bitrate() )
            return track->bitrate();
    }
    return 0;
}

QDateTime
AggregateTrack::createDate() const
{
    QDateTime result;
    for( const Meta::TrackPtr &track : m_tracks )
    {
        //use the track's firstPlayed value if it represents an earlier timestamp than
        //the current result, or use it directly if result has not been set yet
        //this should result in the earliest timestamp for first play of all internal
        //tracks being returned
        if( ( track->createDate().isValid() && result.isValid() && track->createDate() < result ) ||
            ( track->createDate().isValid() && !result.isValid() ) )
        {
            result = track->createDate();
        }
    }
    return result;
}

int
AggregateTrack::trackNumber() const
{
    int result = 0;
    for( const Meta::TrackPtr &track : m_tracks )
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
AggregateTrack::discNumber() const
{
    int result = 0;
    for( const Meta::TrackPtr &track : m_tracks )
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
AggregateTrack::type() const
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

Collections::Collection*
AggregateTrack::collection() const
{
    return m_collection;
}

bool
AggregateTrack::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    if( m_tracks.count() == 1 )
        // if we aggregate only one track, simply return the tracks capability directly
        return m_tracks.first()->hasCapabilityInterface( type );
    else
        return false;
}

Capabilities::Capability*
AggregateTrack::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_tracks.count() == 1 )
        return m_tracks.first()->createCapabilityInterface( type );
    else
        return nullptr;
}

TrackEditorPtr
AggregateTrack::editor()
{
    if( m_tracks.count() == 1 )
        return m_tracks.first()->editor();

    QList<Meta::TrackEditorPtr> editors;
    for( Meta::TrackPtr track : m_tracks )
    {
        Meta::TrackEditorPtr ec = track->editor();
        if( ec )
            editors << ec;
        else
            return TrackEditorPtr();
    }
    return TrackEditorPtr( new AggregateTrackEditor( m_collection, editors ) );
}

void
AggregateTrack::addLabel( const QString &label )
{
    for( Meta::TrackPtr track : m_tracks )
    {
        track->addLabel( label );
    }
}

void
AggregateTrack::addLabel( const Meta::LabelPtr &label )
{
    for( Meta::TrackPtr track : m_tracks )
    {
        track->addLabel( label );
    }
}

void
AggregateTrack::removeLabel( const Meta::LabelPtr &label )
{
    for( Meta::TrackPtr track : m_tracks )
    {
        track->removeLabel( label );
    }
}

Meta::LabelList
AggregateTrack::labels() const
{
    QSet<AggregateLabel *> aggregateLabels;
    for( const Meta::TrackPtr &track : m_tracks )
    {
        for( Meta::LabelPtr label : track->labels() )
        {
            aggregateLabels.insert( m_collection->getLabel( label ) );
        }
    }
    Meta::LabelList result;
    for( AggregateLabel *label : aggregateLabels )
    {
        result << Meta::LabelPtr( label );
    }
    return result;
}

StatisticsPtr
AggregateTrack::statistics()
{
    return StatisticsPtr( this );
}

void
AggregateTrack::add( const Meta::TrackPtr &track )
{
    if( !track || m_tracks.contains( track ) )
        return;

    m_tracks.append( track );
    subscribeTo( track );

    notifyObservers();
}

void
AggregateTrack::metadataChanged(const TrackPtr &track )
{
    if( !track )
        return;

    if( !m_tracks.contains( track ) )
    {
        //why are we subscribed?
        unsubscribeFrom( track );
        return;
    }

    const TrackKey myKey( Meta::TrackPtr( this ) );
    const TrackKey otherKey( track );
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

AggregateAlbum::AggregateAlbum( Collections::AggregateCollection *coll, Meta::AlbumPtr album )
        : Meta::Album()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( album->name() )
{
    m_albums.append( album );
    if( album->hasAlbumArtist() )
        m_albumArtist = Meta::ArtistPtr( m_collection->getArtist( album->albumArtist() ) );
}

AggregateAlbum::~AggregateAlbum()
{
}

QString
AggregateAlbum::name() const
{
    return m_name;
}

QString
AggregateAlbum::prettyName() const
{
    return m_name;
}

QString
AggregateAlbum::sortableName() const
{
    if( !m_albums.isEmpty() )
        return m_albums.first()->sortableName();

    return m_name;
}

Meta::TrackList
AggregateAlbum::tracks()
{
    QSet<AggregateTrack*> tracks;
    for( Meta::AlbumPtr album : m_albums )
    {
        Meta::TrackList tmp = album->tracks();
        for( const Meta::TrackPtr &track : tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    for( AggregateTrack *track : tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

Meta::ArtistPtr
AggregateAlbum::albumArtist() const
{
    return m_albumArtist;
}

bool
AggregateAlbum::isCompilation() const
{
    return m_albumArtist.isNull();
}

bool
AggregateAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

bool
AggregateAlbum::hasCapabilityInterface(Capabilities::Capability::Type type ) const
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
AggregateAlbum::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_albums.count() == 1 )
    {
        return m_albums.first()->createCapabilityInterface( type );
    }
    else
    {
        return nullptr;
    }
}

void
AggregateAlbum::add( const Meta::AlbumPtr &album )
{
    if( !album || m_albums.contains( album ) )
        return;

    m_albums.append( album );
    subscribeTo( album );

    notifyObservers();
}

bool
AggregateAlbum::hasImage( int size ) const
{
    for( const Meta::AlbumPtr &album : m_albums )
    {
        if( album->hasImage( size ) )
            return true;
    }
    return false;
}

QImage
AggregateAlbum::image( int size ) const
{
    for( Meta::AlbumPtr album : m_albums )
    {
        if( album->hasImage( size ) )
        {
            return album->image( size );
        }
    }
    return Meta::Album::image( size );
}

QUrl
AggregateAlbum::imageLocation( int size )
{
    for( Meta::AlbumPtr album : m_albums )
    {
        if( album->hasImage( size ) )
        {
            QUrl url = album->imageLocation( size );
            if( url.isValid() )
            {
                return url;
            }
        }
    }
    return QUrl();
}

QPixmap
AggregateAlbum::imageWithBorder( int size, int borderWidth )
{
    for( Meta::AlbumPtr album : m_albums )
    {
        if( album->hasImage( size ) )
        {
            return The::svgHandler()->imageWithBorder( album, size, borderWidth );
        }
    }
    return QPixmap();
}

bool
AggregateAlbum::canUpdateImage() const
{
    if( m_albums.isEmpty() )
        return false;

    for( const Meta::AlbumPtr &album : m_albums )
    {
        //we can only update the image for all albums at the same time
        if( !album->canUpdateImage() )
            return false;
    }
    return true;
}

void
AggregateAlbum::setImage( const QImage &image )
{
    for( Meta::AlbumPtr album : m_albums )
    {
        album->setImage( image );
    }
}

void
AggregateAlbum::removeImage()
{
    for( Meta::AlbumPtr album : m_albums )
    {
        album->removeImage();
    }
}

void
AggregateAlbum::setSuppressImageAutoFetch( bool suppress )
{
    for( Meta::AlbumPtr album : m_albums )
    {
        album->setSuppressImageAutoFetch( suppress );
    }
}

bool
AggregateAlbum::suppressImageAutoFetch() const
{
    for( const Meta::AlbumPtr &album : m_albums )
    {
        if( !album->suppressImageAutoFetch() )
            return false;
    }
    return true;
}

void
AggregateAlbum::metadataChanged(const AlbumPtr &album )
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

AggregateArtist::AggregateArtist( Collections::AggregateCollection *coll, const Meta::ArtistPtr &artist )
        : Meta::Artist()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( artist->name() )
{
    m_artists.append( artist );
    subscribeTo( artist );
}

AggregateArtist::~AggregateArtist()
{
}

QString
AggregateArtist::name() const
{
    return m_name;
}

QString
AggregateArtist::prettyName() const
{
    return m_name;
}

QString
AggregateArtist::sortableName() const
{
    if( !m_artists.isEmpty() )
        return m_artists.first()->sortableName();

    return m_name;
}

Meta::TrackList
AggregateArtist::tracks()
{
    QSet<AggregateTrack*> tracks;
    for( Meta::ArtistPtr artist : m_artists )
    {
        Meta::TrackList tmp = artist->tracks();
        for( const Meta::TrackPtr &track : tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    for( AggregateTrack *track : tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

bool
AggregateArtist::hasCapabilityInterface(Capabilities::Capability::Type type ) const
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
AggregateArtist::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_artists.count() == 1 )
    {
        return m_artists.first()->createCapabilityInterface( type );
    }
    else
    {
        return nullptr;
    }
}

void
AggregateArtist::add( const Meta::ArtistPtr &artist )
{
    if( !artist || m_artists.contains( artist ) )
        return;

    m_artists.append( artist );
    subscribeTo( artist );

    notifyObservers();
}

void
AggregateArtist::metadataChanged(const ArtistPtr &artist )
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

AggregateGenre::AggregateGenre( Collections::AggregateCollection *coll, const Meta::GenrePtr &genre )
        : Meta::Genre()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( genre->name() )
{
    m_genres.append( genre );
    subscribeTo( genre );
}

AggregateGenre::~AggregateGenre()
{
}

QString
AggregateGenre::name() const
{
    return m_name;
}

QString
AggregateGenre::prettyName() const
{
    return m_name;
}

QString
AggregateGenre::sortableName() const
{
    if( !m_genres.isEmpty() )
        return m_genres.first()->sortableName();

    return m_name;
}

Meta::TrackList
AggregateGenre::tracks()
{
    QSet<AggregateTrack*> tracks;
    for( Meta::GenrePtr genre : m_genres )
    {
        Meta::TrackList tmp = genre->tracks();
        for( const Meta::TrackPtr &track : tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    for( AggregateTrack *track : tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

bool
AggregateGenre::hasCapabilityInterface(Capabilities::Capability::Type type ) const
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
AggregateGenre::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_genres.count() == 1 )
    {
        return m_genres.first()->createCapabilityInterface( type );
    }
    else
    {
        return nullptr;
    }
}

void
AggregateGenre::add( const Meta::GenrePtr &genre )
{
    if( !genre || m_genres.contains( genre ) )
        return;

    m_genres.append( genre );
    subscribeTo( genre );

    notifyObservers();
}

void
AggregateGenre::metadataChanged( const Meta::GenrePtr &genre )
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

AggregateComposer::AggregateComposer( Collections::AggregateCollection *coll, const Meta::ComposerPtr &composer )
        : Meta::Composer()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( composer->name() )
{
    m_composers.append( composer );
    subscribeTo( composer );
}

AggregateComposer::~AggregateComposer()
{
}

QString
AggregateComposer::name() const
{
    return m_name;
}

QString
AggregateComposer::prettyName() const
{
    return m_name;
}

QString
AggregateComposer::sortableName() const
{
    if( !m_composers.isEmpty() )
        return m_composers.first()->sortableName();

    return m_name;
}

Meta::TrackList
AggregateComposer::tracks()
{
    QSet<AggregateTrack*> tracks;
    for( Meta::ComposerPtr composer : m_composers )
    {
        Meta::TrackList tmp = composer->tracks();
        for( const Meta::TrackPtr &track : tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    for( AggregateTrack *track : tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

bool
AggregateComposer::hasCapabilityInterface(Capabilities::Capability::Type type ) const
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
AggregateComposer::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_composers.count() == 1 )
    {
        return m_composers.first()->createCapabilityInterface( type );
    }
    else
    {
        return nullptr;
    }
}

void
AggregateComposer::add( const Meta::ComposerPtr &composer )
{
    if( !composer || m_composers.contains( composer ) )
        return;

    m_composers.append( composer );
    subscribeTo( composer );

    notifyObservers();
}

void
AggregateComposer::metadataChanged(const ComposerPtr &composer )
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

AggreagateYear::AggreagateYear( Collections::AggregateCollection *coll, const Meta::YearPtr &year )
        : Meta::Year()
        , Meta::Observer()
        , m_collection( coll )
        , m_name( year->name() )
{
    m_years.append( year );
    subscribeTo( year );
}

AggreagateYear::~AggreagateYear()
{
    //nothing to do
}

QString
AggreagateYear::name() const
{
    return m_name;
}

QString
AggreagateYear::prettyName() const
{
    return m_name;
}

QString
AggreagateYear::sortableName() const
{
    if( !m_years.isEmpty() )
        return m_years.first()->sortableName();

    return m_name;
}

Meta::TrackList
AggreagateYear::tracks()
{
    QSet<AggregateTrack*> tracks;
    for( Meta::YearPtr year : m_years )
    {
        Meta::TrackList tmp = year->tracks();
        for( const Meta::TrackPtr &track : tmp )
        {
            tracks.insert( m_collection->getTrack( track ) );
        }
    }

    Meta::TrackList result;
    for( AggregateTrack *track : tracks )
    {
        result.append( Meta::TrackPtr( track ) );
    }
    return result;
}

bool
AggreagateYear::hasCapabilityInterface(Capabilities::Capability::Type type ) const
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
AggreagateYear::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_years.count() == 1 )
    {
        return m_years.first()->createCapabilityInterface( type );
    }
    else
    {
        return nullptr;
    }
}

void
AggreagateYear::add( const Meta::YearPtr &year )
{
    if( !year || m_years.contains( year ) )
        return;

    m_years.append( year );
    subscribeTo( year );

    notifyObservers();
}

void
AggreagateYear::metadataChanged( const Meta::YearPtr &year )
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
                // be careful with the ordering of instructions here
                // AggregateCollection uses AmarokSharedPointer internally
                // so we have to make sure that there is more than one pointer
                // to this instance by registering this instance under the new name
                // before removing the old one. Otherwise kSharedPtr might delete this
                // instance in removeYear()
                QString tmpName = m_name;
                m_name = year->name();
                m_collection->setYear( this );
                m_collection->removeYear( tmpName );
            }
        }
    }

    notifyObservers();
}

AggregateLabel::AggregateLabel( Collections::AggregateCollection *coll, const Meta::LabelPtr &label )
    : Meta::Label()
    , m_collection( coll )
    , m_name( label->name() )
{
    m_labels.append( label );
    Q_UNUSED(m_collection); // might be needed later
}

AggregateLabel::~AggregateLabel()
{
    //nothing to do
}

QString
AggregateLabel::name() const
{
    return m_name;
}

QString
AggregateLabel::prettyName() const
{
    return m_name;
}

QString
AggregateLabel::sortableName() const
{
    if( !m_labels.isEmpty() )
        return m_labels.first()->sortableName();

    return m_name;
}

bool
AggregateLabel::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{

    if( m_labels.count() == 1 )
    {
        return m_labels.first()->hasCapabilityInterface( type );
    }
    else
    {
        return false;
    }
}

Capabilities::Capability*
AggregateLabel::createCapabilityInterface( Capabilities::Capability::Type type )
{
    if( m_labels.count() == 1 )
    {
        return m_labels.first()->createCapabilityInterface( type );
    }
    else
    {
        return nullptr;
    }
}

void
AggregateLabel::add( const Meta::LabelPtr &label )
{
    if( !label || m_labels.contains( label ) )
        return;

    m_labels.append( label );
}

} //namespace Meta
