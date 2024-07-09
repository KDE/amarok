/****************************************************************************************
 * Copyright (c) 2007-2008 Leo Franchi <lfranchi@gmail.com>                             *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#define DEBUG_PREFIX "LyricsEngine"

#include "LyricsEngine.h"

#include "EngineController.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "lyrics/LyricsManager.h"

#include <QFont>

#include <KFontChooser>


LyricsEngine::LyricsEngine( QObject* parent )
    : QObject( parent )
    , m_fetching( false )
    , m_isUpdateInProgress( false )
{

    EngineController* engine = The::engineController();
    LyricsManager* lyricsManager = LyricsManager::instance();

    connect( engine, &EngineController::trackChanged,
             this, &LyricsEngine::update );
    connect( engine, &EngineController::trackMetadataChanged,
             this, &LyricsEngine::onTrackMetadataChanged );
    connect( engine, &EngineController::trackPositionChanged,
             this, &LyricsEngine::positionChanged );
    connect( lyricsManager, &LyricsManager::newLyrics, this, &LyricsEngine::newLyrics );
    connect( lyricsManager, &LyricsManager::newSuggestions, this, &LyricsEngine::newSuggestions );
}

void LyricsEngine::onTrackMetadataChanged( Meta::TrackPtr track )
{
    DEBUG_BLOCK

    // Only update if the lyrics have changed.
    if( m_lyrics != track->cachedLyrics() )
        update();
}

void LyricsEngine::update()
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
    {
        clearLyrics();
        return;
    }

    if( LyricsManager::instance()->isEmpty( track->cachedLyrics() ) )
    {
        clearLyrics();
        return;
    }

    newLyrics( track );
}

void LyricsEngine::newLyrics( const Meta::TrackPtr &track )
{
    DEBUG_BLOCK

    if( track != The::engineController()->currentTrack() )
        return;

    m_lyrics = track->cachedLyrics();
    Q_EMIT lyricsChanged();

    m_fetching = false;
    Q_EMIT fetchingChanged();
}

void LyricsEngine::newSuggestions( const QVariantList &suggested )
{
    DEBUG_BLOCK

    // each string is in "title - artist <url>" form
    m_suggestions = suggested;
    clearLyrics();
}

void LyricsEngine::lyricsMessage( const QString& key, const QString &val )
{
    DEBUG_BLOCK

    clearLyrics();
    Q_EMIT newLyricsMessage( key, val );
}

void LyricsEngine::clearLyrics()
{
    m_fetching = false;
    Q_EMIT fetchingChanged();

    m_lyrics.clear();
    Q_EMIT lyricsChanged();
}

qreal LyricsEngine::position() const
{
    return (qreal)The::engineController()->trackPosition() * 1000 / The::engineController()->trackLength();
}

void LyricsEngine::refetchLyrics() const
{
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( !currentTrack )
        return;

    LyricsManager::instance()->loadLyrics( currentTrack, true );
}

qreal LyricsEngine::fontSize() const
{
    return Amarok::config( QStringLiteral("Context") ).group( QStringLiteral("Lyrics") ).readEntry( "fontSize", 18 );
}

void LyricsEngine::setFontSize(qreal fontSize)
{
    DEBUG_BLOCK

    if( fontSize == this->fontSize() )
        return;

    Amarok::config( QStringLiteral("Context") ).group( QStringLiteral("Lyrics") ).writeEntry( "fontSize", fontSize );
    Q_EMIT fontSizeChanged();
}

int LyricsEngine::alignment() const
{
    return Amarok::config( QStringLiteral("Context") ).group( QStringLiteral("Lyrics") ).readEntry( "alignment", 2 );
}

void LyricsEngine::setAlignment(int alignment)
{
    DEBUG_BLOCK

    if( alignment == this->alignment() )
        return;

    Amarok::config( QStringLiteral("Context") ).group( QStringLiteral("Lyrics") ).writeEntry( "alignment", alignment );
    Q_EMIT alignmentChanged();
}

QString LyricsEngine::font() const
{
    return Amarok::config( QStringLiteral("Context") ).group( QStringLiteral("Lyrics") ).readEntry( "font", QFont().family() );
}

void LyricsEngine::setFont(const QString& font)
{
    DEBUG_BLOCK

    if( font == this->font() )
        return;

    Amarok::config( QStringLiteral("Context") ).group( QStringLiteral("Lyrics") ).writeEntry( "font", font );
    Q_EMIT fontChanged();
}

QStringList LyricsEngine::availableFonts() const
{
    return KFontChooser::createFontList( 0 );
}

