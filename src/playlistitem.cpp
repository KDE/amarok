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

#include "amarok.h"
#include "amarokconfig.h"
#include <cmath>
#include "debug.h"
#include "playlist.h"
#include "collectiondb.h"
#include <kfilemetainfo.h>
#include <kiconloader.h>
#include <kstringhandler.h>
#include <kglobal.h>
#include "metabundle.h"
#include "playlistitem.h"
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qrect.h>

QColor PlaylistItem::glowText = Qt::white;
QColor PlaylistItem::glowBase = Qt::white;
bool   PlaylistItem::s_pixmapChanged = false;


/// These are untranslated and used for storing/retrieving XML playlist
const QString PlaylistItem::columnName( int c ) //static
{
    switch( c ) {
        case Filename:  return "Filename";
        case Title:     return "Title";
        case Artist:    return "Artist";
        case Album:     return "Album";
        case Year:      return "Year";
        case Comment:   return "Comment";
        case Genre:     return "Genre";
        case Track:     return "Track";
        case Directory: return "Directory";
        case Length:    return "Length";
        case Bitrate:   return "Bitrate";
        case Score:     return "Score";
        case Type:      return "Type";
        case Playcount: return "Playcount";
    }
    return "<ERROR>";
}


//statics
QString PlaylistItem::stringStore[STRING_STORE_SIZE];


PlaylistItem::PlaylistItem( QListView *listview, QListViewItem *item )
        : KListViewItem( listview, item )
{
    KListViewItem::setVisible( false );
}

PlaylistItem::PlaylistItem( const MetaBundle &bundle, QListViewItem *lvi )
        : KListViewItem( lvi->listView(), lvi->itemAbove(), filename( bundle.url() ) )
        , m_url( bundle.url() )
        , m_missing( false )
        , m_enabled( true )
{
    setDragEnabled( true );

    setText( bundle );

    const int length = seconds().toInt();
    listView()->m_totalCount++;
    listView()->m_totalLength += length;
    if( isSelected() )
    {
        listView()->m_selCount++;
        listView()->m_selLength += length;
    }
    if( isVisible() )
    {
        listView()->m_visCount++;
        listView()->m_visLength += length;
    }

    listView()->setFilterForItem( listView()->m_filter, this );

    listView()->countChanged();
}

PlaylistItem::PlaylistItem( QDomNode node, QListViewItem *item )
        : KListViewItem( item->listView(), item->itemAbove() )
        , m_url( node.toElement().attribute( "url" ) )
        , m_missing( false )
        , m_enabled( true )
{
    setDragEnabled( true );
    KListViewItem::setText( Filename, filename( m_url ) );

    //NOTE we use base versions to speed this up (this function is called 100s of times during startup)
    for( uint x = 1, n = listView()->columns(); x < n; ++x ) {
        const QString text = node.namedItem( columnName( x ) ).toElement().text();

        switch( x ) {
        case Artist:
        case Album:
        case Genre:
        case Year:
        case Directory:
            KListViewItem::setText( x, attemptStore( text ) );
            continue;
        case Score:
            KListViewItem::setText( x,
                    QString::number( CollectionDB::instance()->getSongPercentage( m_url.path() ) ) );
            continue;
        case Playcount:
            KListViewItem::setText( x,
                    QString::number( CollectionDB::instance()->getPlayCount( m_url.path() ) ) );
            continue;
        case Type:
        default:
            KListViewItem::setText( x, text );
        }
    }

    const int length = seconds().toInt();
    listView()->m_totalCount++;
    listView()->m_totalLength += length;
    if( isSelected() )
    {
        listView()->m_selCount++;
        listView()->m_selLength += length;
    }
    if( isVisible() )
    {
        listView()->m_visCount++;
        listView()->m_visLength += length;
    }

    listView()->setFilterForItem( listView()->m_filter, this );

    listView()->countChanged();
}

PlaylistItem::~PlaylistItem()
{
    if( text( 0 ) == "MARKERITEM" )
        return;

    const int length = seconds().toInt();
    listView()->m_totalCount--;
    listView()->m_totalLength -= length;
    if( isSelected() )
    {
        listView()->m_selCount--;
        listView()->m_selLength -= length;
    }
    if( isVisible() )
    {
        listView()->m_visCount--;
        listView()->m_visLength -= length;
    }

    listView()->countChanged();
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////


QString PlaylistItem::text( int column ) const
{
    //if there is no text set for the title, return a pretty version of the filename

    if( column == Title && KListViewItem::text( Title ).isEmpty()
            // this is important, as we don't want to show the filename twice
            && listView()->columnWidth( Filename ) == 0 )
    {
        return MetaBundle::prettyTitle( KListViewItem::text( Filename ) );
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

void PlaylistItem::setEnabled( bool enabled )
{
    m_enabled = enabled;
    setDropEnabled( enabled ); // this forbids items to be dropped into a history queue.

    repaint();
}

void PlaylistItem::setSelected( bool selected )
{
    if( isVisible() )
    {
        const bool prevSelected = isSelected();
        const int length = seconds().toInt();
        KListViewItem::setSelected( selected );
        if( prevSelected && !isSelected() )
        {
            listView()->m_selCount--;
            listView()->m_selLength -= length;
            listView()->countChanged();
        }
        else if( !prevSelected && isSelected() )
        {
            listView()->m_selCount++;
            listView()->m_selLength += length;
            listView()->countChanged();
        }
    }
}

void PlaylistItem::setVisible( bool visible )
{
    const int length = seconds().toInt();
    if( !visible && isSelected() )
    {
        listView()->m_selCount--;
        listView()->m_selLength -= length;
        KListViewItem::setSelected( false );
        listView()->countChanged();
    }

    const bool prevVisible = isVisible();
    KListViewItem::setVisible( visible );
    if( prevVisible && !isVisible() )
    {
        listView()->m_visCount--;
        listView()->m_visLength -= length;
        listView()->countChanged();
    }
    else if( !prevVisible && isVisible() )
    {
        listView()->m_visCount++;
        listView()->m_visLength += length;
        listView()->countChanged();
    }
}

void PlaylistItem::setText( const MetaBundle &bundle )
{
    for( int i = 0; i < NUM_COLUMNS; ++i )
        setText( i, bundle.infoByColumn( i, true ) );

    m_missing = !bundle.exists();

    const int score = CollectionDB::instance()->getSongPercentage( bundle.url().path() );
    if ( score )
        setText( Score, QString::number( score ) );

    const int playcount = CollectionDB::instance()->getPlayCount( bundle.url().path() );
    if ( playcount )
        setText( Playcount, QString::number( playcount ) );
    else
        setText( Playcount, QString::number( 0 ) );  //Never played before.
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
    case Directory:

        //these are good candidates for the stringStore
        //NOTE title is not a good candidate, it probably will never repeat in the playlist
        KListViewItem::setText( column, attemptStore( newText ) );
        break;

    case Length:
        //TODO consider making this a dynamically generated string
        KListViewItem::setText( Length, newText.isEmpty() ? newText : newText + ' ' ); //padding makes it neater
        break;

    case Track:
    case Year:
        KListViewItem::setText( column, newText == "0" ? QString::null : attemptStore( newText ) );
        break;

     case Type:
     case Playcount:

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
    QString a =    text( col ).lower();
    QString b = i->text( col ).lower();

    switch( col )  //we must pad numbers to sort them lexically, so we must special case those columns
    {
        case Track:
        case Score:
        case Length:
        case Type:
        case Playcount:
        case Bitrate:
            a = a.rightJustify( b.length(), '0' ); //all these columns shouldn't become negative
            b = b.rightJustify( a.length(), '0' ); //so simply left-padding is sufficient
            break;

        case Year:
            if( a == b )
                return this->compare( i, Artist, ascending );
            break;

        case Artist:
            if( a == b ) //if same artist, try to sort by album
                return this->compare( i, Album, ascending );
            break;

        case Album:
            if( a == b ) //if same album, try to sort by track
                //TODO only sort in ascending order?
                return this->compare( i, Track, true ) * (ascending ? 1 : -1);
            break;

        default:;
    }

    return QString::localeAwareCompare( a, b );
}

void PlaylistItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    //TODO add spacing on either side of items
    //p->translate( 2, 0 ); width -= 3;

    const bool isCurrent = this == listView()->currentTrack();

    if( isCurrent && !isSelected() )
    {
        static paintCacheItem paintCache[NUM_COLUMNS];

        // Convert QColor to string for use as key in QMap
        const QString colorKey =
            QString::number( glowBase.red() ) +
            QString::number( glowBase.green() ) +
            QString::number( glowBase.blue() );

        const bool cacheValid =
            paintCache[column].width == width &&
            paintCache[column].height == height() &&
            paintCache[column].text == text( column ) &&
            paintCache[column].font == p->font() &&
            !s_pixmapChanged;

        // If any parameter changed, we must regenerate all pixmaps
        if ( !cacheValid )
        {
            for( int i = 0; i < NUM_COLUMNS; ++i)
                paintCache[i].map.clear();
            s_pixmapChanged = false;
        }

        // Determine if we need to repaint the pixmap, or paint from cache
        if ( paintCache[column].map.find( colorKey ) == paintCache[column].map.end() )
        {
            // Update painting cache
            paintCache[column].width = width;
            paintCache[column].height = height();
            paintCache[column].text = text( column );
            paintCache[column].font = p->font();
            paintCache[column].map[colorKey] = QPixmap( width, height() );

            // Don't try to draw if width or height is 0, as this crashes Qt
            if ( paintCache[column].map[colorKey].isNull() ) return;

            QPainter paint( &paintCache[column].map[colorKey], true );

            // Here we draw the shaded background
            int h, s, v;
            glowBase.getHsv( &h, &s, &v );
            QColor col;

            for ( int i = 0; i < height(); i++ ) {
                col.setHsv( h, s, static_cast<int>( sin( (float)i / ( (float)height() / 4 ) ) * 32.0 + 196 ) );
                paint.setPen( col );
                paint.drawLine( 0, i, width, i );
            }

            // Draw the pixmap, if present
            const int margin = listView()->itemMargin();
            int leftMargin = margin;
            if ( pixmap( column ) ) {
                paint.drawPixmap( leftMargin, height() / 2 - pixmap( column )->height() / 2, *pixmap( column ) );
                leftMargin += pixmap( column )->width() + margin;
            }

            if( align != Qt::AlignCenter )
               align |= Qt::AlignVCenter;

            // Draw the text
            static QFont font;
            static int minbearing = 1337 + 666;
            if( minbearing == 2003 || font != p->font() )
            {
                font = p->font();
                minbearing = p->fontMetrics().minLeftBearing() + p->fontMetrics().minRightBearing();
            }
            paint.setFont( font );
            paint.setPen( glowText );
            const int _width = width - leftMargin - margin + minbearing - 1; // -1 seems to be necessary *shrug*
            const QString _text = KStringHandler::rPixelSqueeze( text( column ), p->fontMetrics(), _width );
            paint.drawText( leftMargin, 0, _width, height(), align, _text );

            paint.end();
        }

        p->drawPixmap( 0, 0, paintCache[column].map[colorKey] );
    }
    else {
        QColorGroup _cg = cg;
        //FIXME not acceptable to hardcode the colour
        QColor disabledText = QColor( 172, 172, 172 );
        if( m_missing || !m_enabled )
            _cg.setColor( QColorGroup::Text, disabledText );

        KListViewItem::paintCell( p, _cg, column, width, align );
    }

    /// Track action symbols
    const int  queue       = listView()->m_nextTracks.findRef( this ) + 1;
    const bool stop        = ( this == listView()->m_stopAfterTrack );
    const bool repeat      = AmarokConfig::repeatTrack() && isCurrent;

    const uint num = ( queue ? 1 : 0 ) + ( stop ? 1 : 0 ) + ( repeat ? 1 : 0 );

    static const QPixmap pixstop   = amaroK::getPNG(  "currenttrack_stop_small"  ),
                         pixrepeat = amaroK::getPNG( "currenttrack_repeat_small" );

    //figure out if we are in the actual physical first column
    if( column == listView()->m_firstColumn && num )
    {
        //margin, height
        const uint m = 2, h = height() - m;

        const QString str = QString::number( queue );

        const uint qw = p->fontMetrics().width( str ), sw = pixstop.width(),  rw = pixrepeat.width(),
                   qh = p->fontMetrics().height(),     sh = pixstop.height(), rh = pixrepeat.height();

        //maxwidth
        const uint mw = kMax( qw, kMax( rw, sw ) );

        //width of first & second column of pixmaps
        const uint w1 = ( num == 3 ) ? kMax( qw, rw )
                      : ( num == 2 && isCurrent ) ? kMax( repeat ? rw : 0, kMax( stop ? sw : 0, queue ? qw : 0 ) )
                      : ( num == 2 ) ? qw
                      : queue ? qw : repeat ? rw : stop ? sw : 0,
                   w2 = ( num == 3 ) ? sw
                      : ( num == 2 && !isCurrent ) ? sw
                      : 0; //phew

        //ellipse width, total width
        const uint ew = 16, tw = w1 + w2 + m * ( w2 ? 2 : 1 );
        p->setBrush( cg.highlight() );
        p->setPen( cg.highlight().dark() ); //TODO blend with background color
        p->drawEllipse( width - tw - ew/2, m / 2, ew, h );
        p->drawRect( width - tw, m / 2, tw, h );
        p->setPen( cg.highlight() );
        p->drawLine( width - tw, m/2 + 1, width - tw, h - m/2 );

        int x = width - m - mw, y = height() / 2, tmp = 0;
        const bool multi = ( isCurrent && num >= 2 );
        if( queue )
        {
            //draw the shadowed inner text
            //NOTE we can't set an arbituary font size or family, these settings are already optional
            //and user defaults should also take presidence if no playlist font has been selected
            //const QFont smallFont( "Arial", (playNext > 9) ? 9 : 12 );
            //p->setFont( smallFont );
            //TODO the shadow is hard to do well when using a dark font color
            //TODO it also looks cluttered for small font sizes
            //p->setPen( cg.highlightedText().dark() );
            //p->drawText( width - w + 2, 3, w, h-1, Qt::AlignCenter, str );

            if( !multi )
                tmp = -(qh / 2);
            y += tmp;
            p->setPen( cg.highlightedText() );
            p->drawText( x, y, -x + width, multi ? h/2 : qh, Qt::AlignCenter, str );
            y -= tmp;
            if( isCurrent )
                y -= height() / 2;
            else
                x -= m + w2;
        }
        if( repeat )
        {
            if( multi )
                tmp = (h/2 - rh)/2 + ( num == 2 && stop ? 0 : 1 );
            else
                tmp = -(rh / 2);
            y += tmp;
            p->drawPixmap( x, y, pixrepeat );
            y -= tmp;
            if( num == 3 )
            {
                x -= m + w2 + 2;
                y = height() / 2;
            }
            else
                y -= height() / 2;
        }
        if( stop )
        {
            if( multi && num != 3 )
                tmp = m + (h/2 - sh)/2;
            else
                tmp = -(sh / 2);
            y += tmp;
            p->drawPixmap( x, y, pixstop );
            y -= tmp;
        }
    }

    if( this != listView()->currentTrack() && !isSelected() )
    {
        p->setPen( QPen( cg.mid(), 0, Qt::SolidLine ) );
        p->drawLine( width - 1, 0, width - 1, height() - 1 );
    }
}


void PlaylistItem::setup()
{
    KListViewItem::setup();

    if( this == listView()->currentTrack() )
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
