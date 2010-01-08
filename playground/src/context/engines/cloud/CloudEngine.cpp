/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhn@kde.org>                *
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



#include "CloudEngine.h"

#include "Amarok.h"
#include "Debug.h"
#include "ContextObserver.h"
#include "ContextView.h"
#include "services/ServiceInfoProxy.h"

#include <QVariant>

using namespace Context;

CloudEngine::CloudEngine( QObject* parent, const QList<QVariant>& args )
    : DataEngine( parent )
    , m_requested( true )
{
    Q_UNUSED( args )
    DEBUG_BLOCK
    m_sources = QStringList();
    m_sources << "cloud";

    The::serviceInfoProxy()->subscribeForCloud( this );
}

CloudEngine::~ CloudEngine()
{
    The::serviceInfoProxy()->unsubscribe( this );
}

QStringList CloudEngine::sources() const
{
    return m_sources; // we don't have sources, if connected, it is enabled.
}

bool CloudEngine::sourceRequested( const QString& name )
{
    Q_UNUSED( name );
/*    m_sources << name;    // we are already enabled if we are alive*/
    setData( name, QVariant());
    update();
    m_requested = true;
    return true;
}

void CloudEngine::message( const ContextState& state )
{
    DEBUG_BLOCK;
    if( state == Current && m_requested ) {
        m_storedCloud = The::serviceInfoProxy()->cloud();
        update();
    }
}


void CloudEngine::serviceInfoChanged(QVariantMap infoMap)
{
    m_storedCloud = infoMap;
    update();
}

void CloudEngine::update()
{
    DEBUG_BLOCK;
    setData( "cloud", "cloud_name", m_storedCloud["cloud_name"] );
    setData( "cloud", "cloud_strings", m_storedCloud["cloud_strings"] );
    setData( "cloud", "cloud_weights", m_storedCloud["cloud_weights"] );
    setData( "cloud", "cloud_actions", m_storedCloud["cloud_actions"] );

}



#include "CloudEngine.moc"


