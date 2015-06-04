/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
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

#ifndef AMAZONITEMTREEVIEW_H
#define AMAZONITEMTREEVIEW_H

#include "widgets/PrettyTreeView.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QModelIndex>
#include <QTreeView>

class PopupDropper;

class AmazonItemTreeView : public Amarok::PrettyTreeView
{
    Q_OBJECT

public:
    AmazonItemTreeView( QWidget *parent = 0 );

    // Reimplemented from QTreeView
    virtual void setModel( QAbstractItemModel *model );

protected:
    // Reimplemented from QAbstractScrollArea
    virtual void contextMenuEvent( QContextMenuEvent *event );

    // Reimplemented from QAbstractItemView
    virtual void startDrag( Qt::DropActions supportedActions );

    // Reimplemented from QTreeView
    virtual void mouseDoubleClickEvent( QMouseEvent *event );
    virtual void mouseReleaseEvent( QMouseEvent *event );

protected Q_SLOTS:
    // Reimplemented from QTreeView
    virtual void dataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight );
    virtual void selectionChanged( const QItemSelection &selected, const QItemSelection &deselected );

    void itemActivatedAction();
    void searchForAlbumAction();

private:
    PopupDropper *m_pd;

    QAction* createAddToCartAction();
    QAction* createAddToPlaylistAction();
    QAction* createDetailsAction();
    QAction* createDirectCheckoutAction();
    QAction* createSearchForAlbumAction();

Q_SIGNALS:
    void addToCart();
    void directCheckout();
    void itemDoubleClicked( QModelIndex index );
    void itemSelected( QModelIndex index );
    void searchForAlbum( QModelIndex index );
};

#endif // AMAZONITEMTREEVIEW_H
