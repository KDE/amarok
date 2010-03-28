/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#include "MediaDeviceMeta.h"
#include "MediaDeviceCollection.h"
#include "MediaDeviceHandler.h"

#include "handler/capabilities/ArtworkCapability.h"

// HACK: used to test disconnect
#include "MediaDeviceMonitor.h"

#include "covermanager/CoverFetchingActions.h"
#include "core/support/Debug.h"
#include "SvgHandler.h"
#include "core/capabilities/CustomActionsCapability.h"
#include "core/capabilities/EditCapability.h"
#include "core/capabilities/UpdateCapability.h"

#include <KIcon>
#include <KUrl>

#include <QAction>

using namespace Meta;
// Currently complaining about some vtable issue

class EditCapabilityMediaDevice : public Capabilities::EditCapability
{
    Q_OBJECT
    public:
        EditCapabilityMediaDevice( MediaDeviceTrack *track )
            : Capabilities::EditCapability()
            , m_track( track ) {}

        virtual bool isEditable() const { return m_track->isEditable(); }
        virtual void setAlbum( const QString &newAlbum ) { m_track->setAlbum( newAlbum ); }
        virtual void setArtist( const QString &newArtist ) { m_track->setArtist( newArtist ); }
        virtual void setComposer( const QString &newComposer ) { m_track->setComposer( newComposer ); }
        virtual void setGenre( const QString &newGenre ) { m_track->setGenre( newGenre ); }
        virtual void setYear( const QString &newYear ) { m_track->setYear( newYear ); }
        virtual void setBpm( const qreal newBpm ) { m_track->setBpm( newBpm ); }
        virtual void setTitle( const QString &newTitle ) { m_track->setTitle( newTitle ); }
        virtual void setComment( const QString &newComment ) { m_track->setComment( newComment ); }
        virtual void setTrackNumber( int newTrackNumber ) { m_track->setTrackNumber( newTrackNumber ); }
        virtual void setDiscNumber( int newDiscNumber ) { m_track->setDiscNumber( newDiscNumber ); }
        virtual void beginMetaDataUpdate() { m_track->beginMetaDataUpdate(); }
        virtual void endMetaDataUpdate() { m_track->endMetaDataUpdate(); }
        virtual void abortMetaDataUpdate() { m_track->abortMetaDataUpdate(); }

    private:
        KSharedPtr<MediaDeviceTrack> m_track;
};

class UpdateCapabilityMediaDevice : public Capabilities::UpdateCapability
{
    Q_OBJECT
    public:
        UpdateCapabilityMediaDevice( Collections::MediaDeviceCollection *coll )
            : Capabilities::UpdateCapability()
            , m_coll( coll )
        {}

        virtual void collectionUpdated() const
        {
            m_coll->collectionUpdated();
            m_coll->writeDatabase();
        }

    private:
        Collections::MediaDeviceCollection *m_coll;
};


MediaDeviceTrack::MediaDeviceTrack( Collections::MediaDeviceCollection *collection )
    : Meta::Track()
    , m_collection( collection )
    , m_artist( 0 )
    , m_album( 0 )
    , m_genre( 0 )
    , m_composer( 0 )
    , m_year( 0 )
    , m_image()
    , m_comment()
    , m_name()
    , m_type()
    , m_bitrate( 0 )
    , m_filesize( 0 )
    , m_length( 0 )
    , m_discNumber( 0 )
    , m_samplerate( 0 )
    , m_trackNumber( 0 )
    , m_playCount( 0 )
    , m_lastPlayed( 0 )
    , m_rating( 0 )
    , m_bpm( 0 )
    , m_playableUrl()
{
}

MediaDeviceTrack::~MediaDeviceTrack()
{
    //nothing to do
}

QString
MediaDeviceTrack::name() const
{
    return m_name;
}

QString
MediaDeviceTrack::prettyName() const
{
    return m_name;
}

KUrl
MediaDeviceTrack::playableUrl() const
{
    return m_playableUrl;
}

QString
MediaDeviceTrack::uidUrl() const
{
    return m_playableUrl.isLocalFile() ? m_playableUrl.toLocalFile() : m_playableUrl.url();
}

QString
MediaDeviceTrack::prettyUrl() const
{
    return m_playableUrl.isLocalFile() ? m_playableUrl.toLocalFile() : QString( collection()->prettyName() + ": " + artist()->prettyName() + " - " + prettyName() );
}

bool
MediaDeviceTrack::isPlayable() const
{
    return true;
}

bool
MediaDeviceTrack::isEditable() const
{
    // TODO: Should only be true if disk mounted read/write, implement check later
    return true;
}

AlbumPtr
MediaDeviceTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr
MediaDeviceTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr
MediaDeviceTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr
MediaDeviceTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr
MediaDeviceTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

QString
MediaDeviceTrack::comment() const
{
    return m_comment;
}

void
MediaDeviceTrack::setComment( const QString &newComment )
{
    m_comment = newComment;
}

double
MediaDeviceTrack::score() const
{
    return 0.0;
}

void
MediaDeviceTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
MediaDeviceTrack::rating() const
{
    return m_rating;
}

void
MediaDeviceTrack::setRating( int newRating )
{
    m_rating = newRating;
    notifyObservers();
}

qint64
MediaDeviceTrack::length() const
{
    return m_length;
}

void
MediaDeviceTrack::setFileSize( int newFileSize )
{
    m_filesize = newFileSize;
}

int
MediaDeviceTrack::filesize() const
{
    // TODO: NYI, seems to cause crashing on transferring tracks to mediadevice
    return m_filesize;
}

int
MediaDeviceTrack::bitrate() const
{
    return m_bitrate;
}

void
MediaDeviceTrack::setBitrate( int newBitrate )
{
    m_bitrate = newBitrate;
}

int
MediaDeviceTrack::sampleRate() const
{
    return m_samplerate;
}

void
MediaDeviceTrack::setSamplerate( int newSamplerate )
{
    m_samplerate = newSamplerate;
}

qreal
MediaDeviceTrack::bpm() const
{
    return m_bpm;
}
void
MediaDeviceTrack::setBpm( const qreal newBpm )
{
    m_bpm = newBpm;
}

int
MediaDeviceTrack::trackNumber() const
{
    return m_trackNumber;
}

void
MediaDeviceTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
MediaDeviceTrack::discNumber() const
{
    return m_discNumber;
}

void
MediaDeviceTrack::setDiscNumber( int newDiscNumber )
{
    m_discNumber = newDiscNumber;
}

int
MediaDeviceTrack::playCount() const
{
    return m_playCount;
}

void
MediaDeviceTrack::setPlayCount( const int newCount )
{
    m_playCount = newCount;
}

uint
MediaDeviceTrack::lastPlayed() const
{
    return m_lastPlayed;
}

void
MediaDeviceTrack::setLastPlayed( const uint newTime )
{
    m_lastPlayed = newTime;
}

QString
MediaDeviceTrack::type() const
{
    if( m_type.isEmpty() && !m_playableUrl.path().isEmpty() )
    {
        QString path = m_playableUrl.path();
        return path.mid( path.lastIndexOf( '.' ) + 1 );
    }
    return m_type;
}

void
MediaDeviceTrack::setType( const QString & type )
{
    m_type = type;
}

void
MediaDeviceTrack::prepareToPlay()
{
    Meta::MediaDeviceTrackPtr ptr = Meta::MediaDeviceTrackPtr( this );

    if( m_collection && m_collection->handler() )
        m_collection->handler()->prepareToPlay( ptr );
}

// TODO: employ observers (e.g. Handler) to take care of updated
// data
/*
void
MediaDeviceTrack::subscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}

void
MediaDeviceTrack::unsubscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}
*/
// TODO: implement this for MediaDeviceCollectionLocation
bool
MediaDeviceTrack::inCollection() const
{
    return true;
}

Collections::Collection*
MediaDeviceTrack::collection() const
{
    return m_collection;
}

bool
MediaDeviceTrack::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    switch( type )
    {
        case Capabilities::Capability::Editable:
            return m_collection ? m_collection->isWritable() : false;

        case Capabilities::Capability::Updatable:
            return m_collection ? m_collection->isWritable() : false;

        default:
            return false;
    }
}

Capabilities::Capability*
MediaDeviceTrack::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::Editable:
            return new EditCapabilityMediaDevice( this );
        case Capabilities::Capability::Updatable:
            return new UpdateCapabilityMediaDevice( m_collection );

        default:
            return 0;
    }
}

void
MediaDeviceTrack::setAlbum( const QString &newAlbum )
{
    MediaDeviceAlbumPtr albumPtr;
    MediaDeviceTrackPtr track( this );
    AlbumMap albumMap = m_collection->memoryCollection()->albumMap();

    // do cleanup of soon to be previous album

    albumPtr = m_album;
    if ( !albumPtr.isNull() )
    {
        // remove track from previous album's tracklist
        albumPtr->remTrack( track );
        // if album's tracklist is empty, remove album from albummap
        if( albumPtr->tracks().isEmpty() )
            albumMap.remove( albumPtr->name() );
    }

    // change to a new album

    // check for the existence of the album to be set to,
    // if album exists, reuse, else create

    if ( albumMap.contains( newAlbum ) )
    {
        albumPtr = MediaDeviceAlbumPtr::staticCast( albumMap.value( newAlbum ) );
    }
    else
    {
        albumPtr = MediaDeviceAlbumPtr( new MediaDeviceAlbum( m_collection, newAlbum ) );
        albumMap.insert( newAlbum, AlbumPtr::staticCast( albumPtr ) );
    }

    // add track to album's tracklist
    albumPtr->addTrack( track );
    // set track's album to the new album
    setAlbum( albumPtr );

    m_collection->memoryCollection()->acquireWriteLock();
    m_collection->memoryCollection()->setAlbumMap( albumMap );
    m_collection->memoryCollection()->releaseLock();
}

void
MediaDeviceTrack::setArtist( const QString &newArtist )
{
    DEBUG_BLOCK

    MediaDeviceArtistPtr artistPtr;
    MediaDeviceTrackPtr track( this );
    ArtistMap artistMap = m_collection->memoryCollection()->artistMap();

    // do cleanup of soon to be previous artist

    artistPtr = m_artist;
    // remove track from previous artist's tracklist
    if ( !artistPtr.isNull() )
    {
        artistPtr->remTrack( track );
        // if artist's tracklist is empty, remove artist from artistmap
        if( artistPtr->tracks().isEmpty() )
            artistMap.remove( artistPtr->name() );
    }

    // change to a new artist

    // check for the existence of the artist to be set to,
    // if artist exists, reuse, else create

    if ( artistMap.contains( newArtist ) )
    {
        artistPtr = MediaDeviceArtistPtr::staticCast( artistMap.value( newArtist ) );
    }
    else
    {
        artistPtr = MediaDeviceArtistPtr( new MediaDeviceArtist( newArtist ) );
        artistMap.insert( newArtist, ArtistPtr::staticCast( artistPtr ) );
    }

    // add track to artist's tracklist
    artistPtr->addTrack( track );
    // set track's artist to the new artist
    setArtist( artistPtr );

    m_collection->memoryCollection()->acquireWriteLock();
    m_collection->memoryCollection()->setArtistMap( artistMap );
    m_collection->memoryCollection()->releaseLock();
}

void
MediaDeviceTrack::setGenre( const QString &newGenre )
{
    DEBUG_BLOCK

    MediaDeviceGenrePtr genrePtr;
    MediaDeviceTrackPtr track( this );
    GenreMap genreMap = m_collection->memoryCollection()->genreMap();

    // do cleanup of soon to be previous genre

    genrePtr = m_genre;
    if ( !genrePtr.isNull() )
    {
        // remove track from previous genre's tracklist
        genrePtr->remTrack( track );
        // if genre's tracklist is empty, remove genre from genremap
        if( genrePtr->tracks().isEmpty() )
            genreMap.remove( genrePtr->name() );
    }

    // change to a new genre

    // check for the existence of the genre to be set to,
    // if genre exists, reuse, else create

    if ( genreMap.contains( newGenre ) )
    {
        genrePtr = MediaDeviceGenrePtr::staticCast( genreMap.value( newGenre ) );
    }
    else
    {
        genrePtr = MediaDeviceGenrePtr( new MediaDeviceGenre( newGenre ) );
        genreMap.insert( newGenre, GenrePtr::staticCast( genrePtr ) );
    }

    // add track to genre's tracklist
    genrePtr->addTrack( track );
    // set track's genre to the new genre
    setGenre( genrePtr );

    m_collection->memoryCollection()->acquireWriteLock();
    m_collection->memoryCollection()->setGenreMap( genreMap );
    m_collection->memoryCollection()->releaseLock();
}

void
MediaDeviceTrack::setComposer( const QString &newComposer )
{
    DEBUG_BLOCK

    MediaDeviceComposerPtr composerPtr;
    MediaDeviceTrackPtr track( this );
    ComposerMap composerMap = m_collection->memoryCollection()->composerMap();

    // do cleanup of soon to be previous composer

    composerPtr = m_composer;
    if ( !composerPtr.isNull() )
    {
        // remove track from previous composer's tracklist
        composerPtr->remTrack( track );
        // if composer's tracklist is empty, remove composer from composermap
        if( composerPtr->tracks().isEmpty() )
            composerMap.remove( composerPtr->name() );
    }

    // change to a new composer

    // check for the existence of the composer to be set to,
    // if composer exists, reuse, else create

    if ( composerMap.contains( newComposer ) )
    {
        composerPtr = MediaDeviceComposerPtr::staticCast( composerMap.value( newComposer ) );
    }
    else
    {
        composerPtr = MediaDeviceComposerPtr( new MediaDeviceComposer( newComposer ) );
        composerMap.insert( newComposer, ComposerPtr::staticCast( composerPtr ) );
    }

    // add track to composer's tracklist
    composerPtr->addTrack( track );
    // set track's composer to the new composer
    setComposer( composerPtr );

    m_collection->memoryCollection()->acquireWriteLock();
    m_collection->memoryCollection()->setComposerMap( composerMap );
    m_collection->memoryCollection()->releaseLock();
}

void
MediaDeviceTrack::setYear( const QString &newYear )
{
    DEBUG_BLOCK

    MediaDeviceYearPtr yearPtr;
    MediaDeviceTrackPtr track( this );
    YearMap yearMap = m_collection->memoryCollection()->yearMap();

    // do cleanup of soon to be previous year

    yearPtr = m_year;
    if ( !yearPtr.isNull() )
    {
        // remove track from previous year's tracklist
        yearPtr->remTrack( track );
        // if year's tracklist is empty, remove year from yearmap
        if( yearPtr->tracks().isEmpty() )
            yearMap.remove( yearPtr->name() );
    }

    // change to a new year

    // check for the existence of the year to be set to,
    // if year exists, reuse, else create

    if ( yearMap.contains( newYear ) )
    {
        yearPtr = MediaDeviceYearPtr::staticCast( yearMap.value( newYear ) );
    }
    else
    {
        yearPtr = MediaDeviceYearPtr( new MediaDeviceYear( newYear ) );
        yearMap.insert( newYear, YearPtr::staticCast( yearPtr ) );
    }

    // add track to year's tracklist
    yearPtr->addTrack( track );
    // set track's year to the new year
    setYear( yearPtr );

    m_collection->memoryCollection()->acquireWriteLock();
    m_collection->memoryCollection()->setYearMap( yearMap );
    m_collection->memoryCollection()->releaseLock();
}

void
MediaDeviceTrack::setAlbum( MediaDeviceAlbumPtr album )
{
    m_album = album;
}

void
MediaDeviceTrack::setArtist( MediaDeviceArtistPtr artist )
{
    m_artist = artist;
}

void
MediaDeviceTrack::setGenre( MediaDeviceGenrePtr genre )
{
    m_genre = genre;
}

void
MediaDeviceTrack::setComposer( MediaDeviceComposerPtr composer )
{
    m_composer = composer;
}

void
MediaDeviceTrack::setYear( MediaDeviceYearPtr year )
{
    m_year = year;
}

QString
MediaDeviceTrack::title() const
{
    return m_name;
}

void
MediaDeviceTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
MediaDeviceTrack::setLength( qint64 length )
{
    m_length = length;
}

void
MediaDeviceTrack::endMetaDataUpdate()
{
    DEBUG_BLOCK
    // Update info in local mediadevice database struct
    debug() << "Observer number: " << m_observers.count();
    notifyObservers();
}

//MediaDeviceArtist

MediaDeviceArtist::MediaDeviceArtist( const QString &name )
    : Meta::Artist()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceArtist::~MediaDeviceArtist()
{
    //nothing to do
}

QString
MediaDeviceArtist::name() const
{
    return m_name;
}

QString
MediaDeviceArtist::prettyName() const
{
    return m_name;
}

TrackList
MediaDeviceArtist::tracks()
{
    return m_tracks;
}

AlbumList
MediaDeviceArtist::albums()
{
    return m_albums;
}

void
MediaDeviceArtist::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MediaDeviceArtist::remTrack( MediaDeviceTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

void
MediaDeviceArtist::addAlbum( MediaDeviceAlbumPtr album )
{
    m_albums.append( AlbumPtr::staticCast( album ) );
}

void
MediaDeviceArtist::remAlbum( MediaDeviceAlbumPtr album )
{
    m_albums.removeOne( AlbumPtr::staticCast( album ) );
}

//---------------MediaDeviceAlbum-----------------------------------

MediaDeviceAlbum::MediaDeviceAlbum( Collections::MediaDeviceCollection *collection, const QString &name )
    : Meta::Album()
    , m_collection( collection )
    , m_artworkCapability( 0 )
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_hasImage( true ) // assume it has a cover until proven otherwise
    , m_hasImageChecked( false )
    , m_image( QPixmap() )
    , m_albumArtist( 0 )
{
    MediaDeviceHandler *handler = m_collection->handler();
    if( handler && handler->hasCapabilityInterface( Handler::Capability::Artwork ) )
        m_artworkCapability = handler->create<Handler::ArtworkCapability>();
}

MediaDeviceAlbum::~MediaDeviceAlbum()
{
    //nothing to do
}

QString
MediaDeviceAlbum::name() const
{
    return m_name;
}

QString
MediaDeviceAlbum::prettyName() const
{
    return m_name;
}

bool
MediaDeviceAlbum::isCompilation() const
{
    return m_isCompilation;
}

void
MediaDeviceAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

bool
MediaDeviceAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
MediaDeviceAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
MediaDeviceAlbum::tracks()
{
    return m_tracks;
}

bool
MediaDeviceAlbum::hasImage( int size ) const
{
    Q_UNUSED( size )

    if( !m_hasImageChecked )
        m_hasImage = ! const_cast<MediaDeviceAlbum*>( this )->image().isNull();
    return m_hasImage;
}

QPixmap
MediaDeviceAlbum::image( int size )
{
    if( m_name.isEmpty() || !m_hasImage )
        return Meta::Album::image( size );

    if( !m_image.isNull() )
    {
        if( !size )
            return m_image;
        return m_image.scaled( QSize( size, size ), Qt::KeepAspectRatio );
    }
    if( m_artworkCapability )
    {
        MediaDeviceTrackPtr track = MediaDeviceTrackPtr::dynamicCast( m_tracks.first() );
        QPixmap cover = m_artworkCapability->getCover( track );

        if( !cover.isNull() )
        {
            m_hasImage = true;
            m_image = cover;
            if( !size )
                return m_image;
            return m_image.scaled( QSize( size, size ), Qt::KeepAspectRatio );
        }
        else
            m_hasImage = false;
        m_hasImageChecked = true;
    }
    return Meta::Album::image( size );
}

bool
MediaDeviceAlbum::canUpdateImage() const
{
    if( m_artworkCapability )
        return m_artworkCapability->canUpdateCover();
    return false;
}

// TODO: forward setImage calls to handler
void
MediaDeviceAlbum::setImage( const QPixmap &pixmap )
{
    if( m_artworkCapability && m_artworkCapability->canUpdateCover() )
    {
        m_image = pixmap;
        m_hasImage = true;
        m_artworkCapability->setCover( MediaDeviceAlbumPtr( this ), pixmap );
    }
}

void
MediaDeviceAlbum::setImagePath( const QString &path )
{
    if( m_artworkCapability && m_artworkCapability->canUpdateCover() )
    {
        m_hasImage = true;
        m_artworkCapability->setCoverPath( MediaDeviceAlbumPtr( this ), path );
    }
}

// TODO: forward call to handler to remove image, etc.
void
MediaDeviceAlbum::removeImage()
{
    Meta::Album::removeImage();
}

bool
MediaDeviceAlbum::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    switch( type )
    {
        case Capabilities::Capability::CustomActions:
            return true;
        default:
            return false;
    }
}

Capabilities::Capability*
MediaDeviceAlbum::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::CustomActions:
        {
            QList<QAction*> actions;
            if( canUpdateImage() )
            {
                QAction *separator          = new QAction( m_collection );
                QAction *displayCoverAction = new DisplayCoverAction( m_collection, AlbumPtr::dynamicCast( MediaDeviceAlbumPtr(this) ) );
                QAction *unsetCoverAction   = new UnsetCoverAction( m_collection, AlbumPtr::dynamicCast( MediaDeviceAlbumPtr(this) ) );

                separator->setSeparator( true );

                actions.append( separator );
                actions.append( displayCoverAction );
                actions.append( new FetchCoverAction( m_collection, AlbumPtr::staticCast( MediaDeviceAlbumPtr(this) ) ) );
                actions.append( new SetCustomCoverAction( m_collection, AlbumPtr::staticCast( MediaDeviceAlbumPtr(this) ) ) );
                if( !hasImage() )
                {
                    displayCoverAction->setEnabled( false );
                    unsetCoverAction->setEnabled( false );
                }
                actions.append( unsetCoverAction );
            }
            return new Capabilities::CustomActionsCapability( actions );
        }

        default:
            return 0;
    }
}

void
MediaDeviceAlbum::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MediaDeviceAlbum::remTrack( MediaDeviceTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

void
MediaDeviceAlbum::setAlbumArtist( MediaDeviceArtistPtr artist )
{
    m_albumArtist = artist;
}

//MediaDeviceComposer

MediaDeviceComposer::MediaDeviceComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceComposer::~MediaDeviceComposer()
{
    //nothing to do
}

QString
MediaDeviceComposer::name() const
{
    return m_name;
}

QString
MediaDeviceComposer::prettyName() const
{
    return m_name;
}

TrackList
MediaDeviceComposer::tracks()
{
    return m_tracks;
}

void
MediaDeviceComposer::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MediaDeviceComposer::remTrack( MediaDeviceTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

//---------------MediaDeviceGenre-----------------------------------

MediaDeviceGenre::MediaDeviceGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceGenre::~MediaDeviceGenre()
{
    //nothing to do
}

QString
MediaDeviceGenre::name() const
{
    return m_name;
}

QString
MediaDeviceGenre::prettyName() const
{
    return m_name;
}

TrackList
MediaDeviceGenre::tracks()
{
    return m_tracks;
}

void
MediaDeviceGenre::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MediaDeviceGenre::remTrack( MediaDeviceTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}


//MediaDeviceYear

MediaDeviceYear::MediaDeviceYear( const QString &name )
    : Meta::Year()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceYear::~MediaDeviceYear()
{
    //nothing to do
}

QString
MediaDeviceYear::name() const
{
    return m_name;
}

QString
MediaDeviceYear::prettyName() const
{
    return m_name;
}

TrackList
MediaDeviceYear::tracks()
{
    return m_tracks;
}

void
MediaDeviceYear::addTrack( MediaDeviceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MediaDeviceYear::remTrack( MediaDeviceTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

#include "mediadevicemeta.moc"
