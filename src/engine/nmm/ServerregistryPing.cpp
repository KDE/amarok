/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2002-2006
 *                    NMM work group,
 *                    Computer Graphics Lab,
 *                    Saarland University, Germany
 *                    http://www.networkmultimedia.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307
 * USA
 */

#include "ServerregistryPing.h"

#include "debug.h"

ServerregistryPing::ServerregistryPing(const QString &host, Q_UINT16 port)
    : QSocket()
{
    connect( this, SIGNAL(connected()),
            SLOT(socketConnected()) );
    connect( this, SIGNAL(connectionClosed()),
            SLOT(socketConnectionClosed()) );
    connect( this, SIGNAL(error(int)),
            SLOT(socketError(int)) );
    
    connectToHost(host, port);
}

void ServerregistryPing::socketConnected()
{
    DEBUG_FUNC_INFO
    emit registryAvailable( true );
}

void ServerregistryPing::socketConnectionClosed()
{
    DEBUG_FUNC_INFO
    emit registryAvailable( false );
}

void ServerregistryPing::socketError( int )
{
    DEBUG_FUNC_INFO
    emit registryAvailable( false );
}

#include "ServerregistryPing.moc"
