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

#include "pmpdevice.h"

#include "pmpbackend.h"
#include "pmpkioslave.h"

#include <solid/device.h>
#include <solid/portablemediaplayer.h>

bool
PMPDevice::isValid() const
{
    return m_device.isValid();
}

void
PMPDevice::initialize()
{
    if( !m_device.isValid() )
    {
        m_slave->error( KIO::ERR_CANNOT_OPEN_FOR_READING,
                i18n( "portable media player : Device not found by Solid.  Ensure the device is turned on, you have permission to access it, and that the UDI's forward slashes are replaced by periods" ) );
        return;
    }

    Solid::PortableMediaPlayer *pmp = m_device.as<Solid::PortableMediaPlayer>();
    if( !pmp )
    {
        m_slave->error( KIO::ERR_CANNOT_OPEN_FOR_READING, i18n( "device : Device %1 is not a portable media player" ).arg( m_device.udi() ) );
        return;
    }

#ifdef HAVE_MTP
    if( pmp->supportedProtocols().contains( "mtp", Qt::CaseInsensitive ) )
    {
        m_backend = new MTPBackend( m_slave, m_device );
        m_slave->setMtpInitialized( true );
        m_backend->initialize();
        return;
    }
#endif

    m_slave->error( KIO::ERR_CANNOT_OPEN_FOR_READING, i18n( "device : No supported protocol found" ) );
    return;
}


