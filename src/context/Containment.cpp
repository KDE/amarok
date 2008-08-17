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

#include "Containment.h"

#include "Debug.h"

#include "plasma/corona.h"

#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsScene>
#include <KMenu>

namespace Context
{

Containment::Containment( QGraphicsItem* parent, const QString& serviceId, uint containmentId ) 
        : Plasma::Containment( parent, serviceId, containmentId )
{}

Containment::Containment( QObject* parent, const QVariantList& args )
        : Plasma::Containment( parent, args )
{}

Containment::~Containment()
{
    DEBUG_BLOCK
}

void
Containment::setZoomLevel( Plasma::ZoomLevel zoomLevel )
{
    m_zoomLevel = zoomLevel;
}

} // namespace Context
#include "Containment.moc"

