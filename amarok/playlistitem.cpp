/***************************************************************************
                        playlistitem.cpp  -  description
                           -------------------
  begin                : Die Dez 3 2002
  copyright            : (C) 2002 by Mark Kretschmann
  email                : markey@web.de
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
    
    if ( listView() )
    {
        PlaylistWidget *parentView = static_cast<PlaylistWidget *>( listView() );
    
        if( parentView->currentTrack() == this )
            parentView->setCurrentTrack( NULL );
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


void PlaylistItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    QColorGroup newCg = cg;
    
    if ( this == PlaylistItem::GlowItem ) //static member
        newCg.setColor( QColorGroup::Text, PlaylistItem::GlowColor );

    KListViewItem::paintCell( p, newCg, column, width, align );   

    { //draw column separator line
        QPen linePen( Qt::darkGray, 0, Qt::DotLine );
        p->setPen( linePen );
        p->drawLine( width - 1, 0, width - 1, height() - 1 );
    }
}


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
