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

#include <string.h>

#include <qlistview.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qstring.h>

#include <kdebug.h>
#include <kurl.h>

#include <fileref.h>
#include <tag.h>

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
        if ( QString( listView() ->name() ) == "PlaylistWidget" )
        {
            PlaylistWidget * parentView = static_cast<PlaylistWidget*>( listView() );

            if ( parentView->currentTrack() == this )
            {
                parentView->setCurrentTrack( NULL );
            }
        }
    }
}


void PlaylistItem::init()
{
    m_hasMetaInfo = false;
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
        m_hasMetaInfo = true;

        TagLib::String str( m_url.path().utf8().data(), TagLib::String::UTF8 );
        TagLib::FileRef f( str );

        if ( !f.isNull() && f.tag() )
        {
            TagLib::Tag * tag = f.tag();

            m_tagTitle = QString( tag->title().toCString() );
            m_tagArtist = QString( tag->artist().toCString() );
            m_tagAlbum = QString( tag->album().toCString() );
            m_tagYear = QString().setNum( tag->year() );
            m_tagComment = QString( tag->comment().toCString() );
            m_tagGenre = QString( tag->genre().toCString() );
        }
    }
}


bool PlaylistItem::hasMetaInfo()
{
    return m_hasMetaInfo;
}


void PlaylistItem::setMetaTitle()
{
    if ( !m_tagTitle.isEmpty() )
        setText( 0, m_tagTitle );

    setText( 1, m_tagArtist );
    setText( 2, m_tagAlbum );
    setText( 3, m_tagYear );
    setText( 4, m_tagComment );
    setText( 5, m_tagGenre );
}


bool PlaylistItem::isDir()
{
    return m_isDir;
}


void PlaylistItem::setDir( bool on )
{
    m_isDir = on;
}


void PlaylistItem::paintCell( QPainter * p, const QColorGroup & cg, int column, int width, int align )
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
        margin += pixmap( 0 ) ->width() + 1;
    }

    pPainterBuf.drawText( margin, 0, width - margin, height(), align, text( column ) );
    pPainterBuf.end();
    p->drawPixmap( 0, 0, *pBufPixmap );

    delete pBufPixmap;
}


// paintFocus is an empty dummy function to disable focus drawing
void PlaylistItem::paintFocus( QPainter* /*p*/, const QColorGroup& /*cg*/, const QRect& /*r*/ )
{}
