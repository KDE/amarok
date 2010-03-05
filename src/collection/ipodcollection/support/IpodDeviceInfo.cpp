/****************************************************************************************
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#include <QStringList>
#include "IpodDeviceInfo.h"
#include "MediaDeviceInfo.h"

IpodDeviceInfo::IpodDeviceInfo( QString mountpoint, QString udi, bool wasMounted )
: MediaDeviceInfo()
, m_mountpoint( mountpoint )
, m_wasMounted( wasMounted )
{
    m_udi = udi;
}

IpodDeviceInfo::~IpodDeviceInfo()
{
}

QString
IpodDeviceInfo::mountPoint() const
{
    return m_mountpoint;
}

void
IpodDeviceInfo::setMountPoint( QString mp )
{
    m_mountpoint = mp;
}

QString
IpodDeviceInfo::deviceUid() const
{
    const QStringList components = m_udi.split( '_' );
    if( components.count() >= 2 )
    {
        QString uid = components[components.count()-2];
        if( uid.length() == 40 )
            return uid;
    }
    return QString();
}

bool
IpodDeviceInfo::wasMounted() const
{
    return m_wasMounted;
}

#include "IpodDeviceInfo.moc"
