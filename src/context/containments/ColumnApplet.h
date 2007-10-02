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

#include "Containment.h"
#include "amarok_export.h"
#include "Svg.h"

#include "plasma/widgets/flowlayout.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>

namespace Context
{

class AMAROK_EXPORT ColumnApplet : public Containment
{
    Q_OBJECT
public:
    ColumnApplet( QObject *parent, const QVariantList &args );
    ~ColumnApplet() {}
    
    virtual QRectF boundingRect() const;
    
    Applet* addApplet( Applet* applet );
    
    void saveToConfig( KConfig& conf );
    void loadConfig( KConfig& conf );
    
    void update();
    
    virtual void paintInterface(QPainter *painter,
                                const QStyleOptionGraphicsItem *option,
                                const QRect& contentsRect);
    
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
    Plasma::Svg* m_background;
    Plasma::Svg* m_logo;
    qreal m_width;
    qreal m_aspectRatio;
};

} // namespace

#endif
