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

#include "amarokconfig.h"
#include "collectiondb.h"
#include "metabundle.h"
#include "playlistitem.h"
#include "playlist.h"

#include <qlistview.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qstring.h>

#include <kdebug.h>
#include <kfilemetainfo.h>
#include <kiconloader.h>


QColor PlaylistItem::glowText = Qt::white;
QColor PlaylistItem::glowBase = Qt::white;


// These are untranslated and used for storing/retrieving XML playlist
const QString PlaylistItem::columnName(int col) //static
{
   switch(col) {
      case TrackName:
         return "TrackName";
         break;
      case Title:
         return "Title";
         break;
      case Artist:
         return "Artist";
         break;
      case Album:
         return "Album";
         break;
      case Year:
         return "Year";
         break;
      case Comment:
         return "Comment";
         break;
      case Genre:
         return "Genre";
         break;
      case Track:
         return "TrackNo";
         break;
      case Directory:
         return "Directory";
         break;
      case Length:
         return "Length";
         break;
      case Bitrate:
         return "Bitrate";
         break;
      case Score:
         return "Score";
         break;
   }
   return "<ERROR>";
}



//statics
QString PlaylistItem::stringStore[STRING_STORE_SIZE];


PlaylistItem::PlaylistItem( const KURL &u, QListView *lv, QListViewItem *lvi )
      : KListViewItem( lv, lvi, trackName( u ) )
      , m_url( u )
{
    setDragEnabled( true );

    setText( Directory, u.directory().section( '/', -1 ) );
}

PlaylistItem::PlaylistItem( const KURL &u, QListViewItem *lvi )
      : KListViewItem( lvi->listView(), lvi->itemAbove(), trackName( u ) )
      , m_url( u )
{
    setDragEnabled( true );

    setText( Directory, u.directory().section( '/', -1 ) );
}


PlaylistItem::PlaylistItem( const KURL &u, QListViewItem *lvi, const QDomNode &n )
      : KListViewItem( lvi->listView(), lvi->itemAbove(), trackName( u ) )
      , m_url( u )
{
    setDragEnabled( true );

    const uint ncol = listView()->columns();

    //NOTE we use base versions to speed this up (this function is called 100s of times during startup)
    for( uint x = 1; x < ncol; ++x )
    {
        const QString text = n.namedItem( columnName( x ) ).toElement().text();

        //FIXME this is duplication of setText()
        //TODO  it would be neat to have all store columns adjacent and at top end so you can use
        //      a simple bit of logic to discern which ones to store
        //FIXME use the MetaBundle implicitly shared bitrate and track # strings
        switch( x ) {
        case Artist:
        case Album:
        case Genre:
        case Year:
        case Directory:
            KListViewItem::setText( x, attemptStore( text ) );
            continue;
        default:
            KListViewItem::setText( x, text );
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

MetaBundle PlaylistItem::metaBundle()
{
    //TODO this meta prop reading causes ogg files to skip, so we need to do it a few seconds before the
    //ogg file starts playing or something

    //Do this everytime to save cost of storing int for length/samplerate/bitrate
    //This function isn't called often (on play request), but playlists can contain
    //thousands of items. So favor saving memory over CPU.

    return MetaBundle( this );
}


QString PlaylistItem::text( int column ) const
{
    //if there is no text set for the title, return a pretty version of the track name

    if( column == Title && KListViewItem::text( Title ).isEmpty() )
    {
        return MetaBundle::prettyTitle( KListViewItem::text( TrackName ) );
    }

    return KListViewItem::text( column );
}


QString
PlaylistItem::seconds() const
{
    QString length = exactText( Length );

    if( length == "?" ) return QString();
    if( length == "-" ) length += '1';
    else if( !length.isEmpty() )
    {
        int m = length.section( ':', 0, 0 ).toInt();
        int s = length.section( ':', 1, 1 ).toInt();

        length.setNum( m * 60 + s );
    }

    return length;
}

void PlaylistItem::setText( const MetaBundle &bundle )
{
    setText( Title,   bundle.title() );
    setText( Artist,  bundle.artist() );
    setText( Album,   bundle.album() );
    setText( Year,    bundle.year() );
    setText( Comment, bundle.comment() );
    setText( Genre,   bundle.genre() );
    setText( Track,   bundle.track() );
    setText( Length,  bundle.prettyLength() );
    setText( Bitrate, bundle.prettyBitrate() );
    CollectionDB *db = new CollectionDB();
    float score = db->getSongPercentage( bundle.url().path() );
    if( score )
        setText( Score, QString::number( score ) );
}


void PlaylistItem::setText( int column, const QString &newText )
{
    //NOTE prettyBitrate() is special and the returned string should not be modified
    //     as it is implicately shared for the common values in class MetaBundle
    //NOTE track() may also be special

    switch( column ) {
    case Artist:
    case Album:
    case Genre:
    case Year: //TODO check this doesn't hog the store
    case Directory:

        //these are good candidates for the stringStore
        //NOTE title is not a good candidate, it probably will never repeat in the playlist
        KListViewItem::setText( column, attemptStore( newText ) );
        break;

    case Length:
        //TODO consider making this a dynamically generated string
        KListViewItem::setText( Length, newText.isEmpty() ? newText : newText + ' ' ); //padding makes it neater
        break;

    default:
        KListViewItem::setText( column, newText );
        break;
    }
}

bool
PlaylistItem::operator== ( const PlaylistItem & item ) const
{
    return item.url() == this->url();
}

bool
PlaylistItem::operator< ( const PlaylistItem & item ) const
{
    return item.url() < this->url();
}

/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
/////////////////////////////////////////////////////////////////////////////////////

int
PlaylistItem::compare( QListViewItem *i, int col, bool ascending ) const
{
    float a, b;

    switch( col )  //we cannot sort numbers lexically, so we must special case those columns
    {
        case Track:
        case Year:
        case Score:
            a =    text( col ).toFloat();
            b = i->text( col ).toFloat();
            break;

        case Length:
            a =    text( Length ).replace( ':', '.' ).toFloat();
            b = i->text( Length ).replace( ':', '.' ).toFloat();
            break;

        case Bitrate:
            a =    text( Bitrate ).left( 3 ).toFloat(); //should work for 10 through 999 kbps
            b = i->text( Bitrate ).left( 3 ).toFloat(); //made this change due to setText space paddings
            break;

        case Artist:
            if( text( Artist ) == i->text( Artist ) ) //if same artist, try to sort by album
            {
                return this->compare( i, Album, ascending );
            }
            else goto lexical;

        case Album:
            if( text( Album ) == i->text( Album ) ) //if same album, try to sort by track
            {
                return this->compare( i, Track, true ); //only sort in ascending order //FIXME don't work
            }

            //else FALL_THROUGH..

        lexical:
        default:
            //is an ordinary string -> sort lexically
            return KListViewItem::compare( i, col, ascending );
    }

    if ( a > b ) return +1;
    if ( a < b ) return -1;

    return 0;    //a == b
}

void PlaylistItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    //TODO add spacing on either side of items
    //p->translate( 2, 0 ); width -= 3;

    //TODO this function is called extremely regularly as QListView sucks a little, optimise it immensely!!!!

    const int playNext = listView()->m_nextTracks.findRef( this ) + 1;

    if( this == listView()->currentTrack() )
    {
        //flicker-free drawing

        QPixmap buffer( width, height() );

        if( buffer.isNull() )
        {
            KListViewItem::paintCell( p, cg, column, width, align );
            return;
        }

        QPainter paint( &buffer, true );
        paint.setFont( p->font() );

        QColorGroup glowCg = cg; //shallow copy

        glowCg.setColor( QColorGroup::Base, glowBase );
        glowCg.setColor( QColorGroup::Text, glowText );

        //KListViewItem enforces alternate color, so we use QListViewItem
        QListViewItem::paintCell( &paint, glowCg, column, width, align );
        paint.end();

        p->drawPixmap( 0, 0, buffer );
    }
    else KListViewItem::paintCell( p, cg, column, width, align );

    //figure out if we are in the actual physical first column
    if( playNext && column == listView()->m_firstColumn )
    {
        QString str = QString::number( playNext );

        //draw the symbol's outline
              uint fw = p->fontMetrics().width( str ) + 2;
        const uint w  = 16; //keep this even
        const uint h  = height() - 2;

        p->setBrush( cg.highlight() );
        p->setPen( cg.highlight().dark() ); //TODO blend with background color
        p->drawEllipse( width - fw - w/2, 1, w, h );
        p->drawRect( width - fw, 1, fw, h );
        p->setPen( cg.highlight() );
        p->drawLine( width - fw, 2, width - fw, h - 1 );

        //draw the shadowed inner text
        //NOTE we can't set an arbituary font size or family, these settings are already optional
        //and user defaults should also take presidence if no playlist font has been selected
        //const QFont smallFont( "Arial", (playNext > 9) ? 9 : 12 );
        //p->setFont( smallFont );
        //TODO the shadow is hard to do well when using a dark font color
        //TODO it also looks cluttered for small font sizes
        //p->setPen( cg.highlightedText().dark() );
        //p->drawText( width - w + 2, 3, w, h-1, Qt::AlignCenter, str );
        fw += 2; //add some more padding
        p->setPen( cg.highlightedText() );
        p->drawText( width - fw, 2, fw, h-1, Qt::AlignCenter, str );
    }

    if( !isSelected() )
    {
        p->setPen( QPen( cg.mid(), 0, Qt::SolidLine ) );
        p->drawLine( width - 1, 0, width - 1, height() - 1 );
    }
}


void PlaylistItem::setup()
{
    KListViewItem::setup();

    if ( this == listView()->currentTrack() )
        setHeight( listView()->fontMetrics().height() * 2 );
}


const QString &PlaylistItem::attemptStore( const QString &candidate ) //static
{
    //principal is to cause collisions at reasonable rate to reduce memory
    //consumption while not using such a big store that it is mostly filled with empty QStrings
    //because collisions are so rare

    if( candidate.isEmpty() ) return candidate; //nothing to try to share

    const uchar hash = candidate[0].unicode() % STRING_STORE_SIZE;


    if( stringStore[hash] != candidate ) //then replace
    {
        stringStore[hash] = candidate;
    }

    return stringStore[hash];
}
