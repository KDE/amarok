/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#include "ConnectionAssistant.h"

#include "MediaDeviceMonitor.h"

ConnectionAssistant::ConnectionAssistant( bool wait )
    : QObject()
    , m_wait( wait )
{
}

ConnectionAssistant::~ConnectionAssistant()
{
}

bool
ConnectionAssistant::identify(const QString& udi)
{
    Q_UNUSED( udi );
    return false;
}

MediaDeviceInfo*
ConnectionAssistant::deviceInfo( const QString& udi )
{
    Q_UNUSED( udi );
    MediaDeviceInfo *info = 0;
    return info;
}

void
ConnectionAssistant::tellIdentified( const QString &udi )
{
    DEBUG_BLOCK
    emit identified( deviceInfo( udi ) );
}

void
ConnectionAssistant::tellDisconnected( const QString& udi )
{
    DEBUG_BLOCK
    emit disconnected( udi );
}

bool
ConnectionAssistant::wait()
{
    return m_wait;
}


#include "ConnectionAssistant.moc"
