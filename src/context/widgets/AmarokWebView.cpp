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


#include "AmarokWebView.h"
#include "core/support/Debug.h"

AmarokWebView::AmarokWebView( QGraphicsItem *parent )
    : Plasma::WebView( parent )
{
}

void AmarokWebView::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    if( event->button() == Qt::RightButton )
    {
        event->accept();
    }
    else
       Plasma::WebView::mousePressEvent( event );
}

void AmarokWebView::contextMenuEvent( QGraphicsSceneContextMenuEvent *event )
{
    event->accept();
}
