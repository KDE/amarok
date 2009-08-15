/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef PRETTYITEMDELEGATE_H
#define PRETTYITEMDELEGATE_H

#include "playlist/layouts/LayoutItemConfig.h"

#include <QModelIndex>
#include <QStyledItemDelegate>

class QPainter;

namespace Playlist
{
class PrettyItemDelegate : public QStyledItemDelegate
{
public:
    PrettyItemDelegate( QObject* parent = 0 );
    ~PrettyItemDelegate();

    QSize sizeHint( const QStyleOptionViewItem&, const QModelIndex& ) const;
    void paint( QPainter*, const QStyleOptionViewItem&, const QModelIndex& ) const;

    // helper function for view which lets us determine if a click is within an album group's header
    static bool insideItemHeader( const QPoint&, const QRect& );

   /**
    * Handle clicks witin a delegate.
    * @param pos The position of the click, in delegate coordinates.
    * @param pos The index of the clicked item.
    * @return True if delegate acts on this click, false otherwise.
    */
    bool clicked( const QPoint &pos, const QRect &itemRect, const QModelIndex& index );

    QWidget * createEditor ( QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

private:
    void paintItem( LayoutItemConfig config, QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, bool ignoreQueueMarker = false ) const;
    void paintActiveTrackExtras( const QRect &rect, QPainter* painter, const QModelIndex& index ) const;

    QPointF centerImage( const QPixmap&, const QRectF& ) const;

    int rowsForItem( const QModelIndex &index ) const;

    static const qreal ALBUM_WIDTH;
    static const qreal SINGLE_TRACK_ALBUM_WIDTH;
    static const qreal MARGIN;
    static const qreal MARGINH;
    static const qreal MARGINBODY;
    static const qreal PADDING;
    static QFontMetricsF* s_nfm; //normal
    static QFontMetricsF* s_ifm; //italic
    static QFontMetricsF* s_bfm; //bold

    static int s_fontHeight;
};

}

#endif

