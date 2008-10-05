/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2007  Leo Franchi <lfranchi@gmail.com>                  * 
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

#include "ServiceEngine.h"

#include "Amarok.h"
#include "Debug.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "services/ServiceInfoProxy.h"

#include <QVariant>

using namespace Context;

ServiceEngine::ServiceEngine( QObject* parent, const QList<QVariant>& args )
    : DataEngine( parent )
    , m_requested( true )
{
    Q_UNUSED( args )
    DEBUG_BLOCK
    m_sources = QStringList();
    m_sources << "service";

    The::serviceInfoProxy()->subscribe( this );
}

ServiceEngine::~ ServiceEngine()
{
    The::serviceInfoProxy()->unsubscribe( this );
}

QStringList ServiceEngine::sources() const
{
    return m_sources; // we don't have sources, if connected, it is enabled.
}

bool ServiceEngine::sourceRequested( const QString& name )
{
    Q_UNUSED( name );
/*    m_sources << name;    // we are already enabled if we are alive*/
    setData( name, QVariant());
    update();
    m_requested = true;
    return true;
}

void ServiceEngine::message( const ContextState& state )
{
    DEBUG_BLOCK;
    if( state == Current && m_requested ) {
        m_storedInfo = The::serviceInfoProxy()->info();
        update();
    }
}


void ServiceEngine::serviceInfoChanged(QVariantMap infoMap)
{
    m_storedInfo = infoMap;
    update();
}

void ServiceEngine::update()
{
    DEBUG_BLOCK;
    setData( "service", "service_name", m_storedInfo["service_name"] );
    setData( "service", "main_info", m_storedInfo["main_info"] );

}



#include "ServiceEngine.moc"


