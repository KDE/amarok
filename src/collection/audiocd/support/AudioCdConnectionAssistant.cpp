/*
   Copyright (C) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "AudioCdConnectionAssistant.h"
#include "AudioCdDeviceInfo.h"

#include "MediaDeviceCache.h" // for mountpoint

#include "Debug.h"

#include <QString>

#include <solid/device.h>
#include <solid/opticaldisc.h>

AudioCdConnectionAssistant::~AudioCdConnectionAssistant()
{
}

bool
AudioCdConnectionAssistant::identify( const QString& udi )
{
    DEBUG_BLOCK

    Solid::Device device;

    device = Solid::Device(udi);
    
    if( device.is<Solid::OpticalDisc>() )
    {
        debug() << "OpticalDisc";
        Solid::OpticalDisc * opt = device.as<Solid::OpticalDisc>();
        if ( opt->availableContent() & Solid::OpticalDisc::Audio )
        {
            debug() << "AudioCd";
            return true;
        }
    }

    return false;
}


MediaDeviceInfo*
AudioCdConnectionAssistant::deviceInfo( const QString& udi )
{

    QString mountpoint = MediaDeviceCache::instance()->volumeMountPoint(udi);

    MediaDeviceInfo* info = new AudioCdDeviceInfo( mountpoint, udi );
    return info;
}

#include "AudioCdConnectionAssistant.moc"
