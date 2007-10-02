/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
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
#include "plasma/widgets/flowlayout.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>

// this class basically joins a QGraphicsItem and Plasma::VBoxLayout
// so we can manipulate it as a QGraphicsItem from ContextView. This allows
// us to use setPos etc to move it around.

typedef QPointer< Context::Applet > AppletPointer;

namespace Context
{

class AMAROK_EXPORT ColumnApplet : public Plasma::Applet
{
    Q_OBJECT
public:
    ColumnApplet( QGraphicsItem * parent = 0 );
    ~ColumnApplet() {}
    
    virtual QRectF boundingRect() const;
    
    AppletPointer addApplet( AppletPointer applet );
    
    void saveToConfig( KConfig& conf );
    void loadConfig( KConfig& conf );
    
    void update();
    
public slots:
    void appletRemoved( QObject* object );
    
protected:
//     virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
//     virtual void mouseMoveEvent( QGraphicsSceneMouseEvent * event );
//     
protected slots:
    void recalculate();
    
private:
    Plasma::FlowLayout* m_columns;
    int m_defaultColumnSize;
};

} // namespace

#endif
