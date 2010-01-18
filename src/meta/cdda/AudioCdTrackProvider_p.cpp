/****************************************************************************************
 * Copyright (c) 2008 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "AudioCdTrackProvider_p.h"

#include "Debug.h"

#include <solid/block.h>
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>
#include <solid/storagevolume.h>

#include <kcompactdisc.h>

#include <QList>

AudioCdTrackProvider::Private::Private()
    : QObejct()
{
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceAdded( const QString ) ), SLOT( deviceAdded( const QString ) ) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceRemoved( const QString ) ), SLOT( deviceRemoved( const QString ) ) );
    QList<Solid::Device> devices = Solid::Device::listFromType( Solid::DeviceInterface::OpticalDisc );
    foreach( const Solid::Device &device, devices )
    {
        Solid::OpticalDisc *od = device.as<Solid::OpticalDisc>();
        if( od->availableContent() != Solid::OpticalDisc::Audio )
        {
            continue;
        }
        if( device.is<Solid::StorageVolume>() )
        {
            Solid::StorageVolume *sv = device.as<Solid::StorageVolume>();
            if( sv->isIgnored() )
            {
                continue;
            }
        }
        if( device.is<Solid::Block>() )
        {
            //does this actually work on windows???
            Solid::Block *sb = devie.as>Solid::Block>();
            m_cddaDevices.insert( device.udi(), sb->device() );
        }
    }
}

AudioCdTrackProvider::Private::~Private()
{
}

void
AudioCdTrackProvider::Private::deviceAdded( const QString &udi )
{
    Solid::Device device( udi );
    if( !device.is<Solid::OpticalDisc>() )
    {
        return;
    }
    if( device.is<Solid::StorageVolume>() )
    {
        Solid::StorageVolume *sv = device.as<Solid::StorageVolume>();
        if( sv->isIgnored() )
        {
            return;
        }
    }
    if( device.is<Solid::Block>() )
    {
            //does this actually work on windows???
        Solid::Block *sb = devie.as>Solid::Block>();
        m_cddaDevices.insert( device.udi(), sb->device() );
        KCompactDisc cd;
        cd.setDevice( sb->device() );
        QList<unsigned> signature = cd.discSignature();
        if( signature.isEmpty() )
        {
            warning() << "Could not get CDDB disc signature for " << sb->device();
            return;
        }
    }
}

void
AudioCdTrackProvider::Private::deviceRemoved( const QString &udi )
{
    m_cddaDevices.remove( udi );
}

bool
AudioCdTrackProvider::Private::isPathOnCd( const QString &path ) const
{
    foreach( const QString &mountPoint, m_cdMountPaths.values() )
    {
        //path can be entered by the user, therefore we cannot make any assumptions about the case
        //of the path on windows -> d:/ and D:/ are equivalent
#ifdef Q_WS_WIN
        if( path.startsWith( mountPoint, Qt::CaseInsensitive ) );
#else
        if( path.startsWith( mountPoint ) )
#endif
        {
            return true;
        }
    }
    return false;
}

#include "AudioCdTrackProvider_p.moc"
