/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "File.h"
#include "File_p.h"

#include <config-amarok.h>
#include "MainWindow.h"
#include "amarokurls/BookmarkMetaActions.h"
#include "amarokurls/PlayUrlRunner.h"
#include "browsers/filebrowser/FileBrowser.h"
#include "core/capabilities/BookmarkThisCapability.h"
#include "core/capabilities/EditCapability.h"
#include "core/capabilities/FindInSourceCapability.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core/playlists/PlaylistFormat.h"
#include "core/support/Amarok.h"
#include "core-impl/capabilities/timecode/TimecodeWriteCapability.h"
#include "core-impl/capabilities/timecode/TimecodeLoadCapability.h"
#include "core-impl/support/UrlStatisticsStore.h"

#include <KMimeType>

#include <QAction>
#include <QFileInfo>
#include <QList>
#include <QWeakPointer>
#include <QString>

#ifdef HAVE_LIBLASTFM
  #include "LastfmReadLabelCapability.h"
#endif
using namespace MetaFile;

class EditCapabilityImpl : public Capabilities::EditCapability
{
    Q_OBJECT
    public:
        EditCapabilityImpl( MetaFile::Track *track )
            : Capabilities::EditCapability()
            , m_track( track )
        {}

        virtual bool isEditable() const { return m_track->isEditable(); }
        virtual void setAlbum( const QString &newAlbum ) { m_track->setAlbum( newAlbum ); }
        virtual void setAlbumArtist( const QString &newAlbumArtist ) { m_track->setAlbumArtist( newAlbumArtist ); }
        virtual void setArtist( const QString &newArtist ) { m_track->setArtist( newArtist ); }
        virtual void setComposer( const QString &newComposer ) { m_track->setComposer( newComposer ); }
        virtual void setGenre( const QString &newGenre ) { m_track->setGenre( newGenre ); }
        virtual void setYear( int newYear ) { m_track->setYear( newYear ); }
        virtual void setBpm( const qreal newBpm ) { m_track->setBpm( newBpm ); }
        virtual void setTitle( const QString &newTitle ) { m_track->setTitle( newTitle ); }
        virtual void setComment( const QString &newComment ) { m_track->setComment( newComment ); }
        virtual void setTrackNumber( int newTrackNumber ) { m_track->setTrackNumber( newTrackNumber ); }
        virtual void setDiscNumber( int newDiscNumber ) { m_track->setDiscNumber( newDiscNumber ); }
        virtual void beginMetaDataUpdate() { m_track->beginMetaDataUpdate(); }
        virtual void endMetaDataUpdate() { m_track->endMetaDataUpdate(); }

    private:
        KSharedPtr<MetaFile::Track> m_track;
};

class TimecodeWriteCapabilityImpl : public Capabilities::TimecodeWriteCapability
{
    public:
        TimecodeWriteCapabilityImpl( MetaFile::Track *track )
            : Capabilities::TimecodeWriteCapability()
            , m_track( track )
        {}

    virtual bool writeTimecode ( qint64 miliseconds )
    {
        DEBUG_BLOCK
        return Capabilities::TimecodeWriteCapability::writeTimecode( miliseconds, Meta::TrackPtr( m_track.data() ) );
    }

    virtual bool writeAutoTimecode ( qint64 miliseconds )
    {
        DEBUG_BLOCK
        return Capabilities::TimecodeWriteCapability::writeAutoTimecode( miliseconds, Meta::TrackPtr( m_track.data() ) );
    }

    private:
        KSharedPtr<MetaFile::Track> m_track;
};

class TimecodeLoadCapabilityImpl : public Capabilities::TimecodeLoadCapability
{
    public:
        TimecodeLoadCapabilityImpl( MetaFile::Track *track )
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
            BookmarkList list = PlayUrlRunner::bookmarksFromUrl( m_track->playableUrl() );
            return list;
        }

    private:
        KSharedPtr<MetaFile::Track> m_track;
};


class FindInSourceCapabilityImpl : public Capabilities::FindInSourceCapability
{
public:
    FindInSourceCapabilityImpl( MetaFile::Track *track )
        : Capabilities::FindInSourceCapability()
        , m_track( track )
        {}

    virtual void findInSource( QFlags<TargetTag> tag )
    {
        Q_UNUSED( tag )
        //first show the filebrowser
        AmarokUrl url;
        url.setCommand( "navigate" );
        url.setPath( "files" );
        url.run();

        //then navigate to the correct directory
        BrowserCategory * fileCategory = The::mainWindow()->browserDock()->list()->activeCategoryRecursive();
        if( fileCategory )
        {
            FileBrowser * fileBrowser = dynamic_cast<FileBrowser *>( fileCategory );
            if( fileBrowser )
            {
                //get the path of the parent directory of the file
                KUrl playableUrl = m_track->playableUrl();
                fileBrowser->setDir( playableUrl.directory() );       
            }
        }
    }

private:
    KSharedPtr<MetaFile::Track> m_track;
};


Track::Track( const KUrl &url )
    : Meta::Track()
    , d( new Track::Private( this ) )
{
    d->url = url;
    d->statsStore = new UrlStatisticsStore( this );
    d->readMetaData();
    d->album = Meta::AlbumPtr( new MetaFile::FileAlbum( d ) );
    d->artist = Meta::ArtistPtr( new MetaFile::FileArtist( d ) );
    d->albumArtist = Meta::ArtistPtr( new MetaFile::FileArtist( d, true ) );
    d->genre = Meta::GenrePtr( new MetaFile::FileGenre( d ) );
    d->composer = Meta::ComposerPtr( new MetaFile::FileComposer( d ) );
    d->year = Meta::YearPtr( new MetaFile::FileYear( d ) );
}

Track::~Track()
{
    delete d;
}

QString
Track::name() const
{
    if( d )
    {
        const QString trackName = d->m_data.title;
        return trackName;
    }
    return "This is a bug!";
}

KUrl
Track::playableUrl() const
{
    return d->url;
}

QString
Track::prettyUrl() const
{
    if(d->url.isLocalFile())
    {
        return d->url.toLocalFile();
    }
    else
    {
        return d->url.path();
    }
}

QString
Track::uidUrl() const
{
    return d->url.url();
}

bool
Track::isPlayable() const
{
    QFileInfo info = QFileInfo( playableUrl().pathOrUrl() );
    return info.isFile() && info.isReadable();
}

bool
Track::isEditable() const
{
    QFileInfo info = QFileInfo( playableUrl().pathOrUrl() );
    return info.isFile() && info.isWritable();
}

Meta::AlbumPtr
Track::album() const
{
    return d->album;
}

Meta::ArtistPtr
Track::artist() const
{
    return d->artist;
}

Meta::GenrePtr
Track::genre() const
{
    return d->genre;
}

Meta::ComposerPtr
Track::composer() const
{
    return d->composer;
}

Meta::YearPtr
Track::year() const
{
    return d->year;
}

void
Track::setAlbum( const QString &newAlbum )
{
    DEBUG_BLOCK
    d->changes.insert( Meta::valAlbum, QVariant( newAlbum ) );
    debug() << "CHANGES HERE: " << d->changes;
    if( !d->batchUpdate )
    {
        d->m_data.album = newAlbum;
        d->writeMetaData();
        notifyObservers();
    }
}

void
Track::setAlbumArtist( const QString &newAlbumArtist )
{
    DEBUG_BLOCK
    d->changes.insert( Meta::valAlbumArtist, QVariant( newAlbumArtist ) );
    if( !d->batchUpdate )
    {
        d->m_data.albumArtist = newAlbumArtist;
        d->writeMetaData();
        notifyObservers();
    }
}

void
Track::setArtist( const QString& newArtist )
{
    d->changes.insert( Meta::valArtist, QVariant( newArtist ) );
    if( !d->batchUpdate )
    {
        d->m_data.artist = newArtist;
        d->writeMetaData();
        notifyObservers();
    }
}

void
Track::setGenre( const QString& newGenre )
{
    d->changes.insert( Meta::valGenre, QVariant( newGenre ) );
    if( !d->batchUpdate )
    {
        d->writeMetaData();
        notifyObservers();
    }
}

void
Track::setComposer( const QString& newComposer )
{
    d->changes.insert( Meta::valComposer, QVariant( newComposer ) );
    if( !d->batchUpdate )
    {
        d->m_data.composer = newComposer;
        d->writeMetaData();
        notifyObservers();
    }
}

void
Track::setYear( int newYear )
{
    d->changes.insert( Meta::valYear, QVariant( newYear ) );
    if( !d->batchUpdate )
    {
        d->m_data.year = newYear;
        d->writeMetaData();
        notifyObservers();
    }
}

void
Track::setTitle( const QString &newTitle )
{
    d->changes.insert( Meta::valTitle, QVariant( newTitle ) );
    if( !d->batchUpdate )
    {
        d->m_data.title = newTitle;
        d->writeMetaData();
        notifyObservers();
    }
}

void
Track::setBpm( const qreal newBpm )
{
    d->changes.insert( Meta::valBpm, QVariant( newBpm ) );
    if( !d->batchUpdate )
    {
        d->m_data.bpm = newBpm;
        d->writeMetaData();
        notifyObservers();
    }
}

qreal
Track::bpm() const
{
    const qreal bpm = d->m_data.bpm;
    return bpm;
}

QString
Track::comment() const
{
    const QString commentName = d->m_data.comment;
    if( !commentName.isEmpty() )
        return commentName;
    return QString();
}

void
Track::setComment( const QString& newComment )
{
    d->changes.insert( Meta::valComment, QVariant( newComment ) );
    if( !d->batchUpdate )
    {
        d->m_data.comment = newComment;
        d->writeMetaData();
        notifyObservers();
    }
}

int
Track::trackNumber() const
{
    return d->m_data.trackNumber;
}

void
Track::setTrackNumber( int newTrackNumber )
{
    d->changes.insert( Meta::valTrackNr, QVariant( newTrackNumber ) );
    if( !d->batchUpdate )
    {
        d->m_data.trackNumber = newTrackNumber;
        d->writeMetaData();
        notifyObservers();
    }
}

int
Track::discNumber() const
{
    return d->m_data.discNumber;
}

void
Track::setDiscNumber( int newDiscNumber )
{
    d->changes.insert( Meta::valDiscNr, QVariant ( newDiscNumber ) );
    if( !d->batchUpdate )
    {
        d->m_data.discNumber = newDiscNumber;
        d->writeMetaData();
        notifyObservers();
    }
}

qint64
Track::length() const
{
    qint64 length = d->m_data.length;
    if( length == -2 /*Undetermined*/ )
        length = 0;
    return length;
}

int
Track::filesize() const
{
    return d->m_data.fileSize;
}

int
Track::sampleRate() const
{
    int sampleRate = d->m_data.sampleRate;
    if( sampleRate == -2 /*Undetermined*/ )
        sampleRate = 0;
    return sampleRate;
}

int
Track::bitrate() const
{
   int bitrate = d->m_data.bitRate;
   if( bitrate == -2 /*Undetermined*/ )
       bitrate = 0;
   return bitrate;
}

QDateTime
Track::createDate() const
{
    if( d->m_data.created > 0 )
        return QDateTime::fromTime_t(d->m_data.created);
    else
        return QDateTime();
}

qreal
Track::replayGain( Meta::ReplayGainTag mode ) const
{
    switch( mode )
    {
    case Meta::ReplayGain_Track_Gain:
        return d->m_data.trackGain;
    case Meta::ReplayGain_Track_Peak:
        return d->m_data.trackPeak;
    case Meta::ReplayGain_Album_Gain:
        return d->m_data.albumGain;
    case Meta::ReplayGain_Album_Peak:
        return d->m_data.albumPeak;
    }
    return 0.0;
}

QString
Track::type() const
{
    return Amarok::extension( d->url.fileName() );
}

bool
Track::isTrack( const KUrl &url )
{
    // some playlists lay under audio/ mime category, filter them
    if( Playlists::isPlaylist( url ) )
        return false;

    // accept remote files, it's too slow to check them at this point
    if( !url.isLocalFile() )
        return true;

    QFileInfo fileInfo( url.toLocalFile() );
    if( fileInfo.size() <= 0 )
        return false;

    // We can't play directories
    if( fileInfo.isDir() )
        return false;

    const KMimeType::Ptr mimeType = KMimeType::findByPath( url.toLocalFile() );
    const QString name = mimeType->name();
    return name.startsWith( "audio/" ) || name.startsWith( "video/" );
}

void
Track::beginMetaDataUpdate()
{
    d->batchUpdate = true;
}

void
Track::endMetaDataUpdate()
{
    DEBUG_BLOCK
    debug() << "CHANGES HERE: " << d->changes;
    d->writeMetaData();
    d->batchUpdate = false;
    notifyObservers();
}

void
Track::finishedPlaying( double playedFraction )
{
    if( d->statsStore )
        d->statsStore->played( playedFraction, Meta::TrackPtr( this ) );
}

bool
Track::inCollection() const
{
    return d->collection; // calls QWeakPointer's (bool) operator
}

Collections::Collection*
Track::collection() const
{
    return d->collection.data();
}

void
Track::setCollection( Collections::Collection *newCollection )
{
    d->collection = newCollection;
}

bool
Track::hasCapabilityInterface( Capabilities::Capability::Type type ) const
{
    bool readlabel = false;
#ifdef HAVE_LIBLASTFM
    readlabel = true;
#endif
    return ( type == Capabilities::Capability::Editable && isEditable() ) ||
           type == Capabilities::Capability::BookmarkThis ||
           type == Capabilities::Capability::WriteTimecode ||
           type == Capabilities::Capability::LoadTimecode ||
           ( type == Capabilities::Capability::ReadLabel && readlabel ) ||
           type == Capabilities::Capability::FindInSource;
}

Capabilities::Capability*
Track::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::Editable:
            if( isEditable() )
                return new EditCapabilityImpl( this );
            else
                return 0;

        case Capabilities::Capability::BookmarkThis:
            return new Capabilities::BookmarkThisCapability( new BookmarkCurrentTrackPositionAction( 0 ) );

        case Capabilities::Capability::WriteTimecode:
            return new TimecodeWriteCapabilityImpl( this );

        case Capabilities::Capability::LoadTimecode:
            return new TimecodeLoadCapabilityImpl( this );

        case Capabilities::Capability::FindInSource:
            return new FindInSourceCapabilityImpl( this );

#ifdef HAVE_LIBLASTFM
       case Capabilities::Capability::ReadLabel:
           if( !d->readLabelCapability )
               d->readLabelCapability = new Capabilities::LastfmReadLabelCapability( this );
#endif

        default: // fall-through


        return 0;
    }
}

Meta::StatisticsPtr
Track::statistics()
{
    if( d->statsStore )
        return Meta::StatisticsPtr( d->statsStore.data() );
    return Meta::Track::statistics();
}

QImage
Track::getEmbeddedCover() const
{
    if( d->m_data.embeddedImage )
        return Meta::Tag::embeddedCover( d->url.path()  );

    return QImage();
}

#include "File.moc"
