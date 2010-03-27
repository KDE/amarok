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

#include "MtpConnectionAssistant.h"
#include "MtpDeviceInfo.h"

#include "MediaDeviceCache.h" // for mountpoint

#include "core/support/Debug.h"

#include <QString>

#include <solid/device.h>
#include <solid/portablemediaplayer.h>

MtpConnectionAssistant::~MtpConnectionAssistant()
{
}

bool
MtpConnectionAssistant::identify( const QString& udi )
{
    DEBUG_BLOCK

    Solid::Device device;

    device = Solid::Device( udi );
    if( !device.is<Solid::PortableMediaPlayer>() )
    {
        debug() << "Not a PMP";
        return false;
    }

    Solid::PortableMediaPlayer *pmp = device.as<Solid::PortableMediaPlayer>();

    debug() << "Supported Protocols: " << pmp->supportedProtocols();

    return pmp->supportedProtocols().contains( "mtp" );
}


MediaDeviceInfo*
MtpConnectionAssistant::deviceInfo( const QString& udi )
{
    MediaDeviceInfo* info = new MtpDeviceInfo( udi );
    return info;
}

#include "MtpConnectionAssistant.moc"
