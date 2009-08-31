/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef COLLECTIONBROWSERTREEVIEW_H
#define COLLECTIONBROWSERTREEVIEW_H

#include "CollectionTreeView.h"

/**
 * Specialized CollectionTreeView that handles actions to top level items ( collections ) in a custom way.
 */
class CollectionBrowserTreeView : public CollectionTreeView 
{
    public:
        CollectionBrowserTreeView( QWidget *parent = 0 );
        ~CollectionBrowserTreeView();

    protected:
        virtual void mouseMoveEvent( QMouseEvent *event );
};

#endif
