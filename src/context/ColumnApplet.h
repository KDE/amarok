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
#include "plasma/widgets/vboxlayout.h"

// this class basically wraps a Plasma::VBoxLayout into a Plasma::Applet
// so we can manipulate it as a QGraphicsItem from ContextView. This allows
// us to use setPos etc to move it around.

namespace Context
{

class AMAROK_EXPORT ColumnApplet : public Applet, 
                     public Plasma::VBoxLayout
{
public:
    ColumnApplet( LayoutItem* parent = 0 ) : Applet(), Plasma::VBoxLayout( parent ) {}
    
    void setGeometry( const QRectF& rect ) { Plasma::VBoxLayout::setGeometry( rect ); }
    QSizeF sizeHint() const { return Plasma::VBoxLayout::sizeHint(); }
    QRectF geometry() const{ return Plasma::VBoxLayout::geometry(); }
};

}

#endif
