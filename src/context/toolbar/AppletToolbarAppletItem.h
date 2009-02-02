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

#ifndef AMAROK_APPLET_TOOLBAR_APPLET_ITEM_H
#define AMAROK_APPLET_TOOLBAR_APPLET_ITEM_H

#include <QGraphicsWidget>

class QStyleOptionGraphicsItem;
class QPainter;
class QGraphicsSceneMouseEvent;

namespace Plasma
{
    class Applet;
    class IconWidget;
}

namespace Context
{
    
class AppletToolbarAppletItem : public QGraphicsWidget
{
    Q_OBJECT
    public:
        explicit AppletToolbarAppletItem( QGraphicsItem* parent = 0, Plasma::Applet* applet = 0 );
        ~AppletToolbarAppletItem();
        
        virtual void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

        void setConfigEnabled( bool config );
        bool configEnabled();

        QSizePolicy sizePolicy () const;
        Plasma::Applet* applet() { return m_applet; }
        // needed for the overlay to check if the click is over the del icon
        QRectF delIconSceneRect();
    signals:
        void appletChosen( Plasma::Applet* );
        void geometryChanged();
        
    protected:
        virtual void resizeEvent( QGraphicsSceneResizeEvent * event );
        virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;
        /**
         * Reimplemented from QGraphicsItem
         */
         virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    
        void mousePressEvent( QGraphicsSceneMouseEvent * event );

    private slots:
        void deleteApplet();
    
    private:
        Plasma::IconWidget* addAction( QAction *action, int size );
        
        Plasma::Applet* m_applet;
        QGraphicsSimpleTextItem* m_label;
        
        
        Plasma::IconWidget* m_deleteIcon;
        
        int m_labelPadding;
        bool m_configEnabled;
};

} // namespace

#endif
