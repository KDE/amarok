/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef INLINEEDITORWIDGET_H
#define INLINEEDITORWIDGET_H

#include "playlist/layouts/LayoutItemConfig.h"

#include <KVBox>

#include <QModelIndex>
#include <QSplitter>

/**
    An inline editor for a playlist item. Relies on the same item layout configuration as is used by the delegate, and strives to have a simmilar look.
*/
class InlineEditorWidget : public KVBox
{
    Q_OBJECT

public:
    InlineEditorWidget( QWidget * parent, const QModelIndex &index, Playlist::PlaylistLayout layout, bool hasHeader );
    ~InlineEditorWidget();

    QMap<int, QString> changedValues();

signals:

    void editingDone( InlineEditorWidget * editor );

protected:
    void paintEvent( QPaintEvent * event );

protected slots:
    void editValueChanged();
    void ratingValueChanged();
    void splitterMoved( int pos, int index );

private:
    void createChildWidgets();
    QPoint centerImage( const QPixmap&, const QRectF& ) const;
    bool eventFilter( QObject *obj, QEvent *event );

    QPersistentModelIndex m_index;
    Playlist::PlaylistLayout m_layout;

    QMap<QWidget *, int> m_editorRoleMap;
    QMap<int, QString> m_changedValues;
    QMap<int, QString> m_orgValues;

    int m_headerHeight;

    QMap<QSplitter *, int> m_splitterRowMap;
    bool m_layoutChanged;

};

namespace Playlist
{
    extern const qreal ALBUM_WIDTH;
    extern const qreal SINGLE_TRACK_ALBUM_WIDTH;
    extern const qreal MARGIN;
    extern const qreal MARGINH;
    extern const qreal MARGINBODY;
    extern const qreal PADDING;
}

#endif
