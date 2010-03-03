/****************************************************************************************
 * Copyright (c) 2009 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 *                       Significant parts of this code is inspired                     *
 *                       and/or copied from KDE Plasma sources, available               *
 *                       at kdebase/workspace/libs/plasmagenericshell                   *
 *                                                                                      *
 ****************************************************************************************/

/****************************************************************************************
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

#ifndef APPLET_ICON_H
#define APPLET_ICON_H

#include "amarok_export.h"
#include "AppletItemModel.h"

#include <plasma/widgets/iconwidget.h>

#include <QPainter>

class QGraphicsSceneMouseEvent;

namespace Context
{

class AMAROK_EXPORT AppletIconWidget: public Plasma::IconWidget
{
    Q_OBJECT

public:
    explicit AppletIconWidget( AppletItem *appletItem = 0, QGraphicsItem *parent = 0 );
    virtual ~AppletIconWidget();

    AppletItem *appletItem() const;

protected:
    void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );

private:
    AppletItem *m_appletItem;
};

}

#endif
