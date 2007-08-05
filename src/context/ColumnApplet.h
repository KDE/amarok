/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
*                                                                         *
*                        Significant parts of this code is inspired       *
*                        and/or copied from KDE Plasma sources, available *
*                        at kdebase/workspace/plasma                      *
*
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef COLUMN_APPLET_H
#define COLUMN_APPLET_H

#include "Applet.h"
#include "amarok_export.h"
#include "plasma/widgets/vboxlayout.h"
#include "plasma/widgets/hboxlayout.h"

// this class basically joins a QGraphicsItem and Plasma::VBoxLayout
// so we can manipulate it as a QGraphicsItem from ContextView. This allows
// us to use setPos etc to move it around.

typedef QPointer< Context::Applet > AppletPointer;

namespace Context
{

class AMAROK_EXPORT ColumnApplet : public QGraphicsItem, public Plasma::HBoxLayout
{
public:
    ColumnApplet( QGraphicsItem * parent = 0 );
//     ~ColumnApplet();
    
    QRectF boundingRect() const;
    void paint( QPainter*, const QStyleOptionGraphicsItem*, QWidget* ) {}
    
    AppletPointer addApplet( AppletPointer applet );
    
    void init();
    void update();
    
private:
    void resizeColumns();
    void balanceColumns();
    
    QList< Plasma::VBoxLayout* > m_layout;
    QRectF m_geometry;
    
    int m_padding;
    int m_defaultColumnSize;
};

} // namespace

#endif
