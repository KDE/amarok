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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_VERTICAL_APPLET_LAYOUT_H
#define AMAROK_VERTICAL_APPLET_LAYOUT_H


#include "amarok_export.h"

#include <QGraphicsWidget>

class KConfigGroup;

class QGraphicsItem;
class QGraphicsSceneResizeEvent;
class QPainter;
class QStyleOptionGraphicsItem;

namespace Plasma
{
    class Applet;
}

namespace Context
{
    
class Containment;
    
class VerticalAppletLayout : public QGraphicsWidget
{
    Q_OBJECT
    public:
        VerticalAppletLayout( QGraphicsItem* parent = 0 );
        ~VerticalAppletLayout();
        
        virtual void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
        
        void addApplet( Plasma::Applet*, int location = -1 );
        
        virtual void saveToConfig( KConfigGroup &conf );
        

        QSizeF totalSize();

        void showAtIndex( int index );
        
        
    signals:
        void appletAdded( Plasma::Applet* applet, int location );
        void noApplets( bool );
        
    public slots:
        void refresh();
        void showApplet( Plasma::Applet* );
        void moveApplet( Plasma::Applet*, int, int);
        void appletRemoved( Plasma::Applet* app );
    
    protected:
        // reimplemented from QGraphicsWidget
        virtual void resizeEvent( QGraphicsSceneResizeEvent * event );
    private:
        int minIndexWithAppletOnScreen( int loc );
        
        QList< Plasma::Applet* > m_appletList;
        int m_showingIndex;
};

}

#endif
