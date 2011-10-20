/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
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

#include <QModelIndex>
#include <QTreeView>

class PopupDropper;

class AmazonItemTreeView : public Amarok::PrettyTreeView
{
    Q_OBJECT

public:
    void startDrag( Qt::DropActions supportedActions );
    void mouseDoubleClickEvent( QMouseEvent *event );

protected slots:
    virtual void selectionChanged( const QItemSelection & selected, const QItemSelection & deselected );

private:
    PopupDropper* m_pd;

signals:
    void itemSelected( QModelIndex item );
};


#endif // AMAZONITEMTREEVIEW_H
