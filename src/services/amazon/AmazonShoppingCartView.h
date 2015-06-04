/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
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

#ifndef AMAZONSHOPPINGCARTVIEW_H
#define AMAZONSHOPPINGCARTVIEW_H

#include <QKeyEvent>
#include <QListView>

class AmazonShoppingCartView : public QListView
{
    Q_OBJECT
public:
    explicit AmazonShoppingCartView( QWidget *parent = 0 );

    /**
     * Reimplemented from QAbstractItemView.
     * Catches DEL key presses to remove items from the view.
     */
    virtual void keyPressEvent( QKeyEvent *event );

protected:
    /**
     * Reimplemented from QAbstractScrollArea.
     * Shows a context menu for items in the view.
     */
    virtual void contextMenuEvent( QContextMenuEvent *event );

protected Q_SLOTS:
    void removeFromCartAction();
};

#endif // AMAZONSHOPPINGCARTVIEW_H
