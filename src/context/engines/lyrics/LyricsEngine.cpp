/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "LyricsEngine.h"

#include "amarok.h"
#include "debug.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "collectiondb.h"
#include "enginecontroller.h"
#include "scriptmanager.h"

#include <QDomDocument>

using namespace Context;

LyricsEngine::LyricsEngine( QObject* parent, const QStringList& args )
: DataEngine( parent )
, ContextObserver( ContextView::self() )
{
    DEBUG_BLOCK
    m_requested = true; // testing
}

QStringList LyricsEngine::sources() const
{
    return QStringList(); // we don't have pre-set sources, as there is only
    // one source---lyrics.
}

bool LyricsEngine::sourceRequested( const QString& name )
{
    m_requested = true; // someone is asking for data, so we turn ourselves on :)
}

void LyricsEngine::message( const ContextState& state )
{
    DEBUG_BLOCK
        if( state == Current && m_requested )
            update();
}

void LyricsEngine::update()
{
    DEBUG_BLOCK
    
    QString lyrics = CollectionDB::instance()->getLyrics( EngineController::instance()->currentTrack()->url() );
    // don't rely on caching for streams
    const bool cached = !lyrics.isEmpty() && !EngineController::engine()->isStream();
    
    QString title  = EngineController::instance()->currentTrack()->name();
    QString artist = EngineController::instance()->currentTrack()->artist()->name();
    
    if( title.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
        title = title.remove(" (PREVIEW: buy it at www.magnatune.com)");
    if( artist.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
        artist = artist.remove(" (PREVIEW: buy it at www.magnatune.com)");
    
    if ( title.isEmpty() ) 
    {
        /* If title is empty, try to use pretty title.
           The fact that it often (but not always) has "artist name" together, can be bad,
           but at least the user will hopefully get nice suggestions. */
        QString prettyTitle = EngineController::instance()->currentTrack()->prettyName();
        int h = prettyTitle.indexOf( '-' );
        if ( h != -1 )
        {
            title = prettyTitle.mid( h+1 ).trimmed();
            if( title.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
                title = title.remove(" (PREVIEW: buy it at www.magnatune.com)");
            if ( artist.isEmpty() ) {
                artist = prettyTitle.mid( 0, h ).trimmed();
                if( artist.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
                    artist = artist.remove(" (PREVIEW: buy it at www.magnatune.com)");
            }
            
        }
    }
    
    if( ( !cached ) && ScriptManager::instance()->lyricsScriptRunning().isEmpty() ) // no lyrics, and no lyrics script!
    {
        setData( "lyrics", "noscriptrunning" );
        return;
    }
    
    if( cached )
        lyricsResult( lyrics.toUtf8(), true );
    else
    { // fetch by lyrics script
        setData( "lyrics", "fetching" );
        debug() << "notifying without url" << endl;
        ScriptManager::instance()->notifyFetchLyrics( artist, title );

    }
    
}
void LyricsEngine::lyricsResult( QByteArray cXmlDoc, bool cached ) //SLOT
{
    DEBUG_BLOCK
    
    QDomDocument doc;
    QString xmldoc = QString::fromUtf8( cXmlDoc );
    if( !doc.setContent( xmldoc ) )
    {
        setData( "lyrics", "error" ); // couldn't fetch
        return;
    }
    
    QString lyrics;
    
    QDomElement el = doc.documentElement();
    
    ScriptManager* const sm = ScriptManager::instance();
    KConfig spec( sm->specForScript( sm->lyricsScriptRunning() ),  KConfig::NoGlobals );
    spec.setGroup( "Lyrics" );
    
    if ( el.tagName() == "suggestions" )
    {
        
        const QDomNodeList l = doc.elementsByTagName( "suggestion" );
        
        if( l.length() ==0 )
        {
            setData( "lyrics", "not found" );
        }
        else
        {
            QVariantList suggested;
            for( uint i = 0; i < l.length(); ++i ) {
                const QString url    = l.item( i ).toElement().attribute( "url" );
                const QString artist = l.item( i ).toElement().attribute( "artist" );
                const QString title  = l.item( i ).toElement().attribute( "title" );
                
                suggested << QString( "%1 - %2 %3" ).arg( title, artist, url );
            }
            setData( "lyrics", "suggested", suggested );
        }
    } else
    {
        lyrics = el.text();
        CollectionDB::instance()->setLyrics( EngineController::instance()->currentTrack()->url(), xmldoc, EngineController::instance()->currentTrack()->prettyUrl() ); // TODO port to new Meta:: api
        
        const QString title      = el.attribute( "title" );
        
        setData( "lyrics", title, lyrics );
        
    }
}

#include "LyricsEngine.moc"