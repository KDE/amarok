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
//#include "playlistloader.h"

#include <string.h>

#include <qcolor.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qrect.h>
#include <qstring.h>

#include <kdebug.h>
#include <klistview.h>
#include <kurl.h>

#include <fileref.h>
#include <tag.h>


//statics
QColor PlaylistItem::GlowColor = Qt::white;
PlaylistItem *PlaylistItem::GlowItem  = 0;

//no reason to make this a member function
static const QString zeroPad( long );



PlaylistItem::PlaylistItem( QListView* lv, QListViewItem *lvi, const KURL &u, Tags *t, bool b )
      : KListViewItem( lv, lvi, ( u.protocol() == "file" ) ? u.fileName() : u.prettyURL() )
      , m_url( u )
      , m_isDir( b )      
      , m_tags( t )
{
    setDragEnabled( true );
    setDropEnabled( true );
    setMetaTitle();
}


PlaylistItem::~PlaylistItem()
{
    //FIXME this is hacky but necessary, it would be nice to tidy it up somewhat
    
    if( listView() && QString( listView()->name() ) == "PlaylistWidget" )
    {      
       PlaylistWidget *parentView = static_cast<PlaylistWidget *>( listView() );

       if( parentView->currentTrack() == this )
       {
           parentView->setCurrentTrack( NULL );
       }
    }
    
    delete m_tags;
}



// METHODS -------------------------------------------------------

void PlaylistItem::setMetaTitle()
{
   if( hasMetaInfo() )
   {
      setText( 1, m_tags->m_title );
      setText( 2, m_tags->m_artist );
      setText( 3, m_tags->m_album );
      setText( 4, m_tags->m_year );
      setText( 5, m_tags->m_comment );
      setText( 6, m_tags->m_genre );
      setText( 7, m_tags->m_directory );
   }
}


void PlaylistItem::paintCell( QPainter *p, const QColorGroup &, int column, int width, int align )
{
    // FIXME: alternative version
//     QColorGroup colGroup( cg );
//     colGroup.setColor( QColorGroup::Text, m_bIsGlowing ? m_glowCol : pApp->m_optBrowserFgColor );
//
//     KListViewItem::paintCell( p, colGroup, column, width, align );

// ----------------------------------------------

    int margin = 1;

    QPixmap *pBufPixmap = new QPixmap( width, height() );
    QPainter pPainterBuf( pBufPixmap, true );

    if ( listView() && QString( listView()->name() ) == "PlaylistWidget" &&
         isAlternate() )
    {
        pPainterBuf.setBackgroundColor( pApp->m_optBrowserBgAltColor );
    }
    else
    {
        pPainterBuf.setBackgroundColor( pApp->m_optBrowserBgColor );
    }

    if ( this == PlaylistItem::GlowItem ) //static member
    {
        pPainterBuf.setPen( PlaylistItem::GlowColor );
    }
    else
    {
        pPainterBuf.setPen( pApp->m_optBrowserFgColor );
    }

    if ( isSelected() )
    {
        pPainterBuf.fillRect( 0, 0, width, height(), pApp->m_optBrowserSelColor );
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

    pPainterBuf.setFont( listView()->font() );
    pPainterBuf.drawText( margin, 0, width - margin, height(), align, text( column ) );

    // draw column separator line
    if ( listView() && QString( listView()->name() ) == "PlaylistWidget" )
    {
        QPen linePen( Qt::darkGray, 0, Qt::DotLine );
        pPainterBuf.setPen( linePen );
        pPainterBuf.drawLine( width - 1, 0, width - 1, height() - 1 );
    }

    pPainterBuf.end();
    p->drawPixmap( 0, 0, *pBufPixmap );

    delete pBufPixmap;
}


// paintFocus is an empty dummy function to disable focus drawing
void PlaylistItem::paintFocus( QPainter*, const QColorGroup&, const QRect& )
{}


const QString PlaylistItem::length( uint se ) const
{
   //FIXME the uint is because mostly taglib doesn't return the track length, probably because we need
   //      to do an extended audioProperties scan, so do that, but can we do it lazily, ie on demand?

   if( seconds() == -1 ) return "--"; //-1 is set for streams by pls files and means "display no length info"

   int s = ( seconds() == 0 ) ? se : seconds();
   int m = s / 60;
   int h = m / 60;
   
   if ( h )
      return QString("%1:%2:%3").arg(h).arg(zeroPad(m % 60)).arg(zeroPad(s % 60));
   else
      return QString("%1:%2").arg(zeroPad(m)).arg(zeroPad(s % 60));
}


//non-member
const QString zeroPad( const long digit )
{
    QString str;
    str.setNum( digit );

    return ( digit > 9 ) ? str : str.prepend( '0' );
}
