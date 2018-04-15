/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
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

#define DEBUG_PREFIX "CurrentEngine"

#include "CurrentEngine.h"

#include "EngineController.h"
#include "core/support/Debug.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/support/MetaUtility.h"
#include "core/meta/Statistics.h"
#include "core/support/Amarok.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "covermanager/CoverCache.h"

#include <QDateTime>

#include <KFormat>
#include <KLocalizedString>


CurrentEngine::CurrentEngine( QObject* parent )
    : QObject( parent )
    , m_lastQueryMaker( Q_NULLPTR )
{
    EngineController* engine = The::engineController();

    // Connect queued to reduce interface stuttering.
    connect( engine, &EngineController::trackPlaying,
             this, &CurrentEngine::slotTrackChanged, Qt::QueuedConnection );
    connect( engine, &EngineController::stopped,
             this, &CurrentEngine::stopped, Qt::QueuedConnection );
    connect( engine, &EngineController::trackMetadataChanged,
             this, &CurrentEngine::slotTrackMetadataChanged, Qt::QueuedConnection );
    connect( engine, &EngineController::albumMetadataChanged,
             this, &CurrentEngine::slotAlbumMetadataChanged, Qt::QueuedConnection );
}

CurrentEngine::~CurrentEngine()
{
}

void
CurrentEngine::slotAlbumMetadataChanged( Meta::AlbumPtr album )
{
    DEBUG_BLOCK

    // disregard changes for other albums (BR: 306735)
    if( !m_currentTrack || m_currentTrack->album() != album )
        return;

    QPixmap cover;

    if( album )
        cover = The::coverCache()->getCover( album, 1 );

    if( m_cover.cacheKey() != cover.cacheKey() )
    {
        m_cover = cover;
        emit albumChanged();
    }
}

void
CurrentEngine::slotTrackMetadataChanged( Meta::TrackPtr track )
{
    if( !track )
        return;

    update( track->album() );
    emit trackChanged();
}

void
CurrentEngine::slotTrackChanged(Meta::TrackPtr track)
{
    DEBUG_BLOCK

    if( !track || track == m_currentTrack )
        return;

    m_currentTrack = track;
    slotTrackMetadataChanged( track );
}


void
CurrentEngine::stopped()
{
    m_currentTrack.clear();
    emit trackChanged();

    m_cover = QPixmap();

    // Collect data for the recently added albums
    m_albums.clear();
    emit albumChanged();

    Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
    qm->setAutoDelete( true );
    qm->setQueryType( Collections::QueryMaker::Album );
    qm->excludeFilter( Meta::valAlbum, QString(), true, true );
    qm->orderBy( Meta::valCreateDate, true );
    qm->limitMaxResultSize( Amarok::config("Albums Applet").readEntry("RecentlyAdded", 5) );

    connect( qm, &Collections::QueryMaker::newAlbumsReady,
                this, &CurrentEngine::resultReady, Qt::QueuedConnection );

    m_lastQueryMaker = qm;
    qm->run();
}

void
CurrentEngine::update( Meta::AlbumPtr album )
{
    m_lastQueryMaker = Q_NULLPTR;

    if( !album )
        return;

    slotAlbumMetadataChanged( album );

    Meta::TrackPtr track = The::engineController()->currentTrack();

    if( !track )
        return;

    Meta::ArtistPtr artist = track->artist();

    // Prefer track artist to album artist BUG: 266682
    if( !artist )
        artist = album->albumArtist();
    
    if( artist && !artist->name().isEmpty() )
    {
        m_albums.clear();

        // -- search the collection for albums with the same artist
        Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
        qm->setAutoDelete( true );
        qm->addFilter( Meta::valArtist, artist->name(), true, true );
        qm->setAlbumQueryMode( Collections::QueryMaker::AllAlbums );
        qm->setQueryType( Collections::QueryMaker::Album );

        connect( qm, &Collections::QueryMaker::newAlbumsReady,
                 this, &CurrentEngine::resultReady, Qt::QueuedConnection );

        m_lastQueryMaker = qm;
        qm->run();
    }
}

void
CurrentEngine::resultReady( const Meta::AlbumList &albums )
{
    if( sender() == m_lastQueryMaker )
        m_albums << albums;
}

QString
CurrentEngine::artist() const
{
    if( !m_currentTrack )
        return QString();

    return m_currentTrack->artist()->prettyName();
}

QString
CurrentEngine::track() const
{
    if( !m_currentTrack )
        return QString();

    return m_currentTrack->prettyName();
}

QString
CurrentEngine::album() const
{
    if( !m_currentTrack )
        return QString();

    return m_currentTrack->album()->prettyName();
}

int
CurrentEngine::rating() const
{
    if( !m_currentTrack )
        return 0;

    return m_currentTrack->statistics()->rating();
}

void
CurrentEngine::setRating(int rating)
{
    DEBUG_BLOCK

    debug() << "New rating:" << rating;

    if( !m_currentTrack )
        return;

    if( rating == m_currentTrack->statistics()->rating() )
        return;

    m_currentTrack->statistics()->setRating( rating );
    emit trackChanged();
}

int
CurrentEngine::score() const
{
    if( !m_currentTrack )
        return 0;

    return m_currentTrack->statistics()->score();
}

int
CurrentEngine::length() const
{
    if( !m_currentTrack )
        return 0;

    return m_currentTrack->length();
}

QString
CurrentEngine::lastPlayed() const
{
    if( !m_currentTrack )
        return QString();

    auto lastPlayed = m_currentTrack->statistics()->lastPlayed();
    QString lastPlayedString;
    if( lastPlayed.isValid() )
        lastPlayedString = KFormat().formatRelativeDateTime( lastPlayed, QLocale::ShortFormat );
    else
        lastPlayedString = i18n( "Never" );

    return lastPlayedString;
}

int
CurrentEngine::timesPlayed() const
{
    if( !m_currentTrack )
        return 0;

    return m_currentTrack->statistics()->playCount();
}
