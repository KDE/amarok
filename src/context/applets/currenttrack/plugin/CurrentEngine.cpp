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
#include "core/meta/support/MetaUtility.h"
#include "core/meta/Statistics.h"
#include "core/support/Amarok.h"
#include "covermanager/CoverCache.h"


#include <KFormat>
#include <KLocalizedString>


CurrentEngine::CurrentEngine( QObject* parent )
    : QObject( parent )
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
CurrentEngine::slotAlbumMetadataChanged( const Meta::AlbumPtr &album )
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
        Q_EMIT albumChanged();
    }
}

void
CurrentEngine::slotTrackMetadataChanged( Meta::TrackPtr track )
{
    if( !track )
        return;

    update( track->album() );
    Q_EMIT trackChanged();
}

void
CurrentEngine::slotTrackChanged(const Meta::TrackPtr &track)
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
    Q_EMIT trackChanged();

    m_cover = QPixmap();
}

void
CurrentEngine::update( Meta::AlbumPtr album )
{

    if( !album )
        return;

    slotAlbumMetadataChanged( album );

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
    Q_EMIT trackChanged();
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
