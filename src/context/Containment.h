/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef AMAROK_CONTAINMENT_H
#define AMAROK_CONTAINMENT_H

#include "amarok_export.h"
#include "ContextView.h"

#include "plasma/containment.h"

#include <QAction>
#include <QRectF>

namespace Context
{

class ContextView;

class AMAROK_EXPORT Containment : public Plasma::Containment
{
    Q_OBJECT
public:
    explicit Containment(QGraphicsItem* parent = 0,
                         const QString& serviceId = QString(),
                         uint containmentId = 0);
    
    Containment(QObject* parent, const QVariantList& args);
    
    ~Containment();
    
    virtual void saveToConfig( KConfigGroup &conf ) = 0;
    virtual void loadConfig( const KConfigGroup &conf ) = 0;

    virtual bool hasPlaceForApplet( int rowSpan ) = 0;

    virtual void setTitle( QString title ) = 0;
    virtual void showTitle() = 0;
    virtual void hideTitle() = 0;
    virtual void addCurrentTrack() = 0;
    virtual void setView( ContextView *newView ) = 0;
    virtual void setZoomLevel( Plasma::ZoomLevel );

    virtual ContextView *view() = 0;
    
signals:
    void appletRejected( QString pluginName, int rowSpan );
  
private:
    Plasma::ZoomLevel m_zoomLevel;      
};

} // Context namespace
#endif
