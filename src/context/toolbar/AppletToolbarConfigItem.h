/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef AMAROK_APPLET_TOOLBAR_CONFIG_ITEM_H
#define AMAROK_APPLET_TOOLBAR_CONFIG_ITEM_H

#include <QGraphicsWidget>
#include "AppletToolbarBase.h"


class QPainter;
class QSizePolicy;
class QStyleOptionGraphicsItem;

namespace Plasma
{
    class IconWidget;
}

namespace Context
{

class AppletToolbarConfigItem : public AppletToolbarBase
{
    Q_OBJECT
    public:
        AppletToolbarConfigItem( QGraphicsItem* parent = 0 );
        ~AppletToolbarConfigItem();
                        
    signals:
        void triggered();
        
    protected:    
         virtual void resizeEvent( QGraphicsSceneResizeEvent * event );
         virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;
    
         void mousePressEvent( QGraphicsSceneMouseEvent * event );
    private:         
        int m_iconPadding;
        
        Plasma::IconWidget* m_icon;
};

}

#endif
