/***************************************************************************
* copyright            : (C) 2009 Leo Franchi <lfranchi@kde.org>         *
****************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef APPLETTOOLBARBASE_H
#define APPLETTOOLBARBASE_H

#include <QGraphicsWidget>

namespace Context
{

class AppletToolbarBase : public QGraphicsWidget
{
public:
    AppletToolbarBase(QGraphicsItem* parent = 0, Qt::WindowFlags wFlags = 0);
    ~AppletToolbarBase();
    
    virtual void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

};

} // namespace

#endif // APPLETTOOLBARBASE_H
