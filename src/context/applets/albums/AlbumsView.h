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

#include "core/meta/Meta.h"

#include <QGraphicsWidget>
#include <QStyledItemDelegate>

class AlbumsModel;
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

public:
    explicit AlbumsView( QGraphicsWidget *parent = 0 );
    ~AlbumsView();

    void appendAlbum( QStandardItem *album );
    void scrollTo( QStandardItem *album );

    void clear();

public slots:
    void setRecursiveExpanded( QStandardItem *item, bool expanded );

protected:
    void contextMenuEvent( QGraphicsSceneContextMenuEvent *event );
    void resizeEvent( QGraphicsSceneResizeEvent *event );
    
private slots:
    void itemClicked( const QModelIndex &index );
    void slotAppendSelected();
    void slotEditSelected();
    void slotPlaySelected();
    void slotQueueSelected();
    void slotScrollBarRangeChanged( int min, int max );

private:
    void updateScrollBarVisibility();
    void setRecursiveExpanded( const QModelIndex &index, bool expanded );

    Meta::TrackList getSelectedTracks() const;
    AlbumsModel *m_model;
    QTreeView *m_treeView;
    QGraphicsProxyWidget *m_treeProxy;
    Plasma::SvgWidget *m_topBorder;
    Plasma::SvgWidget *m_bottomBorder;
    Plasma::ScrollBar *m_scrollBar;
};

class AlbumsItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    AlbumsItemDelegate( QObject *parent = 0 ) : QStyledItemDelegate( parent ) {}
    ~AlbumsItemDelegate() {}

    void paint( QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index ) const;

private:
    void drawAlbumText( QPainter *p, const QStyleOptionViewItemV4 &option ) const;
    void drawTrackText( QPainter *p, const QStyleOptionViewItemV4 &option ) const;
    void applyCommonStyle( QPainter *p, const QStyleOptionViewItemV4 &option ) const;
};

#endif // multiple inclusion guard
