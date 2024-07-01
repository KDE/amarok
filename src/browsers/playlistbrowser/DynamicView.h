/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef DYNAMICVIEW_H
#define DYNAMICVIEW_H

#include "widgets/PrettyTreeView.h"

class QKeyEvent;
class QMouseEvent;
class QContextMenuEvent;

namespace PlaylistBrowserNS {

class DynamicView : public Amarok::PrettyTreeView
{
Q_OBJECT
public:
    explicit DynamicView( QWidget *parent = nullptr );
    ~DynamicView() override;

Q_SIGNALS:
    void currentItemChanged( const QModelIndex &current );

public Q_SLOTS:
    void addPlaylist();
    void addToSelected();
    void cloneSelected();
    void editSelected();
    void removeSelected();

protected Q_SLOTS:
    void expandRecursive(const QModelIndex &index);
    void collapseRecursive(const QModelIndex &index);

protected:
    void keyPressEvent( QKeyEvent *event ) override;
    void mouseDoubleClickEvent( QMouseEvent *event ) override;

    void contextMenuEvent( QContextMenuEvent* event ) override;
};

} // namespace PlaylistBrowserNS

#endif // DYNAMICVIEW_H
