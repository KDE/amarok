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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "core-impl/meta/timecode/TimecodeMeta.h"

#include "core/support/Debug.h"
#include "covermanager/CoverFetchingActions.h"
#include "covermanager/CoverFetcher.h"
#include "core/capabilities/CustomActionsCapability.h"
#include "core/capabilities/Capability.h"
#include "core/capabilities/BoundedPlaybackCapability.h"
#include "core-impl/capabilities/timecode/TimecodeEditCapability.h"
#include "core-impl/capabilities/timecode/TimecodeBoundedPlaybackCapability.h"

using namespace Meta;
using namespace Capabilities;

////////////////// TRACK //////////////////

TimecodeTrack::TimecodeTrack( const QString & name, const QString & url, qint64 start, qint64 end )
    : m_name( name )
    , m_start( start )
    , m_end( end )
    , m_length( end - start )
    , m_trackNumber( 0 )
    , m_discNumber( 0 )
    , m_comment( QString() )
    , m_playableUrl( url )
    , m_updatedFields( 0 )
{
    DEBUG_BLOCK
    m_displayUrl = url + ':' + QString::number( start ) + '-' + QString::number( end );

    debug() << "created with length: " << m_length;

}

TimecodeTrack::~ TimecodeTrack()
{
}

QString
TimecodeTrack::name() const
{
    return m_name;
}

QString
TimecodeTrack::prettyName() const
{
    return name();
}

KUrl
TimecodeTrack::playableUrl() const
{
    return m_playableUrl;
}

QString
TimecodeTrack::uidUrl() const
{
    return m_displayUrl;
}

QString
TimecodeTrack::prettyUrl() const
{
    return m_displayUrl;
}

AlbumPtr
TimecodeTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

bool
TimecodeTrack::isEditable() const
{
    return true;
}

bool
TimecodeTrack::isPlayable() const
{
     return true;
}

ArtistPtr
TimecodeTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr
TimecodeTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr
TimecodeTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr
TimecodeTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

qreal
TimecodeTrack::bpm() const
{
    return m_bpm;
}

QString
TimecodeTrack::comment() const
{
    return m_comment;
}

double
TimecodeTrack::score() const
{
    return 0.0;
}

void
TimecodeTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
TimecodeTrack::rating() const
{
    return -1;
}

int
TimecodeTrack::bitrate() const
{
    return -1;
}

int
TimecodeTrack::sampleRate() const
{
    return -1;
}

int
TimecodeTrack::filesize() const
{
    return -1;
}

qint64
TimecodeTrack::length() const
{
    return m_length;
}

void
TimecodeTrack::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

int
TimecodeTrack::trackNumber() const
{
    return m_trackNumber;
}

int
TimecodeTrack::discNumber() const
{
    return m_discNumber;
}

uint
TimecodeTrack::lastPlayed() const
{
    return 0;
}

int
TimecodeTrack::playCount() const
{
    return 0;
}

QString
TimecodeTrack::type() const
{
    return QString();
}

void
TimecodeTrack::setAlbum( const QString &newAlbum )
{
    m_updatedFields |= ALBUM_UPDATED;
    m_fields.insert( ALBUM_UPDATED, newAlbum );
}

void
TimecodeTrack::setArtist( const QString &newArtist )
{
    m_updatedFields |= ARTIST_UPDATED;
    m_fields.insert( ARTIST_UPDATED, newArtist );
}

void
TimecodeTrack::setComposer( const QString &newComposer )
{
    m_updatedFields |= COMPOSER_UPDATED;
    m_fields.insert( COMPOSER_UPDATED, newComposer );
}

void
TimecodeTrack::setGenre( const QString &newGenre )
{
    m_updatedFields |= GENRE_UPDATED;
    m_fields.insert( GENRE_UPDATED, newGenre );
}

void
TimecodeTrack::setYear( const QString &newYear )
{
    m_updatedFields |= YEAR_UPDATED;
    m_fields.insert( YEAR_UPDATED, newYear );
}

void
TimecodeTrack::setBpm( const qreal newBpm )
{
    m_updatedFields |= BPM_UPDATED;
    m_fields.insert( BPM_UPDATED, QString::number( (qreal) newBpm ) );
}

void
TimecodeTrack::setTitle( const QString &newTitle )
{
    m_updatedFields |= TITLE_UPDATED;
    m_fields.insert( TITLE_UPDATED, newTitle );
}

void
TimecodeTrack::setComment( const QString &newComment )
{
    m_updatedFields |= COMMENT_UPDATED;
    m_fields.insert( COMMENT_UPDATED, newComment );
}

void
TimecodeTrack::setTrackNumber( int newTrackNumber )
{
    m_updatedFields |= TRACKNUMBER_UPDATED;
    m_fields.insert( TRACKNUMBER_UPDATED, QString::number( newTrackNumber ) );
}

void
TimecodeTrack::setDiscNumber( int newDiscNumber )
{
    m_updatedFields |= DISCNUMBER_UPDATED;
    m_fields.insert( DISCNUMBER_UPDATED, QString::number( newDiscNumber ) );
}

void TimecodeTrack::beginMetaDataUpdate()
{
    m_updatedFields = 0;
    m_fields.clear();
}

void TimecodeTrack::endMetaDataUpdate()
{

    bool updateCover = false;

    if ( m_updatedFields & ALBUM_UPDATED )
    {
        //create a new album:
        m_album = TimecodeAlbumPtr( new TimecodeAlbum( m_fields.value( ALBUM_UPDATED ) ) );
        m_album->addTrack( TimecodeTrackPtr( this ) );
        setAlbum( m_album );
        m_album->setAlbumArtist( m_artist );
    }

    if ( m_updatedFields & ARTIST_UPDATED )
    {
        //create a new album:
        m_artist = TimecodeArtistPtr( new TimecodeArtist( m_fields.value( ARTIST_UPDATED ) ) );
        m_artist->addTrack( TimecodeTrackPtr( this ) );
        setArtist( m_artist );
        m_album->setAlbumArtist( m_artist );
        updateCover = true;
    }

    if ( m_updatedFields & COMPOSER_UPDATED )
    {
        //create a new album:
        m_composer = TimecodeComposerPtr( new TimecodeComposer( m_fields.value( COMPOSER_UPDATED ) ) );
        m_composer->addTrack( TimecodeTrackPtr( this ) );
        setComposer( m_composer );
    }

    if ( m_updatedFields & GENRE_UPDATED )
    {
        //create a new album:
        m_genre = TimecodeGenrePtr( new TimecodeGenre( m_fields.value( GENRE_UPDATED ) ) );
        m_genre->addTrack( TimecodeTrackPtr( this ) );
        setGenre( m_genre );
    }

    if ( m_updatedFields & YEAR_UPDATED )
    {
        //create a new album:
        m_year = TimecodeYearPtr( new TimecodeYear( m_fields.value( YEAR_UPDATED ) ) );
        m_year->addTrack( TimecodeTrackPtr( this ) );
        setYear( m_year );
    }

    if ( m_updatedFields & BPM_UPDATED )
    {
        m_bpm = m_fields.value( BPM_UPDATED ).toDouble();
    }

    if ( m_updatedFields & TITLE_UPDATED )
    {
        //create a new album:
        m_name = m_fields.value( TITLE_UPDATED );
        updateCover = true;
    }

    if ( m_updatedFields & COMMENT_UPDATED )
    {
        //create a new album:
        m_comment = m_fields.value( COMMENT_UPDATED );
    }

    if ( m_updatedFields & TRACKNUMBER_UPDATED )
    {
        //create a new album:
        m_trackNumber = m_fields.value( TRACKNUMBER_UPDATED ).toInt();
    }

    if ( m_updatedFields & DISCNUMBER_UPDATED )
    {
        //create a new album:
        m_discNumber = m_fields.value( DISCNUMBER_UPDATED ).toInt();
    }

    if ( updateCover )
        The::coverFetcher()->queueAlbum( AlbumPtr::staticCast( m_album ) );

    m_updatedFields = 0;
    m_fields.clear();

    notifyObservers();
}

void TimecodeTrack::abortMetaDataUpdate()
{
    m_updatedFields = 0;
    m_fields.clear();
}

void
TimecodeTrack::setAlbum( TimecodeAlbumPtr album )
{
    m_album = album;
}

void
TimecodeTrack::setYear( TimecodeYearPtr year )
{
    m_year = year;
}

void
TimecodeTrack::setGenre( TimecodeGenrePtr genre )
{
    m_genre = genre;
}

void
TimecodeTrack::setComposer( TimecodeComposerPtr composer )
{
    m_composer = composer;
}

void
TimecodeTrack::setArtist( TimecodeArtistPtr artist )
{
    m_artist = artist;
}


bool
TimecodeTrack::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    return type == Capabilities::Capability::BoundedPlayback
           || type == Capabilities::Capability::Editable;
}

Capabilities::Capability *
TimecodeTrack::createCapabilityInterface( Capabilities::Capability::Type type )
{
    DEBUG_BLOCK

    if ( type == Capabilities::Capability::BoundedPlayback )
        return new Capabilities::TimecodeBoundedPlaybackCapability( this );
    else if( type == Capabilities::Capability::Editable )
        return new Capabilities::TimecodeEditCapability( this );
    else
        return 0;
}

qint64 Meta::TimecodeTrack::start()
{
    return m_start;
}

qint64 Meta::TimecodeTrack::end()
{
    return m_end;
}


////////////////// ARTIST //////////////////

TimecodeArtist::TimecodeArtist( const QString & name )
    : m_name( name )
{
}

TimecodeArtist::~ TimecodeArtist()
{
}

QString
TimecodeArtist::name() const
{
    return m_name;
}

QString
TimecodeArtist::prettyName() const
{
    return name();
}

TrackList
TimecodeArtist::tracks()
{
    return m_tracks;
}

AlbumList
TimecodeArtist::albums()
{
    return AlbumList();
}

void
TimecodeArtist::addTrack( TimecodeTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

////////////////// ALBUM //////////////////

TimecodeAlbum::TimecodeAlbum( const QString & name )
    : QObject()
    , m_name( name )
    , m_isCompilation( false )
    , m_separator( 0 )
    , m_displayCoverAction( 0 )
    , m_fetchCoverAction( 0 )
    , m_setCustomCoverAction( 0 )
    , m_unsetCoverAction( 0 )
{
}

TimecodeAlbum::~ TimecodeAlbum()
{
}

QString
TimecodeAlbum::name() const
{
    return m_name;
}

QString
TimecodeAlbum::prettyName() const
{
    return name();
}

bool
TimecodeAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool TimecodeAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr TimecodeAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
TimecodeAlbum::tracks()
{
    return m_tracks;
}

QPixmap
TimecodeAlbum::image( int size )
{
    if ( m_cover.isNull() )
        return Meta::Album::image( size );

    //only cache during session
    if ( m_coverSizeMap.contains( size ) )
         return m_coverSizeMap.value( size );

    QPixmap scaled = m_cover.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );

    m_coverSizeMap.insert( size, scaled );
    return scaled;
}

bool
TimecodeAlbum::canUpdateImage() const
{
    return true;
}

void
TimecodeAlbum::setImage( const QPixmap & pixmap )
{
    m_cover = pixmap;
    notifyObservers();
}

void
TimecodeAlbum::addTrack( TimecodeTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void TimecodeAlbum::setAlbumArtist( TimecodeArtistPtr artist )
{
    m_albumArtist = artist;
}

void TimecodeAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

bool TimecodeAlbum::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    return type == Capabilities::Capability::CustomActions;
}

Capabilities::Capability* TimecodeAlbum::asCapabilityInterface( Capabilities::Capability::Type type )
{
    if( type == Capabilities::Capability::CustomActions )
    {
        QList<QAction*> actions;

        if ( m_separator == 0 )
        {
            m_separator          = new QAction( this );
            m_displayCoverAction = new DisplayCoverAction( this, AlbumPtr( this ) );
            m_fetchCoverAction   = new FetchCoverAction( this, AlbumPtr( this ) );
            m_setCustomCoverAction = new SetCustomCoverAction( this, AlbumPtr( this ) );
            m_unsetCoverAction   = new UnsetCoverAction( this, AlbumPtr( this ) );
        }

        m_separator->setSeparator( true );

        actions.append( m_separator );
        actions.append( m_displayCoverAction );
        actions.append( m_fetchCoverAction );
        actions.append( m_setCustomCoverAction );
        actions.append( m_unsetCoverAction );

        if( m_cover.isNull() )
        {
            m_displayCoverAction->setEnabled( false );
            m_unsetCoverAction->setEnabled( false );
        }
        else
        {
            m_displayCoverAction->setEnabled( true );
            m_unsetCoverAction->setEnabled( true );
        }

        return new CustomActionsCapability( actions );
    }

    return 0;
}

////////////////// GENRE //////////////////

TimecodeGenre::TimecodeGenre(const QString & name)
    : m_name( name )
{
}

TimecodeGenre::~ TimecodeGenre()
{
}

QString
TimecodeGenre::name() const
{
    return m_name;
}

QString
TimecodeGenre::prettyName() const
{
    return name();
}

TrackList
TimecodeGenre::tracks()
{
    return tracks();
}

void
TimecodeGenre::addTrack( TimecodeTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

////////////////// COMPOSER //////////////////

TimecodeComposer::TimecodeComposer( const QString & name )
    : m_name( name )
{
}

TimecodeComposer::~ TimecodeComposer()
{
}

QString
TimecodeComposer::name() const
{
    return m_name;
}

QString
TimecodeComposer::prettyName() const
{
    return name();
}

TrackList
TimecodeComposer::tracks()
{
    return m_tracks;
}

void
TimecodeComposer::addTrack( TimecodeTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

////////////////// YEAR //////////////////

TimecodeYear::TimecodeYear( const QString & name )
    : m_name( name )
{
}

TimecodeYear::~ TimecodeYear()
{
}

QString
TimecodeYear::name() const
{
    return m_name;
}

QString
TimecodeYear::prettyName() const
{
    return name();
}

TrackList
TimecodeYear::tracks()
{
    return m_tracks;
}

void
TimecodeYear::addTrack( TimecodeTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

#include "TimecodeMeta.moc"
