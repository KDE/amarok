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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
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

    
    DEBUG_BLOCK;
    //for testing


    QList<QVariant> strings;
    QList<QVariant> weights;

    strings << "This" << "is" << "just" << "a" << "very" << "small" << "and" << "quite" << "silly" << "defalt" << "text"
            << "as" << "I" << "currently" << "have" <<  "nothing" << "better" << "to" << "show";
    
    weights << 10 << 4 << 8 << 2 << 6 << 5 << 10 << 9 << 3 << 1 << 3 << 5 << 7 << 9 << 3 << 2 << 10 << 6 << 4;

    m_storedCloud["cloud_name"] = QVariant( "test cloud" );
    m_storedCloud["cloud_strings"] = QVariant( strings );
    m_storedCloud["cloud_weights"] = QVariant( weights );
}

ServiceInfoProxy::~ServiceInfoProxy()
{
}


void ServiceInfoProxy::subscribe(ServiceInfoObserver * observer)
{
    DEBUG_BLOCK;
    if( observer ) {
        m_observers.insert( observer );
        observer->serviceInfoChanged( m_storedInfo );
    }
}

void ServiceInfoProxy::subscribeForCloud(ServiceInfoObserver * observer)
{
    DEBUG_BLOCK;
    if( observer ) {
        m_cloudObservers.insert( observer );
        observer->serviceInfoChanged( m_storedCloud );
    }
}

void ServiceInfoProxy::unsubscribe(ServiceInfoObserver * observer)
{
    m_observers.remove( observer );
    m_cloudObservers.remove( observer );
}

void ServiceInfoProxy::notifyObservers( QVariantMap infoMap ) const
{
    foreach( ServiceInfoObserver *observer, m_observers )
        observer->serviceInfoChanged( infoMap );
}

void ServiceInfoProxy::notifyCloudObservers( QVariantMap cloudMap) const
{
    foreach( ServiceInfoObserver *observer, m_cloudObservers )
        observer->serviceInfoChanged( cloudMap );
}

void ServiceInfoProxy::setInfo(QVariantMap infoMap)
{
    DEBUG_BLOCK;
    m_storedInfo = infoMap;
    notifyObservers( m_storedInfo );
}

void ServiceInfoProxy::setCloud(QVariantMap cloudMap)
{
    DEBUG_BLOCK;
    m_storedCloud = cloudMap;
    notifyCloudObservers( m_storedCloud );
}

QVariantMap ServiceInfoProxy::info()
{
    return m_storedInfo;
}

QVariantMap ServiceInfoProxy::cloud()
{
    return m_storedCloud;
}


namespace The {
    AMAROK_EXPORT ServiceInfoProxy* serviceInfoProxy() { return ServiceInfoProxy::instance(); }
}










