/***************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>         *
****************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef AMAROK_APPLET_TOOLBAR_H
#define AMAROK_APPLET_TOOLBAR_H

#include "amarok_export.h"

#include <QGraphicsWidget>

class QGraphicsItem;
class QGraphicsSceneResizeEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QSizePolicy;
class QGraphicsLinearLayout;

// this provides a simple toolbar to switch between applets in the CV
namespace Plasma
{
    class Applet;
}

namespace Context
{
    
class AppletToolbarAddItem;
class Containment;

class AMAROK_EXPORT AppletToolbar : public QGraphicsWidget
{
    Q_OBJECT
    public:
        AppletToolbar( QGraphicsItem* parent = 0 );
        ~AppletToolbar();
        
        virtual void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
        
        QSizePolicy sizePolicy () const;  
        
        void appletRemoved( Plasma::Applet* applet );
    signals:
        void showApplet( Plasma::Applet* );
    
    
    protected:
        // reimplemented dfrom QGraphicsWidget
        virtual void resizeEvent( QGraphicsSceneResizeEvent * event );
        virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;

        void mousePressEvent( QGraphicsSceneMouseEvent *event );
        
    private slots:
        void appletRemoved( Plasma::Applet*, int );
        void appletAdded( Plasma::Applet*, int );
        
    private:
        qreal m_width;    
        QGraphicsLinearLayout* m_appletLayout;
                
        Containment* m_cont;
        AppletToolbarAddItem* m_addItem;
};

}

#endif
