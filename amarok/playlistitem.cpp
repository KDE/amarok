/***************************************************************************
                          playlistitem.cpp  -  description
                             -------------------
    begin                : Die Dez 3 2002
    copyright            : (C) 2002 by Mark Kretschmann
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "playerapp.h"
#include "playlistitem.h"
#include "playlistwidget.h"
#include "browserwin.h"

#include <qlistview.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qstring.h>

#include <kdebug.h>
#include <kurl.h>
#include <kfilemetainfo.h>

PlaylistItem::PlaylistItem( QListView* parent, const KURL &url ) :
QListViewItem( parent, nameForUrl( url ) )
{

    m_url = url;
    init();
}



PlaylistItem::PlaylistItem( QListView* parent, QListViewItem *after, const KURL &url ) :
QListViewItem( parent, after )
{
    m_url = url;
    init();

    setText( 0, nameForUrl( url ) );
}



PlaylistItem::~PlaylistItem()
{
    if ( listView() )
    {
        if ( QString( listView()->name() ) == "PlaylistWidget" )
        {
            PlaylistWidget *parentView = static_cast<PlaylistWidget*>( listView() );

            if ( parentView->currentTrack() == this )
            {
                parentView->setCurrentTrack( NULL );
            }
        }
    }

    delete m_pMetaInfo;
}



void PlaylistItem::init()
{
    m_pMetaInfo = NULL;
    m_isDir = false;
    m_bIsGlowing = false;
    setDragEnabled( true );
    setDropEnabled( true );
}



QString PlaylistItem::nameForUrl( const KURL &url ) const
{
// only files have a filename.. for all other protocols the url itself is used as the name
    if ( url.protocol() == "file" )
        return url.fileName();
    else
        return url.url();
}


// METHODS -------------------------------------------------------

void PlaylistItem::readMetaInfo()
{
    if ( m_url.protocol() == "file" )
    {
        m_pMetaInfo = new KFileMetaInfo( m_url.path(), QString::null, KFileMetaInfo::Everything );
    }
}



KFileMetaInfo* PlaylistItem::metaInfo()
{
    return m_pMetaInfo;
}



void PlaylistItem::setMetaTitle()
{
    if ( m_pMetaInfo)
    {       
        if ( m_pMetaInfo->isValid() && !m_pMetaInfo->isEmpty() )
        {
            if ( m_pMetaInfo->item( "Title" ).string() != "---" )    
            {        
                QString str;

                str += m_pMetaInfo->item( "Artist" ).string();
                str += " - ";
                str += m_pMetaInfo->item( "Title" ).string();

                setText( 0, str );
            }
        }
    }
}
    

                
bool PlaylistItem::isDir()
{
    return m_isDir;
}



void PlaylistItem::setDir( bool on )
{
     m_isDir = on;
}



void PlaylistItem::paintCell( QPainter* p, const QColorGroup& cg, int column, int width, int align  )
{
    QColor col( 0x80, 0xa0, 0xff );
    int margin = 1;

    QPixmap *pBufPixmap = new QPixmap( width, height() );

    QPainter pPainterBuf( pBufPixmap, true );
    pPainterBuf.setBackgroundColor( Qt::black );

    if ( m_bIsGlowing )
    {
        pPainterBuf.setPen( m_glowCol );
    }
    else
    {
        pPainterBuf.setPen( col );
    }

    if ( isSelected() )
    {
        pPainterBuf.fillRect( 0, 0, width, height(), col.dark( 290 ) );
    }
    else
    {
        pPainterBuf.eraseRect( 0, 0, width, height() );
    }

    if ( pixmap( 0 ) )
    {
        pPainterBuf.drawPixmap( margin, 0, *pixmap( 0 ) );
        margin += pixmap( 0 )->width() + 1;
    }

    pPainterBuf.drawText( margin, 0, width-margin, height(), align, text(0) );
    pPainterBuf.end();
    p->drawPixmap( 0, 0, *pBufPixmap );

    delete pBufPixmap;
}



// paintFocus is an empty dummy function to disable focus drawing
void PlaylistItem::paintFocus( QPainter* p, const QColorGroup& cg, const QRect& r )
{
}
