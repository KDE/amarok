/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef FILETREEVIEW_H
#define FILETREEVIEW_H

#include "widgets/PrettyTreeView.h"

#include <QAction>
#include <QList>

class FileTreeView : public Amarok::PrettyTreeView
{
    Q_OBJECT
public:
    FileTreeView( QWidget * parent );


protected slots:

    void slotAppendToPlaylist();
    void slotReplacePlaylist();
    
protected:

    QList<QAction *> actionsForIndices( const QModelIndexList &indices );
    void addSelectionToPlaylist( bool replace );
    
    virtual void contextMenuEvent ( QContextMenuEvent * e );

private:
    QAction * m_appendAction;
    QAction * m_loadAction;
};

#endif // FILETREEVIEW_H
