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
#include "scripting/scriptmanager/ScriptManager.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QFont>

#include <KFontChooser>


LyricsEngine::LyricsEngine( QObject* parent )
    : QObject( parent )
    , LyricsObserver( LyricsManager::self() )
    , m_fetching( false )
    , m_isUpdateInProgress( false )
{

    EngineController* engine = The::engineController();

    connect( engine, &EngineController::trackChanged,
             this, &LyricsEngine::update );
    connect( engine, &EngineController::trackMetadataChanged,
             this, &LyricsEngine::onTrackMetadataChanged );
    connect( engine, &EngineController::trackPositionChanged,
             this, &LyricsEngine::positionChanged );
}

void LyricsEngine::onTrackMetadataChanged( Meta::TrackPtr track )
{
    DEBUG_BLOCK

    // Only update if the lyrics have changed.
    QString artist = track->artist() ? track->artist()->name() : QString();
    if( m_lyrics.artist != artist ||
        m_lyrics.title != track->name() ||
        m_lyrics.text != track->cachedLyrics() )
        update();
}

void LyricsEngine::update()
{
    if( m_isUpdateInProgress )
        return;

    m_isUpdateInProgress = true;

    // -- get current title and artist
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( !currentTrack )
    {
        debug() << "no current track";
        m_lyrics.clear();
        emit lyricsChanged();
        m_isUpdateInProgress = false;
        return;
    }

    QString title = currentTrack->name();
    QString artist = currentTrack->artist() ? currentTrack->artist()->name() : QString();

    // -- clean up title
    const QString magnatunePreviewString = QLatin1String( "PREVIEW: buy it at www.magnatune.com" );
    if( title.contains(magnatunePreviewString, Qt::CaseSensitive) )
        title = title.remove( " (" + magnatunePreviewString + ')' );
    if( artist.contains(magnatunePreviewString, Qt::CaseSensitive) )
        artist = artist.remove( " (" + magnatunePreviewString + ')' );

    if( title.isEmpty() && currentTrack )
    {
        /* If title is empty, try to use pretty title.
           The fact that it often (but not always) has "artist name" together, can be bad,
           but at least the user will hopefully get nice suggestions. */
        QString prettyTitle = currentTrack->prettyName();
        int h = prettyTitle.indexOf( QLatin1Char('-') );
        if ( h != -1 )
        {
            title = prettyTitle.mid( h + 1 ).trimmed();
            if( title.contains(magnatunePreviewString, Qt::CaseSensitive) )
                title = title.remove( " (" + magnatunePreviewString + ')' );

            if( artist.isEmpty() )
            {
                artist = prettyTitle.mid( 0, h ).trimmed();
                if( artist.contains(magnatunePreviewString, Qt::CaseSensitive) )
                    artist = artist.remove( " (" + magnatunePreviewString + ')' );
            }
        }
    }

    LyricsData lyrics = { currentTrack->cachedLyrics(), title, artist, QUrl() };

    // Check if the title, the artist and the lyrics are still the same.
    if( !lyrics.text.isEmpty() && (lyrics.text == m_lyrics.text) )
    {
        debug() << "nothing changed:" << lyrics.title;
        newLyrics( lyrics );
        m_isUpdateInProgress = false;
        return;
    }

    // don't rely on caching for streams
    const bool cached = !LyricsManager::self()->isEmpty( lyrics.text )
        && !The::engineController()->isStream();

    if( cached )
    {
        newLyrics( lyrics );
    }
    else
    {
        // no lyrics, and no lyrics script!
        if( !ScriptManager::instance()->lyricsScriptRunning() )
        {
            debug() << "no lyrics script running";
            clearLyrics();
            disconnect( ScriptManager::instance(), &ScriptManager::lyricsScriptStarted, this, 0 );
            connect( ScriptManager::instance(), &ScriptManager::lyricsScriptStarted, this, &LyricsEngine::update );
            m_isUpdateInProgress = false;
            return;
        }

        // fetch by lyrics script
        clearLyrics();
        m_fetching = true;
        emit fetchingChanged();
        ScriptManager::instance()->notifyFetchLyrics( lyrics.artist, lyrics.title, "", currentTrack );
    }
    m_isUpdateInProgress = false;
}

void LyricsEngine::newLyrics( const LyricsData &lyrics )
{
    DEBUG_BLOCK

    m_lyrics = lyrics;
    emit lyricsChanged();

    m_fetching = false;
    emit fetchingChanged();
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
    emit newLyricsMessage( key, val );
}

void LyricsEngine::clearLyrics()
{
    m_fetching = false;
    emit fetchingChanged();

    m_lyrics.clear();
    emit lyricsChanged();
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

    ScriptManager::instance()->notifyFetchLyrics( m_lyrics.artist, m_lyrics.title, "", currentTrack );
}

void LyricsEngine::refetchLyrics()
{
    DEBUG_BLOCK

    auto currentTrack = The::engineController()->currentTrack();

    if( currentTrack )
        ScriptManager::instance()->notifyFetchLyrics( currentTrack->artist()->name(),
                                                      currentTrack->name(), "", currentTrack );

    m_fetching = true;
    emit fetchingChanged();
}

void LyricsEngine::fetchLyrics(const QString& artist, const QString& title, const QString& url)
{
    DEBUG_BLOCK

    if( !QUrl( url ).isValid() )
        return;

    debug() << "clicked suggestion" << url;

    ScriptManager::instance()->notifyFetchLyrics( artist, title, url, Meta::TrackPtr() );

    m_fetching = true;
    emit fetchingChanged();
}

qreal LyricsEngine::fontSize() const
{
    return Amarok::config( "Context" ).group( "Lyrics" ).readEntry( "fontSize", 18 );
}

void LyricsEngine::setFontSize(qreal fontSize)
{
    DEBUG_BLOCK

    if( fontSize == this->fontSize() )
        return;

    Amarok::config( "Context" ).group( "Lyrics" ).writeEntry( "fontSize", fontSize );
    emit fontSizeChanged();
}

int LyricsEngine::alignment() const
{
    return Amarok::config( "Context" ).group( "Lyrics" ).readEntry( "alignment", 2 );
}

void LyricsEngine::setAlignment(int alignment)
{
    DEBUG_BLOCK

    if( alignment == this->alignment() )
        return;

    Amarok::config( "Context" ).group( "Lyrics" ).writeEntry( "alignment", alignment );
    emit alignmentChanged();
}

QString LyricsEngine::font() const
{
    return Amarok::config( "Context" ).group( "Lyrics" ).readEntry( "font", QFont().family() );
}

void LyricsEngine::setFont(const QString& font)
{
    DEBUG_BLOCK

    if( font == this->font() )
        return;

    Amarok::config( "Context" ).group( "Lyrics" ).writeEntry( "font", font );
    emit fontChanged();
}

QStringList LyricsEngine::availableFonts() const
{
    QStringList list;

    KFontChooser::getFontList( list, 0 );

    return list;
}

