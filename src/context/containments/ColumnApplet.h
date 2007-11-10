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

#include "context/Applet.h"
#include "Containment.h"
#include "amarok_export.h"
#include "Svg.h"

#include "plasma/widgets/flowlayout.h"
#include "plasma/appletbrowser.h"

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
    
    void saveToConfig( KConfig& conf );
    void loadConfig( KConfig& conf );
    
    void updateSize();
    
    QSizeF sizeHint() const;
    
    virtual void paintInterface(QPainter *painter,
                                const QStyleOptionGraphicsItem *option,
                                const QRect& contentsRect);
    
    QList<QAction*> contextActions();
    
public slots:
    void appletRemoved( QObject* object );
    Plasma::Applet* addApplet( Plasma::Applet* applet );
    
    
protected:
//     virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event );
//     virtual void mouseMoveEvent( QGraphicsSceneMouseEvent * event );
//     
protected slots:
    void launchAppletBrowser();
    
    void recalculate();
    
private:
    QAction* m_appletBrowserAction;
    Plasma::AppletBrowser* m_appletBrowser;
    
    QRectF m_geometry;
    
    Plasma::FlowLayout* m_columns;
    int m_defaultColumnSize;
    Plasma::Svg* m_background;
    Plasma::Svg* m_logo;
    qreal m_width;
    qreal m_aspectRatio;
};

K_EXPORT_AMAROK_APPLET( context, ColumnApplet )
    
} // namespace

#endif
