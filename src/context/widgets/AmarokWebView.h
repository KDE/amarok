/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef AMAROKWEBVIEW_H
#define AMAROKWEBVIEW_H

#include "amarok_export.h"

#include <plasma/widgets/webview.h>

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>

/**
 * A simple Plasma::WebView sublcass that ignores right click events as we do not support any of the normal web context actions.
 */

class AMAROK_EXPORT AmarokWebView : public Plasma::WebView
{
public:
    AmarokWebView(  QGraphicsItem *parent = 0 );

protected:
    void mousePressEvent( QGraphicsSceneMouseEvent *event );
    void contextMenuEvent( QGraphicsSceneContextMenuEvent *event );
        
    
};

#endif // AMAROKWEBVIEW_H
