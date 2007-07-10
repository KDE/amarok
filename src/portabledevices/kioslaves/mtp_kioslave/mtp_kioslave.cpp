/*
 *  Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "mtp_kioslave.h"
#include <kdebug.h>

MTPProtocol::MTPProtocol( const QByteArray &protocol, const QByteArray &pool,
                          const QByteArray &app )
                        : SlaveBase( protocol, pool, app )
                        , m_deviceCount( 0 )
{

    LIBMTP_Init();

    if( LIBMTP_Get_Connected_Devices( &m_deviceList )  != LIBMTP_ERROR_NONE )
        error( KIO::INTERNAL, "Could not get a connected device list from libmtp." );
    if( m_deviceCount = !LIBMTP_Number_Devices_In_List( m_deviceList ) == 0 )
        error( KIO::INTERNAL, "libmtp found no devices." );
}

MTPProtocol::~MTPProtocol()
{
    if( m_deviceCount )
        LIBMTP_Release_Device_List( m_deviceList );
}

MTPProtocol::setHost( const QString &host, quint16 port,
                      const QString &user, const QString &pass )
{
    foreach( LIBMTP_mtpdevice_t *currdevice, m_deviceList )
    {
        if( LIBMTP_Get_Serialnumber( currdevice ) == host )
            m_device = currdevice;    
    }
}

#include "mtp_kioslave.moc"

