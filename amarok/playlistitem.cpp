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

#include "metabundle.h"
#include "playerapp.h"
#include "playlistitem.h"
#include "playlistwidget.h" //dtor

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


PlaylistItem::~PlaylistItem()
{
    if ( listView() )
    {
        PlaylistWidget *parentView = static_cast<PlaylistWidget *>( listView() );

        if( parentView->currentTrack() == this )
        {
            //FIXME requires friendship
            //FIXME friendship requires rebuilding world when we modify playlistitem.h!
            parentView->setCurrentTrack( NULL );
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

#include <taglib/fileref.h>
#include <taglib/audioproperties.h>

MetaBundle PlaylistItem::metaBundle() const
{
    //FIXME only do once!
    //Here we do an accurate scan before every bundle is requested because:
    //1) they are only ever requested individually
    //2) we REQUIRE this data to get the song length right!
    TagLib::FileRef f( m_url.path().local8Bit(), true, TagLib::AudioProperties::Accurate );

    //FIXME hold a small cache of metabundles?
    //then return by reference
    return MetaBundle( text( 1 ),
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
    QString length( "-" );

    if( !bundle.m_title.isEmpty() ) setText( 1, bundle.m_title ); //can be already set by playlist files
    setText( 2, bundle.m_artist );
    setText( 3, bundle.m_album );
    setText( 4, bundle.m_year );
    setText( 5, bundle.m_comment );
    setText( 6, bundle.m_genre );
    setText( 7, bundle.m_track );

    if( bundle.m_length != -1 )
    {
        int min = bundle.m_length / 60 % 60;
        int sec = bundle.m_length % 60;

        length  = QString::number( min ); //don't zeroPad, instead we rightAlign the column
        length += QChar( ':' );
        length += zeroPad( sec );
    }

    setText(  9, length );
    setText( 10, QString::number( bundle.m_bitrate ) + "kbps" );
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

/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
/////////////////////////////////////////////////////////////////////////////////////

int PlaylistItem::compare( QListViewItem *i, int col, bool ascending ) const
{
    float a, b;

    switch( col )  //we cannot sort numbers lexically, so we must special case those columns
    {
        case 4:    //year
            a =    text( 4 ).toFloat();
            b = i->text( 4 ).toFloat();
            break;

        case 7:    //track
            a =    text( 7 ).toFloat();
            b = i->text( 7 ).toFloat();
            break;

        case 9:    //length
            a =    text( 9 ).replace( ":", "." ).toFloat();
            b = i->text( 9 ).replace( ":", "." ).toFloat();
            break;

        case 10:   //bitrate
            a =    text( 10 ).section( "kbps", 0, 0 ).toFloat();
            b = i->text( 10 ).section( "kbps", 0, 0 ).toFloat();
            break;

        default:   //ordinary string -> sort lexically
            return KListViewItem::compare( i, col, ascending );
    }

    if ( a > b ) return +1;
    if ( a < b ) return -1;

    return 0;    //a == b
}


void PlaylistItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    if ( this == PlaylistItem::GlowItem )
    {
        const QColor GlowText( 0xff, 0x40, 0x40 ); //FIXME extend QColorGroup and add this member
        QColorGroup glowCg = cg; //shallow copy
        int h, s, v;

        GlowText.getHsv( &h, &s, &v );
        QColor glowBase( h, ( s > 50 ) ? s - 50 : s + 50, v, QColor::Hsv );
        QColor normBase( cg.base() );
        glowBase.setRgb( (normBase.red()*3   + glowBase.red()*2)   /5,
                         (normBase.green()*3 + glowBase.green()*2) /5,
                         (normBase.blue()*3  + glowBase.blue()*2)  /5 );

        glowCg.setColor( QColorGroup::Text, GlowText );
        glowCg.setColor( QColorGroup::Base, glowBase );

        //KListViewItem enforces alternate color, so we use QListViewItem
        QListViewItem::paintCell( p, glowCg, column, width, align );

    } else {

        KListViewItem::paintCell( p, cg, column, width, align );
    }

    p->setPen( QPen( cg.dark(), 0, Qt::DotLine ) );
    p->drawLine( width - 1, 0, width - 1, height() - 1 );
}
