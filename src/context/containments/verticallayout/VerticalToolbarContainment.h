/***************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>           *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef AMAROK_HORIZONTAL_TOOLBAR_CONTAINMENT_H
#define AMAROK_HORIZONTAL_TOOLBAR_CONTAINMENT_H

#include "Applet.h"
#include "Containment.h"
#include "ContextView.h"

class KConfigGroup;

class QGraphicsLinearLayout;

namespace Context
{

class AppletToolbar;
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
        
    signals:
        void updatedContainment( Containment* );
    
    private:
        ContextView* m_view;
        AppletToolbar* m_toolbar;
        VerticalAppletLayout* m_applets;
};

K_EXPORT_PLASMA_APPLET( amarok_containment_vertical, VerticalToolbarContainment )


}

#endif