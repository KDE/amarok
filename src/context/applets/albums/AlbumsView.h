/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_ALBUMSVIEW_H
#define AMAROK_ALBUMSVIEW_H

#include "core/meta/forward_declarations.h"
#include "AlbumsModel.h"

#include <QGraphicsWidget>
#include <QStyledItemDelegate>

class QAbstractItemModel;
class QGraphicsSceneContextMenuEvent;
class QGraphicsProxyWidget;
class QStandardItem;
class QTreeView;
namespace Plasma
{
    class SvgWidget;
    class ScrollBar;
}

class AlbumsView : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY( AlbumsProxyModel::Mode mode READ mode WRITE setMode )
    Q_PROPERTY( Qt::Alignment lengthAlignment READ lengthAlignment WRITE setLengthAlignment )
    Q_PROPERTY( QString filterPattern READ filterPattern WRITE setFilterPattern )

public:
    explicit AlbumsView( QGraphicsWidget *parent = 0 );
    ~AlbumsView();

    void appendAlbum( QStandardItem *album );
    void scrollTo( QStandardItem *album );

    AlbumsProxyModel::Mode mode() const;
    void setMode( AlbumsProxyModel::Mode mode );

    Qt::Alignment lengthAlignment() const;
    void setLengthAlignment( Qt::Alignment alignment );

    QString filterPattern() const;
    void setFilterPattern( const QString &pattern );

    void clear();

public Q_SLOTS:
    void setRecursiveExpanded( QStandardItem *item, bool expanded );
    void sort();

protected:
    void contextMenuEvent( QGraphicsSceneContextMenuEvent *event );
    void resizeEvent( QGraphicsSceneResizeEvent *event );

private Q_SLOTS:
    void itemClicked( const QModelIndex &index );
    void slotDoubleClicked();
    void slotAppendSelected();
    void slotEditSelected();
    void slotReplaceWithSelected();
    void slotQueueSelected();
    void slotScrollBarRangeChanged( int min, int max );

private:
    void updateScrollBarVisibility();
    void setRecursiveExpanded( const QModelIndex &index, bool expanded );

    Meta::TrackList getSelectedTracks() const;
    AlbumsModel *m_model;
    AlbumsProxyModel *m_proxyModel;
    QTreeView *m_treeView;
    QGraphicsProxyWidget *m_treeProxy;
    Plasma::SvgWidget *m_topBorder;
    Plasma::SvgWidget *m_bottomBorder;
    Plasma::ScrollBar *m_scrollBar;
};

class AlbumsItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
    Q_PROPERTY( Qt::Alignment lengthAlignment READ lengthAlignment WRITE setLengthAlignment )

public:
    AlbumsItemDelegate( QObject *parent = 0 );
    ~AlbumsItemDelegate() {}

    Qt::Alignment lengthAlignment() const;
    void setLengthAlignment( Qt::Alignment alignment );

    void paint( QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index ) const;

private:
    void drawAlbumText( QPainter *p, const QStyleOptionViewItemV4 &option ) const;
    void drawTrackText( QPainter *p, const QStyleOptionViewItemV4 &option ) const;
    void applyCommonStyle( QPainter *p, const QStyleOptionViewItemV4 &option ) const;
    Qt::Alignment m_lengthAlignment;
};

#endif // multiple inclusion guard
