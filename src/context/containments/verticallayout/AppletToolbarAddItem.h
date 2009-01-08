/***************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org  >        *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef AMAROK_APPLET_TOOLBAR_ADD_ITEM_H
#define AMAROK_APPLET_TOOLBAR_ADD_ITEM_H


#include "amarok_export.h"

#include "plasma/widgets/iconwidget.h"

#include <QGraphicsWidget>

class QGraphicsItem;
class QGraphicsSceneResizeEvent;
class QGraphicsSimpleTextItem;
class QPainter;
class QStyleOptionGraphicsItem;

// this provides a simple toolbar to switch between applets in the CV

namespace Context
{
    
class AmarokToolBoxMenu;
class Containment;

class AppletToolbarAddItem : public QGraphicsWidget
{
    Q_OBJECT
    public:
        AppletToolbarAddItem( QGraphicsItem* parent = 0, Containment* cont = 0 );
        ~AppletToolbarAddItem();
        
        virtual void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
        
        QSizePolicy sizePolicy () const;
        
    public slots:
        void updatedContainment( Containment* cont );
    
    protected:    
         virtual void resizeEvent( QGraphicsSceneResizeEvent * event );
         virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;
    
         void mousePressEvent( QGraphicsSceneMouseEvent * event );
    private: 
        void showAddAppletsMenu( QPointF pos = QPointF() );
        
        Plasma::IconWidget* m_icon;
        QGraphicsSimpleTextItem* m_label;
        AmarokToolBoxMenu* m_addMenu;
};

}

#endif
