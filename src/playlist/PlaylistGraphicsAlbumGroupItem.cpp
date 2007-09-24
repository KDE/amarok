/***************************************************************************
 * copyright     : (C) 2007 Ian Monroe <ian@monroe.nu>                     *
 *                 (C) 2007 Nikolaj Hals Nielsen <nhnFreespirit@gmail.com  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "debug.h"
#include "meta/MetaUtility.h"
#include "AmarokMimeData.h"
#include "PlaylistGraphicsAlbumGroupItem.h"
#include "PlaylistGraphicsView.h"
#include "PlaylistDropVis.h"
#include "PlaylistModel.h"
#include "PlaylistTextItem.h"
#include "TheInstances.h"

#include <QBrush>
#include <QDrag>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QMimeData>
#include <QPen>
#include <QRadialGradient>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>

using namespace Meta;

const qreal Playlist::GraphicsAlbumGroupItem::ALBUM_WIDTH = 50.0;
const qreal Playlist::GraphicsAlbumGroupItem::MARGIN = 2.0;
QFontMetricsF* Playlist::GraphicsAlbumGroupItem::s_fm = 0;

Playlist::GraphicsAlbumGroupItem::GraphicsAlbumGroupItem()
    : QGraphicsItem()
    , m_lastWidth( -1 )
{
    setZValue( 1.0 );
    if( !s_fm )
    {
        s_fm = new QFontMetricsF( QFont() );
        m_height =  qMax( ALBUM_WIDTH, s_fm->height() * 2 ) + 2 * MARGIN;
    }

    //setFlag( QGraphicsAlbumGroupItem::ItemIsSelectable );
    //setFlag( QGraphicsAlbumGroupItem::ItemIsMovable );
    //setAcceptDrops( true );
   // setHandlesChildEvents( true ); // don't let drops etc hit the text items, doing stupid things
}

Playlist::GraphicsAlbumGroupItem::~GraphicsAlbumGroupItem()
{
}

void Playlist::GraphicsAlbumGroupItem::setAlbum( AlbumPtr album ) {

    m_album = album;
}

void Playlist::GraphicsAlbumGroupItem::addTrack( TrackPtr track ) {

    m_tracks << track;

    //TODO: make the font metrics global!!
    m_height = ALBUM_WIDTH + 2 * MARGIN + 16 * m_tracks.count() + 16;
}

void 
Playlist::GraphicsAlbumGroupItem::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
   
    Q_UNUSED( widget );

    //paint background;
    painter->fillRect( option->rect, option->palette.alternateBase() );

    //check if we have an album, othervise, get the hell out of here...
    if ( m_album.data() == 0 ) return;

    //paint albumImage:
    painter->drawPixmap( MARGIN, MARGIN, m_album->image( ALBUM_WIDTH, false ) );

    QFont headerFont;
    headerFont.setPointSize( headerFont.pointSize() + 2 );
    painter->setFont( headerFont );
    QFontMetrics hfm( headerFont );

    ArtistPtr artistPtr = m_album->albumArtist();
     
    QString artist;
    if ( artistPtr.data() != 0 )
        artist = artistPtr->prettyName();
    else  
        artist = "Unknown Artist";

    int yOffset = MARGIN + ( ALBUM_WIDTH - hfm.height() * 2 ) + hfm.height();
    int xOffset = ( ( option->rect.width() + ( MARGIN * 2 + ALBUM_WIDTH ) ) - hfm.width( artist ) ) / 2;

    painter->drawText( xOffset, yOffset, artist );

    yOffset += hfm.height();
    xOffset = ( ( option->rect.width() + ( MARGIN * 2 + ALBUM_WIDTH ) ) - hfm.width( m_album->prettyName() ) ) / 2;
    painter->drawText( xOffset, yOffset,  m_album->prettyName() );


    QFont font;
    font.setPointSize( font.pointSize() - 1 );
    painter->setFont( font );

    QFontMetrics fm( font );

    int trackYOffset = MARGIN * 2 + ALBUM_WIDTH;

    bool alternateBackground = false;
    foreach ( TrackPtr track, m_tracks ) {

        trackYOffset += fm.height();

        if ( alternateBackground ) 
            painter->fillRect( 10, trackYOffset + 5 - fm.height(), option->rect.width() - 20, fm.height(), option->palette.alternateBase() );
        else
            painter->fillRect( 10, trackYOffset + 5 - fm.height(), option->rect.width() - 20, fm.height(), option->palette.base());

        painter->drawText( 15, trackYOffset, QString::number(  track->trackNumber() ) + " - " + track->prettyName() );

        alternateBackground = !alternateBackground;
    }

}


void
Playlist::GraphicsAlbumGroupItem::resize( Meta::TrackPtr track, int totalWidth )
{
    if( totalWidth == -1 || totalWidth == m_lastWidth ) //no change needed
        return;

}

QRectF
Playlist::GraphicsAlbumGroupItem::boundingRect() const
{
    // the viewport()->size() takes scrollbars into account

    return QRectF( 0.0, 0.0, The::playlistView()->viewport()->size().width(), m_height );
}




