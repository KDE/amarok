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


#include "GenericInfoBox.h"
#include "amarok.h"
#include "contextview.h"
#include "debug.h"
#include "scriptmanager.h"

#include <kurl.h>

#include <QGraphicsTextItem>

using namespace Context;

GenericInfoBox::GenericInfoBox( QGraphicsItem* parent, QGraphicsScene *scene ) : ContextBox( parent, scene )  {}

void GenericInfoBox::setContents( const QString& html )
{
    m_content = new QGraphicsTextItem( "", m_contentRect );
    m_content->setHtml( html );
    init();
	
}

void GenericInfoBox::init()
{
    m_content->setTextWidth( m_contentRect->rect().width() );// respect the boundaries given to us by the parent!
    m_content->setTextInteractionFlags( Qt::TextSelectableByMouse |
                                        Qt::LinksAccessibleByMouse );
    connect( m_content, SIGNAL( linkActivated ( QString ) ), this, SLOT( externalUrl( QString ) ) );
    int width =  (int) m_content->boundingRect().width();
    int height = (int) m_content->boundingRect().height();
    
    setContentRectSize( QSize( width, height ) );
}

void GenericInfoBox::clearContents() {
    debug() << "m_contentRect: " << m_contentRect << endl;
    if( m_content != 0 )
        delete m_content;
    /*m_content = new QGraphicsTextItem( m_contentRect );
    init();*/
}

// we want to make sure the text wraps to fit inside the new box size
void GenericInfoBox::ensureWidthFits( const int width )
{    
    const int newWidth = width - (int)ContextView::BOX_PADDING * 2;
    
    m_content->setTextWidth( newWidth );
    const int height = (int) m_content->boundingRect().height();
    
    QSize newSize = QSize( newWidth, height );
    setContentRectSize( newSize, false );
}


// NOTE: i can't make this work... m_content (QGraphicsTextItem) never emits the linkActivated signal...

void GenericInfoBox::externalUrl( const QString& urlS ) // SLOT
{
    DEBUG_BLOCK
        
    QString artist, album, track;
    KUrl* url = new  KUrl( urlS );
    
    Amarok::albumArtistTrackFromUrl( url->path(), artist, album, track );
    
    // All http links should be loaded inside wikipedia tab, as that is the only tab that should contain them.
    // Streams should use stream:// protocol.
    if ( url->protocol() == "http" )
    {
        /*if ( url->hasHTMLRef() )
        {
            KUrl base = url;
            base.setRef(QString());
            // Wikipedia also has links to otherpages with Anchors, so we have to check if it's for the current one
            if ( m_wikiCurrentUrl == base.url() ) {
                m_wikiPage->gotoAnchor( url->htmlRef() );
                return;
            }
        }
        // new page
        m_dirtyWikiPage = true;
        m_wikiCurrentEntry.clear();
        showWikipedia( url->url() ); */
        Amarok::invokeBrowser( url->url() );
    }
    
    else if ( url->protocol() == "show" )
    {
        if ( url->path() == "scriptmanager" )
        {
            ScriptManager::instance()->show();
            ScriptManager::instance()->raise();
        }
    } else if ( url->protocol() == "runscript" )
    {
        ScriptManager::instance()->runScript( url->path() );
    }
    else if ( url->protocol() == "externalurl" )
        Amarok::invokeBrowser( url->url().replace( QRegExp( "^externalurl:" ), "http:") );
    /*else if ( url->protocol() == "wikipedia" )
    {
        m_dirtyWikiPage = true;
        QString entry = unescapeHTMLAttr( url->path() );
        showWikipediaEntry( entry );
    }
    
    else if( url->protocol() == "ggartist" )
    {
        const QString url2 = QString( "http://www.google.com/musicsearch?q=%1&res=artist" )
            .arg( QString( KUrl::toPercentEncoding( unescapeHTMLAttr( url->path() ).replace( " ", "+" ), "/" ) ) );
        Amarok::invokeBrowser( url2 );
    }
    
    else if( url->protocol() == "file" )
    {
        The::playlistModel()->insertMedia( url, PlaylistNS::AppendAndPlay );
    }
    
    else if( url->protocol() == "stream" )
    {
        The::playlistModel()->insertMedia( KUrl( url->url().replace( QRegExp( "^stream:" ), "http:" ) ), PlaylistNS::AppendAndPlay );
    }
    
    else if( url->protocol() == "compilationdisc" || url->protocol() == "albumdisc" )
    {
        The::playlistModel()->insertMedia( expandURL( url ) , PlaylistNS::AppendAndPlay );
    }
    
    else
        HTMLView::openUrlRequest( url ); */
    delete url;
}

#include "GenericInfoBox.moc"
