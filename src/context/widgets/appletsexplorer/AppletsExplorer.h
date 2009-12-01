/****************************************************************************************
 * Copyright (c) 2009 William Viana Soares <vianasw@gmail.com>                          *
 *                       Significant parts of this code is inspired                     *
 *                       and/or copied from KDE Plasma sources, available               *
 *                       at kdebase/workspace/libs/plasmagenericshell                   *
 *                                                                                      *
 ****************************************************************************************/

/****************************************************************************************
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


#ifndef WIDGET_EXPLORER_H
#define WIDGET_EXPLORER_H

#include "amarok_export.h"
#include "AppletItemModel.h"
#include "AppletsList.h"
#include "Containment.h"

#include <QGraphicsLinearLayout>
#include <QGraphicsWidget>
#include <QPainter>

class QAction;
class QStyleOptionGraphicsItem;
class QSizePolicy;
 
namespace Context
{

class AMAROK_EXPORT AppletsExplorer: public QGraphicsWidget
{

    Q_OBJECT

    public:

        AppletsExplorer( QGraphicsItem *parent = 0 );
        ~AppletsExplorer();

        void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );

        void setContainment( Containment *containment );
    
        QSizePolicy sizePolicy () const;  

        Containment *containment() const;

    signals:
        void addAppletToContainment( const QString &pluginName );
        
    private slots:
        void addApplet( AppletItem *appletItem );
        void hideMenu();
        
    protected:

        virtual void resizeEvent( QGraphicsSceneResizeEvent *event );


    private:

        void init();
        
        Containment *m_containment;
        QGraphicsLinearLayout *m_mainLayout;

        AppletItemModel m_model;
        AppletsListWidget *m_appletsListWidget;
        Plasma::IconWidget *m_hideIcon;
        
};

}// namespace Context

#endif 
