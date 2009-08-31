/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "Containment.h"

#include "Debug.h"

#include <plasma/corona.h>


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


} // namespace Context
#include "Containment.moc"

