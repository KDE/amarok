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
#include "scriptmanager/ScriptManager.h"
#include "context/ContextView.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QTimer>
#include <QTextDocument>

using namespace Context;

LyricsEngine::LyricsEngine( QObject* parent, const QList<QVariant>& /*args*/ )
    : DataEngine( parent )
    , LyricsObserver( LyricsManager::self() )
    , m_isUpdateInProgress( false )
{

    EngineController* engine = The::engineController();
    connect( engine, SIGNAL(trackChanged(Meta::TrackPtr)),
             this, SLOT(update()), Qt::QueuedConnection );
    connect( engine, SIGNAL(trackMetadataChanged(Meta::TrackPtr)),
             this, SLOT(onTrackMetadataChanged(Meta::TrackPtr)), Qt::QueuedConnection );
}

QStringList LyricsEngine::sources() const
{
    QStringList sourcesList;
    sourcesList << "lyrics" << "suggested";

    return sourcesList;
}

bool LyricsEngine::sourceRequestEvent( const QString& name )
{
    removeAllData( name );
    setData( name, QVariant());
    // in the case where we are resuming playback on startup. Need to be sure
    // the script manager is running and a lyrics script is loaded first.
    QTimer::singleShot( 0, this, SLOT(update()) );
    return true;
}

void LyricsEngine::onTrackMetadataChanged( Meta::TrackPtr track )
{
    DEBUG_BLOCK

    // Only update if the lyrics have changed.
    QString artist = track->artist() ? track->artist()->name() : QString();
    if( m_prevLyrics.artist != artist ||
        m_prevLyrics.title != track->name() ||
        m_prevLyrics.text != track->cachedLyrics() )
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
        m_prevLyrics.clear();
        removeAllData( "lyrics" );
        setData( "lyrics", "stopped", "stopped" );
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

    LyricsData lyrics = { currentTrack->cachedLyrics(), title, artist, KUrl() };

    // Check if the title, the artist and the lyrics are still the same.
    if( !lyrics.text.isEmpty() && (lyrics.text == m_prevLyrics.text) )
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
            removeAllData( "lyrics" );
            setData( "lyrics", "noscriptrunning", "noscriptrunning" );
            disconnect( ScriptManager::instance(), SIGNAL(lyricsScriptStarted()), this, 0 );
            connect( ScriptManager::instance(), SIGNAL(lyricsScriptStarted()), SLOT(update()) );
            m_isUpdateInProgress = false;
            return;
        }

        // fetch by lyrics script
        removeAllData( "lyrics" );
        setData( "lyrics", "fetching", "fetching" );
        ScriptManager::instance()->notifyFetchLyrics( lyrics.artist, lyrics.title );
    }
    m_isUpdateInProgress = false;
}

void LyricsEngine::newLyrics( const LyricsData &lyrics )
{
    QString key = Qt::mightBeRichText( lyrics.text ) ? QLatin1String( "html" )
                                                     : QLatin1String( "lyrics" );
    removeAllData( "lyrics" );
    setData( "lyrics", key, QVariant::fromValue(lyrics) );
    m_prevLyrics = lyrics;
}

void LyricsEngine::newSuggestions( const QVariantList &suggested )
{
    DEBUG_BLOCK
    // each string is in "title - artist <url>" form
    removeAllData( "lyrics" );
    setData( "lyrics", "suggested", suggested );
}

void LyricsEngine::lyricsMessage( const QString& key, const QString &val )
{
    DEBUG_BLOCK

    removeAllData( "lyrics" );
    setData( "lyrics", key, val );
}

#include "LyricsEngine.moc"

