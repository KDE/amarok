/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef INLINEEDITORWIDGET_H
#define INLINEEDITORWIDGET_H

#include "playlist/layouts/LayoutItemConfig.h"

#include <KHBox>

#include <QModelIndex>

/**
An inline editor for a playlist item. Relies on the same item layout configuration as is used by the delegate, and strives to have a simmilar look.

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class InlineEditorWidget : public KHBox
{
    Q_OBJECT
public:
    InlineEditorWidget( QWidget * parent, const QModelIndex &index, Playlist::PlaylistLayout layout, int groupMode );

    ~InlineEditorWidget();

    QMap<int, QString> changedValues();

protected:
    void paintEvent( QPaintEvent * event );

protected slots:
    void editValueChanged();
    void ratingValueChanged();
        
private:

    void createChildWidgets();
    QPoint centerImage( const QPixmap&, const QRectF& ) const;

    static const qreal ALBUM_WIDTH;
    static const qreal SINGLE_TRACK_ALBUM_WIDTH;
    static const qreal MARGIN;
    static const qreal MARGINH;
    static const qreal MARGINBODY;
    static const qreal PADDING;

    QModelIndex m_index;
    Playlist::PlaylistLayout m_layout;
    int m_groupMode;

    QMap<QWidget *, int> m_editorRoleMap;
    QMap<int, QString> m_changedValues;

};

#endif
