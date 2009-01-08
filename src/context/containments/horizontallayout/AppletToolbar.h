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

namespace Context
{
    
class AppletToolbarAddItem;
    
class AMAROK_EXPORT AppletToolbar : public QGraphicsWidget
{
    Q_OBJECT
    public:
        AppletToolbar( QGraphicsItem* parent = 0 );
        ~AppletToolbar();
        
        virtual void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
        
        QSizePolicy sizePolicy () const;
    protected:
        // reimplemented dfrom QGraphicsWidget
        virtual void resizeEvent( QGraphicsSceneResizeEvent * event );
        virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;
    
    private:
        qreal m_width;    
        QGraphicsLinearLayout* m_appletLayout;
        
        AppletToolbarAddItem* m_test1;
        AppletToolbarAddItem* m_test2;
};

}

#endif
