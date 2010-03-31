/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "CapabilityDelegateImpl.h"

#include "core/support/Debug.h"
#include "SqlBookmarkThisCapability.h"
#include "SqlCollection.h"
#include "SqlMeta.h"
#include "SqlReadLabelCapability.h"
#include "SqlWriteLabelCapability.h"
#include "amarokurls/BookmarkMetaActions.h"
#include "covermanager/CoverFetchingActions.h"
#include "core/capabilities/CustomActionsCapability.h"
#include "core/capabilities/CurrentTrackActionsCapability.h"
#include "core/capabilities/EditCapability.h"
#include "core/capabilities/FindInSourceCapability.h"
#include "core/capabilities/StatisticsCapability.h"
#include "core/capabilities/OrganiseCapability.h"
#include "core-impl/capabilities/timecode/TimecodeLoadCapability.h"
#include "core-impl/capabilities/timecode/TimecodeWriteCapability.h"
#include "core/capabilities/OrganiseCapability.h"
#include "core/capabilities/UpdateCapability.h"
#include "amarokurls/PlayUrlRunner.h"

#include <QAction>
#include <QFile>
#include <QList>

class CompilationAction : public QAction
{
    Q_OBJECT
    public:
        CompilationAction( QObject* parent, Meta::SqlAlbum *album )
            : QAction( parent )
                , m_album( album )
                , m_isCompilation( album->isCompilation() )
            {
                connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
                if( m_isCompilation )
                    setText( i18n( "Do not show under Various Artists" ) );
                else
                    setText( i18n( "Show under Various Artists" ) );
            }

    private slots:
        void slotTriggered()
        {
            m_album->setCompilation( !m_isCompilation );
        }
    private:
        KSharedPtr<Meta::SqlAlbum> m_album;
        bool m_isCompilation;
};

namespace Capabilities {

class EditCapabilityImpl : public Capabilities::EditCapability
{
    Q_OBJECT
    public:
    EditCapabilityImpl( Meta::SqlTrack *track )
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
        KSharedPtr<Meta::SqlTrack> m_track;
};

class StatisticsCapabilityImpl : public Capabilities::StatisticsCapability
{
    public:
        StatisticsCapabilityImpl( Meta::SqlTrack *track )
            : Capabilities::StatisticsCapability()
            , m_track( track ) {}

        virtual void setScore( const int score ) {
            if( score > 0 ) // don't reset it
                m_track->setScore( score );
        }
        virtual void setRating( const int rating ) {
            if( rating > 0 ) // don't reset it
                m_track->setRating( rating );
        }
        virtual void setFirstPlayed( const uint time ) {
            if( time < m_track->firstPlayed() ) // only update if older
                m_track->setFirstPlayed( time );
        }
        virtual void setLastPlayed( const uint time ) {
            if( time > m_track->lastPlayed() ) // only update if newer
                m_track->setLastPlayed( time );
        }
        virtual void setPlayCount( const int playcount ) {
            if( playcount > 0 ) // don't reset it
                m_track->setPlayCount( playcount );
        }
        virtual void beginStatisticsUpdate()
        {
            m_track->setWriteAllStatisticsFields( true );
            m_track->beginMetaDataUpdate();
        }
        virtual void endStatisticsUpdate()
        {
            m_track->endMetaDataUpdate();
            m_track->setWriteAllStatisticsFields( false );
        }
        virtual void abortStatisticsUpdate()
        {
            m_track->abortMetaDataUpdate();
            m_track->setWriteAllStatisticsFields( false );
        }

    private:
        KSharedPtr<Meta::SqlTrack> m_track;
};

class OrganiseCapabilityImpl : public Capabilities::OrganiseCapability
{
    Q_OBJECT
    public:
        OrganiseCapabilityImpl( Meta::SqlTrack *track )
            : Capabilities::OrganiseCapability()
            , m_track( track ) {}

        virtual void deleteTrack()
        {
            if( QFile::remove( m_track->playableUrl().path() ) )
            {
                QString sql = QString( "DELETE FROM tracks WHERE id = %1;" ).arg( m_track->trackId() );
                m_track->sqlCollection()->sqlStorage()->query( sql );
            }
        }

    private:
        KSharedPtr<Meta::SqlTrack> m_track;
};

class UpdateCapabilityImpl : public Capabilities::UpdateCapability
{
    Q_OBJECT
    public:
        UpdateCapabilityImpl( Meta::SqlTrack *track )
            : Capabilities::UpdateCapability()
            , m_track( track ) {}

        virtual void collectionUpdated() const { m_track->collection()->collectionUpdated(); }


    private:
        KSharedPtr<Meta::SqlTrack> m_track;
};

class TimecodeWriteCapabilityImpl : public Capabilities::TimecodeWriteCapability
{
    Q_OBJECT
    public:
        TimecodeWriteCapabilityImpl( Meta::SqlTrack *track )
        : Capabilities::TimecodeWriteCapability()
        , m_track( track )
        {}

        virtual bool writeTimecode ( qint64 miliseconds )
        {
            return Capabilities::TimecodeWriteCapability::writeTimecode( miliseconds, Meta::TrackPtr( m_track.data() ) );
        }

        virtual bool writeAutoTimecode ( qint64 miliseconds )
        {
            return Capabilities::TimecodeWriteCapability::writeAutoTimecode( miliseconds, Meta::TrackPtr( m_track.data() ) );
        }

    private:
        KSharedPtr<Meta::SqlTrack> m_track;
};

class TimecodeLoadCapabilityImpl : public Capabilities::TimecodeLoadCapability
{
    Q_OBJECT
    public:
        TimecodeLoadCapabilityImpl( Meta::SqlTrack *track )
        : Capabilities::TimecodeLoadCapability()
        , m_track( track )
        {}

        virtual bool hasTimecodes()
        {
            if ( loadTimecodes().size() > 0 )
                return true;
            return false;
        }

        virtual QList<KSharedPtr<AmarokUrl> > loadTimecodes()
        {
            QList<KSharedPtr<AmarokUrl> > list = PlayUrlRunner::bookmarksFromUrl( m_track->playableUrl() );
            return list;
        }

    private:
        KSharedPtr<Meta::SqlTrack> m_track;
};


class FindInSourceCapabilityImpl : public Capabilities::FindInSourceCapability
{
    Q_OBJECT
    public:
        FindInSourceCapabilityImpl( Meta::SqlTrack *track )
            : Capabilities::FindInSourceCapability()
            , m_track( track ) {}

        virtual void findInSource()
        {
            DEBUG_BLOCK
            if( m_track->artist() && m_track->album() )
            {
                QString artist = m_track->artist()->prettyName();
                QString album = m_track->album()->prettyName();

                AmarokUrl url;
                url.setCommand( "navigate" );
                url.setPath( "collections" );
                url.appendArg( "filter", "artist:\"" + artist + "\" AND album:\"" + album + "\"" );
                url.appendArg( "levels", "artist-album" );

                debug() << "running url: " << url.url();
                url.run();
            }
        }

    private:
        KSharedPtr<Meta::SqlTrack> m_track;
};


TrackCapabilityDelegateImpl::TrackCapabilityDelegateImpl()
    : TrackCapabilityDelegate()
{
}

bool
TrackCapabilityDelegateImpl::hasCapabilityInterface( Capabilities::Capability::Type type, const Meta::SqlTrack *track ) const
{
    if( !track )
        return false;

    switch( type )
    {
        case Capabilities::Capability::CustomActions:
        case Capabilities::Capability::Importable:
        case Capabilities::Capability::Organisable:
        case Capabilities::Capability::Updatable:
        case Capabilities::Capability::CurrentTrackActions:
        case Capabilities::Capability::WriteTimecode:
        case Capabilities::Capability::LoadTimecode:
        case Capabilities::Capability::ReadLabel:
        case Capabilities::Capability::WriteLabel:
        case Capabilities::Capability::FindInSource:
            return true;

        case Capabilities::Capability::Editable:
            return track->isEditable();

        default:
            return false;
    }
}

Capabilities::Capability*
TrackCapabilityDelegateImpl::createCapabilityInterface( Capabilities::Capability::Type type, Meta::SqlTrack *track )
{
    if( !track )
    {
        return 0;
    }



    switch( type )
    {
        case Capabilities::Capability::Editable:
            return new EditCapabilityImpl( track );

        case Capabilities::Capability::Importable:
            return new StatisticsCapabilityImpl( track );

        case Capabilities::Capability::CustomActions:
        {
            QList<QAction*> actions;
            //TODO These actions will hang around until m_collection is destructed.
            // Find a better parent to avoid this memory leak.
            //actions.append( new CopyToDeviceAction( m_collection, this ) );

            return new Capabilities::CustomActionsCapability( actions );
        }

        case Capabilities::Capability::Organisable:
            return new OrganiseCapabilityImpl( track );

        case Capabilities::Capability::Updatable:
            return new UpdateCapabilityImpl( track );

        case Capabilities::Capability::CurrentTrackActions:
        {
            QList< QAction * > actions;
            QAction* flag = new BookmarkCurrentTrackPositionAction( track->collection() );
            actions << flag;
            debug() << "returning bookmarkcurrenttrack action";
            return new Capabilities::CurrentTrackActionsCapability( actions );
        }
        case Capabilities::Capability::WriteTimecode:
            return new TimecodeWriteCapabilityImpl( track );
        case Capabilities::Capability::LoadTimecode:
            debug() << "creating load timecode capability";
            return new TimecodeLoadCapabilityImpl( track );
        case Capabilities::Capability::ReadLabel:
            return new Capabilities::SqlReadLabelCapability( track, track->sqlCollection()->sqlStorage() );
        case Capabilities::Capability::WriteLabel:
            return new Capabilities::SqlWriteLabelCapability( track, track->sqlCollection()->sqlStorage() );
        case Capabilities::Capability::FindInSource:
            return new FindInSourceCapabilityImpl( track );

        default:
            return 0;
    }
}


ArtistCapabilityDelegateImpl::ArtistCapabilityDelegateImpl()
    : ArtistCapabilityDelegate()
    , m_bookmarkAction( 0 )
{
}

ArtistCapabilityDelegateImpl::~ArtistCapabilityDelegateImpl()
{
    delete m_bookmarkAction;
}

bool
ArtistCapabilityDelegateImpl::hasCapabilityInterface( Capabilities::Capability::Type type, const Meta::SqlArtist *artist ) const
{
    if( !artist )
        return false;

    switch( type )
    {
        case Capabilities::Capability::BookmarkThis:
            return true;
        default:
            return false;
    }
}

Capabilities::Capability*
ArtistCapabilityDelegateImpl::createCapabilityInterface( Capabilities::Capability::Type type, Meta::SqlArtist *artist )
{
    if( !artist )
    {
        return 0;
    }

    switch( type )
    {
        case Capabilities::Capability::BookmarkThis:
        {
            if ( !m_bookmarkAction )
                m_bookmarkAction = new BookmarkArtistAction( 0, Meta::ArtistPtr( artist ) );
            return new Capabilities::SqlBookmarkThisCapability( m_bookmarkAction );
        }
        default:
            return 0;
    }
}


//---------------Album compilation management actions-----

AlbumCapabilityDelegateImpl::AlbumCapabilityDelegateImpl()
    : AlbumCapabilityDelegate()
    , m_bookmarkAction( 0 )
{
}

AlbumCapabilityDelegateImpl::~AlbumCapabilityDelegateImpl()
{
    delete m_bookmarkAction;
}

bool
AlbumCapabilityDelegateImpl::hasCapabilityInterface( Capabilities::Capability::Type type, const Meta::SqlAlbum *album ) const
{
    if( !album )
        return false;

    switch( type )
    {
        case Capabilities::Capability::CustomActions:
            return true;
        case Capabilities::Capability::BookmarkThis:
            return true;
        default:
            return false;
    }
}

Capabilities::Capability*
AlbumCapabilityDelegateImpl::createCapabilityInterface( Capabilities::Capability::Type type, Meta::SqlAlbum *album )
{
    if( !album )
    {
        return 0;
    }

    switch( type )
    {
        case Capabilities::Capability::CustomActions:
        {
            QList<QAction*> actions;
            actions.append( new CompilationAction( album->sqlCollection(), album ) );

            QAction *separator          = new QAction( album->sqlCollection() );
            QAction *displayCoverAction = new DisplayCoverAction( album->sqlCollection(), Meta::AlbumPtr( album ) );
            QAction *unsetCoverAction   = new UnsetCoverAction( album->sqlCollection(), Meta::AlbumPtr( album ) );

            separator->setSeparator( true );
            actions.append( separator );
            actions.append( displayCoverAction );
            actions.append( new FetchCoverAction( album->sqlCollection(), Meta::AlbumPtr( album ) ) );
            actions.append( new SetCustomCoverAction( album->sqlCollection(), Meta::AlbumPtr( album ) ) );
            if( !album->hasImage() )
            {
                displayCoverAction->setEnabled( false );
                unsetCoverAction->setEnabled( false );
            }
            actions.append( unsetCoverAction );
            return new Capabilities::CustomActionsCapability( actions );
        }
        case Capabilities::Capability::BookmarkThis:
        {
            if ( !m_bookmarkAction )
                m_bookmarkAction = new BookmarkAlbumAction( 0, Meta::AlbumPtr( album ) );
            return new Capabilities::SqlBookmarkThisCapability( m_bookmarkAction );
        }

        default:
            return 0;
    }
}

CollectionCapabilityDelegateImpl::CollectionCapabilityDelegateImpl()
    : CollectionCapabilityDelegate()
{
}

bool CollectionCapabilityDelegateImpl::hasCapabilityInterface( Capabilities::Capability::Type type, const Collections::SqlCollection *collection ) const
{
    if( !collection )
        return 0;

    switch( type )
    {
        default:
            return false;
    }
}

Capabilities::Capability*
CollectionCapabilityDelegateImpl::createCapabilityInterface( Capabilities::Capability::Type type, Collections::SqlCollection *collection )
{
    if( !collection )
        return 0;

    switch( type )
    {
        default:
            return 0;
    }
}

} //namespace Capabilities

#include "CapabilityDelegateImpl.moc"

