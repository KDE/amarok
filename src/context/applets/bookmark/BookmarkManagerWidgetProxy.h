/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef AMAROK_BOOKMARK_MANAGER_WIDGET_PROXY_H
#define AMAROK_BOOKMARK_MANAGER_WIDGET_PROXY_H

#include <QGraphicsProxyWidget>

class KMenu;
class BookmarkManagerWidget;
class QGraphicsSceneContextMenuEvent;

class BookmarkManagerWidgetProxy : public QGraphicsProxyWidget
{
    Q_OBJECT

public:
    explicit BookmarkManagerWidgetProxy( QGraphicsWidget *parent = 0 );
    ~BookmarkManagerWidgetProxy() { }
    
protected slots:
    void showMenu( KMenu* menu, const QPointF& localPos );
    
private:
    BookmarkManagerWidget* m_manager;
};

#endif
