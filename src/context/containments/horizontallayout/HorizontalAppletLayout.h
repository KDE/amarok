/***************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>          *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef AMAROK_HORIZONTAL_APPLET_LAYOUT_H
#define AMAROK_HORIZONTAL_APPLET_LAYOUT_H


#include "amarok_export.h"

#include <QGraphicsWidget>

class QGraphicsItem;
class QGraphicsSceneResizeEvent;
class QPainter;
class QStyleOptionGraphicsItem;

// this provides a simple toolbar to switch between applets in the CV

namespace Context
{
    
class AMAROK_EXPORT HorizontalAppletLayout : public QGraphicsWidget
{
    Q_OBJECT
    public:
        HorizontalAppletLayout( QGraphicsItem* parent = 0 );
        ~HorizontalAppletLayout();
        
        virtual void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
    protected:
        // reimplemented dfrom QGraphicsWidget
        virtual void resizeEvent( QGraphicsSceneResizeEvent * event );
        
        
};

}

#endif
