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

#include "LyricsItem.h"

#include "amarok.h" //oldForeach
#include "collectiondb.h"
#include "../contextview.h"
#include "enginecontroller.h"
#include "debug.h"
#include "meta/meta.h"
#include "scriptmanager.h"

#include <qdom.h>

using namespace Context;

LyricsItem *LyricsItem::s_instance = 0;

LyricsItem::LyricsItem()
    : ContextObserver( ContextView::instance() )
    , m_lyricsBox( 0 )
    , m_lyricsVisible( false )
    , m_HTMLSource( QString() )
    , m_enabled( false )
{
    s_instance = this;
}


void LyricsItem::message( const QString& message )
{
    if( message == QString( "boxRemoved" ) || message == QString( "boxesRemoved" ) )
        m_lyricsVisible = false;
    else if( message == QString( "showCurrentTrack" ) )
    {
        if( m_enabled )
            showLyrics( QString() );
    }
}

void LyricsItem::showLyrics( const QString& url )
{
    DEBUG_BLOCK
        
    // NOTE: we check if our lyrics box is visible at the end,
    // once we populate it.
        
    m_lyricsBox = new GenericInfoBox();
    if( !m_lyricsVisible )
    {
        ContextView::instance()->addContextBox( m_lyricsBox, m_order /* index */, false /* fadein */ );
        m_lyricsBox->ensureVisible();
        m_lyricsVisible = true;
    }

    Meta::TrackPtr track = EngineController::instance()->currentTrack();
    if( !track )
        return;

    QString lyrics = CollectionDB::instance()->getLyrics( track->playableUrl().path() );
    // don't rely on caching for streams
    const bool cached = !lyrics.isEmpty() && !EngineController::engine()->isStream();
    QString title  = track->prettyName();
    QString artist = track->artist()->prettyName();
    
    
    m_lyricsBox->setTitle( QString( "Lyrics of %1").arg( title ) );
    // magnatune cleaning
    if( title.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
        title = title.remove(" (PREVIEW: buy it at www.magnatune.com)");
    if( artist.contains("PREVIEW: buy it at www.magnatune.com", Qt::CaseSensitive) )
        artist = artist.remove(" (PREVIEW: buy it at www.magnatune.com)");
    
    if ( title.isEmpty() ) {
        /* If title is empty, try to use pretty title.
           The fact that it often (but not always) has "artist name" together, can be bad,
           but at least the user will hopefully get nice suggestions. */
        QString prettyTitle = track->prettyName();
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
    
    
    if( ( !cached || url == "reload" ) && ScriptManager::instance()->lyricsScriptRunning().isEmpty() )
    {
        const QStringList scripts = ScriptManager::instance()->lyricsScripts();
        lyrics =
            i18n( "Sorry, no lyrics script running.") + "<br />\n" +
            "<br /><div class='info'>\n"+
            i18n( "Available Lyrics Scripts:" ) + "<br />\n";
        oldForeach ( scripts ) {
            lyrics += QString( "<a href=\"runscript:%1\">%2</a><br />\n" ).arg( *it, *it );
        }
        lyrics += "<br />\n" + i18n( "Click on one of the scripts to run it, or use the Script Manager, to be able"
                                     " to see all the scripts, and download new ones from the Web." );
        lyrics += "<br /><div align='center'>\n"
            "<input type='button' onClick='window.location.href=\"show:scriptmanager\";' value='" +
            i18n( "Run Script Manager..." ) +
            "'></div><br /></div>\n";
        
        m_HTMLSource = QString (
                                 "<html><body>\n"
                                 "<div id='lyrics_box' class='box'>\n"
                                 "<div id='lyrics_box-header' class='box-header'>\n"
                                 "<span id='lyrics_box-header-title' class='box-header-title'>\n"
                                 + ( cached ? i18n( "Cached Lyrics" ) : i18n( "Lyrics" ) ) +
                                 "</span>\n"
                                 "</div>\n"
                                 "<div id='lyrics_box-body' class='box-body'>\n"
                                 + lyrics +
                                 "</div>\n"
                                 "</div>\n"
                                 "</body></html>\n"
                               );
        m_lyricsBox->setContents( m_HTMLSource );
        
        if( !m_lyricsVisible )
        {
            ContextView::instance()->addContextBox( m_lyricsBox, m_order /* index */, false /* fadein */ );
            m_lyricsBox->ensureVisible();
            m_lyricsVisible = true;
        }
        m_lyricsBox->ensureVisible();
        // saveHtmlData(); // Send html code to file
        
        return;
    }
    
    
    if( cached && url.isEmpty() )
    {
        lyricsResult( lyrics.toUtf8(), true );
    }
    else
    {
        m_HTMLSource = QString (
                                 "<html><body>\n"
                                 "<div id='lyrics_box' class='box'>\n"
                                 "<div id='lyrics_box-header' class='box-header'>\n"
                                 "<span id='lyrics_box-header-title' class='box-header-title'>\n"
                                 + i18n( "Fetching Lyrics" ) +
                                 "</span>\n"
                                 "</div>\n"
                                 "<div id='lyrics_box-body' class='box-body'>\n"
                                 "<div class='info'><p>\n" + i18n( "Fetching Lyrics..." ) + "</p></div>\n"
                                 "</div>\n"
                                 "</div>\n"
                                 "</body></html>\n"
                               );
        m_lyricsBox->setContents( m_HTMLSource );
        //saveHtmlData(); // Send html code to file
        
        
        if( url.isNull() || url == "reload" )
        {
            debug() << "notifying without url" << endl;
            ScriptManager::instance()->notifyFetchLyrics( artist, title );
        } else
        {
            debug() << "notifying by url, url is: " << url  << endl;
            ScriptManager::instance()->notifyFetchLyricsByUrl( url );
        }
    }
    
    
    if( !m_lyricsVisible )
    {
        ContextView::instance()->addContextBox( m_lyricsBox, m_order /* index */, false /* fadein */ );
        m_lyricsBox->ensureVisible();
        m_lyricsVisible = true;
    }
}



void
LyricsItem::lyricsResult( QByteArray cXmlDoc, bool cached ) //SLOT
{
    DEBUG_BLOCK
        QDomDocument doc;
    QString xmldoc = QString::fromUtf8( cXmlDoc );
    if( !doc.setContent( xmldoc ) )
    {
        m_HTMLSource="";
        m_HTMLSource.append(
                             "<html><body>\n"
                             "<div id='lyrics_box' class='box'>\n"
                             "<div id='lyrics_box-header' class='box-header'>\n"
                             "<span id='lyrics_box-header-title' class='box-header-title'>\n"
                             + i18n( "Error" ) +
                             "</span>\n"
                             "</div>\n"
                             "<div id='lyrics_box-body' class='box-body'><p>\n"
                             + i18n( "Lyrics could not be retrieved because the server was not reachable." ) +
                             "</p></div>\n"
                             "</div>\n"
                             "</body></html>\n"
                           );
        m_lyricsBox->clearContents();
        m_lyricsBox->setContents( m_HTMLSource );
        
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
            lyrics = i18n( "Lyrics for track not found" );
        }
        else
        {
            lyrics = i18n( "Lyrics for track not found, here are some suggestions:" ) + "<br/><br/>\n";
            for( uint i = 0; i < l.length(); ++i ) {
                const QString url    = l.item( i ).toElement().attribute( "url" );
                const QString artist = l.item( i ).toElement().attribute( "artist" );
                const QString title  = l.item( i ).toElement().attribute( "title" );
                
                lyrics += "<a href='show:suggestLyric-" + url + "'>\n" + i18n("%1 - %2", artist, title );
                lyrics += "</a><br/>\n";
            }
        }
        /*lyrics += i18n( "<p>You can <a href=\"%1\">search for the lyrics</a> on the Web.</p>" )
            .arg( QString( m_lyricSearchUrl ).replace( QRegExp( "^http:" ), "externalurl:" ) ); */
    }
    else {
        lyrics = el.text();
        lyrics.replace( "\n", "<br/>\n" ); // Plaintext -> HTML
        
        const QString title      = el.attribute( "title" );
        const QString artist     = el.attribute( "artist" );
        const QString site       = spec.readEntry( "site" );
        const QString site_url   = spec.readEntry( "site_url" );
        
        lyrics.prepend( "<font size='2'><b>\n" + title + "</b><br/><u>\n" + artist+ "</font></u></font><br/>\n" );
        
        if( !cached ) {
            lyrics.append( "<br/><br/><i>\n" + i18n( "Powered by %1 (%2)", site, site_url ) + "</i>\n" );
            //CollectionDB::instance()->setLyrics( EngineController::instance()->bundle().url().path(), xmldoc, EngineController::instance()->bundle().uniqueId() );
        }
    }
    
    m_HTMLSource="";
    m_HTMLSource.append(
                         "<html><body>\n"
                         "<div id='lyrics_box' class='box'>\n"
                         "<div id='lyrics_box-header' class='box-header'>\n"
                         "<span id='lyrics_box-header-title' class='box-header-title'>\n"
                         + ( cached ? i18n( "Cached Lyrics" ) : i18n( "Lyrics" ) ) +
                         "</span>\n"
                         "</div>\n"
                         "<div id='lyrics_box-body' class='box-body'>\n"
                         + lyrics +
                         "</div>\n"
                         "</div>\n"
                         "</body></html>\n"
                       );
    
    
    m_lyricsBox->setContents( m_HTMLSource );
    //Reset scroll
    
    // m_lyricsPage->view()->setContentsPos(0, 0);
    
    if( !m_lyricsVisible )
    {
        ContextView::instance()->addContextBox( m_lyricsBox, m_order /* index */, false /* fadein */ );
        m_lyricsBox->ensureVisible();
        m_lyricsVisible = true;
    }
    
    //saveHtmlData(); // Send html code to file
    
    //wikiExternalPageAction->setEnabled( !m_lyricCurrentUrl.isEmpty() );
    //m_lyricsToolBar->getButton( LYRICS_BROWSER )->setEnabled( !m_lyricCurrentUrl.isEmpty() );
}

#include "LyricsItem.moc"
