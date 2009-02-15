/* This file is part of the KDE project

   Note: Mostly taken from Daap code:
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/



#include "IpodMeta.h"
#include "IpodCollection.h"
#include "handler/IpodHandler.h"

#include "Debug.h"
#include "SvgHandler.h"
#include "meta/capabilities/EditCapability.h"
#include "meta/capabilities/CustomActionsCapability.h"
#include "meta/capabilities/UpdateCapability.h"

#include "context/popupdropper/libpud/PopupDropperAction.h"

#include <KIcon>
#include <KUrl>

using namespace Meta;
// Currently complaining about some vtable issue

class EditCapabilityIpod : public Meta::EditCapability
{
    Q_OBJECT
    public:
        EditCapabilityIpod( IpodTrack *track )
            : Meta::EditCapability()
            , m_track( track ) {}

        virtual bool isEditable() const { return m_track->isEditable(); }
        virtual void setAlbum( const QString &newAlbum ) { m_track->setAlbum( newAlbum ); }
        virtual void setArtist( const QString &newArtist ) { m_track->setArtist( newArtist ); }
        virtual void setComposer( const QString &newComposer ) { m_track->setComposer( newComposer ); }
        virtual void setGenre( const QString &newGenre ) { m_track->setGenre( newGenre ); }
        virtual void setYear( const QString &newYear ) { m_track->setYear( newYear ); }
        virtual void setTitle( const QString &newTitle ) { m_track->setTitle( newTitle ); }
        virtual void setComment( const QString &newComment ) { m_track->setComment( newComment ); }
        virtual void setTrackNumber( int newTrackNumber ) { m_track->setTrackNumber( newTrackNumber ); }
        virtual void setDiscNumber( int newDiscNumber ) { m_track->setDiscNumber( newDiscNumber ); }
        virtual void beginMetaDataUpdate() { m_track->beginMetaDataUpdate(); }
        virtual void endMetaDataUpdate() { m_track->endMetaDataUpdate(); }
        virtual void abortMetaDataUpdate() { m_track->abortMetaDataUpdate(); }

    private:
        KSharedPtr<IpodTrack> m_track;
};

class CustomActionsCapabilityIpod : public Meta::CustomActionsCapability
{
    Q_OBJECT
    public:
        CustomActionsCapabilityIpod( IpodTrack* track )
            : Meta::CustomActionsCapability()
            , m_track( track )
        {
            DEBUG_BLOCK

            // Setup the remove action

            PopupDropperAction *removeAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), 
                                                                    "delete", KIcon( "remove-amarok" ), i18n( "&Remove from iPod" ), 0 );
            debug() << "Remove-action created";

            IpodCollection *coll = dynamic_cast<IpodCollection*>( m_track->collection() );

            // set track to be deleted
            coll->setTrackToDelete( m_track );

            // when action is selected, collection deletes track
            connect( removeAction, SIGNAL( triggered() ), coll, SLOT(deleteTrackToDelete()) );

            // Add the action to the list of custom actions
            m_actions.append( removeAction );

            // Setup the disconnect action
            PopupDropperAction *disconnectAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), 
                                                        "delete", KIcon( "media-track-remove-amarok" ), i18n( "&Disconnect the iPod" ), 0 );
            debug() << "Disconnect-action created";

            // when action is selected, collection emits remove()
            connect( disconnectAction, SIGNAL( triggered() ),
                     coll, SLOT( slotDisconnect() ) );

            // Add the action to the list of custom actions
            m_actions.append( disconnectAction );
            debug() << "Disconnect action appended to local QList";
        }
    
        virtual ~CustomActionsCapabilityIpod() {}

        virtual QList< PopupDropperAction *> customActions() const {
            return m_actions;
        }

    private:
        QList< PopupDropperAction* > m_actions;
        IpodTrackPtr m_track;

};

class UpdateCapabilityIpod : public Meta::UpdateCapability
{
    Q_OBJECT
    public:
        UpdateCapabilityIpod( IpodCollection *coll )
            : Meta::UpdateCapability()
            , m_coll( coll )
        {}

        virtual void collectionUpdated() const
        {
            m_coll->collectionUpdated();
            m_coll->writeDatabase();
        }


    private:
        IpodCollection *m_coll;
};


IpodTrack::IpodTrack( IpodCollection *collection )
    : Meta::Track()
    , m_collection( collection )
    , m_artist( 0 )
    , m_album( 0 )
    , m_genre( 0 )
    , m_composer( 0 )
    , m_year( 0 )
    , m_ipodtrack( 0 )
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
    , m_displayUrl()
    , m_playableUrl()
{
  //QString url = QString( "ipod://%1:%2/%3/%4.%5" )
  //                .arg( host, QString::number( port ), dbId, itemId, format );
}

IpodTrack::~IpodTrack()
{
    //nothing to do
    
}

QString
IpodTrack::name() const
{
    return m_name;
}

QString
IpodTrack::prettyName() const
{
    return m_name;
}

KUrl
IpodTrack::playableUrl() const
{
    KUrl url( m_playableUrl );
    return url;
}

QString
IpodTrack::uidUrl() const
{
    return m_playableUrl;
}

QString
IpodTrack::prettyUrl() const
{
    return m_displayUrl;
}

bool
IpodTrack::isPlayable() const
{
    return true;
}

bool
IpodTrack::isEditable() const
{
    // TODO: Should only be true if disk mounted read/write, implement check later
    return true;
}

Itdb_Track*
IpodTrack::getIpodTrack() const
{
    return m_ipodtrack;
}

void
IpodTrack::setIpodTrack ( Itdb_Track *ipodtrack )
{
    m_ipodtrack = ipodtrack;
}

QList<Itdb_Playlist*>
IpodTrack::getIpodPlaylists() const
{
    return m_ipodplaylists;
}

void
IpodTrack::addIpodPlaylist ( Itdb_Playlist *ipodplaylist )
{
    m_ipodplaylists << ipodplaylist;
}



AlbumPtr
IpodTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr
IpodTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr
IpodTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr
IpodTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr
IpodTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

QString
IpodTrack::comment() const
{
    return QString();
}

void
IpodTrack::setComment( const QString &newComment )
{
    m_comment = newComment;
}

double
IpodTrack::score() const
{
    return 0.0;
}

void
IpodTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
IpodTrack::rating() const
{
    return m_rating;
}

void
IpodTrack::setRating( int newRating )
{
    m_rating = newRating;
    notifyObservers();
}

int
IpodTrack::length() const
{
    return m_length;
}

void
IpodTrack::setFileSize( int newFileSize )
{
    m_filesize = newFileSize;
}

int
IpodTrack::filesize() const
{
    // TODO: NYI, seems to cause crashing on transferring tracks to ipod
    return m_filesize;
}

int
IpodTrack::bitrate() const
{
    return m_bitrate;
}

void
IpodTrack::setBitrate( int newBitrate )
{
    m_bitrate = newBitrate;
}

int
IpodTrack::sampleRate() const
{
    return m_samplerate;
}

void
IpodTrack::setSamplerate( int newSamplerate )
{
    m_samplerate = newSamplerate;
}

float
IpodTrack::bpm() const
{
    return m_bpm;
}
void
IpodTrack::setBpm( float newBpm )
{
    m_bpm = newBpm;
}

int
IpodTrack::trackNumber() const
{
    return m_trackNumber;
}

void
IpodTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
IpodTrack::discNumber() const
{
    return m_discNumber;
}

void
IpodTrack::setDiscNumber( int newDiscNumber )
{
    m_discNumber = newDiscNumber;
}

int
IpodTrack::playCount() const
{
    return m_playCount;
}

void
IpodTrack::setPlayCount( const int newCount )
{
    m_playCount = newCount;
}

uint
IpodTrack::lastPlayed() const
{
    return m_lastPlayed;
}

void
IpodTrack::setLastPlayed( const uint newTime )
{
    m_lastPlayed = newTime;
}

QString
IpodTrack::type() const
{
    if( m_type.isEmpty() && !m_playableUrl.isEmpty() )
        return m_playableUrl.mid( m_playableUrl.lastIndexOf( '.' ) + 1 );
    return m_type;
}

void
IpodTrack::setType( const QString & type )
{
    m_type = type;
}
/*
void
IpodTrack::subscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}

void
IpodTrack::unsubscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}
*/
// TODO: implement this for IpodCollectionLocation
bool
IpodTrack::inCollection() const
{
    return true;
}

Amarok::Collection*
IpodTrack::collection() const
{
    return m_collection;
}

bool
IpodTrack::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    switch( type )
    {
        case Meta::Capability::Editable:
            return true;
        case Meta::Capability::CustomActions:
            return false;
        case Meta::Capability::Updatable:
            return true;

        default:
            return false;
    }
}

Meta::Capability*
IpodTrack::asCapabilityInterface( Meta::Capability::Type type )
{
    switch( type )
    {
        case Meta::Capability::Editable:
            return new EditCapabilityIpod( this );
        case Meta::Capability::CustomActions:
            return 0;
            //return new CustomActionsCapabilityIpod( this );
        case Meta::Capability::Updatable:
            return new UpdateCapabilityIpod( m_collection );

        default:
            return 0;
    }
}

void
IpodTrack::setAlbum( const QString &newAlbum )
{
    IpodAlbumPtr albumPtr;
    IpodTrackPtr track( this );
    AlbumMap albumMap = m_collection->albumMap();

    // do cleanup of soon to be previous album

    albumPtr = m_album;
    // remove track from previous album's tracklist
    albumPtr->remTrack( track );
    // if album's tracklist is empty, remove album from albummap
    if( albumPtr->tracks().isEmpty() )
        albumMap.remove( albumPtr->name() );

    // change to a new album
    
    // check for the existence of the album to be set to,
    // if album exists, reuse, else create
    
    if (  albumMap.contains( newAlbum ) )
    {
        albumPtr = IpodAlbumPtr::staticCast( albumMap.value(  newAlbum ) );
    }
    else
    {
        albumPtr = IpodAlbumPtr( new IpodAlbum( newAlbum ) );
        albumMap.insert( newAlbum, AlbumPtr::staticCast( albumPtr ) );
    }

    // add track to album's tracklist
    albumPtr->addTrack( track );
    // set track's album to the new album
    setAlbum( albumPtr );

    m_collection->acquireWriteLock();
    m_collection->setAlbumMap( albumMap );
    m_collection->releaseLock();

}

void
IpodTrack::setArtist( const QString &newArtist )
{
    DEBUG_BLOCK

    IpodArtistPtr artistPtr;
    IpodTrackPtr track( this );
    ArtistMap artistMap = m_collection->artistMap();

    // do cleanup of soon to be previous artist

    artistPtr = m_artist;
    // remove track from previous artist's tracklist
    artistPtr->remTrack( track );
    // if artist's tracklist is empty, remove artist from artistmap
    if( artistPtr->tracks().isEmpty() )
        artistMap.remove( artistPtr->name() );

    // change to a new artist
    
    // check for the existence of the artist to be set to,
    // if artist exists, reuse, else create
    
    if (  artistMap.contains( newArtist ) )
    {
        artistPtr = IpodArtistPtr::staticCast( artistMap.value(  newArtist ) );
    }
    else
    {
        artistPtr = IpodArtistPtr(  new IpodArtist(  newArtist ) );
        artistMap.insert(  newArtist,  ArtistPtr::staticCast(  artistPtr ) );
    }

    // add track to artist's tracklist
    artistPtr->addTrack( track );
    // set track's artist to the new artist
    setArtist( artistPtr );

    m_collection->acquireWriteLock();
    m_collection->setArtistMap(  artistMap );
    m_collection->releaseLock();

}

void
IpodTrack::setGenre( const QString &newGenre )
{
    DEBUG_BLOCK

    IpodGenrePtr genrePtr;
    IpodTrackPtr track( this );
    GenreMap genreMap = m_collection->genreMap();

    // do cleanup of soon to be previous genre

    genrePtr = m_genre;
    // remove track from previous genre's tracklist
    genrePtr->remTrack( track );
    // if genre's tracklist is empty, remove genre from genremap
    if( genrePtr->tracks().isEmpty() )
        genreMap.remove( genrePtr->name() );

    // change to a new genre
    
    // check for the existence of the genre to be set to,
    // if genre exists, reuse, else create
    
    if (  genreMap.contains( newGenre ) )
    {
        genrePtr = IpodGenrePtr::staticCast( genreMap.value(  newGenre ) );
    }
    else
    {
        genrePtr = IpodGenrePtr(  new IpodGenre(  newGenre ) );
        genreMap.insert(  newGenre,  GenrePtr::staticCast(  genrePtr ) );
    }

    // add track to genre's tracklist
    genrePtr->addTrack( track );
    // set track's genre to the new genre
    setGenre( genrePtr );

    m_collection->acquireWriteLock();
    m_collection->setGenreMap(  genreMap );
    m_collection->releaseLock();

}

void
IpodTrack::setComposer( const QString &newComposer )
{
    DEBUG_BLOCK

    IpodComposerPtr composerPtr;
    IpodTrackPtr track( this );
    ComposerMap composerMap = m_collection->composerMap();

    // do cleanup of soon to be previous composer

    composerPtr = m_composer;
    // remove track from previous composer's tracklist
    composerPtr->remTrack( track );
    // if composer's tracklist is empty, remove composer from composermap
    if( composerPtr->tracks().isEmpty() )
        composerMap.remove( composerPtr->name() );

    // change to a new composer
    
    // check for the existence of the composer to be set to,
    // if composer exists, reuse, else create
    
    if (  composerMap.contains( newComposer ) )
    {
        composerPtr = IpodComposerPtr::staticCast( composerMap.value(  newComposer ) );
    }
    else
    {
        composerPtr = IpodComposerPtr(  new IpodComposer(  newComposer ) );
        composerMap.insert(  newComposer,  ComposerPtr::staticCast(  composerPtr ) );
    }

    // add track to composer's tracklist
    composerPtr->addTrack( track );
    // set track's composer to the new composer
    setComposer( composerPtr );

    m_collection->acquireWriteLock();
    m_collection->setComposerMap(  composerMap );
    m_collection->releaseLock();

}

void
IpodTrack::setYear( const QString &newYear )
{
    DEBUG_BLOCK

    IpodYearPtr yearPtr;
    IpodTrackPtr track( this );
    YearMap yearMap = m_collection->yearMap();

    // do cleanup of soon to be previous year

    yearPtr = m_year;
    // remove track from previous year's tracklist
    yearPtr->remTrack( track );
    // if year's tracklist is empty, remove year from yearmap
    if( yearPtr->tracks().isEmpty() )
        yearMap.remove( yearPtr->name() );

    // change to a new year
    
    // check for the existence of the year to be set to,
    // if year exists, reuse, else create
    
    if (  yearMap.contains( newYear ) )
    {
        yearPtr = IpodYearPtr::staticCast( yearMap.value(  newYear ) );
    }
    else
    {
        yearPtr = IpodYearPtr(  new IpodYear(  newYear ) );
        yearMap.insert(  newYear,  YearPtr::staticCast(  yearPtr ) );
    }

    // add track to year's tracklist
    yearPtr->addTrack( track );
    // set track's year to the new year
    setYear( yearPtr );

    m_collection->acquireWriteLock();
    m_collection->setYearMap(  yearMap );
    m_collection->releaseLock();

}

void
IpodTrack::setAlbum( IpodAlbumPtr album )
{
    m_album = album;
}

void
IpodTrack::setArtist( IpodArtistPtr artist )
{
    m_artist = artist;
}

void
IpodTrack::setGenre( IpodGenrePtr genre )
{
    m_genre = genre;
}

void
IpodTrack::setComposer( IpodComposerPtr composer )
{
    m_composer = composer;
}

void
IpodTrack::setYear( IpodYearPtr year )
{
    m_year = year;
}

QString
IpodTrack::title() const
{
    return m_name;
}

void
IpodTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
IpodTrack::setLength( int length )
{
    m_length = length;
}

void
IpodTrack::endMetaDataUpdate()
{
    // Update info in local ipod database struct

    m_collection->updateTags( this );
    

    notifyObservers();
}

//IpodArtist

IpodArtist::IpodArtist( const QString &name )
    : Meta::Artist()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

IpodArtist::~IpodArtist()
{
    //nothing to do
}

QString
IpodArtist::name() const
{
    return m_name;
}

QString
IpodArtist::prettyName() const
{
    return m_name;
}

TrackList
IpodArtist::tracks()
{
    return m_tracks;
}

AlbumList
IpodArtist::albums()
{
    //TODO
    return AlbumList();
}

void
IpodArtist::addTrack( IpodTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
IpodArtist::remTrack( IpodTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

IpodAlbum::IpodAlbum( const QString &name )
    : Meta::Album()
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_hasCover( false )
    , m_image()
    , m_albumArtist( 0 )
{
    //nothing to do
}

IpodAlbum::~IpodAlbum()
{
    //nothing to do
}

QString
IpodAlbum::name() const
{
    return m_name;
}

QString
IpodAlbum::prettyName() const
{
    return m_name;
}

bool
IpodAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool
IpodAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
IpodAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
IpodAlbum::tracks()
{
    return m_tracks;
}

QPixmap
IpodAlbum::image( int size )
{
    //DEBUG_BLOCK
    if( m_hasCover )
        return ( QPixmap::fromImage( m_image.scaled( QSize( size, size ), Qt::KeepAspectRatio ) ) );
    
    return Meta::Album::image( size );
}

bool
IpodAlbum::canUpdateImage() const
{
    return false;
}

void
IpodAlbum::setImage( const QImage &image )
{
    m_image = image;
    m_hasCover = true;
}

void
IpodAlbum::setImagePath( const QString &path )
{
    m_coverPath = path;
    m_hasCover = true;
}

void
IpodAlbum::addTrack( IpodTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
IpodAlbum::remTrack( IpodTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

void
IpodAlbum::setAlbumArtist( IpodArtistPtr artist )
{
    m_albumArtist = artist;
}

void
IpodAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

//IpodGenre

IpodGenre::IpodGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

IpodGenre::~IpodGenre()
{
    //nothing to do
}

QString
IpodGenre::name() const
{
    return m_name;
}

QString
IpodGenre::prettyName() const
{
    return m_name;
}

TrackList
IpodGenre::tracks()
{
    return m_tracks;
}

void
IpodGenre::addTrack( IpodTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
IpodGenre::remTrack( IpodTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

//IpodComposer

IpodComposer::IpodComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

IpodComposer::~IpodComposer()
{
    //nothing to do
}

QString
IpodComposer::name() const
{
    return m_name;
}

QString
IpodComposer::prettyName() const
{
    return m_name;
}

TrackList
IpodComposer::tracks()
{
    return m_tracks;
}

void
IpodComposer::addTrack( IpodTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
IpodComposer::remTrack( IpodTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

//IpodYear

IpodYear::IpodYear( const QString &name )
    : Meta::Year()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

IpodYear::~IpodYear()
{
    //nothing to do
}

QString
IpodYear::name() const
{
    return m_name;
}

QString
IpodYear::prettyName() const
{
    return m_name;
}

TrackList
IpodYear::tracks()
{
    return m_tracks;
}

void
IpodYear::addTrack( IpodTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
IpodYear::remTrack( IpodTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

#include "ipodmeta.moc"
