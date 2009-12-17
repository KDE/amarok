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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef PRETTYITEMDELEGATE_H
#define PRETTYITEMDELEGATE_H

#include "playlist/layouts/LayoutItemConfig.h"

#include <QModelIndex>
#include <QStyledItemDelegate>

class InlineEditorWidget;
class QPainter;

namespace Playlist
{
class PrettyItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
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

    void setModelData ( QWidget * editor, QAbstractItemModel * model, const QModelIndex & index ) const;
    void updateEditorGeometry ( QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index ) const;


    /**
    * Paint a playlist item based on a LayoutItemConfig
    *
    * @param config The LayoutItemConfig that defines how the item should look and which info it should include.
    * @param painter The QPainter used to paint the item.
    * @param option Additional state options used to paint the item..
    * @param index The model index of the track in the playlist that we are painting.
    * @param ignoreMarkers A boolean value specifying wheter we should ignore any "markers" when painting this item.
    *                      Markers can be such things as the "now playing" background, queue markers, multi track markers and the likes.
    *                      The main reason for wanting to ignore these is that when painting the head part of the first track in the group, these
    *                      things should not be shown as they will be hown in the track part of the item.
    */
    void paintItem( LayoutItemConfig config, QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, bool ignoreMarkers = false ) const;


protected slots:

    void editorDone( InlineEditorWidget * editor );

private:
    void paintActiveTrackExtras( const QRect &rect, QPainter* painter, const QModelIndex& index ) const;

    QPointF centerImage( const QPixmap&, const QRectF& ) const;

    int rowsForItem( const QModelIndex &index ) const;

    int getGroupMode( const QModelIndex &index) const;

    static QFontMetricsF* s_nfm; //normal
    static QFontMetricsF* s_ufm; //underline
    static QFontMetricsF* s_ifm; //italic
    static QFontMetricsF* s_bfm; //bold

    static int s_fontHeight;
};

}

#endif

