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
#include "metabundle.h"

#include "amarokconfig.h"

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

#include <klistview.h> //baseclass
#include <kdebug.h>
#include <kurl.h>


//statics
QColor PlaylistItem::GlowColor = Qt::white;
PlaylistItem *PlaylistItem::GlowItem  = 0;

//no reason to make this a member function
static const QString zeroPad( long );



PlaylistItem::PlaylistItem( QListView* lv, QListViewItem *lvi, const KURL &u, const MetaBundle *t )
      : KListViewItem( lv, lvi, ( u.protocol() == "file" ) ? u.fileName() : u.prettyURL() )
      , m_url( u )
{
    setDragEnabled( true );
    setDropEnabled( true );
    
    if( t ) setMeta( *t );
}


#include "playlistwidget.h"
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
}



// METHODS -------------------------------------------------------

#include <taglib/fileref.h>
#include <taglib/audioproperties.h>

const MetaBundle *PlaylistItem::metaBundle() const
{
    //FIXME only do once!
    TagLib::FileRef f( m_url.path().local8Bit(), true, TagLib::AudioProperties::Fast );
    
    return new MetaBundle( text( 1 ),
                           text( 2 ),
                           text( 3 ),
                           text( 4 ), 
                           text( 5 ),
                           text( 6 ),
                           text( 7 ),
                           QString( "" ),//text( 8 ), //TODO track #
                           ( f.isNull() ) ? 0 : f.audioProperties() );
}


void PlaylistItem::setMeta( const MetaBundle &bundle )
{
    int m_hours, m_minutes, m_seconds;
    QString str;
    
    if( !bundle.m_title.isEmpty() ) setText( 1, bundle.m_title ); //can be set by playlists, so the check is worthwhile
    setText( 2, bundle.m_artist );
    setText( 3, bundle.m_album );
    setText( 4, bundle.m_year );
    setText( 5, bundle.m_comment );
    setText( 6, bundle.m_genre );
    setText( 7, bundle.m_directory );
    
    m_minutes = bundle.m_length / 60 % 60;
    m_seconds = bundle.m_length % 60;
    
    if ( m_minutes < 10 ) str += "0";
    str += QString::number( m_minutes );
    str += ":";

    if ( m_seconds < 10 ) str += "0";
    str += QString::number( m_seconds );

    setText( 8, str );
    setText( 9, QString::number( bundle.m_bitrate ) + "kbps" );
}


void PlaylistItem::paintCell( QPainter *p, const QColorGroup &, int column, int width, int align )
{
    // FIXME: alternative version
//     QColorGroup colGroup( cg );
//     colGroup.setColor( QColorGroup::Text, m_bIsGlowing ? m_glowCol : pApp->config()->browserFgColor() );
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
        pPainterBuf.setBackgroundColor( pApp->config()->browserBgColor() );
    }

    if ( this == PlaylistItem::GlowItem ) //static member
    {
        pPainterBuf.setPen( PlaylistItem::GlowColor );
    }
    else
    {
        pPainterBuf.setPen( pApp->config()->browserFgColor() );
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
   //FIXME, we rely on arts. This function is silly currently
   //FIXME  store length in the playlistItem or use taglib everytime, or always use arts BUT move this function!

   int s = se;
   int m = s / 60;
   int h = m / 60;
   
   if ( h )
      return QString("%1:%2:%3").arg(h).arg(zeroPad(m % 60)).arg(zeroPad(s % 60));
   else
      return QString("%1:%2").arg(zeroPad(m)).arg(zeroPad(s % 60));
}


//non-member
static const QString zeroPad( const long digit )
{
    QString str;
    str.setNum( digit );

    return ( digit > 9 ) ? str : str.prepend( '0' );
}
