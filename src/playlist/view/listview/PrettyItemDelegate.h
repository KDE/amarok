/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2013 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "core/meta/forward_declarations.h"
#include "playlist/layouts/LayoutItemConfig.h"

#include <QModelIndex>
#include <QStyledItemDelegate>

class InlineEditorWidget;
class QPainter;
class QTimeLine;

namespace Playlist
{
class PrettyItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    static int rowsForItem( const QModelIndex &index );

    explicit PrettyItemDelegate( QObject* parent = nullptr );
    ~PrettyItemDelegate() override;

    QSize sizeHint( const QStyleOptionViewItem&, const QModelIndex& ) const override;
    void paint( QPainter*, const QStyleOptionViewItem&, const QModelIndex& ) const override;

    // helper function for view which lets us determine if a click is within an album group's header
    bool insideItemHeader( const QPoint&, const QRect& );

    /** Returns the height an item header would have */
    int headerHeight() const;

   /**
    * Handle clicks within a delegate.
    * @param pos The position of the click, in delegate coordinates.
    * @param itemRect QRect for the item.
    * @param index The index of the clicked item.
    * @return True if delegate acts on this click, false otherwise.
    */
    bool clicked( const QPoint &pos, const QRect &itemRect, const QModelIndex& index );

    QWidget * createEditor ( QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index ) const override;

    void setModelData ( QWidget * editor, QAbstractItemModel * model, const QModelIndex & index ) const override;
    void updateEditorGeometry ( QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index ) const override;


    /**
    * Paint a playlist item based on a LayoutItemConfig
    *
    * @param config The LayoutItemConfig that defines how the item should look and which info it should include.
    * @param painter The QPainter used to paint the item.
    * @param option Additional state options used to paint the item..
    * @param index The model index of the track in the playlist that we are painting.
    * @param headerRow A boolean value specifying whether we should ignore any "markers" when painting this item.
    *                      Markers can be such things as the "now playing" background, queue markers, multi track markers and the likes.
    *                      The main reason for wanting to ignore these is that when painting the head part of the first track in the group, these
    *                      things should not be shown as they will be hown in the track part of the item.
    */
    void paintItem( const LayoutItemConfig &config,
                    QPainter* painter,
                    const QStyleOptionViewItem& option,
                    const QModelIndex& index,
                    bool headerRow = false ) const;


protected Q_SLOTS:
    void editorDone( InlineEditorWidget * editor );

Q_SIGNALS:
    void redrawRequested();

private Q_SLOTS:
    void drawNeedChanged( const bool );

private:
    void paintActiveTrackExtras( const QRect &rect, QPainter* painter, const QModelIndex& index ) const;

    QTimeLine * m_animationTimeLine;
    QPointF centerImage( const QPixmap&, const QRectF& ) const;

    static int getGroupMode( const QModelIndex &index);

    QMap<QString, QString> buildTrackArgsMap( const Meta::TrackPtr &track ) const;

    static QFontMetricsF* s_nfm; //normal
    static QFontMetricsF* s_ufm; //underline
    static QFontMetricsF* s_ifm; //italic
    static QFontMetricsF* s_bfm; //bold

    static int s_fontHeight;
};

}

#endif

