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
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qstring.h>

#include <kdebug.h>
#include <kurl.h>


//statics
QColor PlaylistItem::GlowColor = Qt::white;
PlaylistItem *PlaylistItem::GlowItem  = 0;

//no reason to make this a member function
//FIXME make it a static member function
static const QString zeroPad( long );



PlaylistItem::PlaylistItem( QListView* lv, QListViewItem *lvi, const KURL &u, const MetaBundle *t )
      : KListViewItem( lv, lvi, ( u.protocol() == "file" ) ? u.fileName() : u.prettyURL() )
      , m_url( u )
{
    setDragEnabled( true );
    setDropEnabled( true );
    
    setText( 8, u.directory().section( '/', -1 ) );
    
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
                           ( f.isNull() ) ? 0 : f.audioProperties() );
}


void PlaylistItem::setMeta( const MetaBundle &bundle )
{
    QString str;
    
    if( !bundle.m_title.isEmpty() ) setText( 1, bundle.m_title ); //can be already set by playlist files
    setText( 2, bundle.m_artist );
    setText( 3, bundle.m_album );
    setText( 4, bundle.m_year );
    setText( 5, bundle.m_comment );
    setText( 6, bundle.m_genre );
    setText( 7, bundle.m_track );
    
    if( bundle.m_length != -1 )
    {
        int m_minutes, m_seconds;    
    
        m_minutes = bundle.m_length / 60 % 60;
        m_seconds = bundle.m_length % 60;
    
        str += QString::number( m_minutes ); //don't zeroPad, instead we rightAlign the column
        str += ":";
        str += zeroPad( m_seconds );
    }
    else str = "-";
    
    setText( 9, str );
    setText( 10, QString::number( bundle.m_bitrate ) + "kbps" );
}


#include <taglib/tag.h>
#include <taglib/tstring.h>

void PlaylistItem::writeTag( const QString &newtag, int col )
{
    TagLib::FileRef f( m_url.path().local8Bit(), false );
    
    if( !f.isNull() )
    {
        TagLib::Tag *t = f.tag(); //it is my impression from looking at the source that tag() never returns 0
        TagLib::String s = QStringToTString( newtag );
    
        switch( col ) {
        case 1:
            t->setTitle( s );
            break;
        case 2:
            t->setArtist( s );
            break;
        case 3:
            t->setAlbum( s );
            break;
        case 4:
            t->setYear( newtag.toInt() );
            break;
        case 5:
            //FIXME how does this work for vorbis files?
            //Are we likely to overwrite some other comments?
            //Vorbis can have multiple comment fields..
            t->setComment( s );
            break;
        case 6:
            t->setGenre( s );
            break;
        case 7:
            t->setTrack( newtag.toInt() );
            break;
        default:
            return;
        }
        
        f.save();
    }
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
