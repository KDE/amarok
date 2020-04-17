/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef SERVICECOLLECTIONTREEVIEW_H
#define SERVICECOLLECTIONTREEVIEW_H

#include "browsers/CollectionTreeView.h"

/**
    A simple extension of the CollectionTreeView class that allows optional 
    support for specifying that tracks are not playable and thus should not 
    be added to the playlist on activation and does not have the add and 
    load options in the context menu.

    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/

class ServiceCollectionTreeView : public CollectionTreeView
{
public:
    explicit ServiceCollectionTreeView( QWidget *parent = nullptr );
    ~ServiceCollectionTreeView() override;

    void setPlayableTracks( bool playable );
    bool playableTracks() const;

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    bool m_playableTracks;
};

#endif
