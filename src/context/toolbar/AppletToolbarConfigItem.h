/**************************************************************************
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

#ifndef AMAROK_APPLET_TOOLBAR_CONFIG_ITEM_H
#define AMAROK_APPLET_TOOLBAR_CONFIG_ITEM_H

#include <QGraphicsWidget>


class QPainter;
class QSizePolicy;
class QStyleOptionGraphicsItem;

namespace Plasma
{
    class IconWidget;
}

namespace Context
{

class AppletToolbarConfigItem : public QGraphicsWidget
{
    Q_OBJECT
    public:
        AppletToolbarConfigItem( QGraphicsItem* parent = 0 );
        ~AppletToolbarConfigItem();
        
        virtual void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
                
    signals:
        void triggered();
        
    protected:    
         virtual void resizeEvent( QGraphicsSceneResizeEvent * event );
         virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;
    
         void mousePressEvent( QGraphicsSceneMouseEvent * event );
    private:         
        int m_iconPadding;
        
        Plasma::IconWidget* m_icon;
};

}

#endif
