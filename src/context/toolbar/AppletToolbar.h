/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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
class AppletToolbarConfigItem;
class Containment;

class AppletToolbar : public QGraphicsWidget
{
    Q_OBJECT
    public:
        AppletToolbar( QGraphicsItem* parent = 0 );
        ~AppletToolbar();
        
        QSizePolicy sizePolicy () const;  
        QGraphicsLinearLayout* appletLayout() const;
        bool configEnabled() const;
        
        void appletRemoved( Plasma::Applet* applet );

        void setContainment( Containment * containment );
        Containment* containment() const;

    Q_SIGNALS:
        void showApplet( Plasma::Applet* );
        void appletAddedToToolbar( Plasma::Applet* applet, int loc );
        void moveApplet( Plasma::Applet*, int, int );
        void configModeToggled();
        void hideAppletExplorer();
        void showAppletExplorer();

    protected:
        virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;
        virtual void resizeEvent( QGraphicsSceneResizeEvent * event );
        virtual void mousePressEvent( QGraphicsSceneMouseEvent *event );

    private Q_SLOTS:
        void appletAdded( Plasma::Applet*, int );
        void toggleConfigMode();
        
    private:
        void newAddItem( int loc );
        
        qreal m_width;    

        bool m_configMode;
        
        QGraphicsLinearLayout* m_appletLayout;
                
        Containment* m_cont;
        AppletToolbarConfigItem* m_configItem;
};

}

#endif
