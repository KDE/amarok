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
#include "playlistwidget.h" //this class is tied to PlaylistWidget

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


PlaylistItem::PlaylistItem( PlaylistWidget* parent, QListViewItem *lvi, const KURL &u, const QString &title, const int length )
      : KListViewItem( parent, lvi, ( u.protocol() == "file" ) ? u.fileName() : u.prettyURL() )
      , m_url( u )
{
    setDragEnabled( true );
    setDropEnabled( true );
    
    // our friend threadweaver will take care of this flag
    corruptFile = FALSE;

    KListViewItem::setText( 1, title );
    KListViewItem::setText( 8, u.directory().section( '/', -1 ) );
    KListViewItem::setText( 9, MetaBundle::prettyLength( length ) );
}


PlaylistItem::~PlaylistItem()
{
    if( listView() && listView()->currentTrack() == this )
    {
        listView()->setCurrentTrack( NULL );
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

#include <taglib/fileref.h>
#include <taglib/audioproperties.h>

MetaBundle PlaylistItem::metaBundle()
{
    //TODO this meta prop reading causes off files to skip, so we need to do it a few seconds before the
    //ogg file starts playing or something

    //Do this everytime to save cost of storing int for length/samplerate/bitrate
    //This function isn't called often (on play request), but playlists can contain
    //thousands of items. So favor saving memory over CPU.
    TagLib::FileRef f( m_url.path().local8Bit(), true, TagLib::AudioProperties::Accurate );

    //FIXME hold a small cache of metabundles?
    //then return by reference
    MetaBundle bundle( this, f.isNull() ? 0 : f.audioProperties() );

    //just set it as we just did an accurate pass
    setText(  9, bundle.prettyLength()  );
    setText( 10, bundle.prettyBitrate() );

    return bundle;
}


QString PlaylistItem::text( int column ) const
{
    //if trackname (column 0) is hidden, then show trackname text in title column if there
    //is no text set for the title (column 1)

    if( column == 1 && listView()->columnWidth( 0 ) == 0 && KListViewItem::text( 1 ).isEmpty() )
    {
            return KListViewItem::text( 0 );
    }

    return KListViewItem::text( column );
}


void PlaylistItem::setText( const MetaBundle &bundle )
{
    setText( 1,  bundle.m_title );
    setText( 2,  bundle.m_artist );
    setText( 3,  bundle.m_album );
    setText( 4,  bundle.m_year );
    setText( 5,  bundle.m_comment );
    setText( 6,  bundle.m_genre );
    setText( 7,  bundle.m_track );
    setText( 9,  bundle.prettyLength() );
    setText( 10, bundle.prettyBitrate() );
}


void PlaylistItem::setText( int column, const QString &newText )
{
    switch( column ) {
    case 1:
    case 9:
    case 10:
        if( newText.isEmpty() )
        {
            //don't overwrite old text with nothing
            //we do this because there are several stages to setting text when items are inserted into the
            //playlist, and not all of them have text set.
            //we only need to do this for columns 1, 9 and 10 currently

            //FIXME removing a tag with inline edit doesn't get updated here, but
            //      you've hacked TagWriter so it sets a space as the new text
            //FIXME that needs fixing because it means the scrolling title has a space! dang.

            //NOTE if you don't setText() it crashes amaroK!
            KListViewItem::setText( column, text( column ) );

            break;
        }
        //else do default -->
    default:
        KListViewItem::setText( column, newText );
    }
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
            a =    text( 10 ).remove( "kbps" ).toFloat();
            b = i->text( 10 ).remove( "kbps" ).toFloat();
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
    if( column == 9 && text( 9 ).isEmpty() ) listView()->readAudioProperties( this );

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
        if ( corruptFile )
        {
            QColorGroup corruptCg = cg;
            QColor corruptBg( 0xcc, 0xcc, 0xcc );
            corruptCg.setColor( QColorGroup::Base, corruptBg );
            KListViewItem::paintCell( p, corruptCg, column, width, align );
        } else
            KListViewItem::paintCell( p, cg, column, width, align );
    }

    p->setPen( QPen( cg.dark(), 0, Qt::DotLine ) );
    p->drawLine( width - 1, 0, width - 1, height() - 1 );
}
