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

#ifndef AMAROK_VERTICAL_TOOLBAR_CONTAINMENT_H
#define AMAROK_VERTICAL_TOOLBAR_CONTAINMENT_H

#include "Applet.h"
#include "Containment.h"
#include "ContextView.h"

class KConfigGroup;

class QGraphicsLinearLayout;

namespace Context
{

class VerticalAppletLayout;

class VerticalToolbarContainment : public Containment
{
    Q_OBJECT
    public:
        VerticalToolbarContainment( QObject *parent, const QVariantList &args );
        ~VerticalToolbarContainment();
        
        void constraintsEvent( Plasma::Constraints constraints );

        QList<QAction*> contextualActions();

        virtual void paintInterface(QPainter *painter,
                                    const QStyleOptionGraphicsItem *option,
                                    const QRect& contentsRect);

        virtual void saveToConfig( KConfigGroup &conf );
        virtual void loadConfig( const KConfigGroup &conf );
        
        virtual void setView( ContextView* view);
        virtual ContextView *view();

        QRectF boundingRect () const;
    public slots:
        Applet* addApplet( const QString& pluginName, const int );
        void    appletRemoved( Plasma::Applet* );
        // these slots below are forwarded to the layout
        void showApplet( Plasma::Applet* );
        void moveApplet( Plasma::Applet*, int, int );
        
    protected:
        virtual void wheelEvent( QGraphicsSceneWheelEvent* event );
    signals:
        void updatedContainment( Containment* );
        void appletAdded( Plasma::Applet*, int );
        
    private slots:
        void showEmptyText( bool );

    private:
        ContextView* m_view;
        VerticalAppletLayout* m_applets;
        
        bool m_noApplets;
        QGraphicsTextItem* m_noAppletText;
};

K_EXPORT_PLASMA_APPLET( amarok_containment_vertical, VerticalToolbarContainment )

}

#endif

