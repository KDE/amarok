/****************************************************************************************
 * Copyright (c) 2011 Emmanuel Wagner <manu.wagner@sfr.fr>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "AlbumItem.h"

#include "playlist/PlaylistModelStack.h"
#include "core/meta/Meta.h"
#include "playlist/PlaylistController.h"

#include <QGraphicsPixmapItem>
#include <QStyleOptionGraphicsItem>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QImage>

#include <math.h>


AlbumItem::AlbumItem( const QPixmap & pixmap, Meta::AlbumPtr album , QWidget * parent, Qt::WindowFlags f )

    : QLabel( parent, f )
{
    m_album = album;
    m_pixmap = pixmap;
    setPixmap( pixmap );
    m_size = pixmap.height();
    setMouseTracking( true );
    setDisabled( false );
    if( album )
    {
        Meta::ArtistPtr artist = album->albumArtist();
        QString label = album->prettyName();
        if( artist ) label += " - " + artist->prettyName();
        setToolTip( label );
    }
}

AlbumItem::~AlbumItem()
{
    // emit the destructor here where actual (non-forward) declaration of Meta::* is known
}

Meta::AlbumPtr AlbumItem::getAlbum()
{
    return m_album;
}
void AlbumItem::mousePressEvent( QMouseEvent  * event )
{
    Q_UNUSED( event )
}
void AlbumItem::mouseDoubleClickEvent( QMouseEvent * event )
{
    Q_UNUSED( event )
    The::playlistController()->insertOptioned( m_album->tracks(), Playlist::AppendAndPlay );
}
void AlbumItem::leaveEvent( QEvent * event )
{
    setPixmap( m_pixmap );
}
void AlbumItem::enterEvent( QEvent * event )
{
    Q_UNUSED( event )
    QImage image = m_pixmap.toImage();
    QPixmap transparent( image.size() );
    transparent.fill( Qt::transparent );
    QPainter p;
    p.begin( &transparent );
    p.setCompositionMode( QPainter::CompositionMode_Source );
    p.drawPixmap( 0, 0, QPixmap::fromImage( image ) );
    p.setCompositionMode( QPainter::CompositionMode_DestinationIn );
    p.fillRect( transparent.rect(), QColor( 0, 0, 0, 150 ) );
    p.end();
    setPixmap( transparent );
}
