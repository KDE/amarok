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

#define DEBUG_PREFIX "CapabilityDelegateImpl"

#include "CapabilityDelegateImpl.h"

#include "core/support/Debug.h"
#include "SqlCollection.h"
#include "SqlMeta.h"
#include "SqlReadLabelCapability.h"
#include "SqlWriteLabelCapability.h"
#include "amarokurls/BookmarkMetaActions.h"
#include "covermanager/CoverFetchingActions.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/capabilities/BookmarkThisCapability.h"
#include "core/capabilities/EditCapability.h"
#include "core/capabilities/FindInSourceCapability.h"
#include "core/capabilities/StatisticsCapability.h"
#include "core/capabilities/OrganiseCapability.h"
#include "core/capabilities/UpdateCapability.h"
#include "core/collections/support/SqlStorage.h"
#include "core-impl/capabilities/timecode/TimecodeLoadCapability.h"
#include "core-impl/capabilities/timecode/TimecodeWriteCapability.h"
#include "amarokurls/PlayUrlRunner.h"

#include <QAction>
#include <QFile>
#include <QList>

class CompilationAction : public QAction
{
    Q_OBJECT
    public:
        CompilationAction( QObject* parent, Meta::SqlAlbumPtr album )
                : QAction( parent )
                , m_album( album )
                , m_isCompilation( album->isCompilation() )
            {
                connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
                if( m_isCompilation )
                {
                    setIcon( KIcon( "filename-artist-amarok" ) );
                    setText( i18n( "Do not show under Various Artists" ) );
                }
                else
                {
                    setIcon( KIcon( "similarartists-amarok" ) );
                    setText( i18n( "Show under Various Artists" ) );
                }
            }

    private slots:
        void slotTriggered()
        {
            m_album->setCompilation( !m_isCompilation );
        }

    private:
        Meta::SqlAlbumPtr m_album;
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
        virtual void setUidUrl( const QString &newUidUrl ) { m_track->setUidUrl( newUidUrl ); }
        virtual void beginMetaDataUpdate() { m_track->beginMetaDataUpdate(); }
        virtual void endMetaDataUpdate() { m_track->endMetaDataUpdate(); }

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
        virtual void setFirstPlayed( const QDateTime &time ) {
            if( time < m_track->firstPlayed() ) // only update if older
                m_track->setFirstPlayed( time );
        }
        virtual void setLastPlayed( const QDateTime &time ) {
            if( time > m_track->lastPlayed() ) // only update if newer
                m_track->setLastPlayed( time );
        }
        virtual void setPlayCount( const int playcount ) {
            if( playcount > 0 ) // don't reset it
                m_track->setPlayCount( playcount );
        }
        virtual void beginStatisticsUpdate()
        {
            m_track->beginMetaDataUpdate();
        }
        virtual void endStatisticsUpdate()
        {
            m_track->endMetaDataUpdate();
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
                m_track->remove();
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

        virtual void findInSource( QFlags<TargetTag> tag )
        {
            DEBUG_BLOCK

            QStringList filters;
            Meta::AlbumPtr album       = m_track->album();
            Meta::ArtistPtr artist     = m_track->artist();
            Meta::ComposerPtr composer = m_track->composer();
            Meta::GenrePtr genre       = m_track->genre();
            Meta::YearPtr year         = m_track->year();
            QString name;

            // NOTE: i18n calls need to use the same strings as filters in CollectionTreeItemModelBase.cpp
            if( tag.testFlag(Artist) && !(name = artist ? artist->prettyName() : QString()).isEmpty() )
                filters << QString( "%1:\"%2\"" ).arg( i18n("artist"), name );
            if( tag.testFlag(Album) && !(name = album ? album->prettyName() : QString()).isEmpty() )
                filters << QString( "%1:\"%2\"" ).arg( i18n("album"), name );
            if( tag.testFlag(Composer) && !(name = composer ? composer->prettyName() : QString()).isEmpty() )
                filters << QString( "%1:\"%2\"" ).arg( i18n("composer"), name );
            if( tag.testFlag(Genre) && !(name = genre ? genre->prettyName() : QString()).isEmpty() )
                filters << QString( "%1:\"%2\"" ).arg( i18n("genre"), name );
            if( tag.testFlag(Track) && !(name = m_track ? m_track->prettyName() : QString()).isEmpty() )
                filters << QString( "%1:\"%2\"" ).arg( i18n("title"), name );
            if( tag.testFlag(Year) && !(name = year ? year->name() : QString()).isEmpty() )
                filters << QString( "%1:%2" ).arg( i18n("year"), name );

            if( !filters.isEmpty() )
            {
                AmarokUrl url;
                url.setCommand( "navigate" );
                url.setPath( "collections" );
                url.appendArg( "filter", filters.join( QLatin1String(" AND ") ) );
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
        case Capabilities::Capability::Actions:
        case Capabilities::Capability::Importable:
        case Capabilities::Capability::Organisable:
        case Capabilities::Capability::Updatable:
        case Capabilities::Capability::BookmarkThis:
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

        case Capabilities::Capability::Actions:
        {
            QList<QAction*> actions;
            //TODO These actions will hang around until m_collection is destructed.
            // Find a better parent to avoid this memory leak.
            //actions.append( new CopyToDeviceAction( m_collection, this ) );

            return new Capabilities::ActionsCapability( actions );
        }

        case Capabilities::Capability::Organisable:
            return new OrganiseCapabilityImpl( track );

        case Capabilities::Capability::Updatable:
            return new UpdateCapabilityImpl( track );

        case Capabilities::Capability::BookmarkThis:
            return new Capabilities::BookmarkThisCapability( new BookmarkCurrentTrackPositionAction( 0 ) );
        case Capabilities::Capability::WriteTimecode:
            return new TimecodeWriteCapabilityImpl( track );
        case Capabilities::Capability::LoadTimecode:
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
{
}

ArtistCapabilityDelegateImpl::~ArtistCapabilityDelegateImpl()
{
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
            return new Capabilities::BookmarkThisCapability( new BookmarkArtistAction( 0, Meta::ArtistPtr( artist ) ) );
        default:
            return 0;
    }
}


//---------------Album compilation management actions-----

AlbumCapabilityDelegateImpl::AlbumCapabilityDelegateImpl()
    : AlbumCapabilityDelegate()
{
}

AlbumCapabilityDelegateImpl::~AlbumCapabilityDelegateImpl()
{
}

bool
AlbumCapabilityDelegateImpl::hasCapabilityInterface( Capabilities::Capability::Type type, const Meta::SqlAlbum *album ) const
{
    if( !album )
        return false;

    switch( type )
    {
        case Capabilities::Capability::Actions:
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
        case Capabilities::Capability::Actions:
        {
            QList<QAction*> actions;
            actions.append( new CompilationAction( 0, Meta::SqlAlbumPtr( album ) ) );
            actions.append( new FetchCoverAction( 0, Meta::AlbumPtr( album ) ) );
            actions.append( new SetCustomCoverAction( 0, Meta::AlbumPtr( album ) ) );
            QAction *displayCoverAction = new DisplayCoverAction( 0, Meta::AlbumPtr( album ) );
            QAction *unsetCoverAction   = new UnsetCoverAction( 0, Meta::AlbumPtr( album ) );

            if( !album->hasImage() )
            {
                displayCoverAction->setEnabled( false );
                unsetCoverAction->setEnabled( false );
            }
            actions.append( displayCoverAction );
            actions.append( unsetCoverAction );
            return new Capabilities::ActionsCapability( actions );
        }
        case Capabilities::Capability::BookmarkThis:
            return new Capabilities::BookmarkThisCapability( new BookmarkAlbumAction( 0, Meta::AlbumPtr( album ) ) );

        default:
            return 0;
    }
}

} //namespace Capabilities

#include "CapabilityDelegateImpl.moc"

