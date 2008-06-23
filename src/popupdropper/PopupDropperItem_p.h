/***************************************************************************
 *   Copyright (c) 2008  Jeff Mitchell <mitchell@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef POPUPDROPPER_ITEM_P_H
#define POPUPDROPPER_ITEM_P_H

#include <QtDebug>
#include <QFont>

class PopupDropperItemPrivate
{
public:
    PopupDropperItemPrivate( PopupDropperItem* parent )
        : action( 0 )
        , text( QString() )
        , hoverTimer( parent )
        , hoverMsecs( 500 )
        , elementId( QString() )
        , textItem( 0 )
        , font()
        , submenuTrigger( false )
        , q( parent )
        {
            hoverTimer.setSingleShot( true );
            q->setAcceptDrops( true );
        }

    ~PopupDropperItemPrivate() {}

    QAction* action;
    QString text;
    QTimer hoverTimer;
    int hoverMsecs;
    QString elementId;
    QGraphicsTextItem* textItem;
    QFont font;
    bool submenuTrigger;

private:
    PopupDropperItem* q;
};

#endif

