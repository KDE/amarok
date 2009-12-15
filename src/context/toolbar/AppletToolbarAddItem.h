/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef AMAROK_APPLET_TOOLBAR_ADD_ITEM_H
#define AMAROK_APPLET_TOOLBAR_ADD_ITEM_H


#include "amarok_export.h"

#include <plasma/widgets/iconwidget.h>

#include <QGraphicsWidget>
#include "AppletToolbarBase.h"

class QGraphicsItem;
class QGraphicsSceneResizeEvent;
class QGraphicsSimpleTextItem;
class QPainter;
class QStyleOptionGraphicsItem;

namespace Context
{

class Containment;
class WidgetExplorer;

class AppletToolbarAddItem : public AppletToolbarBase
{
    Q_OBJECT
    public:
        explicit AppletToolbarAddItem( QGraphicsItem* parent = 0, Containment* cont = 0, bool fixedAdd = false );
        ~AppletToolbarAddItem();
        
    signals:
        void addApplet( const QString&, AppletToolbarAddItem*  );
        void installApplets();
        void hideAppletExplorer();
        void showAppletExplorer();
        
    public slots:
        void appletExplorerHid();
        void iconClicked();

    protected:    
         virtual void resizeEvent( QGraphicsSceneResizeEvent * event );
         virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;
    
         void mousePressEvent( QGraphicsSceneMouseEvent * event );
    private: 
        int m_iconPadding;
        bool m_fixedAdd;
        bool m_showingAppletExplorer;

        Containment* m_cont;
        Plasma::IconWidget* m_icon;
        QGraphicsSimpleTextItem* m_label;
};

}

#endif
