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

#include "LyricsEngine.h"

#include "Amarok.h"
#include "Debug.h"
#include "ContextView.h"
#include "EngineController.h"
#include "ScriptManager.h"


using namespace Context;

LyricsEngine::LyricsEngine( QObject* parent, const QList<QVariant>& /*args*/ )
    : DataEngine( parent )
    , ContextObserver( ContextView::self() )
    , LyricsObserver( LyricsManager::self() )
{
    m_requested = true; // testing
}

QStringList LyricsEngine::sources() const
{
    QStringList sourcesList;
    sourcesList << "lyrics" << "suggested";

    return sourcesList;
}

bool LyricsEngine::sourceRequestEvent( const QString& name )
{
    Q_UNUSED( name )

    m_requested = true; // someone is asking for data, so we turn ourselves on :)
    if( name.contains( "previous lyrics" ) )
    {
        removeAllData( "lyrics" );
        setData( "lyrics", "label", "previous Track Information" );
        
        if( m_prevLyricsList.size() == 0 || m_prevSuggestionsList.size() == 0 || m_prevLyrics.contains( "Unavailable" ) )
            setData( "lyrics", "Unavailable" , "Unavailable" );

        if( m_prevLyricsList.size() > 0 )
            setData( "lyrics", "lyrics", m_prevLyricsList );

        else if( !m_prevLyrics.isEmpty() )
            setData( "lyrics", "html", m_prevLyrics );

        if( m_prevSuggestionsList.size() > 0 )
            setData( "lyrics", "suggested", m_prevSuggestionsList );

        return true;
    }
    removeAllData( name );
    setData( name, QVariant());
    update();

    return true;
}

void LyricsEngine::message( const ContextState& state )
{
    DEBUG_BLOCK
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( currentTrack && m_currentTrack && currentTrack != m_currentTrack )
    {
        m_prevLyrics = m_currentLyrics;
        m_prevLyricsList = m_currentLyricsList;
        m_prevSuggestionsList = m_currentSuggestionsList;

        m_currentLyrics.clear();
        m_currentLyricsList.clear();
        m_currentSuggestionsList.clear();
    }
    if( state == Current && m_requested )
    {
        update();
    } else if( state == Home )
    {
        removeAllData( "lyrics" );
        setData( "lyrics", "stopped" ,"stopped" );
    }
    
}

void LyricsEngine::metadataChanged( Meta::TrackPtr track )
{
    const bool hasChanged = track->name() != m_title || 
                            track->artist()->name() != m_artist;

    if( hasChanged )
        update();
}

void LyricsEngine::update()
{
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( !currentTrack || !currentTrack->artist() )
        return;

    unsubscribeFrom( m_currentTrack );
    m_currentTrack = currentTrack;
    subscribeTo( currentTrack );

    QString lyrics = currentTrack->cachedLyrics();
    
    // don't rely on caching for streams
    const bool cached = !lyrics.isEmpty() && !The::engineController()->isStream();
    
    m_title = currentTrack->name();
    m_artist = currentTrack->artist()->name();

    if( m_title.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
        m_title = m_title.remove(" (PREVIEW: buy it at www.magnatune.com)");
    if( m_artist.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
        m_artist = m_artist.remove(" (PREVIEW: buy it at www.magnatune.com)");

    if( m_title.isEmpty() )
    {
        /* If title is empty, try to use pretty title.
           The fact that it often (but not always) has "artist name" together, can be bad,
           but at least the user will hopefully get nice suggestions. */
        QString prettyTitle = The::engineController()->currentTrack()->prettyName();
        int h = prettyTitle.indexOf( '-' );
        if ( h != -1 )
        {
            m_title = prettyTitle.mid( h+1 ).trimmed();
            if( m_title.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
                m_title = m_title.remove(" (PREVIEW: buy it at www.magnatune.com)");
            if( m_artist.isEmpty() ) {
                m_artist = prettyTitle.mid( 0, h ).trimmed();
                if( m_artist.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
                    m_artist = m_artist.remove(" (PREVIEW: buy it at www.magnatune.com)");
            }
        }
    }

    if( cached )
    {
        if( lyrics.contains( "<html>" ) )
            newLyricsHtml( lyrics );
        else
        {
            QStringList info;
            info << m_title << m_artist << QString() <<  lyrics;
            newLyrics( info );
        }
    }
    else if( !ScriptManager::instance()->lyricsScriptRunning() ) // no lyrics, and no lyrics script!
    {
        removeAllData( "lyrics" );
        setData( "lyrics", "noscriptrunning", "noscriptrunning" );
        m_currentLyrics = "Lyrics  Unavailable";
    }
    else
    {
        // fetch by lyrics script
        removeAllData( "lyrics" );
        setData( "lyrics", "fetching", "fetching" );
        m_currentLyrics = "Lyrics Unavailable";
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

void LyricsEngine::newSuggestions( QStringList& suggested )
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

