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

#include "WikipediaItem.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "../contextview.h"
#include "enginecontroller.h"
#include "debug.h"
#include "meta/meta.h"
#include "../GenericInfoBox.h"
#include "statusbar.h"

using namespace Context;

QString WikipediaItem::s_wikiLocale = "en";

WikipediaItem::WikipediaItem()
    : ContextObserver( ContextView::instance() )
    , m_wikiBox( 0 )
    , m_wikiJob( 0 )
    , m_wikiCurrentEntry( QString() )
    , m_wikiCurrentUrl( QString() )
    , m_wikiBaseUrl( QString() )
    , m_wikiVisible( false )
    , m_wikiHTMLSource( QString() )
    , m_wikiLanguages( QString() )
    , m_wiki( QString() )
    , m_enabled( false )
    //, m_wikiBackHistory( new QStringList() )
    //, m_wikiForwardHistory( new QStringList() );
{
}

void WikipediaItem::message( const QString& message )
{
    if( message == QString( "boxRemoved" ) || message == QString( "boxesRemoved" ) )
        m_wikiVisible = false;
    else if( message == QString( "showCurrentTrack" ) )
    {
        if( m_enabled ) 
            showWikipedia();
    } else if( message == QString( "showHome" ) )
    {
        m_wikiJob = 0; // cancel job if track ends
    }
}

QString
WikipediaItem::wikiArtistPostfix()
{
    if( wikiLocale() == "en" )
        return " (band)";
    else if( wikiLocale() == "de" )
        return " (Band)";
    else
        return "";
}

QString
WikipediaItem::wikiAlbumPostfix()
{
    if( wikiLocale() == "en" )
        return " (album)";
    else
        return "";
}

QString
WikipediaItem::wikiTrackPostfix()
{
    if( wikiLocale() == "en" )
        return " (song)";
    else
        return "";
}

QString
WikipediaItem::wikiLocale()
{
    if( s_wikiLocale.isEmpty() )
        return QString( "en" );
    
    return s_wikiLocale;
}

void
WikipediaItem::setWikiLocale( const QString &locale )
{
    AmarokConfig::setWikipediaLocale( locale );
    s_wikiLocale = locale;
}

QString
WikipediaItem::wikiURL( const QString &item )
{
    return QString( "http://%1.wikipedia.org/wiki/" ).arg( wikiLocale() )
        + KUrl::toPercentEncoding( item, "/" );
}

void
WikipediaItem::reloadWikipedia()
{
    m_wikiJob = NULL;
    showWikipediaEntry( m_wikiCurrentEntry, true );
}

void
WikipediaItem::showWikipediaEntry( const QString &entry, bool replaceHistory )
{
    m_wikiCurrentEntry = entry;
    showWikipedia( wikiURL( entry ), false, replaceHistory );
}


void WikipediaItem::showWikipedia( const QString &url, bool fromHistory, bool replaceHistory )
{
    DEBUG_BLOCK
#if 0
    if( BrowserBar::instance()->currentBrowser() != this )
    {
        debug() << "current browser is not context, aborting showWikipedia()" << endl;
        return;
    }
#endif
    
    if ( m_wikiJob ) return;

    Meta::TrackPtr track = EngineController::instance()->currentTrack();
    if( track ) return;

    m_wikiBox = new GenericInfoBox();
    m_wikiBox->setTitle( QString( "Artist Info for %1" ).arg( track->artist()->prettyName() ) );
    m_wikiHTMLSource="";
    m_wikiHTMLSource.append(
                             "<html><body>\n"
                             "<div id='wiki_box' class='box'>\n"
                             "<div id='wiki_box-header' class='box-header'>\n"
                             "<span id='wiki_box-header-title' class='box-header-title'>\n"
                             + i18n( "Wikipedia" ) +
                             "</span>\n"
                             "</div>\n"
                             "<div id='wiki_box-body' class='box-body'>\n"
                             "<div class='info'><p>\n" + i18n( "Fetching Wikipedia Information" ) + " ...</p></div>\n"
                             "</div>\n"
                             "</div>\n"
                             "</body></html>\n"
                           );
    
    m_wikiBox->setContents( m_wikiHTMLSource );
    if( !m_wikiVisible )
    {
        ContextView::instance()->addContextBox( m_wikiBox, m_order /* index */, false /* fadein */ );
        m_wikiBox->ensureVisible();
        m_wikiVisible = true;
    }
    
    if ( url.isEmpty() )
    {
        QString tmpWikiStr;
        
        if ( /*(EngineController::instance()->bundle().url().protocol() == "lastfm") ||
             (EngineController::instance()->bundle().url().protocol() == "daap") ||
             !EngineController::engine()->isStream() )*/ false )
        {
            if ( !track->artist()->name().isEmpty() )
            {
                tmpWikiStr = track->artist()->name();
                tmpWikiStr += wikiArtistPostfix();
            }
            else if ( !track->name().isEmpty() )
            {
                tmpWikiStr = track->name();
            }
            else
            {
                tmpWikiStr = track->prettyName();
            }
        }
        else
        {
            tmpWikiStr = track->prettyName();
        }
        
        //Hack to make wiki searches work with magnatune preview tracks
        
        if ( tmpWikiStr.contains( "PREVIEW: buy it at www.magnatune.com" ) ) {
            tmpWikiStr = tmpWikiStr.remove(" (PREVIEW: buy it at www.magnatune.com)" );
            int index = tmpWikiStr.indexOf( '-' );
            if ( index != -1 ) {
                tmpWikiStr = tmpWikiStr.left (index - 1);
            }
            
        }
        m_wikiCurrentEntry = tmpWikiStr;
        
        m_wikiCurrentUrl = wikiURL( tmpWikiStr );
    }
    else
    {
        m_wikiCurrentUrl = url;
    }
    
    // Append new URL to history
    if ( replaceHistory )
    {
        m_wikiBackHistory.back() = m_wikiCurrentUrl;
    }
    else if ( !fromHistory ) {
        m_wikiBackHistory += m_wikiCurrentUrl;
        m_wikiForwardHistory.clear();
    }
    // Limit number of items in history
    if ( m_wikiBackHistory.count() > WIKI_MAX_HISTORY )
        m_wikiBackHistory.pop_front();
    
    m_wikiBaseUrl = m_wikiCurrentUrl.mid(0 , m_wikiCurrentUrl.indexOf("wiki/"));
    m_wikiJob = KIO::storedGet( m_wikiCurrentUrl, false, false );
    
    Amarok::StatusBar::instance()->newProgressOperation( m_wikiJob )
        .setDescription( i18n( "Fetching Wikipedia Information" ) );
    
    connect( m_wikiJob, SIGNAL( result( KJob* ) ), SLOT( wikiResult( KJob* ) ) );
}


void
WikipediaItem::wikiArtistPage() //SLOT
{
    showWikipedia(); // Will fall back to title, if artist is empty(streams!).
}


void
WikipediaItem::wikiAlbumPage() //SLOT
{
    //TODO fix this
    //showWikipediaEntry( EngineController::instance()->bundle().album() + wikiAlbumPostfix() );
}


void
WikipediaItem::wikiTitlePage() //SLOT
{
    //showWikipediaEntry( EngineController::instance()->bundle().title() + wikiTrackPostfix() );
}


void
WikipediaItem::wikiExternalPage() //SLOT
{
    Amarok::invokeBrowser( m_wikiCurrentUrl );
}


void
WikipediaItem::wikiResult( KJob* job ) //SLOT
{
    DEBUG_BLOCK
    
    if( !m_wikiJob ) return; //track changed while we were fetching
    
    if ( !job->error() == 0 && job == m_wikiJob )
    { // make sure its not the wrong job (e.g. wiki request for now changed song
        m_wikiHTMLSource="";
        m_wikiHTMLSource.append(
                                    "<div id='wiki_box' class='box'>\n"
                                    "<div id='wiki_box-header' class='box-header'>\n"
                                    "<span id='wiki_box-header-title' class='box-header-title'>\n"
                                    + i18n( "Error" ) +
                                    "</span>\n"
                                    "</div>\n"
                                    "<div id='wiki_box-body' class='box-body'><p>\n"
                                    + i18n( "Artist information could not be retrieved because the server was not reachable." ) +
                                    "</p></div>\n"
                                    "</div>\n"
                                );
        m_wikiBox->clearContents();
        m_wikiBox->setContents( m_wikiHTMLSource );
        if( !m_wikiVisible )
        {
            ContextView::instance()->addContextBox( m_wikiBox, m_order /* index */, false /* fadein */ );
            m_wikiVisible = true;
        }
        
        warning() << "[WikiFetcher] KIO error! errno: " << job->error() << endl;
        m_wikiJob = 0; // clear job
        return;
    }
    if ( job != m_wikiJob )
        return; //not the right job, so let's ignore it
    
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    m_wiki = QString( storedJob->data() );
    
    // Enable the Open in a Brower button, Disabled while loading, guz it would open wikipedia main page.
    //m_wikiToolBar->setItemEnabled( WIKI_BROWSER, true );
    //wikiExternalPageAction->setEnabled( true );
    
    // FIXME: Get a safer Regexp here, to match only inside of <head> </head> at least.
    if ( m_wiki.contains( "charset=utf-8"  ) ) {
        m_wiki = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );
    }
    
    if( m_wiki.indexOf( "var wgArticleId = 0" ) != -1 )
    {
        // article was not found
        if( m_wikiCurrentEntry.endsWith( wikiArtistPostfix() ) )
        {
            m_wikiCurrentEntry = m_wikiCurrentEntry.left( m_wikiCurrentEntry.length() - wikiArtistPostfix().length() );
            reloadWikipedia();
            return;
        }
        else if( m_wikiCurrentEntry.endsWith( wikiAlbumPostfix() ) )
        {
            m_wikiCurrentEntry = m_wikiCurrentEntry.left( m_wikiCurrentEntry.length() - wikiAlbumPostfix().length() );
            reloadWikipedia();
            return;
        }
        else if( m_wikiCurrentEntry.endsWith( wikiTrackPostfix() ) )
        {
            m_wikiCurrentEntry = m_wikiCurrentEntry.left( m_wikiCurrentEntry.length() - wikiTrackPostfix().length() );
            reloadWikipedia();
            return;
        }
    }
    
    //remove the new-lines and tabs(replace with spaces IS needed).
    m_wiki.replace( "\n", " " );
    m_wiki.replace( "\t", " " );
    
    m_wikiLanguages.clear();
    // Get the available language list
    if ( m_wiki.indexOf("<div id=\"p-lang\" class=\"portlet\">") != -1 )
    {
        m_wikiLanguages = m_wiki.mid( m_wiki.indexOf("<div id=\"p-lang\" class=\"portlet\">") );
        m_wikiLanguages = m_wikiLanguages.mid( m_wikiLanguages.indexOf("<ul>") );
        m_wikiLanguages = m_wikiLanguages.mid( 0, m_wikiLanguages.indexOf( "</div>" ) );
    }
    
    QString copyright;
    QString copyrightMark = "<li id=\"f-copyright\">";
    if ( m_wiki.indexOf( copyrightMark ) != -1 )
    {
        copyright = m_wiki.mid( m_wiki.indexOf(copyrightMark) + copyrightMark.length() );
        copyright = copyright.mid( 0, copyright.indexOf( "</li>" ) );
        copyright.replace( "<br />", QString() );
        //only one br at the beginning
        copyright.prepend( "<br />" );
    }
    
    // Ok lets remove the top and bottom parts of the page
    m_wiki = m_wiki.mid( m_wiki.indexOf( "<h1 class=\"firstHeading\">" ) );
    m_wiki = m_wiki.mid( 0, m_wiki.indexOf( "<div class=\"printfooter\">" ) );
    // Adding back license information
    m_wiki += copyright;
    m_wiki.append( "</div>" );
    m_wiki.replace( QRegExp("<h3 id=\"siteSub\">[^<]*</h3>"), QString() );
    
    m_wiki.replace( QRegExp( "<span class=\"editsection\"[^>]*>[^<]*<[^>]*>[^<]*<[^>]*>[^<]*</span>" ), QString() );
    
    m_wiki.replace( QRegExp( "<a href=\"[^\"]*\" class=\"new\"[^>]*>([^<]*)</a>" ), "\\1" );
    
    // Remove anything inside of a class called urlexpansion, as it's pointless for us
    m_wiki.replace( QRegExp( "<span class= *'urlexpansion'>[^(]*[(][^)]*[)]</span>" ), QString() );
    
    // Remove hidden table rows as well
    QRegExp hidden( "<tr *class= *[\"\']hiddenStructure[\"\']>.*</tr>", Qt::CaseInsensitive );
    hidden.setMinimal( true ); //greedy behaviour wouldn't be any good!
    m_wiki.replace( hidden, QString() );
    
    // we want to keep our own style (we need to modify the stylesheet a bit to handle things nicely)
    m_wiki.replace( QRegExp( "style= *\"[^\"]*\"" ), QString() );
    m_wiki.replace( QRegExp( "class= *\"[^\"]*\"" ), QString() );
    // let's remove the form elements, we don't want them.
    m_wiki.replace( QRegExp( "<input[^>]*>" ), QString() );
    m_wiki.replace( QRegExp( "<select[^>]*>" ), QString() );
    m_wiki.replace( "</select>\n" , QString() );
    m_wiki.replace( QRegExp( "<option[^>]*>" ), QString() );
    m_wiki.replace( "</option>\n" , QString() );
    m_wiki.replace( QRegExp( "<textarea[^>]*>" ), QString() );
    m_wiki.replace( "</textarea>" , QString() );
    
    //first we convert all the links with protocol to external, as they should all be External Links.
    m_wiki.replace( QRegExp( "href= *\"http:" ), "href=\"externalurl:" );
    m_wiki.replace( QRegExp( "href= *\"/" ), "href=\"" +m_wikiBaseUrl );
    m_wiki.replace( QRegExp( "href= *\"#" ), "href=\"" +m_wikiCurrentUrl + '#' );
    
    m_wikiHTMLSource = "<html><body>\n";
    m_wikiHTMLSource.append(
                             "<div id='wiki_box' class='box'>\n"
                             "<div id='wiki_box-header' class='box-header'>\n"
                             "<span id='wiki_box-header-title' class='box-header-title'>\n"
                             + i18n( "Wikipedia Information" ) +
                             "</span>\n"
                             "</div>\n"
                             "<div id='wiki_box-body' class='box-body'>\n"
                             + m_wiki +
                             "</div>\n"
                             "</div>\n"
                           );
    if ( !m_wikiLanguages.isEmpty() )
    {
        m_wikiHTMLSource.append(
                                 "<div id='wiki_box' class='box'>\n"
                                 "<div id='wiki_box-header' class='box-header'>\n"
                                 "<span id='wiki_box-header-title' class='box-header-title'>\n"
                                 + i18n( "Wikipedia Other Languages" ) +
                                 "</span>\n"
                                 "</div>\n"
                                 "<div id='wiki_box-body' class='box-body'>\n"
                                 + m_wikiLanguages +
                                 "</div>\n"
                                 "</div>\n"
                               );
    }
    m_wikiHTMLSource.append( "</body></html>\n" );
    m_wikiBox->clearContents();
    m_wikiBox->setContents( m_wikiHTMLSource );
    if( !m_wikiVisible )
    {
        ContextView::instance()->addContextBox( m_wikiBox, m_order /* index */, false /* fadein */ );
        m_wikiVisible = true;
    }
    
    m_wikiJob = 0;
}

#include "WikipediaItem.moc"

