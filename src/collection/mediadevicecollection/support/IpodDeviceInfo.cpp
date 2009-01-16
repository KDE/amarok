/*****************************************************************************
* copyright            : (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>
*
 *****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "IpodDeviceInfo.h"
#include "MediaDeviceInfo.h"

IpodDeviceInfo::IpodDeviceInfo( QString mountpoint, QString udi )
: MediaDeviceInfo(),
m_mountpoint( mountpoint )
{
    m_type = IPOD_T;
    m_udi = udi;
}

IpodDeviceInfo::~IpodDeviceInfo()
{
}

QString
IpodDeviceInfo::mountpoint()
{
    return m_mountpoint;
}

#include "IpodDeviceInfo.moc"
