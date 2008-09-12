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



#include "MtpMeta.h"
#include "MtpCollection.h"
#include "handler/MtpHandler.h"

#include "Debug.h"
#include "SvgHandler.h"
#include "meta/EditCapability.h"
#include "meta/CustomActionsCapability.h"
#include "meta/UpdateCapability.h"

#include "context/popupdropper/PopupDropperAction.h"

#include <KIcon>
#include <KTemporaryFile>
#include <KUrl>

#include <QFileInfo>

using namespace Meta;

class EditCapabilityMtp : public Meta::EditCapability
{
    Q_OBJECT
    public:
        EditCapabilityMtp( MtpTrack *track )
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
        KSharedPtr<MtpTrack> m_track;
};

class CustomActionsCapabilityMtp : public Meta::CustomActionsCapability {
    Q_OBJECT
    public:
        CustomActionsCapabilityMtp( MtpTrack* track )
    : Meta::CustomActionsCapability()
    , m_track( track )
    {
            DEBUG_BLOCK

            MtpCollection *coll = dynamic_cast<MtpCollection*>( m_track->collection() );

            // Setup the remove action
            
            PopupDropperAction *removeAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "delete", KIcon( "amarok_remove" ), i18n( "&Remove from MTP Device" ), 0 );

            debug() << "Remove-action created";

            

            // set track to be deleted

            coll->setTrackToDelete( m_track );

            // when action is selected, collection deletes track

            connect( removeAction, SIGNAL( triggered() ),
                     coll, SLOT(deleteTrackToDelete()) );

            // Add the action to the list of custom actions
            
            m_actions.append( removeAction );



            //TODO: this should only be available in the top-level
            // node of the tree, not every individual track

            // Setup the disconnect action
            
            PopupDropperAction *disconnectAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), "delete", KIcon( "media-track-remove-amarok" ), i18n( "&Disconnect the MTP Device" ), 0 );

            debug() << "Disconnect-action created";

            // when action is selected, collection emits remove()

            connect( disconnectAction, SIGNAL( triggered() ),
                     coll, SLOT( slotDisconnect() ) );

            // Add the action to the list of custom actions

            m_actions.append( disconnectAction );

            debug() << "Disconnect action appended to local QList";

    }
    
        virtual ~CustomActionsCapabilityMtp() {}

        virtual QList< PopupDropperAction *> customActions() const {
            return m_actions;
        }

    private:
        QList< PopupDropperAction* > m_actions;
        MtpTrackPtr m_track;

};

class UpdateCapabilityMtp : public Meta::UpdateCapability
{
    Q_OBJECT
    public:
        UpdateCapabilityMtp( MtpCollection *coll )
    : Meta::UpdateCapability()
                , m_coll( coll ) {}

        virtual void collectionUpdated() const {
            m_coll->collectionUpdated();
        }


    private:
        MtpCollection *m_coll;
};


MtpTrack::MtpTrack( MtpCollection *collection, const QString &format)
    : Meta::Track()
    , m_collection( collection )
    , m_artist( 0 )
    , m_album( 0 )
    , m_genre( 0 )
    , m_composer( 0 )
    , m_year( 0 )
    , m_name()
    , m_type( format )
    , m_length( 0 )
    , m_trackNumber( 0 )
    , m_displayUrl()
    , m_playableUrl()
{
    m_displayUrl = QString();
    m_playableUrl = QString();
    m_isCopied = false;
}

MtpTrack::~MtpTrack()
{
    // clean up temporary file

//    if ( m_tempfile )
//        delete m_tempfile;
}

QString
MtpTrack::name() const
{
    return m_name;
}

QString
MtpTrack::prettyName() const
{
    return m_name;
}

void
MtpTrack::prepareToPlay()
{
    KUrl url;
    if( m_isCopied )
    {
        debug() << "File is already copied, simply return";
        //m_playableUrl = KUrl::fromPath( m_playableUrl );
    }
    else
    {
        debug() << "Beginning temporary file copy";
        m_tempfile.open();
        bool success = !( m_collection->handler()->getTrackToFile( m_id, m_playableUrl ) );
        debug() << "File transfer complete";
        if( success )
        {
            debug() << "File transfer successful!";
            //m_playableUrl = KUrl::fromPath( m_playableUrl );
            m_isCopied = true;
        }
        else
        {
            debug() << "File transfer failed!";
            //m_playableUrl = KUrl::fromPath( "" );
            m_isCopied = false;
        }
    }
}

QString
MtpTrack::setTempFile( const QString &format )
{
    m_tempfile.setSuffix( ("." + format) ); // set suffix based on info from libmtp
    m_tempfile.open();
    QFileInfo tempFileInfo( m_tempfile ); // get info for path
    QString tempPath = tempFileInfo.absoluteFilePath(); // path

//    tempfile->setAutoRemove( false );

    return tempPath;
}

KUrl
MtpTrack::playableUrl() const
{
    KUrl url( m_playableUrl );
    return url;
}

QString
MtpTrack::uidUrl() const
{
    return m_url;
}

QString
MtpTrack::prettyUrl() const
{
    return m_displayUrl;
}

bool
MtpTrack::isPlayable() const
{
    // TODO: somehow temporarily copy file to local disk to play
    return false;
}

bool
MtpTrack::isEditable() const
{
    // TODO: Should only be true if disk mounted read/write, implement check later
    return true;
}

//TODO: port to MTP

LIBMTP_track_t*
MtpTrack::getMtpTrack() const
{
    return m_mtptrack;
}

void
MtpTrack::setMtpTrack ( LIBMTP_track_t *mtptrack )
{
    m_mtptrack = mtptrack;
}
/*
QList<MTP_Playlist*>
MtpTrack::getMtpPlaylists() const
{
    return m_mtpplaylists;
}

void
MtpTrack::addMtpPlaylist ( MTP_Playlist *mtpplaylist )
{
    m_mtpplaylists << mtpplaylist;
}
*/


AlbumPtr
MtpTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr
MtpTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr
MtpTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr
MtpTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr
MtpTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

QString
MtpTrack::comment() const
{
    return QString();
}

void
MtpTrack::setComment( const QString &newComment )
{
    m_comment = newComment;
}

double
MtpTrack::score() const
{
    return 0.0;
}

void
MtpTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
MtpTrack::rating() const
{
    return 0;
}

void
MtpTrack::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

int
MtpTrack::length() const
{
    return m_length;
}

void
MtpTrack::setFileSize( int newFileSize )
{
    m_filesize = newFileSize;
}

int
MtpTrack::filesize() const
{
    // TODO: NYI, seems to cause crashing on transferring tracks to mtp
    return 0;
}

int
MtpTrack::sampleRate() const
{
    return 0;
}

int
MtpTrack::bitrate() const
{
    return m_bitrate;
}

void
MtpTrack::setBitrate( int newBitrate )
{
    m_bitrate = newBitrate;
}

int
MtpTrack::samplerate() const
{
    return m_samplerate;
}

void
MtpTrack::setSamplerate( int newSamplerate )
{
    m_samplerate = newSamplerate;
}

float
MtpTrack::bpm() const
{
    return m_bpm;
}
void
MtpTrack::setBpm( float newBpm )
{
    m_bpm = newBpm;
}

int
MtpTrack::trackNumber() const
{
    return m_trackNumber;
}

void
MtpTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
MtpTrack::discNumber() const
{
    return m_discNumber;
}

void
MtpTrack::setDiscNumber( int newDiscNumber )
{
    m_discNumber = newDiscNumber;
}

int
MtpTrack::playCount() const
{
    return 0;
}

uint
MtpTrack::lastPlayed() const
{
    return 0;
}

QString
MtpTrack::type() const
{
    return m_type;
}

void
MtpTrack::subscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}

void
MtpTrack::unsubscribe( Observer *observer )
{
    Q_UNUSED( observer )    //read only
}
// TODO: implement this for MtpCollectionLocation
bool
MtpTrack::inCollection() const
{
    return true;
}

Collection*
MtpTrack::collection() const
{
    return m_collection;
}

bool
MtpTrack::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    DEBUG_BLOCK
// TODO: NYI
            
        
        switch(  type )
        {
            
            
        case Meta::Capability::Editable:
            return true;
        case Meta::Capability::CustomActions:
            return true;
        case Meta::Capability::Updatable:
            return true;
            

        default:
            return false;
        }
}

Meta::Capability*
MtpTrack::asCapabilityInterface( Meta::Capability::Type type )
{
        DEBUG_BLOCK
        switch( type )
        {
        case Meta::Capability::Editable:
            return new EditCapabilityMtp( this );
            
        case Meta::Capability::CustomActions:
            return new CustomActionsCapabilityMtp( this );
        case Meta::Capability::Updatable:
            return new UpdateCapabilityMtp( m_collection );

        default:
            return 0;
        }
}

void
MtpTrack::setAlbum( const QString &newAlbum )
{

    MtpAlbumPtr albumPtr;
    MtpTrackPtr track( this );
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
        albumPtr = MtpAlbumPtr::staticCast( albumMap.value(  newAlbum ) );
    }
    else
    {
        albumPtr = MtpAlbumPtr(  new MtpAlbum(  newAlbum ) );
        albumMap.insert(  newAlbum,  AlbumPtr::staticCast(  albumPtr ) );
    }

    // add track to album's tracklist
    albumPtr->addTrack( track );
    // set track's album to the new album
    setAlbum( albumPtr );

    m_collection->acquireWriteLock();
    m_collection->setAlbumMap(  albumMap );
    m_collection->releaseLock();

}

void
MtpTrack::setArtist( const QString &newArtist )
{
    DEBUG_BLOCK

    MtpArtistPtr artistPtr;
    MtpTrackPtr track( this );
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
        artistPtr = MtpArtistPtr::staticCast( artistMap.value(  newArtist ) );
    }
    else
    {
        artistPtr = MtpArtistPtr(  new MtpArtist(  newArtist ) );
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
MtpTrack::setGenre( const QString &newGenre )
{
    DEBUG_BLOCK

    MtpGenrePtr genrePtr;
    MtpTrackPtr track( this );
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
        genrePtr = MtpGenrePtr::staticCast( genreMap.value(  newGenre ) );
    }
    else
    {
        genrePtr = MtpGenrePtr(  new MtpGenre(  newGenre ) );
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
MtpTrack::setComposer( const QString &newComposer )
{
    DEBUG_BLOCK

    MtpComposerPtr composerPtr;
    MtpTrackPtr track( this );
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
        composerPtr = MtpComposerPtr::staticCast( composerMap.value(  newComposer ) );
    }
    else
    {
        composerPtr = MtpComposerPtr(  new MtpComposer(  newComposer ) );
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
MtpTrack::setYear( const QString &newYear )
{
    DEBUG_BLOCK

    MtpYearPtr yearPtr;
    MtpTrackPtr track( this );
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
        yearPtr = MtpYearPtr::staticCast( yearMap.value(  newYear ) );
    }
    else
    {
        yearPtr = MtpYearPtr(  new MtpYear(  newYear ) );
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
MtpTrack::setAlbum( MtpAlbumPtr album )
{
    m_album = album;
}

void
MtpTrack::setArtist( MtpArtistPtr artist )
{
    m_artist = artist;
}

void
MtpTrack::setGenre( MtpGenrePtr genre )
{
    m_genre = genre;
}

void
MtpTrack::setComposer( MtpComposerPtr composer )
{
    m_composer = composer;
}

void
MtpTrack::setYear( MtpYearPtr year )
{
    m_year = year;
}

QString
MtpTrack::title() const
{
    return m_name;
}

void
MtpTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
MtpTrack::setLength( int length )
{
    m_length = length;
}

void
MtpTrack::endMetaDataUpdate()
{
    // Update info in local mtp database struct

    m_collection->updateTags( this );
    

    notifyObservers();
}

//MtpArtist

MtpArtist::MtpArtist( const QString &name )
    : Meta::Artist()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MtpArtist::~MtpArtist()
{
    //nothing to do
}

QString
MtpArtist::name() const
{
    return m_name;
}

QString
MtpArtist::prettyName() const
{
    return m_name;
}

TrackList
MtpArtist::tracks()
{
    return m_tracks;
}

AlbumList
MtpArtist::albums()
{
    //TODO
    return AlbumList();
}

void
MtpArtist::addTrack( MtpTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MtpArtist::remTrack( MtpTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

MtpAlbum::MtpAlbum( const QString &name )
    : Meta::Album()
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_albumArtist( 0 )
{
    //nothing to do
}

MtpAlbum::~MtpAlbum()
{
    //nothing to do
}

QString
MtpAlbum::name() const
{
    return m_name;
}

QString
MtpAlbum::prettyName() const
{
    return m_name;
}

bool
MtpAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool
MtpAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
MtpAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
MtpAlbum::tracks()
{
    return m_tracks;
}

QPixmap
MtpAlbum::image( int size, bool withShadow )
{
    return Meta::Album::image( size, withShadow );
}

bool
MtpAlbum::canUpdateImage() const
{
    return false;
}

void
MtpAlbum::setImage( const QImage &image )
{
    Q_UNUSED(image);
    //TODO
}

void
MtpAlbum::addTrack( MtpTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MtpAlbum::remTrack( MtpTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

void
MtpAlbum::setAlbumArtist( MtpArtistPtr artist )
{
    m_albumArtist = artist;
}

void
MtpAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

//MtpGenre

MtpGenre::MtpGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MtpGenre::~MtpGenre()
{
    //nothing to do
}

QString
MtpGenre::name() const
{
    return m_name;
}

QString
MtpGenre::prettyName() const
{
    return m_name;
}

TrackList
MtpGenre::tracks()
{
    return m_tracks;
}

void
MtpGenre::addTrack( MtpTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MtpGenre::remTrack( MtpTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

//MtpComposer

MtpComposer::MtpComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MtpComposer::~MtpComposer()
{
    //nothing to do
}

QString
MtpComposer::name() const
{
    return m_name;
}

QString
MtpComposer::prettyName() const
{
    return m_name;
}

TrackList
MtpComposer::tracks()
{
    return m_tracks;
}

void
MtpComposer::addTrack( MtpTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MtpComposer::remTrack( MtpTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

//MtpYear

MtpYear::MtpYear( const QString &name )
    : Meta::Year()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MtpYear::~MtpYear()
{
    //nothing to do
}

QString
MtpYear::name() const
{
    return m_name;
}

QString
MtpYear::prettyName() const
{
    return m_name;
}

TrackList
MtpYear::tracks()
{
    return m_tracks;
}

void
MtpYear::addTrack( MtpTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
MtpYear::remTrack( MtpTrackPtr track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

#include "mtpmeta.moc"
