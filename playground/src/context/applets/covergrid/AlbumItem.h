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

#ifndef ALBUM_ITEM_H
#define ALBUM_ITEM_H

#include "core/meta/forward_declarations.h"

#include <QLabel>

class QMouseEvent;
class QStyleOptionGraphicsItem;
class QPaintEvent;
class QEvent;

class AlbumItem : public QLabel
{
    Q_OBJECT

public:
    AlbumItem( const QPixmap & pixmap, Meta::AlbumPtr album, QWidget * parent = nullptr, Qt::WindowFlags f = 0  );
    ~AlbumItem();

    Meta::AlbumPtr getAlbum();

protected :
    void mousePressEvent( QMouseEvent  * event );
    void leaveEvent( QEvent * event);
    void enterEvent( QEvent * event);
    void mouseDoubleClickEvent( QMouseEvent * event );

private:
    Meta::AlbumPtr m_album;
    int m_size;
    QPixmap m_pixmap;
};
#endif
