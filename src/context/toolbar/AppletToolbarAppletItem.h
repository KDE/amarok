/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_APPLET_TOOLBAR_APPLET_ITEM_H
#define AMAROK_APPLET_TOOLBAR_APPLET_ITEM_H

#include <QGraphicsWidget>
#include "AppletToolbarBase.h"

class QStyleOptionGraphicsItem;
class QPainter;
class QGraphicsSceneMouseEvent;

namespace Plasma
{
    class Applet;
    class IconWidget;
    class Label;
}

namespace Context
{
    
class AppletToolbarAppletItem : public AppletToolbarBase
{
    Q_OBJECT

    public:
        explicit AppletToolbarAppletItem( QGraphicsItem* parent = 0, Plasma::Applet* applet = 0 );
        ~AppletToolbarAppletItem();
        
        void setConfigEnabled( bool config );
        bool configEnabled();

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

        virtual void hoverEnterEvent( QGraphicsSceneHoverEvent * event );
        virtual void hoverLeaveEvent( QGraphicsSceneHoverEvent * event );
    
        void mousePressEvent( QGraphicsSceneMouseEvent * event );

    private slots:
        void deleteApplet();
        void animateHoverIn( qreal progress );
        void animateHoverOut( qreal progress );

    private:
        Plasma::IconWidget* addAction( QAction *action, int size );
        
        Plasma::Applet* m_applet;
        Plasma::Label * m_label;
        
        
        Plasma::IconWidget* m_deleteIcon;
        
        int m_labelPadding;
        bool m_configEnabled;
};

} // namespace

#endif
