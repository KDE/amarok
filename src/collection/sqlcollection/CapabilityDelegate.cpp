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

#include "CapabilityDelegate.h"

#include "SqlBookmarkThisCapability.h"
#include "SqlCollection.h"
#include "SqlMeta.h"
#include "SqlReadLabelCapability.h"
#include "SqlWriteLabelCapability.h"
#include "amarokurls/BookmarkMetaActions.h"
#include "covermanager/CoverFetchingActions.h"
#include "meta/capabilities/CustomActionsCapability.h"
#include "meta/capabilities/CurrentTrackActionsCapability.h"
#include "meta/capabilities/EditCapability.h"
#include "meta/capabilities/StatisticsCapability.h"
#include "meta/capabilities/TimecodeLoadCapability.h"
#include "meta/capabilities/TimecodeWriteCapability.h"
#include "meta/capabilities/OrganiseCapability.h"
#include "meta/capabilities/UpdateCapability.h"
#include "amarokurls/PlayUrlRunner.h"

#include <QAction>
#include <QFile>
#include <QList>

class EditCapabilityImpl : public Meta::EditCapability
{
    Q_OBJECT
    public:
    EditCapabilityImpl( Meta::SqlTrack *track )
            : Meta::EditCapability()
            , m_track( track ) {}

        virtual bool isEditable() const { return m_track->isEditable(); }
        virtual void setAlbum( const QString &newAlbum ) { m_track->setAlbum( newAlbum ); }
        virtual void setArtist( const QString &newArtist ) { m_track->setArtist( newArtist ); }
        virtual void setComposer( const QString &newComposer ) { m_track->setComposer( newComposer ); }
        virtual void setGenre( const QString &newGenre ) { m_track->setGenre( newGenre ); }
        virtual void setYear( const QString &newYear ) { m_track->setYear( newYear ); }
        virtual void setBpm( const float newBpm ) { m_track->setBpm( newBpm ); }
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

class StatisticsCapabilityImpl : public Meta::StatisticsCapability
{
    public:
        StatisticsCapabilityImpl( Meta::SqlTrack *track )
            : Meta::StatisticsCapability()
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

class OrganiseCapabilityImpl : public Meta::OrganiseCapability
{
    Q_OBJECT
    public:
        OrganiseCapabilityImpl( Meta::SqlTrack *track )
            : Meta::OrganiseCapability()
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

class UpdateCapabilityImpl : public Meta::UpdateCapability
{
    Q_OBJECT
    public:
        UpdateCapabilityImpl( Meta::SqlTrack *track )
            : Meta::UpdateCapability()
            , m_track( track ) {}

        virtual void collectionUpdated() const { m_track->collection()->collectionUpdated(); }


    private:
        KSharedPtr<Meta::SqlTrack> m_track;
};

class TimecodeWriteCapabilityImpl : public Meta::TimecodeWriteCapability
{
    Q_OBJECT
    public:
        TimecodeWriteCapabilityImpl( Meta::SqlTrack *track )
        : Meta::TimecodeWriteCapability()
        , m_track( track )
        {}

        virtual bool writeTimecode ( int seconds )
        {
            return Meta::TimecodeWriteCapability::writeTimecode( seconds, Meta::TrackPtr( m_track.data() ) );
        }

        virtual bool writeAutoTimecode ( int seconds )
        {
            return Meta::TimecodeWriteCapability::writeAutoTimecode( seconds, Meta::TrackPtr( m_track.data() ) );
        }

    private:
        KSharedPtr<Meta::SqlTrack> m_track;
};

class TimecodeLoadCapabilityImpl : public Meta::TimecodeLoadCapability
{
    Q_OBJECT
    public:
        TimecodeLoadCapabilityImpl( Meta::SqlTrack *track )
        : Meta::TimecodeLoadCapability()
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

TrackCapabilityDelegate::TrackCapabilityDelegate()
{
}

bool
TrackCapabilityDelegate::hasCapabilityInterface( Meta::Capability::Type type, const Meta::SqlTrack *track ) const
{
    if( !track )
        return false;

    switch( type )
    {
        case Meta::Capability::CustomActions:
        case Meta::Capability::Importable:
        case Meta::Capability::Organisable:
        case Meta::Capability::Updatable:
        case Meta::Capability::CurrentTrackActions:
        case Meta::Capability::WriteTimecode:
        case Meta::Capability::LoadTimecode:
        case Meta::Capability::ReadLabel:
        case Meta::Capability::WriteLabel:
            return true;

        case Meta::Capability::Editable:
            return track->isEditable();

        default:
            return false;
    }
}

Meta::Capability*
TrackCapabilityDelegate::createCapabilityInterface( Meta::Capability::Type type, Meta::SqlTrack *track )
{
    if( !track )
    {
        return 0;
    }

    switch( type )
    {
        case Meta::Capability::Editable:
            return new EditCapabilityImpl( track );

        case Meta::Capability::Importable:
            return new StatisticsCapabilityImpl( track );

        case Meta::Capability::CustomActions:
        {
            QList<QAction*> actions;
            //TODO These actions will hang around until m_collection is destructed.
            // Find a better parent to avoid this memory leak.
            //actions.append( new CopyToDeviceAction( m_collection, this ) );

            return new Meta::CustomActionsCapability( actions );
        }

        case Meta::Capability::Organisable:
            return new OrganiseCapabilityImpl( track );

        case Meta::Capability::Updatable:
            return new UpdateCapabilityImpl( track );

        case Meta::Capability::CurrentTrackActions:
        {
            QList< QAction * > actions;
            QAction* flag = new BookmarkCurrentTrackPositionAction( track->collection() );
            actions << flag;
            debug() << "returning bookmarkcurrenttrack action";
            return new Meta::CurrentTrackActionsCapability( actions );
        }
        case Meta::Capability::WriteTimecode:
            return new TimecodeWriteCapabilityImpl( track );
        case Meta::Capability::LoadTimecode:
            return new TimecodeLoadCapabilityImpl( track );
        case Meta::Capability::ReadLabel:
            return new Meta::SqlReadLabelCapability( track, track->sqlCollection()->sqlStorage() );
        case Meta::Capability::WriteLabel:
            return new Meta::SqlWriteLabelCapability( track, track->sqlCollection()->sqlStorage() );

        default:
            return 0;
    }
}


ArtistCapabilityDelegate::ArtistCapabilityDelegate()
    : m_bookmarkAction( 0 )
{
}

ArtistCapabilityDelegate::~ArtistCapabilityDelegate()
{
    delete m_bookmarkAction;
}

bool
ArtistCapabilityDelegate::hasCapabilityInterface( Meta::Capability::Type type, const Meta::SqlArtist *artist ) const
{
    if( !artist )
        return false;

    switch( type )
    {
        case Meta::Capability::BookmarkThis:
            return true;
        default:
            return false;
    }
}

Meta::Capability*
ArtistCapabilityDelegate::createCapabilityInterface( Meta::Capability::Type type, Meta::SqlArtist *artist )
{
    if( !artist )
    {
        return 0;
    }

    switch( type )
    {
        case Meta::Capability::BookmarkThis:
        {
            if ( !m_bookmarkAction )
                m_bookmarkAction = new BookmarkArtistAction( 0, Meta::ArtistPtr( artist ) );
            return new Meta::SqlBookmarkThisCapability( m_bookmarkAction );
        }
        default:
            return 0;
    }
}


//---------------Album compilation management actions-----

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

AlbumCapabilityDelegate::AlbumCapabilityDelegate()
    : m_bookmarkAction( 0 )
{
}

AlbumCapabilityDelegate::~AlbumCapabilityDelegate()
{
    delete m_bookmarkAction;
}

bool
AlbumCapabilityDelegate::hasCapabilityInterface( Meta::Capability::Type type, const Meta::SqlAlbum *album ) const
{
    if( !album )
        return false;

    switch( type )
    {
        case Meta::Capability::CustomActions:
            return true;
        case Meta::Capability::BookmarkThis:
            return true;
        default:
            return false;
    }
}

Meta::Capability*
AlbumCapabilityDelegate::createCapabilityInterface( Meta::Capability::Type type, Meta::SqlAlbum *album )
{
    if( !album )
    {
        return 0;
    }

    switch( type )
    {
        case Meta::Capability::CustomActions:
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
            return new Meta::CustomActionsCapability( actions );
        }
        case Meta::Capability::BookmarkThis:
        {
            if ( !m_bookmarkAction )
                m_bookmarkAction = new BookmarkAlbumAction( 0, Meta::AlbumPtr( album ) );
            return new Meta::SqlBookmarkThisCapability( m_bookmarkAction );
        }

        default:
            return 0;
    }
}

CollectionCapabilityDelegate::CollectionCapabilityDelegate()
{
}

bool CollectionCapabilityDelegate::hasCapabilityInterface( Meta::Capability::Type type, const SqlCollection *collection ) const
{
    if( !collection )
        return 0;

    switch( type )
    {
        default:
            return false;
    }
}

Meta::Capability*
CollectionCapabilityDelegate::createCapabilityInterface( Meta::Capability::Type type, SqlCollection *collection )
{
    if( !collection )
        return 0;

    switch( type )
    {
        default:
            return 0;
    }
}

#include "CapabilityDelegate.moc"

