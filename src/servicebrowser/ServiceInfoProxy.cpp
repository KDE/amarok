/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#include "ServiceInfoProxy.h"
#include "debug.h"

ServiceInfoProxy * ServiceInfoProxy::m_instance = 0;

ServiceInfoProxy * ServiceInfoProxy::instance()
{
    if ( m_instance == 0 )
        m_instance = new ServiceInfoProxy();

    return m_instance;
}


ServiceInfoProxy::ServiceInfoProxy()
{
}

ServiceInfoProxy::~ServiceInfoProxy()
{
}


void ServiceInfoProxy::subscribe(ServiceInfoObserver * observer)
{
    DEBUG_BLOCK;
    if( observer )
        m_observers.insert( observer );
}

void ServiceInfoProxy::unsubscribe(ServiceInfoObserver * observer)
{
    m_observers.remove( observer );
}

void ServiceInfoProxy::notifyObservers(QVariantMap infoMap) const
{
    foreach( ServiceInfoObserver *observer, m_observers )
        observer->serviceInfoChanged( infoMap );
}

void ServiceInfoProxy::setInfo(QVariantMap infoMap)
{
    DEBUG_BLOCK;
    m_storedInfo = infoMap;
    notifyObservers( m_storedInfo );
}

QVariantMap ServiceInfoProxy::info()
{
    return m_storedInfo;
}


namespace The {
    ServiceInfoProxy* serviceInfoProxy() { return ServiceInfoProxy::instance(); }
}


