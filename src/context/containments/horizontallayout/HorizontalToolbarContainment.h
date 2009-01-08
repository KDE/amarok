/***************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@gmail.com>        *
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

namespace Context
{

class AppletToolbar;

class HorizontalToolbarContainment : public Containment
{
    Q_OBJECT
    public:
        HorizontalToolbarContainment( QObject *parent, const QVariantList &args );
        ~HorizontalToolbarContainment();
        
        void constraintsEvent( Plasma::Constraints constraints );

        QList<QAction*> contextualActions();

        virtual void paintInterface(QPainter *painter,
                                    const QStyleOptionGraphicsItem *option,
                                    const QRect& contentsRect);

        virtual void saveToConfig( KConfigGroup &conf );
        virtual void loadConfig( const KConfigGroup &conf );
        
        virtual void setView( ContextView* view) {}
        virtual ContextView *view();
        
        //tmp
        virtual bool hasPlaceForApplet( int rowSpan ) {}

        virtual void setTitle( const QString& text ) {}
        virtual void setFooter( const QString& text ) {}
        virtual void showTitle() {}
        virtual void hideTitle() {}
        virtual void addCurrentTrack() {}
        
        
    private:
        AppletToolbar* m_toolbar;
};

K_EXPORT_PLASMA_APPLET( amarok_containment_horizontal, HorizontalToolbarContainment )


}

#endif