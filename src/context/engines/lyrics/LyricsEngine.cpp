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

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "ContextView.h"
#include "EngineController.h"
#include "ScriptManager.h"

#include <QTimer>

using namespace Context;

LyricsEngine::LyricsEngine( QObject* parent, const QList<QVariant>& /*args*/ )
    : DataEngine( parent )
    , LyricsObserver( LyricsManager::self() )
{

    EngineController* engine = The::engineController();
    connect( engine, SIGNAL( trackChanged( Meta::TrackPtr ) ),
             this, SLOT( update() ), Qt::QueuedConnection );
    connect( engine, SIGNAL( trackMetadataChanged( Meta::TrackPtr ) ),
             this, SLOT( update() ), Qt::QueuedConnection );
}

QStringList LyricsEngine::sources() const
{
    QStringList sourcesList;
    sourcesList << "lyrics" << "suggested";

    return sourcesList;
}

bool LyricsEngine::sourceRequestEvent( const QString& name )
{
    if( name.contains( "previous lyrics" ) )
    {
        removeAllData( "lyrics" );
        setData( "lyrics", "label", "previous Track Information" );
        
        if( m_prevLyricsList.size() == 0 || m_prevSuggestionsList.size() == 0 || m_prevLyrics.contains( "Unavailable" ) )
            setData( "lyrics", "Unavailable" , "Unavailable" );

        if( m_prevLyricsList.size() > 0 )
            setData( "lyrics", "lyrics", m_prevLyricsList );

        else if( !LyricsManager::self()->isEmpty( m_prevLyrics ) )
            setData( "lyrics", "html", m_prevLyrics );

        if( m_prevSuggestionsList.size() > 0 )
            setData( "lyrics", "suggested", m_prevSuggestionsList );

        return true;
    }
    removeAllData( name );
    setData( name, QVariant());
    // in the case where we are resuming playback on startup. Need to be sure
    // the script manager is running and a lyrics script is loaded first.
    QTimer::singleShot( 0, this, SLOT(update()) );
    return true;
}

bool LyricsEngine::testLyricsChanged( const QString& newLyrics,
                                      const QString& oldHtmlLyrics,
                                      QStringList oldPlainLyrics ) const
{
    DEBUG_BLOCK

    bool retVal = false;

    if ( LyricsManager::self()->isHtmlLyrics( newLyrics ) )
        // Compare the old HTML lyrics with the new lyrics.
        retVal = newLyrics != oldHtmlLyrics;
    else
    {
        // If the given oldPlainLyrics list has >= 3 items
        // then plaintext lyrics were provided.
        bool hasPlainLyrics = oldPlainLyrics.count() >= 3;

        if ( hasPlainLyrics )
            // Previously we got plaintext lyrics -> compare them with
            // the new ones.
            retVal = newLyrics != oldPlainLyrics[ 3 ];
        else
            // There were no old lyrics -> if the new lyrics are
            // non-empty they have changed.
            retVal = !LyricsManager::self()->isEmpty( newLyrics );
    }

    debug() << "compared lyrics are the same = " << retVal;

    return retVal;
}

void LyricsEngine::update()
{
    DEBUG_BLOCK
    if( !ScriptManager::instance()->lyricsScriptRunning() ) // no lyrics, and no lyrics script!
    {
        debug() << "no lyrics script running";
        removeAllData( "lyrics" );
        setData( "lyrics", "noscriptrunning", "noscriptrunning" );
        disconnect( ScriptManager::instance(), SIGNAL(lyricsScriptStarted()), this, 0 );
        connect( ScriptManager::instance(), SIGNAL(lyricsScriptStarted()), SLOT(update()) );
        return;
    }

    // -- get current title and artist
    QString title;
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    QString artist;
    Meta::ArtistPtr currentArtist;
    if( currentTrack )
    {
        title = currentTrack->name();
        currentArtist = currentTrack->artist();
        if( currentArtist )
            artist = currentArtist->name();
        debug() << "current track is" << title;
    }
    else
    {
        debug() << "no current track";
        m_title.clear();
        m_artist.clear();

        removeAllData( "lyrics" );
        setData( "lyrics", "stopped" ,"stopped" );
        return;
    }

    // -- clean up title
    if( title.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
        title = title.remove(" (PREVIEW: buy it at www.magnatune.com)");
    if( artist.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
        artist = artist.remove(" (PREVIEW: buy it at www.magnatune.com)");

    if( title.isEmpty() && currentTrack )
    {
        /* If title is empty, try to use pretty title.
           The fact that it often (but not always) has "artist name" together, can be bad,
           but at least the user will hopefully get nice suggestions. */
        QString prettyTitle = currentTrack->prettyName();
        int h = prettyTitle.indexOf( '-' );
        if ( h != -1 )
        {
            title = prettyTitle.mid( h+1 ).trimmed();
            if( title.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
                title = title.remove(" (PREVIEW: buy it at www.magnatune.com)");
            if( artist.isEmpty() ) {
                artist = prettyTitle.mid( 0, h ).trimmed();
                if( artist.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
                    artist = artist.remove(" (PREVIEW: buy it at www.magnatune.com)");
            }
        }
    }

    // Check if the title, the artist and the lyrics are still the same.
    if( title == m_title &&
        artist == m_artist &&
        !testLyricsChanged( currentTrack->cachedLyrics(), m_currentLyrics, m_currentLyricsList ) )
        return; // nothing changed

    // -- really need new lyrics
    m_title = title;
    m_artist = artist;
    m_prevLyrics = m_currentLyrics;
    m_prevLyricsList = m_currentLyricsList;
    m_prevSuggestionsList = m_currentSuggestionsList;

    m_currentLyrics.clear();
    m_currentLyricsList.clear();
    m_currentSuggestionsList.clear();

    QString lyrics = currentTrack->cachedLyrics();

    // don't rely on caching for streams
    const bool cached = !LyricsManager::self()->isEmpty( lyrics ) &&
                        !The::engineController()->isStream();

    if( cached )
    {
        if( LyricsManager::self()->isHtmlLyrics( lyrics ) )
            newLyricsHtml( lyrics );
        else
        {
            QStringList info;
            info << m_title << m_artist << QString() <<  lyrics;
            newLyrics( info );
        }
    }
    else
    {
        // fetch by lyrics script
        removeAllData( "lyrics" );
        setData( "lyrics", "fetching", "fetching" );
        ScriptManager::instance()->notifyFetchLyrics( m_artist, m_title );
    }
}

void LyricsEngine::newLyrics( QStringList& lyrics )
{
    DEBUG_BLOCK

    removeAllData( "lyrics" );
    setData( "lyrics", "lyrics", lyrics );
    m_currentLyricsList = lyrics;
}

void LyricsEngine::newLyricsHtml( QString& lyrics )
{
    removeAllData( "lyrics" );
    setData( "lyrics", "html", lyrics );
    m_currentLyrics = lyrics;
}

void LyricsEngine::newSuggestions( const QVariantList &suggested )
{
    DEBUG_BLOCK
    // each string is in "title - artist <url>" form
    removeAllData( "lyrics" );
    setData( "lyrics", "suggested", suggested );
    m_currentSuggestionsList = suggested;
}

void LyricsEngine::lyricsMessage( QString& key, QString &val )
{
    DEBUG_BLOCK

    removeAllData( "lyrics" );
    setData( "lyrics", key, val );
}

#include "LyricsEngine.moc"

